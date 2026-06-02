from __future__ import annotations

from typing import TYPE_CHECKING, Dict, List, Set

if TYPE_CHECKING:
    from luau_codegen.model.codegen_context import CodegenContext

from luau_codegen.parse.broma import Arg, Class, Field, Method
from luau_codegen.policy.fields import bindable_field
from luau_codegen.policy.containers import _CONTAINER_KINDS
from luau_codegen.policy.filtering import call_label, returns_owned
from luau_codegen.emit.cxx_templates import file_preamble
from luau_codegen.emit.hooks import emit_hook_target, hook_id, hook_suffix
from luau_codegen.convert.marshalling import (
    check_arg,
    check_sel_handler,
    push_return,
    push_value,
    sel_call_args,
    sel_selector_call_arg,
)
from luau_codegen.convert.sel_args import LuaMethodArg, iter_lua_method_args
from luau_codegen.model.domain import cxx_name, lua_namespace, short_name
from luau_codegen.convert.type_map import (
    TypeInfo,
    method_input_arg_count,
    require_classify_arg,
    require_classify_return,
)
from luau_codegen.util.identifiers import cxx_id


def _classify_method_args(
    cls: Class,
    m: Method,
    objects: Dict[str, Class],
    ctx: CodegenContext | None = None,
) -> List[TypeInfo]:
    return [
        require_classify_arg(arg.type, objects, owner_class=cls.name, ctx=ctx)
        for arg in m.args
    ]


def _emit_vector_return_push(ret: TypeInfo, m: Method, expr: str) -> List[str]:
    if ret.kind != "vector_view":
        return push_return(ret, expr, returns_owned(m) or m.is_bound_ctor)
    if ret.is_ref and not m.is_static:
        return push_return(ret, expr, False, owner_expr="self")
    return push_return(ret, expr, False, vector_owned=True)


def _emit_vector_out_push(info: TypeInfo, var: str) -> List[str]:
    if info.kind == "vector_view":
        return push_value(info, var, False, vector_owned=True)
    return push_value(info, var, False)


def _gen_ns(cls: Class) -> str:
    return f"ns_{cxx_id(cls.name)}"


def _emit_trampoline_anchors(
    selector_handlers: List[tuple[str, str]],
    delegate_trampolines: List[str],
    ret: TypeInfo,
    m: Method,
) -> List[str]:
    if not selector_handlers and not delegate_trampolines:
        return []
    lines: List[str] = []

    def anchor_handler(handler: str, variant: str, anchor: str) -> str:
        if variant == "menu":
            return f"luax::anchorMenuHandler({anchor}, {handler});\n"
        return f"luax::anchorTrampoline({anchor}, {handler});\n"

    def orphan_handler(handler: str, variant: str) -> str:
        if variant == "menu":
            return f"luax::registerOrphanMenuHandler({handler});\n"
        return f"luax::registerOrphanTrampoline({handler});\n"

    if ret.kind == "object":
        for handler, variant in selector_handlers:
            lines.append(f"        {anchor_handler(handler, variant, 'result')}")
        for trampoline in delegate_trampolines:
            lines.append(f"        luax::anchorDelegate(result, {trampoline});\n")
    elif not m.is_static:
        for handler, variant in selector_handlers:
            lines.append(f"        {anchor_handler(handler, variant, 'self')}")
        for trampoline in delegate_trampolines:
            lines.append(f"        luax::anchorDelegate(self, {trampoline});\n")
    else:
        for handler, variant in selector_handlers:
            lines.append(f"        {orphan_handler(handler, variant)}")
        for trampoline in delegate_trampolines:
            lines.append(f"        luax::registerOrphanTrampoline({trampoline});\n")
    return lines


def _emit_ccnode_schedule(
    cls: Class,
    m: Method,
    label: str,
    lua_idx: int,
    sel_info: TypeInfo,
    remaining_call_args: List[str],
) -> List[str]:
    sel_var = "sel0"
    out = check_sel_handler(lua_idx, sel_var, sel_info, label)
    interval = remaining_call_args[0] if remaining_call_args else "0.f"
    out.append(
        "        cocos2d::CCDirector::sharedDirector()->getScheduler()->scheduleSelector("
        f"schedule_selector(luax::LuaScheduleHandler::onSchedule), {sel_var}_handler, {interval}, !self->isRunning());\n"
    )
    out.append(f"        luax::anchorTrampoline(self, {sel_var}_handler);\n")
    out.append("        return 0;\n")
    return out


