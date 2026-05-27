from __future__ import annotations

import re
from collections import defaultdict
from dataclasses import dataclass, field
from typing import Dict, List, Set

from broma_parser import Class, Method, Root
from filtering import (
    call_label,
    group_supported,
    linkless_class_names,
    prune_skipped_class_refs,
    returns_owned,
)
from cxx_templates import emit_internal_hpp, file_preamble
from hooks import emit_hook_support, emit_hook_target, hook_id, hook_suffix, hookable
from marshalling import check_arg, push_return, push_value
from model import (
    build_class_lookup,
    codegen_object_map,
    cxx_name,
    lua_namespace,
    object_classes,
    resolve_base,
    short_name,
)
from type_map import (
    TypeInfo,
    require_classify_arg,
    require_classify_return,
    method_input_arg_count,
)


def _classify_method_args(m: Method, objects: Dict[str, Class]) -> List[TypeInfo]:
    return [require_classify_arg(arg.type, objects) for arg in m.args]


def _id(value: str) -> str:
    return re.sub(r"[^A-Za-z0-9_]", "_", value)


def _gen_ns(cls: Class) -> str:
    return f"ns_{_id(cls.name)}"


def _emit_invoke(cls: Class, m: Method, objects: Dict[str, Class], suffix: str) -> str:
    fn = f"luaapi_{_id(cls.name)}_{_id(m.name)}{suffix}"
    label = call_label(cls, m)
    ret = require_classify_return(m.ret, objects)
    arg_infos = _classify_method_args(m, objects)

    input_count = 0
    for arg, info in zip(m.args, arg_infos):
        if ret.kind == "void" and info.is_out:
            continue
        input_count += 1

    out = [f"    int {fn}(lua_State* L) {{\n"]
    expected = input_count + (0 if m.is_static else 1)
    out.append(
        f'        if (lua_gettop(L) != {expected}) luaL_error(L, "{label} expected {expected} args");\n'
    )
    call_args: List[str] = []
    if not m.is_static:
        out.append(
            f'        auto self = luax::Usertype<{cxx_name(cls)}>::check(L, 1, "{label}");\n'
        )
    lua_idx = 1 if m.is_static else 2
    for arg_idx, (arg, info) in enumerate(zip(m.args, arg_infos)):
        var = f"arg{arg_idx}"
        if ret.kind == "void" and info.is_out:
            if info.kind == "value" and info.lua_type == "UIButtonConfig":
                out.append(f"        UIButtonConfig {var}{{}};\n")
            elif info.kind == "enum":
                out.append(f"        {info.cxx_type} {var}{{}};\n")
            else:
                out.append(f"        {info.cxx_type} {var}{{}};\n")
            call_args.append(var)
            continue
        out.extend(check_arg(arg, info, lua_idx, var, label))
        call_args.append(var)
        lua_idx += 1

    args = ", ".join(call_args)
    target = (
        f"{cxx_name(cls)}::{m.name}({args})"
        if m.is_static
        else f"self->{m.name}({args})"
    )
    out_refs = [
        (arg_idx, info)
        for arg_idx, (_, info) in enumerate(zip(m.args, arg_infos))
        if ret.kind == "void" and info.is_out
    ]
    if ret.kind == "void":
        out.append(f"        {target};\n")
        if out_refs:
            for arg_idx, info in out_refs:
                out.extend(push_value(info, f"arg{arg_idx}", False))
            out.append(f"        return {len(out_refs)};\n")
        else:
            out.extend(push_return(ret, "", False))
    else:
        out.append(f"        auto result = {target};\n")
        out.extend(push_return(ret, "result", returns_owned(m)))
    out.append("    }\n\n")
    return "".join(out)


def _input_arg_count(m: Method, objects: Dict[str, Class]) -> int:
    return method_input_arg_count(m, objects)


