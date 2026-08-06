from __future__ import annotations

from pathlib import Path
from typing import TYPE_CHECKING

from luau_codegen.cli.io import _write_if_changed
from luau_codegen.model.value_types import (
    CocosValueStructDescriptor,
    PushFieldDescriptor,
    _check_lines,
)

if TYPE_CHECKING:
    from luau_codegen.convert.type_primitives import TypeInfo
    from luau_codegen.model.codegen_context import CodegenContext

TYPES_GEN_REL_PATH = "src/framework/stack/Types.generated.hpp"
TYPES_GEN_CONTAINERS_REL_PATH = "src/framework/stack/Types.generated.containers.hpp"


def types_gen_rel_path() -> str:
    return TYPES_GEN_REL_PATH


def types_gen_containers_rel_path() -> str:
    return TYPES_GEN_CONTAINERS_REL_PATH


def _needs_deferred_emission(desc: CocosValueStructDescriptor, ctx: CodegenContext) -> bool:
    return desc.cxx_type in ctx.value_types.deferred_cxx_types


def emit_value_local_decl(info: TypeInfo, name: str) -> str:
    if info.value_deferred_init:
        return f"{info.cxx_type} {name};"
    return f"{info.cxx_type} {name}{{}};"


def emit_value_default_expr(info: TypeInfo) -> str:
    if info.value_deferred_init:
        return f"{info.cxx_type}()"
    return f"{info.cxx_type}{{}}"


def _emit_check_struct(desc: CocosValueStructDescriptor) -> str:
    body = "".join(line for field in desc.check_fields for line in _check_lines(field))
    return (
        f"    template <>\n"
        f"    inline {desc.cxx_type} check<{desc.cxx_type}>(lua_State* L, int idx, char const* method) {{\n"
        f"        {desc.cxx_type} value;\n"
        f"{body}"
        f"        return value;\n"
        f"    }}\n"
    )


def _emit_ccrect_check() -> str:
    return (
        "    template <>\n"
        "    inline cocos2d::CCRect check<cocos2d::CCRect>(lua_State* L, int idx, char const* method) {\n"
        "        return {\n"
        '            {fieldNumber(L, idx, "x", method), fieldNumber(L, idx, "y", method)},\n'
        '            {fieldNumber(L, idx, "width", method), fieldNumber(L, idx, "height", method)},\n'
        "        };\n"
        "    }\n"
    )


def _emit_push_field(field: PushFieldDescriptor) -> str:
    if field.kind == "number":
        if field.name in ("src", "dst"):
            return (
                f"        lua_pushnumber(L, static_cast<double>({field.member}));\n"
                f'        lua_setfield(L, -2, "{field.name}");\n'
            )
        return (
            f"        lua_pushnumber(L, {field.member});\n"
            f'        lua_setfield(L, -2, "{field.name}");\n'
        )
    if field.kind == "integer":
        return (
            f"        lua_pushinteger(L, {field.member});\n"
            f'        lua_setfield(L, -2, "{field.name}");\n'
        )
    if field.kind == "bool":
        return (
            f"        luax::push(L, {field.member});\n"
            f'        lua_setfield(L, -2, "{field.name}");\n'
        )
    if field.kind == "string":
        return (
            f"        push(L, std::string({field.member}));\n"
            f'        lua_setfield(L, -2, "{field.name}");\n'
        )
    if field.kind == "enum":
        return (
            f"        lua_pushnumber(L, static_cast<double>(std::to_underlying({field.member})));\n"
            f'        lua_setfield(L, -2, "{field.name}");\n'
        )
    if field.kind == "object_nullable":
        obj = field.cxx_type or (field.nested_type or "")
        return (
            f"        if ({field.member} == nullptr) {{\n"
            f"            lua_pushnil(L);\n"
            f"        }} else {{\n"
            f"            luax::Usertype<{obj}>::pushBorrowed(L, {field.member});\n"
            f"        }}\n"
            f'        lua_setfield(L, -2, "{field.name}");\n'
        )
    if field.kind == "opaque_nullable":
        return (
            f"        if ({field.member} == nullptr) {{\n"
            f"            lua_pushnil(L);\n"
            f"        }} else {{\n"
            f"            luax::pushOpaqueHandle(L, {field.member});\n"
            f"        }}\n"
            f'        lua_setfield(L, -2, "{field.name}");\n'
        )
    if field.kind == "nested":
        if field.nested_type is None:
            raise ValueError(f"nested push field requires nested_type: {field.name}")
        return f'        push(L, {field.member});\n        lua_setfield(L, -2, "{field.name}");\n'
    if field.kind == "container":
        return "".join(field.push_override)
    raise ValueError(f"unsupported push field kind: {field.kind}")


def _emit_push_struct(desc: CocosValueStructDescriptor, param_name: str) -> str:
    body = "".join(_emit_push_field(field) for field in desc.push_fields)
    return (
        f"    inline void push(lua_State* L, {desc.cxx_type} const& {param_name}) {{\n"
        f"        lua_createtable(L, 0, {desc.push_capacity});\n"
        f"{body}"
        "    }\n"
    )


def _emit_ccrect_push() -> str:
    return (
        "    inline void push(lua_State* L, cocos2d::CCRect const& rect) {\n"
        "        lua_createtable(L, 0, 2);\n"
        "        push(L, rect.origin);\n"
        '        lua_setfield(L, -2, "origin");\n'
        "        push(L, rect.size);\n"
        '        lua_setfield(L, -2, "size");\n'
        "    }\n"
    )


def _types_preamble() -> list[str]:
    return [
        "#pragma once\n",
        "\n",
        "#include <cocos2d.h>\n",
        "#include <lua.h>\n",
        "#include <utility>\n",
        "\n",
        "namespace luax {\n",
    ]


def emit_types_generated_hpp(ctx: CodegenContext) -> str:
    structs = [
        desc for desc in ctx.value_types.cocos_structs if not _needs_deferred_emission(desc, ctx)
    ]
    emit_ccrect = any(spec.cocos_emit == "ccrect" for spec in ctx.value_types.specs)
    lines = _types_preamble()
    for desc in structs:
        lines.append("\n")
        lines.append(_emit_check_struct(desc))
    if emit_ccrect:
        lines.append("\n")
        lines.append(_emit_ccrect_check())
    lines.append("\n")
    for desc in structs:
        param = desc.push_fields[0].member.split(".", 1)[0]
        lines.append(_emit_push_struct(desc, param))
    if emit_ccrect:
        lines.append("\n")
        lines.append(_emit_ccrect_push())
    lines.append("} // namespace luax\n")
    return "".join(lines)


def emit_types_generated_containers_hpp(ctx: CodegenContext) -> str:
    structs = [
        desc for desc in ctx.value_types.cocos_structs if _needs_deferred_emission(desc, ctx)
    ]
    lines = _types_preamble()
    for desc in structs:
        lines.append("\n")
        lines.append(_emit_check_struct(desc))
    lines.append("\n")
    for desc in structs:
        param = desc.push_fields[0].member.split(".", 1)[0]
        lines.append(_emit_push_struct(desc, param))
    lines.append("} // namespace luax\n")
    return "".join(lines)


def write_types_generated(gen_out: Path | str, ctx: CodegenContext) -> str:
    base_path = Path(gen_out) / TYPES_GEN_REL_PATH
    containers_path = Path(gen_out) / TYPES_GEN_CONTAINERS_REL_PATH
    _write_if_changed(str(base_path), emit_types_generated_hpp(ctx))
    _write_if_changed(str(containers_path), emit_types_generated_containers_hpp(ctx))
    return str(base_path)