def _emit_out_arg(info: TypeInfo, var: str) -> tuple[List[str], str]:
    if info.kind == "value" and info.lua_type == "UIButtonConfig":
        return [f"        UIButtonConfig {var}{{}};\n"], var
    if info.kind == "enum":
        return [f"        {info.cxx_type} {var}{{}};\n"], var
    if info.kind in _CONTAINER_KINDS:
        return [f"        {info.cxx_type} {var}{{}};\n"], (
            f"&{var}" if info.is_vector_ptr else var
        )
    return [f"        {info.cxx_type} {var}{{}};\n"], var


def _emit_lua_method_arg(
    lua_arg: LuaMethodArg,
    *,
    lua_idx: int,
    label: str,
    call_args: List[str],
    selector_handlers: List[tuple[str, str]],
    delegate_trampolines: List[str],
) -> tuple[List[str], int]:
    out: List[str] = []
    info = lua_arg.info
    arg_idx = lua_arg.arg_index
    var = f"arg{arg_idx}"

    if lua_arg.out_only:
        lines, call_expr = _emit_out_arg(info, var)
        out.extend(lines)
        call_args.append(call_expr)
        return out, lua_idx

    if lua_arg.orphan:
        sel_var = f"sel{arg_idx}"
        out.extend(check_sel_handler(lua_idx, sel_var, info, label))
        call_args.append(sel_selector_call_arg(info))
        selector_handlers.append((f"{sel_var}_handler", info.class_name or "menu"))
        return out, lua_idx + 1

    if lua_arg.sel_pair and not lua_arg.implicit_self_target:
        sel_var = f"sel{arg_idx}"
        out.extend(check_sel_handler(lua_idx, sel_var, info, label))
        call_args.extend(
            sel_call_args(sel_var, info, handler_first=lua_arg.handler_first)
        )
        selector_handlers.append((f"{sel_var}_handler", info.class_name or "menu"))
        return out, lua_idx + 1

    out.extend(check_arg(lua_arg.arg, info, lua_idx, var, label))
    if info.kind == "callback":
        call_args.append(f"std::move({var})")
    elif info.kind == "delegate":
        call_args.append(var)
        delegate_trampolines.append(f"{var}_trampoline")
    else:
        call_args.append(var)
    return out, lua_idx + 1