def _emit_dispatcher(
    cls: Class, name: str, methods: List[Method], objects: Dict[str, Class]
) -> str:
    if len(methods) == 1:
        return ""
    first = methods[0]
    fn = f"luaapi_{_id(cls.name)}_{_id(name)}"
    out = [f"    int {fn}(lua_State* L) {{\n"]
    adjust = 0 if first.is_static else 1
    out.append(f"        switch (lua_gettop(L) - {adjust}) {{\n")
    for idx, m in enumerate(methods):
        out.append(
            f"            case {_input_arg_count(m, objects)}: return luaapi_{_id(cls.name)}_{_id(name)}_{idx}(L);\n"
        )
    out.append("            default: break;\n")
    out.append("        }\n")
    out.append(
        f'        luaL_error(L, "{call_label(cls, first)} unsupported overload arity");\n'
    )
    out.append("    }\n\n")
    return "".join(out)


def _inheritance_depth(
    cls: Class, lookup: Dict[str, Class], skipped_classes: Set[str]
) -> int:
    if cls.name == "CCObject":
        return 0
    values: List[int] = []
    for base in cls.bases:
        base_cls = resolve_base(lookup, base)
        if base_cls and base_cls.name not in skipped_classes:
            values.append(_inheritance_depth(base_cls, lookup, skipped_classes) + 1)
    return max(values) if values else 1


def _binding_filename(cls_name: str) -> str:
    return f"bindings_{cls_name}.cpp"


@dataclass
class EmitPlan:
    classes: List[Class]
    objects: Dict[str, Class]
    skipped: List[tuple[str, str, str]]
    skipped_classes: Set[str]
    supported_by_class: dict[str, dict[str, list[Method]]]
    hook_targets_by_class: dict[str, List[tuple[Class, Method]]]
    depths: dict[str, int]
    emitted_classes: List[Class] = field(default_factory=list)


def _collect_plan(root: Root, target_platform: str) -> EmitPlan:
    classes = object_classes(root)
    objects = codegen_object_map(root)
    lookup = build_class_lookup(classes)
    skipped: list[tuple[str, str, str]] = []
    supported_by_class: dict[str, dict[str, list[Method]]] = {}
    skipped_by_class: dict[str, list[tuple[Method, str]]] = {}

    for cls in classes:
        grouped, cls_skipped = group_supported(cls, objects, target_platform)
        supported_by_class[cls.name] = grouped
        skipped_by_class[cls.name] = cls_skipped
        for m, reason in cls_skipped:
            skipped.append((cls.name, m.name, reason))

    skipped_classes = linkless_class_names(
        classes, objects, supported_by_class, skipped_by_class, target_platform
    )
    skipped.extend(
        prune_skipped_class_refs(
            supported_by_class,
            skipped_by_class,
            objects,
            skipped_classes,
            target_platform,
        )
    )
    depths = {
        cls.name: _inheritance_depth(cls, lookup, skipped_classes) for cls in classes
    }

    hook_targets_by_class: dict[str, List[tuple[Class, Method]]] = defaultdict(list)
    for cls in classes:
        if cls.name in skipped_classes:
            continue
        grouped = supported_by_class[cls.name]
        for methods in grouped.values():
            for m in methods:
                if hookable(cls, m, objects, target_platform):
                    hook_targets_by_class[cls.name].append((cls, m))

    emitted_classes: List[Class] = []
    for cls in classes:
        if cls.name in skipped_classes:
            continue
        grouped = supported_by_class[cls.name]
        if grouped or hook_targets_by_class[cls.name]:
            emitted_classes.append(cls)

    return EmitPlan(
        classes=classes,
        objects=objects,
        skipped=skipped,
        skipped_classes=skipped_classes,
        supported_by_class=supported_by_class,
        hook_targets_by_class=hook_targets_by_class,
        depths=depths,
        emitted_classes=emitted_classes,
    )


def collect_plan(root: Root, target_platform: str = "win") -> EmitPlan:
    return _collect_plan(root, target_platform)


def plan_outputs(
    root: Root, target_platform: str = "win", plan: EmitPlan | None = None
) -> List[str]:
    if plan is None:
        plan = _collect_plan(root, target_platform)
    outputs = ["bindings_internal.hpp", "bindings_common.cpp"]
    outputs.extend(_binding_filename(cls.name) for cls in plan.emitted_classes)
    return outputs


