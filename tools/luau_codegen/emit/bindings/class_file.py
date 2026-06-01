from __future__ import annotations

from typing import Dict, List, Set

from luau_codegen.parse.broma import Arg, Class, Field, Method
from luau_codegen.policy.fields import bindable_field
from luau_codegen.policy.filtering import call_label, returns_owned
from luau_codegen.emit.cxx_templates import file_preamble
from luau_codegen.emit.hooks import emit_hook_target, hook_id, hook_suffix
from luau_codegen.convert.marshalling import check_arg, push_return, push_value
from luau_codegen.model.domain import cxx_name, lua_namespace, short_name
from luau_codegen.convert.type_map import (
    TypeInfo,
    method_input_arg_count,
    require_classify_arg,
    require_classify_return,
)
from luau_codegen.util.identifiers import cxx_id


def _classify_method_args(m: Method, objects: Dict[str, Class]) -> List[TypeInfo]:
    return [require_classify_arg(arg.type, objects) for arg in m.args]


def _gen_ns(cls: Class) -> str:
    return f"ns_{cxx_id(cls.name)}"


def _emit_invoke(cls: Class, m: Method, objects: Dict[str, Class], suffix: str) -> str:
    fn = f"luaapi_{cxx_id(cls.name)}_{cxx_id(m.name)}{suffix}"
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
    getter = f"luaapi_{cxx_id(cls.name)}_field_get_{cxx_id(field.name)}"
    setter = f"luaapi_{cxx_id(cls.name)}_field_set_{cxx_id(field.name)}"
    register = f"luaapi_{cxx_id(cls.name)}_field_register_{cxx_id(field.name)}"
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


def _emit_dispatcher(
    cls: Class, name: str, methods: List[Method], objects: Dict[str, Class]
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
            f"            case {method_input_arg_count(m, objects)}: return luaapi_{cxx_id(cls.name)}_{cxx_id(name)}_{idx}(L);\n"
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
) -> str:
    ns_name = cxx_id(cls.name)
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