def _emit_invoke(
    cls: Class,
    m: Method,
    objects: Dict[str, Class],
    suffix: str,
    ctx: CodegenContext | None = None,
) -> str:
    fn = f"luaapi_{cxx_id(cls.name)}_{cxx_id(m.name)}{suffix}"
    label = call_label(cls, m)
    ret = require_classify_return(m.ret, objects, ctx=ctx)
    arg_infos = _classify_method_args(cls, m, objects, ctx=ctx)

    input_count = sum(
        1
        for lua_arg in iter_lua_method_args(
            m, arg_infos, ret_kind=ret.kind, is_instance=not m.is_static
        )
        if not lua_arg.out_only
    )

    out = [f"    int {fn}(lua_State* L) {{\n"]
    expected = input_count + (0 if m.is_static else 1)
    out.append(
        f'        if (lua_gettop(L) != {expected}) luaL_error(L, "{label} expected {expected} args");\n'
    )
    call_args: List[str] = []
    selector_handlers: List[tuple[str, str]] = []
    delegate_trampolines: List[str] = []
    if not m.is_static:
        out.append(
            f'        auto self = luax::Usertype<{cxx_name(cls)}>::check(L, 1, "{label}");\n'
        )
    lua_idx = 1 if m.is_static else 2
    for lua_arg in iter_lua_method_args(
        m, arg_infos, ret_kind=ret.kind, is_instance=not m.is_static
    ):
        if (
            not lua_arg.out_only
            and lua_arg.info.kind == "sel"
            and not m.is_static
            and cls.name == "CCNode"
            and m.name == "unschedule"
            and (lua_arg.info.class_name or "menu") == "schedule"
        ):
            sel_var = f"sel{lua_arg.arg_index}"
            out.extend(check_sel_handler(lua_idx, sel_var, lua_arg.info, label))
            call_args.append("schedule_selector(luax::LuaScheduleHandler::onSchedule)")
            selector_handlers.append((f"{sel_var}_handler", "schedule"))
            lua_idx += 1
            continue
        if (
            not lua_arg.out_only
            and lua_arg.info.kind == "sel"
            and not m.is_static
            and cls.name == "CCNode"
            and m.name in ("schedule", "scheduleOnce")
            and (lua_arg.info.class_name or "menu") == "schedule"
            and lua_arg.arg_index == 0
        ):
            remaining: List[str] = []
            temp_lua = lua_idx + 1
            for tail in iter_lua_method_args(
                m, arg_infos, ret_kind=ret.kind, is_instance=not m.is_static
            ):
                if tail.out_only or tail.arg_index <= lua_arg.arg_index:
                    continue
                if tail.sel_pair or tail.orphan:
                    break
                out.extend(
                    check_arg(
                        tail.arg, tail.info, temp_lua, f"arg{tail.arg_index}", label
                    )
                )
                remaining.append(f"arg{tail.arg_index}")
                temp_lua += 1
            out.extend(
                _emit_ccnode_schedule(cls, m, label, lua_idx, lua_arg.info, remaining)
            )
            out.append("    }\n\n")
            return "".join(out)
        lines, lua_idx = _emit_lua_method_arg(
            lua_arg,
            lua_idx=lua_idx,
            label=label,
            call_args=call_args,
            selector_handlers=selector_handlers,
            delegate_trampolines=delegate_trampolines,
        )
        out.extend(lines)

    args = ", ".join(call_args)
    if m.is_bound_ctor:
        target = f"new {cxx_name(cls)}({args})"
    elif m.is_static:
        target = f"{cxx_name(cls)}::{m.name}({args})"
    else:
        target = f"self->{m.name}({args})"
    out_refs = [
        (i, arg_infos[i])
        for i in range(len(m.args))
        if ret.kind == "void" and arg_infos[i].is_out
    ]
    if ret.kind == "void":
        out.append(f"        {target};\n")
        out.extend(
            _emit_trampoline_anchors(selector_handlers, delegate_trampolines, ret, m)
        )
        if out_refs:
            for i, oinfo in out_refs:
                out.extend(_emit_vector_out_push(oinfo, f"arg{i}"))
            out.append(f"        return {len(out_refs)};\n")
        else:
            out.extend(push_return(ret, "", False))
    else:
        out.append(f"        auto result = {target};\n")
        if m.is_bound_ctor:
            out.append("        result->autorelease();\n")
        out.extend(
            _emit_trampoline_anchors(selector_handlers, delegate_trampolines, ret, m)
        )
        out.extend(_emit_vector_return_push(ret, m, "result"))
    out.append("    }\n\n")
    return "".join(out)


