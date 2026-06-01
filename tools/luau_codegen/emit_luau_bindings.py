from __future__ import annotations

import re
from typing import Dict, List, Set

from broma_parser import Arg, Class, Field, Function, Method, Root
from fields import bindable_field
from filtering import (
    call_label,
    returns_owned,
)
from free_fn_overrides import free_fn_allowed
from cxx_templates import emit_hook_support, emit_internal_hpp, file_preamble
from hooks import emit_hook_target, hook_address_expr, hook_id, hook_suffix
from marshalling import check_arg, push_return, push_value
from model import (
    cxx_name,
    lua_namespace,
    short_name,
)
from plan import (
    EmitPlan,
    collect_plan,
)
from type_map import (
    TypeInfo,
    classify_arg,
    classify_return,
    require_classify_arg,
    require_classify_return,
    method_input_arg_count,
)
from collections import defaultdict

FREE_FUNCTIONS_FILE = "bindings_free_functions.cpp"


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
        call_args.append(f"std::move({var})" if info.kind == "callback" else var)
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


def _emit_field_accessors(cls: Class, field: Field, objects: Dict[str, Class]) -> str:
    ok, reason, arg_info, ret_info = bindable_field(field, objects, cls)
    if not ok or not arg_info or not ret_info:
        raise ValueError(f"unsupported field {cls.name}.{field.name}: {reason}")
    label = f"{cls.name}.{field.name}"
    getter = f"luaapi_{_id(cls.name)}_field_get_{_id(field.name)}"
    setter = f"luaapi_{_id(cls.name)}_field_set_{_id(field.name)}"
    register = f"luaapi_{_id(cls.name)}_field_register_{_id(field.name)}"
    getter_impl = f"{getter}_impl"
    setter_impl = f"{setter}_impl"
    out = [f"    template <class T>\n"]
    out.append(f"    int {getter_impl}(lua_State* L, T* self) {{\n")
    out.append(f"        if constexpr (requires(T* obj) {{ obj->{field.name}; }}) {{\n")
    out.extend(
        f"    {line}" for line in push_value(ret_info, f"self->{field.name}", False)
    )
    out.append("            return 1;\n")
    out.append("        } else {\n")
    out.append(
        f'            luaL_error(L, "{label} field is not available in current SDK headers");\n'
    )
    out.append("            return 0;\n")
    out.append("        }\n")
    out.append("    }\n\n")
    out.append(f"    int {getter}(lua_State* L) {{\n")
    out.append(
        f'        if (lua_gettop(L) != 1) luaL_error(L, "{label} getter expected 1 arg");\n'
    )
    out.append(
        f'        auto self = luax::Usertype<{cxx_name(cls)}>::check(L, 1, "{label}");\n'
    )
    out.append(f"        return {getter_impl}(L, self);\n")
    out.append("    }\n\n")
    out.append(f"    template <class T>\n")
    out.append(
        f"    int {setter_impl}(lua_State* L, T* self, {arg_info.cxx_type} value) {{\n"
    )
    out.append(f"        if constexpr (requires(T* obj) {{ obj->{field.name}; }}) {{\n")
    out.append(
        f"            self->{field.name} = static_cast<decltype(self->{field.name})>(value);\n"
    )
    out.append("            return 0;\n")
    out.append("        } else {\n")
    out.append(
        f'            luaL_error(L, "{label} field is not available in current SDK headers");\n'
    )
    out.append("            return 0;\n")
    out.append("        }\n")
    out.append("    }\n\n")
    out.append(f"    int {setter}(lua_State* L) {{\n")
    out.append(
        f'        if (lua_gettop(L) != 2) luaL_error(L, "{label} setter expected 2 args");\n'
    )
    out.append(
        f'        auto self = luax::Usertype<{cxx_name(cls)}>::check(L, 1, "{label}");\n'
    )
    out.extend(check_arg(Arg(field.type, field.name), arg_info, 2, "value", label))
    out.append(f"        return {setter_impl}(L, self, value);\n")
    out.append("    }\n\n")
    out.append("    template <class T>\n")
    out.append(f"    void {register}(lua_State* L) {{\n")
    out.append(f"        if constexpr (requires(T* obj) {{ obj->{field.name}; }}) {{\n")
    out.append(
        f'            luax::Usertype<T>::field(L, "{field.name}", &{getter}, &{setter});\n'
    )
    out.append("        }\n")
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


