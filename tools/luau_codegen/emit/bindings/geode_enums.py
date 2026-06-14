from __future__ import annotations

from luau_codegen.model.codegen_context import CodegenContext
from luau_codegen.emit.cxx_templates import file_preamble
from luau_codegen.util.identifiers import cxx_id

GEODE_ENUMS_MANIFEST = "bindings/geode/GeodeEnums.manifest.hpp"
GEODE_ENUMS_BINDING = "bindings_geode_enums.cpp"

_INT_ENUM_ENTRY = "#define LUAX_INT_ENUM_ENTRY(name, value) {#name, value},\n"


def _sorted_enum_members(
    members: dict[str, tuple[tuple[str, int], ...]],
) -> list[tuple[str, tuple[tuple[str, int], ...]]]:
    return sorted(
        [(name, entries) for name, entries in members.items() if entries],
        key=lambda item: item[0],
    )


def _emit_enum_macro_lines(
    prefix: str, enum_name: str, entries: tuple[tuple[str, int], ...]
) -> list[str]:
    macro = f"{prefix}_{cxx_id(enum_name)}"
    lines = [f"#define {macro}(X)"]
    if not entries:
        lines.append("\n")
        return lines
    for index, (member, value) in enumerate(entries):
        suffix = " \\\n" if index < len(entries) - 1 else "\n"
        lines.append(
            f" \\\n    X({member}, {value})" if index == 0 else f"    X({member}, {value})"
        )
        lines.append(suffix)
    return lines


def emit_geode_enums_manifest(ctx: CodegenContext) -> str:
    gd_enums = _sorted_enum_members(dict(ctx.gd_enum_members))
    geode_enums = _sorted_enum_members(dict(ctx.geode_enum_members))
    lines = ["#pragma once\n", "\n", _INT_ENUM_ENTRY, "\n"]
    for enum_name, entries in gd_enums:
        lines.extend(_emit_enum_macro_lines("LUAX_GD_ENUM", enum_name, entries))
        lines.append("\n")
    for enum_name, entries in geode_enums:
        lines.extend(_emit_enum_macro_lines("LUAX_GEODE_ENUM", enum_name, entries))
        lines.append("\n")
    return "".join(lines)


def _emit_entries_array(macro_prefix: str, array_prefix: str, enum_name: str) -> str:
    macro = f"{macro_prefix}_{cxx_id(enum_name)}"
    return (
        f"        constexpr IntEnumEntry k{array_prefix}_{cxx_id(enum_name)}Entries[] = {{\n"
        f"            {macro}(LUAX_INT_ENUM_ENTRY)\n"
        f"        }};\n"
    )


def emit_geode_enums_binding(ctx: CodegenContext) -> str:
    gd_enums = _sorted_enum_members(dict(ctx.gd_enum_members))
    geode_enums = _sorted_enum_members(dict(ctx.geode_enum_members))
    out = [
        file_preamble(),
        '#include "bindings/geode/GeodeEnums.manifest.hpp"\n',
        '#include "framework/Binding.hpp"\n',
        '#include "framework/stack/TableUtil.hpp"\n\n',
        "#include <cstddef>\n",
        "#include <iterator>\n",
        "#include <lua.h>\n",
        "#include <lualib.h>\n\n",
        "namespace {\n",
        "    using namespace luax;\n\n",
        "    struct IntEnumEntry {\n",
        "        char const* name;\n",
        "        int value;\n",
        "    };\n\n",
    ]
    for enum_name, _ in gd_enums:
        out.append(_emit_entries_array("LUAX_GD_ENUM", "GdEnum", enum_name))
        out.append("\n")
    for enum_name, _ in geode_enums:
        out.append(_emit_entries_array("LUAX_GEODE_ENUM", "GeodeEnum", enum_name))
        out.append("\n")
    out.append("} // namespace\n\n")
    out.append("namespace luauapi_gen::geode_enums {\n\n")
    out.append("geode::Result<void> bindGeodeEnums(lua_State* L) {\n")
    if gd_enums:
        out.append('    luax::getOrCreateTable(L, "geode.gd");\n')
        for enum_name, _ in gd_enums:
            array = f"kGdEnum_{cxx_id(enum_name)}Entries"
            out.append(f"    lua_createtable(L, 0, static_cast<int>(std::size({array})));\n")
            out.append(f"    for (auto const& entry : {array}) {{\n")
            out.append("        setIntField(L, entry.name, entry.value);\n")
            out.append("    }\n")
            out.append(f'    lua_setfield(L, -2, "{enum_name}");\n')
        out.append("    lua_pop(L, 1);\n")
    if geode_enums:
        out.append('    luax::getOrCreateTable(L, "geode");\n')
        for enum_name, _ in geode_enums:
            array = f"kGeodeEnum_{cxx_id(enum_name)}Entries"
            out.append(f"    lua_createtable(L, 0, static_cast<int>(std::size({array})));\n")
            out.append(f"    for (auto const& entry : {array}) {{\n")
            out.append("        setIntField(L, entry.name, entry.value);\n")
            out.append("    }\n")
            out.append(f'    lua_setfield(L, -2, "{enum_name}");\n')
        out.append("    lua_pop(L, 1);\n")
    out.append("    return geode::Ok();\n")
    out.append("}\n\n")
    out.append("} // namespace luauapi_gen::geode_enums\n\n")
    out.append("namespace {\n")
    out.append(
        "    LUAX_BINDING_PRIORITY(GeneratedGeodeEnums, luauapi_gen::geode_enums::bindGeodeEnums, 4)\n"
    )
    out.append("}\n")
    return "".join(out)