def _emit_field_accessors(
    cls: Class,
    field: Field,
    objects: Dict[str, Class],
    ctx: CodegenContext | None = None,
) -> str:
    ok, reason, arg_info, ret_info = bindable_field(field, objects, cls, ctx=ctx)
    if not ok or not arg_info or not ret_info:
        raise ValueError(f"unsupported field {cls.name}.{field.name}: {reason}")
    label = f"{cls.name}.{field.name}"
    getter = f"luaapi_{cxx_id(cls.name)}_field_get_{cxx_id(field.name)}"
    setter = f"luaapi_{cxx_id(cls.name)}_field_set_{cxx_id(field.name)}"
    register = f"luaapi_{cxx_id(cls.name)}_field_register_{cxx_id(field.name)}"
    getter_impl = f"{getter}_impl"
    setter_impl = f"{setter}_impl"
    if ret_info.kind == "vector_view":
        out = [f"    template <class T>\n"]
        out.append(f"    int {getter_impl}(lua_State* L, T* self) {{\n")
        out.append(
            f"        if constexpr (requires(T* obj) {{ obj->{field.name}; }}) {{\n"
        )
        out.extend(
            f"    {line}"
            for line in push_value(
                ret_info, f"self->{field.name}", False, owner_expr="self"
            )
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
        out.append(f"    void {register}(lua_State* L) {{\n")
        out.append(
            f"        if constexpr (requires(T* obj) {{ obj->{field.name}; }}) {{\n"
        )
        out.append(
            f'            luax::Usertype<T>::readonlyField(L, "{field.name}", &{getter});\n'
        )
        out.append("        }\n")
        out.append("    }\n\n")
        return "".join(out)
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
    if arg_info.is_vector_ptr:
        out.append(f"            if (self->{field.name} == nullptr) {{\n")
        out.append(f'                luaL_error(L, "{label} field pointer is null");\n')
        out.append("            }\n")
        if arg_info.kind == "primitive_vector":
            out.append(
                f"            luax::assignPrimitiveVector(*self->{field.name}, std::move(value));\n"
            )
        else:
            out.append(f"            *self->{field.name} = std::move(value);\n")
    elif arg_info.kind == "primitive_vector":
        out.append(
            f"            luax::assignPrimitiveVector(self->{field.name}, std::move(value));\n"
        )
    elif arg_info.kind == "map":
        out.append(
            f"            luax::assignMap(self->{field.name}, std::move(value));\n"
        )
    elif arg_info.kind == "unordered_map":
        out.append(
            f"            luax::assignUnorderedMap(self->{field.name}, std::move(value));\n"
        )
    elif arg_info.kind == "set":
        out.append(
            f"            luax::assignSet(self->{field.name}, std::move(value));\n"
        )
    elif arg_info.kind == "unordered_set":
        out.append(
            f"            luax::assignUnorderedSet(self->{field.name}, std::move(value));\n"
        )
    else:
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


def _emit_dispatcher(
    cls: Class,
    name: str,
    methods: List[Method],
    objects: Dict[str, Class],
    ctx: CodegenContext | None = None,
) -> str:
    if len(methods) == 1:
        return ""
    first = methods[0]
    fn = f"luaapi_{cxx_id(cls.name)}_{cxx_id(name)}"
    out = [f"    int {fn}(lua_State* L) {{\n"]
    adjust = 0 if first.is_static else 1
    out.append(f"        switch (lua_gettop(L) - {adjust}) {{\n")
    for idx, m in enumerate(methods):
        out.append(
            f"            case {method_input_arg_count(m, objects, owner_class=cls.name, ctx=ctx)}: return luaapi_{cxx_id(cls.name)}_{cxx_id(name)}_{idx}(L);\n"
        )
    out.append("            default: break;\n")
    out.append("        }\n")
    out.append(
        f'        luaL_error(L, "{call_label(cls, first)} unsupported overload arity");\n'
    )
    out.append("    }\n\n")
    return "".join(out)


def _emit_class_file(
    cls: Class,
    grouped: dict[str, list[Method]],
    hook_targets: List[tuple[Class, Method]],
    field_targets: List[tuple[Class, Field]],
    objects: Dict[str, Class],
    skipped_classes: Set[str],
    depth: int,
    target_platform: str,
    ctx: CodegenContext | None = None,
) -> str:
    ns_name = cxx_id(cls.name)
    gen_ns = _gen_ns(cls)
    out = [file_preamble(), '#include "bindings_internal.hpp"\n\n']
    out.append(f"namespace luauapi_gen::{gen_ns} {{\n\n")
    out.append("namespace {\n\n")

    for methods in grouped.values():
        for idx, m in enumerate(methods):
            suffix = "" if len(methods) == 1 else f"_{idx}"
            out.append(_emit_invoke(cls, m, objects, suffix, ctx=ctx))
        out.append(_emit_dispatcher(cls, methods[0].name, methods, objects, ctx=ctx))

    for _, field in field_targets:
        out.append(_emit_field_accessors(cls, field, objects, ctx=ctx))

    for _, m in hook_targets:
        out.append(emit_hook_target(cls, m, objects, target_platform, ctx=ctx))

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
        fn = f"luaapi_{cxx_id(cls.name)}_{cxx_id(name)}"
        out.append(
            f'    luax::Usertype<{cxx_name(cls)}>::method(L, "{name}", &{fn});\n'
        )

    for _, field in field_targets:
        register = f"luaapi_{cxx_id(cls.name)}_field_register_{cxx_id(field.name)}"
        out.append(f"    {register}<{cxx_name(cls)}>(L);\n")

    static_methods = [
        (name, methods) for name, methods in grouped.items() if methods[0].is_static
    ]
    if static_methods:
        ns = lua_namespace(cls)
        out.append(f'\n    luax::getOrCreateTable(L, "{ns}");\n')
        out.append(f"    lua_createtable(L, 0, {len(static_methods)});\n")
        for name, methods in static_methods:
            fn = f"luaapi_{cxx_id(cls.name)}_{cxx_id(name)}"
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