def _binding_filename(cls_name: str) -> str:
    return f"bindings_{cls_name}.cpp"


def _emit_class_file(
    cls: Class,
    grouped: dict[str, list[Method]],
    hook_targets: List[tuple[Class, Method]],
    field_targets: List[tuple[Class, Field]],
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

    for _, field in field_targets:
        out.append(_emit_field_accessors(cls, field, objects))

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

    for _, field in field_targets:
        register = f"luaapi_{_id(cls.name)}_field_register_{_id(field.name)}"
        out.append(f"    {register}<{cxx_name(cls)}>(L);\n")

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


def _emit_common_file(
    emitted_classes: List[Class], plan: EmitPlan, target_platform: str
) -> str:
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
    ccobject = plan.objects.get("CCObject")
    release = None
    if ccobject:
        release = next(
            (m for m in ccobject.methods if m.name == "release" and not m.args), None
        )
    release_address = (
        hook_address_expr(ccobject, release, target_platform)
        if ccobject and release
        else ""
    )
    if release_address:
        out.append("void luaapi_fields_release_hook(cocos2d::CCObject* self) {\n")
        out.append("    luax::Fields::evictIfFinalRelease(self);\n")
        out.append("    self->release();\n")
        out.append("}\n\n")
        out.append("void installFieldsReleaseHook() {\n")
        out.append("    static bool attempted = false;\n")
        out.append("    static geode::Hook* hook = nullptr;\n")
        out.append("    if (attempted) return;\n")
        out.append("    attempted = true;\n")
        out.append(
            f'    auto result = geode::Mod::get()->hook({release_address}, &luaapi_fields_release_hook, "LuauAPI CCObject::release fields cleanup", tulip::hook::TulipConvention::Default);\n'
        )
        out.append("    if (result.isErr()) {\n")
        out.append(
            '        geode::log::warn("luau fields release hook failed: {}", result.unwrapErr());\n'
        )
        out.append("        return;\n")
        out.append("    }\n")
        out.append("    hook = result.unwrap();\n")
        out.append("    if (hook && !hook->isEnabled()) {\n")
        out.append("        auto enableResult = hook->enable();\n")
        out.append(
            '        if (enableResult.isErr()) geode::log::warn("luau fields release hook enable failed: {}", enableResult.unwrapErr());\n'
        )
        out.append("    }\n")
        out.append("}\n\n")
    else:
        out.append("void installFieldsReleaseHook() {}\n\n")

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
    out.append('    lua_pushcfunction(L, &luaapi_geode_skip, "geode.skip");\n')
    out.append('    lua_setfield(L, -2, "skip");\n')
    out.append('    lua_pushcfunction(L, &luaapi_geode_fields, "geode.fields");\n')
    out.append('    lua_setfield(L, -2, "fields");\n')
    out.append("    lua_pop(L, 1);\n")
    out.append("    installFieldsReleaseHook();\n")
    out.append(
        "    if (auto* runtime = static_cast<luax::Runtime*>(lua_callbacks(L)->userdata)) {\n"
    )
    out.append("        runtime->registerShutdownHook(&clearHookRegistry);\n")
    out.append("        runtime->registerShutdownHook(&luax::Fields::clear);\n")
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


def _free_fn_base(fn: Function) -> str:
    return f"luaapi_fn_{_id(fn.namespace)}_{_id(fn.name)}"


def free_function_supported(fn: Function, objects: Dict[str, Class]) -> bool:
    if classify_return(fn.ret, objects) is None:
        return False
    return all(classify_arg(arg.type, objects) is not None for arg in fn.args)


def _emit_free_invoke(fn: Function, objects: Dict[str, Class], suffix: str) -> str:
    cname = _free_fn_base(fn) + suffix
    label = f"{fn.lua_path}.{fn.name}"
    ret = require_classify_return(fn.ret, objects)
    arg_infos = [require_classify_arg(arg.type, objects) for arg in fn.args]

    out = [f"    int {cname}(lua_State* L) {{\n"]
    out.append(
        f'        if (lua_gettop(L) != {len(fn.args)}) luaL_error(L, "{label} expected {len(fn.args)} args");\n'
    )
    call_args: List[str] = []
    for arg_idx, (arg, info) in enumerate(zip(fn.args, arg_infos)):
        var = f"arg{arg_idx}"
        out.extend(check_arg(arg, info, arg_idx + 1, var, label))
        call_args.append(f"std::move({var})" if info.kind == "callback" else var)

    target = f"{fn.namespace}::{fn.name}({', '.join(call_args)})"
    if ret.kind == "void":
        out.append(f"        {target};\n")
        out.extend(push_return(ret, "", False))
    else:
        out.append(f"        auto result = {target};\n")
        out.extend(push_return(ret, "result", False))
    out.append("    }\n\n")
    return "".join(out)


def _emit_free_dispatcher(fns: List[Function], objects: Dict[str, Class]) -> str:
    if len(fns) == 1:
        return ""
    base = _free_fn_base(fns[0])
    label = f"{fns[0].lua_path}.{fns[0].name}"
    out = [f"    int {base}(lua_State* L) {{\n", "        switch (lua_gettop(L)) {\n"]
    for idx, fn in enumerate(fns):
        out.append(f"            case {len(fn.args)}: return {base}_{idx}(L);\n")
    out.append("            default: break;\n")
    out.append("        }\n")
    out.append(f'        luaL_error(L, "{label} unsupported overload arity");\n')
    out.append("    }\n\n")
    return "".join(out)


def emit_free_functions_file(
    functions: List[Function],
    objects: Dict[str, Class],
    target_platform: str = "win",
    skipped: list[tuple[str, str, str]] | None = None,
) -> str:
    by_key: dict[tuple[str, str], list[Function]] = defaultdict(list)
    for fn in functions:
        if not free_function_supported(fn, objects):
            continue
        if not free_fn_allowed(fn, target_platform):
            if skipped is not None:
                skipped.append(
                    (
                        fn.lua_path,
                        fn.name,
                        f"free-function-override-arity:{target_platform}",
                    )
                )
            continue
        by_key[(fn.namespace, fn.name)].append(fn)

    out = [
        file_preamble(),
        "#include <Geode/ui/Popup.hpp>\n",
        "#include <Geode/utils/general.hpp>\n\n",
        "namespace luauapi_gen::free_functions {\n\n",
        "namespace {\n\n",
    ]
    for fns in by_key.values():
        for idx, fn in enumerate(fns):
            suffix = "" if len(fns) == 1 else f"_{idx}"
            out.append(_emit_free_invoke(fn, objects, suffix))
        out.append(_emit_free_dispatcher(fns, objects))
    out.append("} // namespace\n\n")

    by_ns: dict[str, list[tuple[str, str]]] = defaultdict(list)
    for (_, name), fns in by_key.items():
        by_ns[fns[0].lua_path].append((name, _free_fn_base(fns[0])))

    out.append("geode::Result<void> bindFreeFunctions(lua_State* L) {\n")
    for lua_path, entries in by_ns.items():
        out.append(f'    luax::getOrCreateTable(L, "{lua_path}");\n')
        for name, base in entries:
            out.append(f'    lua_pushcfunction(L, &{base}, "{lua_path}.{name}");\n')
            out.append(f'    lua_setfield(L, -2, "{name}");\n')
        out.append("    lua_pop(L, 1);\n")
    out.append("    return geode::Ok();\n")
    out.append("}\n\n")
    out.append("} // namespace luauapi_gen::free_functions\n\n")
    out.append("namespace {\n")
    out.append(
        "    LUAX_BINDING_PRIORITY(GeneratedFreeFunctions, luauapi_gen::free_functions::bindFreeFunctions, 3)\n"
    )
    out.append("}\n")
    return "".join(out)


def emit(
    root: Root, target_platform: str = "win", plan: EmitPlan | None = None
) -> tuple[dict[str, str], list[tuple[str, str, str]]]:
    if plan is None:
        plan = collect_plan(root, target_platform)

    files: dict[str, str] = {
        "bindings_internal.hpp": emit_internal_hpp(),
        "bindings_common.cpp": _emit_common_file(
            plan.emitted_classes, plan, target_platform
        ),
        FREE_FUNCTIONS_FILE: emit_free_functions_file(
            root.functions, plan.objects, target_platform, plan.skipped
        ),
    }

    for cls in plan.emitted_classes:
        files[_binding_filename(cls.name)] = _emit_class_file(
            cls,
            plan.supported_by_class[cls.name],
            plan.hook_targets_by_class[cls.name],
            plan.field_targets_by_class.get(cls.name, []),
            plan.objects,
            plan.skipped_classes,
            plan.depths[cls.name],
            target_platform,
        )

    return files, plan.skipped
