from __future__ import annotations

import dataclasses
from typing import TYPE_CHECKING, Dict, List, Set

if TYPE_CHECKING:
    from luau_codegen.model.codegen_context import CodegenContext

from luau_codegen.parse.broma import Arg, Class, Field, Method
from luau_codegen.policy.fields import bindable_field
from luau_codegen.policy.containers import _CONTAINER_KINDS
from luau_codegen.policy.filtering import call_label, returns_owned
from luau_codegen.emit.cxx_templates import file_preamble
from luau_codegen.emit.bindings.dispatch import emit_arity_dispatcher
from luau_codegen.emit.hooks import emit_hook_target, hook_id, hook_suffix
from luau_codegen.emit.bindings.invoke_common import (
    emit_invoke_return_tail,
    emit_invoke_void_tail,
    emit_lua_invoke_arg,
)
from luau_codegen.convert.marshalling import (
    check_arg,
    check_sel_handler,
    push_value,
    _map_push_call,
    _nested_object_pointee_cxx,
    _set_push_call,
)
from luau_codegen.convert.sel_args import iter_lua_method_args
from luau_codegen.model.domain import cxx_name, lua_namespace, short_name
from luau_codegen.convert.type_map import (
    TypeInfo,
    method_input_arg_count,
    require_classify_arg,
    require_classify_return,
)
from luau_codegen.util import cxx_id
from luau_codegen.emit.types_binding import _deferred_cxx_types
from luau_codegen.emit.luau_types.method_types import lua_export_name


def _classify_method_args(
    cls: Class,
    m: Method,
    objects: Dict[str, Class],
    ctx: CodegenContext | None = None,
) -> List[TypeInfo]:
    return [
        require_classify_arg(arg.type, objects, owner_class=cls.name, ctx=ctx) for arg in m.args
    ]