def hook_target_count(
    root: Root, target_platform: str = "win", plan: EmitPlan | None = None
) -> int:
    if plan is None:
        plan = _collect_plan(root, target_platform)
    return sum(len(targets) for targets in plan.hook_targets_by_class.values())


def _emit_class_file(
    cls: Class,
    grouped: dict[str, list[Method]],
    hook_targets: List[tuple[Class, Method]],
    objects: Dict[str, Class],
    skipped_classes: Set[str],
    depth: int,
    target_platform: str,
) -> str:
    ns_name = _id(cls.name)
    gen_ns = _gen_ns(cls)
    out = [file_preamble(), '#include "bindings_internal.hpp"\n\n']
    out.append(f"namespace luauapi_gen::{gen_ns} {{\n\n")
    out.append("namespace {\n\n")

    for methods in grouped.values():
        for idx, m in enumerate(methods):
            suffix = "" if len(methods) == 1 else f"_{idx}"
            out.append(_emit_invoke(cls, m, objects, suffix))
        out.append(_emit_dispatcher(cls, methods[0].name, methods, objects))

    for _, m in hook_targets:
        out.append(emit_hook_target(cls, m, objects, target_platform))

    out.append("} // namespace\n\n")

    if hook_targets:
        out.append("namespace {\n\n")
        out.append("LuaHookTarget const kHookTargetsData[] = {\n")
        for _, m in hook_targets:
            suffix = hook_suffix(cls, m)
            out.append(
                f'    {{ "{hook_id(cls, m)}", "{cls.name}::{m.name}", &luaapi_create_hook_{suffix} }},\n'
            )
        out.append("};\n\n")
        out.append("} // namespace\n\n")
        out.append("LuaHookTarget const* hookTargets() {\n")
        out.append("    return kHookTargetsData;\n")
        out.append("}\n\n")
        out.append("std::size_t hookTargetCount() {\n")
        out.append("    return sizeof(kHookTargetsData) / sizeof(LuaHookTarget);\n")
        out.append("}\n\n")
    else:
        out.append("LuaHookTarget const* hookTargets() {\n")
        out.append("    return nullptr;\n")
        out.append("}\n\n")
        out.append("std::size_t hookTargetCount() {\n")
        out.append("    return 0;\n")
        out.append("}\n\n")

    out.append("geode::Result<void> bind(lua_State* L) {\n")
    bases = []
    for base in cls.bases:
        base_cls = objects.get(short_name(base))
        if base_cls and base_cls.name not in skipped_classes:
            bases.append(f"luax::Usertype<{cxx_name(base_cls)}>::tag()")
    base_expr = "{ " + ", ".join(bases) + " }" if bases else "{}"
    out.append(
        f'    auto registerResult = luax::Usertype<{cxx_name(cls)}>::registerType(L, "{cls.name}", {base_expr});\n'
    )
    out.append(
        "    if (registerResult.isErr()) return geode::Err(registerResult.unwrapErr());\n"
    )

    for name, methods in grouped.items():
        if methods[0].is_static:
            continue
        fn = f"luaapi_{_id(cls.name)}_{_id(name)}"
        out.append(
            f'    luax::Usertype<{cxx_name(cls)}>::method(L, "{name}", &{fn});\n'
        )

    static_methods = [
        (name, methods) for name, methods in grouped.items() if methods[0].is_static
    ]
    if static_methods:
        ns = lua_namespace(cls)
        out.append(f'\n    luax::getOrCreateTable(L, "{ns}");\n')
        out.append(f"    lua_createtable(L, 0, {len(static_methods)});\n")
        for name, methods in static_methods:
            fn = f"luaapi_{_id(cls.name)}_{_id(name)}"
            out.append(f'    lua_pushcfunction(L, &{fn}, "{cls.name}.{name}");\n')
            out.append(f'    lua_setfield(L, -2, "{name}");\n')
        out.append(f'    lua_setfield(L, -2, "{cls.name}");\n')
        out.append("    lua_pop(L, 1);\n")

    out.append("    return geode::Ok();\n")
    out.append("}\n\n")
    out.append(f"}} // namespace luauapi_gen::{gen_ns}\n\n")
    out.append("namespace {\n")
    out.append(
        f"    LUAX_BINDING_PRIORITY(GeneratedBroma_{ns_name}, luauapi_gen::{gen_ns}::bind, {depth})\n"
    )
    out.append("}\n")
    return "".join(out)


