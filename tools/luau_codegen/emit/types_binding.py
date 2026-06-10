from __future__ import annotations

from pathlib import Path

from luau_codegen.cli.io import _write_if_changed
from luau_codegen.model.cocos_value_types import (
    COCOS_VALUE_STRUCTS,
    CocosValueStructDescriptor,
    PushFieldDescriptor,
    _check_expr,
)

TYPES_GEN_REL_PATH = "src/framework/stack/Types.generated.hpp"


def types_gen_rel_path() -> str:
    return TYPES_GEN_REL_PATH


def _emit_check_struct(desc: CocosValueStructDescriptor) -> str:
    fields = ", ".join(_check_expr(field) for field in desc.check_fields)
    return (
        f"    template <>\n"
        f"    inline {desc.cxx_type} check<{desc.cxx_type}>(lua_State* L, int idx, char const* method) {{\n"
        f"        return {{{fields}}};\n"
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
    if field.kind == "nested":
        if field.nested_type is None:
            raise ValueError(f"nested push field requires nested_type: {field.name}")
        return f'        push(L, {field.member});\n        lua_setfield(L, -2, "{field.name}");\n'
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


def emit_types_generated_hpp() -> str:
    lines = [
        "#pragma once\n",
        "\n",
        "#include <cocos2d.h>\n",
        "#include <lua.h>\n",
        "\n",
        "namespace luax {\n",
    ]
    for desc in COCOS_VALUE_STRUCTS:
        lines.append("\n")
        lines.append(_emit_check_struct(desc))
    lines.append("\n")
    lines.append(_emit_ccrect_check())
    lines.append("\n")
    for desc in COCOS_VALUE_STRUCTS:
        param = desc.push_fields[0].member.split(".", 1)[0]
        lines.append(_emit_push_struct(desc, param))
    lines.append("\n")
    lines.append(_emit_ccrect_push())
    lines.append("} // namespace luax\n")
    return "".join(lines)


def write_types_generated(gen_out: Path | str) -> str:
    content = emit_types_generated_hpp()
    out_path = Path(gen_out) / TYPES_GEN_REL_PATH
    _write_if_changed(str(out_path), content)
    return str(out_path)