def _gen_ns(cls: Class) -> str:
    return f"ns_{cxx_id(cls.name)}"


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
    selector = "schedule_selector(luax::LuaScheduleHandler::onSchedule)"
    paused = "!self->isRunning()"
    if m.name == "scheduleOnce":
        if len(remaining_call_args) > 1:
            raise ValueError(f"{label} has unsupported scheduleOnce tail args")
        delay = remaining_call_args[0] if remaining_call_args else "0.f"
        out.append(
            "        cocos2d::CCDirector::sharedDirector()->getScheduler()->scheduleSelector("
            f"{selector}, {sel_var}_handler, 0.f, 0u, {delay}, {paused});\n"
        )
    elif len(remaining_call_args) == 3:
        interval, repeat, delay = remaining_call_args[:3]
        out.append(
            "        cocos2d::CCDirector::sharedDirector()->getScheduler()->scheduleSelector("
            f"{selector}, {sel_var}_handler, {interval}, {repeat}, {delay}, {paused});\n"
        )
    elif len(remaining_call_args) <= 1:
        interval = remaining_call_args[0] if remaining_call_args else "0.f"
        out.append(
            "        cocos2d::CCDirector::sharedDirector()->getScheduler()->scheduleSelector("
            f"{selector}, {sel_var}_handler, {interval}, {paused});\n"
        )
    else:
        raise ValueError(f"{label} has unsupported schedule tail args")
    out.append(f"        luax::anchorTrampoline(self, {sel_var}_handler);\n")
    out.append("        return 0;\n")
    return out


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
    out.append(
        f'        auto const boundary = luax::diag::recordBindingEntry(L, "{label}", luax::diag::BoundaryKind::GeneratedBinding);\n'
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
                out.extend(check_arg(tail.arg, tail.info, temp_lua, f"arg{tail.arg_index}", label))
                remaining.append(f"arg{tail.arg_index}")
                temp_lua += 1
            out.extend(_emit_ccnode_schedule(cls, m, label, lua_idx, lua_arg.info, remaining))
            out.append("    }\n\n")
            return "".join(out)
        lines, lua_idx = emit_lua_invoke_arg(
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
    out_refs = [(i, arg_infos[i]) for i in range(len(m.args)) if arg_infos[i].is_out]
    if ret.kind == "void":
        out.extend(
            emit_invoke_void_tail(
                target,
                ret=ret,
                out_refs=out_refs,
                selector_handlers=selector_handlers,
                delegate_trampolines=delegate_trampolines,
                is_static=m.is_static,
            )
        )
    else:
        ref_owner = "self" if ret.kind == "vector_view" and ret.is_ref and not m.is_static else None
        out.extend(
            emit_invoke_return_tail(
                target,
                ret=ret,
                selector_handlers=selector_handlers,
                delegate_trampolines=delegate_trampolines,
                is_static=m.is_static,
                owned_return=returns_owned(m) or m.is_bound_ctor,
                ref_owner_expr=ref_owner,
                pre_push_lines=["        result->autorelease();\n"] if m.is_bound_ctor else None,
                out_refs=out_refs,
            )
        )
    out.append("    }\n\n")
    return "".join(out)


_CONTAINER_FIELD_ASSIGN_FNS = {
    "primitive_vector": "assignPrimitiveVector",
    "map_vector": "assignPrimitiveVector",
    "map": "detail::assignAssociativeMap",
    "unordered_map": "detail::assignAssociativeMap",
    "set": "detail::assignSetContainer",
    "unordered_set": "detail::assignSetContainer",
}


def _nested_field_assign_fn(info: TypeInfo) -> str | None:
    if info.kind == "nested_bool_vector_view":
        if info.element_type is None or info.element_type.element_type is None:
            raise ValueError("nested bool vector requires inner element type")
        elem = info.element_type.element_type.cxx_type
        return f"assignNestedPrimitiveVectorPointers<{elem}>"
    if info.kind == "nested_object_vector_view":
        pointee = _nested_object_pointee_cxx(info)
        return f"assignNestedObjectVectorPointers<{pointee}>"
    if info.kind == "nested_object_grid_view":
        pointee = _nested_object_pointee_cxx(info)
        return f"assignNestedObjectGridPointers<{pointee}>"
    return None


def _container_field_assign_fn(info: TypeInfo) -> str | None:
    if info.kind in ("map", "unordered_map") and info.key_type is not None:
        if info.key_type.kind == "pair":
            return "detail::assignPairKeyAssociativeMap"
    return _CONTAINER_FIELD_ASSIGN_FNS.get(info.kind)


def _field_push_info(arg_info: TypeInfo, ret_info: TypeInfo) -> TypeInfo:
    if arg_info.is_vector_ptr and ret_info.kind in _CONTAINER_KINDS:
        return dataclasses.replace(ret_info, is_vector_ptr=True)
    return ret_info


def _emit_std_array_field_assign_lines(field: Field, label: str, info: TypeInfo) -> list[str]:
    if info.element_type is None or info.array_size <= 0:
        raise ValueError("std array field requires element type and size")
    elem = info.element_type.cxx_type
    size = info.array_size
    name = field.name
    return [
        "            if constexpr (std::is_pointer_v<std::remove_reference_t<decltype("
        f"self->{name})>>) {{\n",
        f"                if (self->{name} == nullptr) {{\n",
        f'                    luaL_error(L, "{label} field pointer is null");\n',
        "                }\n",
        f"                luax::assignStdArray<{elem}, {size}>(*self->{name}, std::move(value));\n",
        "            } else {\n",
        f"                luax::assignStdArray<{elem}, {size}>(self->{name}, std::move(value));\n",
        "            }\n",
    ]


def _emit_nested_field_assign_lines(field: Field, label: str, info: TypeInfo) -> list[str]:
    fn = _nested_field_assign_fn(info)
    if fn is None:
        raise ValueError(f"unsupported nested field assign: {info.kind}")
    name = field.name
    return [
        "            if constexpr (std::is_pointer_v<std::remove_reference_t<decltype("
        f"self->{name})>>) {{\n",
        f"                if (self->{name} == nullptr) {{\n",
        f'                    luaL_error(L, "{label} field pointer is null");\n',
        "                }\n",
        f"                luax::{fn}(*self->{name}, value);\n",
        "            } else {\n",
        f"                luax::{fn}(self->{name}, value);\n",
        "            }\n",
    ]


def _emit_container_field_assign_lines(field: Field, label: str, info: TypeInfo) -> list[str]:
    fn = _container_field_assign_fn(info)
    if fn is None:
        raise ValueError(f"unsupported container field assign: {info.kind}")
    name = field.name
    return [
        "            if constexpr (std::is_pointer_v<std::remove_reference_t<decltype("
        f"self->{name})>>) {{\n",
        f"                if (self->{name} == nullptr) {{\n",
        f'                    luaL_error(L, "{label} field pointer is null");\n',
        "                }\n",
        f"                luax::{fn}(*self->{name}, value);\n",
        "            } else {\n",
        f"                luax::{fn}(self->{name}, value);\n",
        "            }\n",
    ]


def _emit_container_field_push_lines(field: Field, info: TypeInfo) -> list[str]:
    name = field.name
    expr = f"self->{name}"
    if info.kind in ("set", "unordered_set"):
        value_fn = _set_push_call(dataclasses.replace(info, is_vector_ptr=False))
        ptr_fn = _set_push_call(dataclasses.replace(info, is_vector_ptr=True))
    elif info.kind in ("map", "unordered_map"):
        value_fn = _map_push_call(dataclasses.replace(info, is_vector_ptr=False))
        ptr_fn = _map_push_call(dataclasses.replace(info, is_vector_ptr=True))
    else:
        raise ValueError(f"unsupported container field push: {info.kind}")
    return [
        "            if constexpr (std::is_pointer_v<std::remove_reference_t<decltype("
        f"self->{name})>>) {{\n",
        f"                luax::{ptr_fn}(L, {expr});\n",
        "            } else {\n",
        f"                luax::{value_fn}(L, {expr});\n",
        "            }\n",
    ]


def _emit_field_push_lines(field: Field, arg_info: TypeInfo, ret_info: TypeInfo) -> list[str]:
    if ret_info.kind in ("map", "unordered_map", "set", "unordered_set"):
        return _emit_container_field_push_lines(field, ret_info)
    push_info = _field_push_info(arg_info, ret_info)
    return push_value(push_info, f"self->{field.name}", False)


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
    if ret_info.kind in (
        "vector_view",
        "nested_primitive_vector_view",
        "cc_c_array_view",
    ):
        out = ["    template <class T>\n"]
        out.append(f"    int {getter_impl}(lua_State* L, T* self) {{\n")
        out.append(f"        if constexpr (requires(T* obj) {{ obj->{field.name}; }}) {{\n")
        out.extend(
            f"    {line}"
            for line in push_value(
                _field_push_info(arg_info, ret_info),
                f"self->{field.name}",
                False,
                owner_expr="self",
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
        out.append("    template <class T>\n")
        out.append(f"    void {register}(lua_State* L) {{\n")
        out.append(f"        if constexpr (requires(T* obj) {{ obj->{field.name}; }}) {{\n")
        out.append(f'            luax::Usertype<T>::readonlyField(L, "{field.name}", &{getter});\n')
        out.append("        }\n")
        out.append("    }\n\n")
        return "".join(out)
    if ret_info.kind in ("nested_object_vector_view", "nested_object_grid_view"):
        out = ["    template <class T>\n"]
        out.append(f"    int {getter_impl}(lua_State* L, T* self) {{\n")
        out.append(f"        if constexpr (requires(T* obj) {{ obj->{field.name}; }}) {{\n")
        out.extend(
            f"    {line}"
            for line in push_value(
                _field_push_info(arg_info, ret_info),
                f"self->{field.name}",
                False,
                owner_expr="self",
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
        out.append("    template <class T>\n")
        out.append(f"    int {setter_impl}(lua_State* L, T* self, {arg_info.cxx_type} value) {{\n")
        out.append(f"        if constexpr (requires(T* obj) {{ obj->{field.name}; }}) {{\n")
        out.extend(_emit_nested_field_assign_lines(field, label, arg_info))
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
    out = ["    template <class T>\n"]
    out.append(f"    int {getter_impl}(lua_State* L, T* self) {{\n")
    out.append(f"        if constexpr (requires(T* obj) {{ obj->{field.name}; }}) {{\n")
    out.extend(f"    {line}" for line in _emit_field_push_lines(field, arg_info, ret_info))
    out.append("            return 1;\n")
    out.append("        } else {\n")
    out.append(
        f'            luaL_error(L, "{label} field is not available in current SDK headers");\n'
    )
    out.append("            return 0;\n")
    out.append("        }\n")
    out.append("    }\n\n")
    out.append(f"    int {getter}(lua_State* L) {{\n")
    out.append(f'        if (lua_gettop(L) != 1) luaL_error(L, "{label} getter expected 1 arg");\n')
    out.append(f'        auto self = luax::Usertype<{cxx_name(cls)}>::check(L, 1, "{label}");\n')
    out.append(f"        return {getter_impl}(L, self);\n")
    out.append("    }\n\n")
    setter_value_type = "int" if arg_info.kind == "seed_value" else arg_info.cxx_type
    out.append("    template <class T>\n")
    out.append(f"    int {setter_impl}(lua_State* L, T* self, {setter_value_type} value) {{\n")
    out.append(f"        if constexpr (requires(T* obj) {{ obj->{field.name}; }}) {{\n")
    if arg_info.kind == "std_array":
        out.extend(_emit_std_array_field_assign_lines(field, label, arg_info))
    elif arg_info.kind == "seed_value":
        out.append(f"            self->{field.name} = value;\n")
    elif _container_field_assign_fn(arg_info) is not None:
        out.extend(_emit_container_field_assign_lines(field, label, arg_info))
    elif _nested_field_assign_fn(arg_info) is not None:
        out.extend(_emit_nested_field_assign_lines(field, label, arg_info))
    elif arg_info.is_vector_ptr:
        out.append(f"            if (self->{field.name} == nullptr) {{\n")
        out.append(f'                luaL_error(L, "{label} field pointer is null");\n')
        out.append("            }\n")
        out.append(f"            *self->{field.name} = std::move(value);\n")
    elif arg_info.kind == "value" and arg_info.cxx_type in _deferred_cxx_types():
        out.append(f"            luax::assignValue(self->{field.name}, std::move(value));\n")
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
    out.append(f'        auto self = luax::Usertype<{cxx_name(cls)}>::check(L, 1, "{label}");\n')
    out.extend(check_arg(Arg(field.type, field.name), arg_info, 2, "value", label))
    out.append(f"        return {setter_impl}(L, self, value);\n")
    out.append("    }\n\n")
    out.append("    template <class T>\n")
    out.append(f"    void {register}(lua_State* L) {{\n")
    out.append(f"        if constexpr (requires(T* obj) {{ obj->{field.name}; }}) {{\n")
    out.append(f'            luax::Usertype<T>::field(L, "{field.name}", &{getter}, &{setter});\n')
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
    adjust = 0 if first.is_static else 1
    cases = [
        (
            method_input_arg_count(m, objects, owner_class=cls.name, ctx=ctx),
            f"luaapi_{cxx_id(cls.name)}_{cxx_id(name)}_{idx}",
        )
        for idx, m in enumerate(methods)
    ]
    return emit_arity_dispatcher(fn, f"lua_gettop(L) - {adjust}", cases, call_label(cls, first))


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
    emitted_class_names: Set[str] | None = None,
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
        if not base_cls:
            continue
        if emitted_class_names is not None:
            if base_cls.name not in emitted_class_names:
                continue
        elif base_cls.name in skipped_classes:
            continue
        bases.append(f"luax::Usertype<{cxx_name(base_cls)}>::tag()")
    base_expr = "{ " + ", ".join(bases) + " }" if bases else "{}"
    out.append(
        f'    auto registerResult = luax::Usertype<{cxx_name(cls)}>::registerType(L, "{cls.name}", {base_expr});\n'
    )
    out.append("    if (registerResult.isErr()) return geode::Err(registerResult.unwrapErr());\n")

    for name, methods in grouped.items():
        if methods[0].is_static:
            continue
        lua_name = lua_export_name(name, grouped)
        if lua_name is None:
            continue
        fn = f"luaapi_{cxx_id(cls.name)}_{cxx_id(name)}"
        out.append(f'    luax::Usertype<{cxx_name(cls)}>::method(L, "{lua_name}", &{fn});\n')

    for _, field in field_targets:
        register = f"luaapi_{cxx_id(cls.name)}_field_register_{cxx_id(field.name)}"
        out.append(f"    {register}<{cxx_name(cls)}>(L);\n")

    static_methods = [
        (name, lua_name, methods)
        for name, methods in grouped.items()
        if methods[0].is_static and (lua_name := lua_export_name(name, grouped)) is not None
    ]
    if static_methods:
        ns = lua_namespace(cls)
        out.append(f'\n    luax::getOrCreateTable(L, "{ns}");\n')
        out.append(f"    lua_createtable(L, 0, {len(static_methods)});\n")
        for cpp_name, lua_name, methods in static_methods:
            fn = f"luaapi_{cxx_id(cls.name)}_{cxx_id(cpp_name)}"
            out.append(f'    lua_pushcfunction(L, &{fn}, "{cls.name}.{cpp_name}");\n')
            out.append(f'    lua_setfield(L, -2, "{lua_name}");\n')
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