def _emit_common_file(emitted_classes: List[Class]) -> str:
    out = [file_preamble(), '#include "bindings_internal.hpp"\n\n']
    for cls in emitted_classes:
        gen_ns = _gen_ns(cls)
        out.append(f"namespace luauapi_gen::{gen_ns} {{\n")
        out.append("LuaHookTarget const* hookTargets();\n")
        out.append("std::size_t hookTargetCount();\n")
        out.append("}\n\n")
    out.append("namespace luauapi_gen {\n\n")
    out.append(
        f'static_assert({len(emitted_classes)} < LUA_UTAG_LIMIT, "LuauAPI generated userdata tags exceed LUA_UTAG_LIMIT");\n\n'
    )
    out.append("LuaHookTarget const* findHookTarget(std::string_view id);\n\n")
    out.append(emit_hook_support())

    if emitted_classes:
        out.append("struct HookRange {\n")
        out.append("    LuaHookTarget const* (*targets)();\n")
        out.append("    std::size_t (*count)();\n")
        out.append("};\n\n")
        out.append("HookRange const kAllHookRanges[] = {\n")
        for cls in emitted_classes:
            gen_ns = _gen_ns(cls)
            out.append(f"    {{ {gen_ns}::hookTargets, {gen_ns}::hookTargetCount }},\n")
        out.append("};\n\n")
    else:
        out.append("struct HookRange {\n")
        out.append("    LuaHookTarget const* (*targets)();\n")
        out.append("    std::size_t (*count)();\n")
        out.append("};\n\n")
        out.append("HookRange const kAllHookRanges[] = {};\n\n")

    out.append("LuaHookTarget const* findHookTarget(std::string_view id) {\n")
    out.append("    for (auto const& range : kAllHookRanges) {\n")
    out.append("        auto const* targets = range.targets();\n")
    out.append("        auto targetCount = range.count();\n")
    out.append("        if (!targets || targetCount == 0) continue;\n")
    out.append("        for (std::size_t i = 0; i < targetCount; ++i) {\n")
    out.append("            if (targets[i].id == id) return &targets[i];\n")
    out.append("        }\n")
    out.append("    }\n")
    out.append("    return nullptr;\n")
    out.append("}\n\n")

    out.append("geode::Result<void> bindCommon(lua_State* L) {\n")
    out.append('    luax::getOrCreateTable(L, "geode");\n')
    out.append('    lua_pushcfunction(L, &luaapi_geode_hook, "geode.hook");\n')
    out.append('    lua_setfield(L, -2, "hook");\n')
    out.append("    lua_pop(L, 1);\n")
    out.append(
        "    if (auto* runtime = static_cast<luax::Runtime*>(lua_callbacks(L)->userdata)) {\n"
    )
    out.append("        runtime->registerShutdownHook(&clearHookRegistry);\n")
    out.append("    }\n")
    out.append("    return geode::Ok();\n")
    out.append("}\n\n")
    out.append("} // namespace luauapi_gen\n\n")
    out.append("namespace {\n")
    out.append(
        "    LUAX_BINDING_PRIORITY(GeneratedBromaCommon, luauapi_gen::bindCommon, -1)\n"
    )
    out.append("}\n")
    return "".join(out)


def emit(
    root: Root, target_platform: str = "win", plan: EmitPlan | None = None
) -> tuple[dict[str, str], list[tuple[str, str, str]]]:
    if plan is None:
        plan = _collect_plan(root, target_platform)

    files: dict[str, str] = {
        "bindings_internal.hpp": emit_internal_hpp(),
        "bindings_common.cpp": _emit_common_file(plan.emitted_classes),
    }

    for cls in plan.emitted_classes:
        files[_binding_filename(cls.name)] = _emit_class_file(
            cls,
            plan.supported_by_class[cls.name],
            plan.hook_targets_by_class[cls.name],
            plan.objects,
            plan.skipped_classes,
            plan.depths[cls.name],
            target_platform,
        )

    return files, plan.skipped
