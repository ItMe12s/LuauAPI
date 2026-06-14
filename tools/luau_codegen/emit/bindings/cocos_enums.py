from __future__ import annotations

from luau_codegen.model.codegen_context import CodegenContext

ENUM_KEY_CODES_MANIFEST = "bindings/geode/EnumKeyCodes.manifest.hpp"


def emit_enum_key_codes_manifest(ctx: CodegenContext) -> str:
    entries = ctx.cocos_enum_members.get("enumKeyCodes", ())
    lines = [
        "#pragma once\n",
        "\n",
        "#define LUAX_ENUM_KEY_CODES_ENTRY(name) {#name, static_cast<int>(cocos2d::name)},\n",
        "\n",
        "#define LUAX_ENUM_KEY_CODES(X)",
    ]
    if not entries:
        lines.append("\n")
        return "".join(lines)
    for index, (name, _) in enumerate(entries):
        suffix = " \\\n" if index < len(entries) - 1 else "\n"
        lines.append(f" \\\n    X({name})" if index == 0 else f"    X({name})")
        lines.append(suffix)
    return "".join(lines)
