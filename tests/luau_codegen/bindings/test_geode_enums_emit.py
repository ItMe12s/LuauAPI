from __future__ import annotations

import unittest

from luau_codegen.emit.bindings.geode_enums import (  # type: ignore[import-unresolved]
    GEODE_ENUMS_BINDING,
    GEODE_ENUMS_MANIFEST,
    emit_geode_enums_binding,
    emit_geode_enums_manifest,
)
from luau_codegen.model.codegen_context import CodegenContext  # type: ignore[import-unresolved]
from luau_codegen.model.geode_enums import EnumInfo, EnumMember  # type: ignore[import-unresolved]
from luau_codegen.parse.collect import collect_bindings_root  # type: ignore[import-unresolved]


class GeodeEnumEmitTests(unittest.TestCase):
    def test_manifest_emits_gd_and_geode_enum_macros(self) -> None:
        ctx = CodegenContext.with_geode_enums(
            {
                "GJLevelType": EnumInfo(
                    name="GJLevelType",
                    cxx_name="GJLevelType",
                    members=(
                        EnumMember("Saved", 3),
                        EnumMember("SearchResult", 4),
                    ),
                ),
                "UniqueGeodeEnum": EnumInfo(
                    name="UniqueGeodeEnum",
                    cxx_name="geode::UniqueGeodeEnum",
                    members=(
                        EnumMember("Alpha", 1),
                        EnumMember("Beta", 2),
                    ),
                ),
            },
            skip={"GJLevelType"},
        )
        text = emit_geode_enums_manifest(ctx)

        self.assertIn("#define LUAX_INT_ENUM_ENTRY(name, value) {#name, value},\n", text)
        self.assertIn("#define LUAX_GD_ENUM_GJLevelType(X)", text)
        self.assertIn("X(Saved, 3)", text)
        self.assertIn("X(SearchResult, 4)", text)
        self.assertIn("#define LUAX_GEODE_ENUM_UniqueGeodeEnum(X)", text)
        self.assertIn("X(Alpha, 1)", text)
        self.assertIn("X(Beta, 2)", text)

    def test_binding_registers_tables_under_existing_namespaces(self) -> None:
        ctx = CodegenContext.with_geode_enums(
            {
                "GJLevelType": EnumInfo(
                    name="GJLevelType",
                    cxx_name="GJLevelType",
                    members=(
                        EnumMember("Saved", 3),
                        EnumMember("SearchResult", 4),
                    ),
                ),
                "UniqueGeodeEnum": EnumInfo(
                    name="UniqueGeodeEnum",
                    cxx_name="geode::UniqueGeodeEnum",
                    members=(EnumMember("Alpha", 1),),
                ),
            },
            skip={"GJLevelType"},
        )
        text = emit_geode_enums_binding(ctx)

        self.assertIn('luax::getOrCreateTable(L, "geode.gd");', text)
        self.assertIn('lua_setfield(L, -2, "GJLevelType");', text)
        self.assertIn("kGdEnum_GJLevelTypeEntries", text)
        self.assertIn('luax::getOrCreateTable(L, "geode");', text)
        self.assertIn('lua_setfield(L, -2, "UniqueGeodeEnum");', text)
        self.assertIn("kGeodeEnum_UniqueGeodeEnumEntries", text)
        self.assertIn("LUAX_BINDING_PRIORITY(GeneratedGeodeEnums", text)
        self.assertIn(
            "LUAX_GD_ENUM_GJLevelType(LUAX_INT_ENUM_ENTRY)",
            text,
        )
        self.assertIn(
            "LUAX_GEODE_ENUM_UniqueGeodeEnum(LUAX_INT_ENUM_ENTRY)",
            text,
        )

    def test_emit_includes_geode_enum_outputs(self) -> None:
        from luau_codegen.emit.bindings import emit as emit_luau_bindings  # type: ignore[import-unresolved]
        from luau_codegen.parse.broma import Class, Root  # type: ignore[import-unresolved]

        root = Root(classes=[Class(name="CCObject", namespace="cocos2d")])
        root.codegen_ctx = CodegenContext.with_geode_enums(
            {
                "GJLevelType": EnumInfo(
                    name="GJLevelType",
                    cxx_name="GJLevelType",
                    members=(EnumMember("Saved", 3),),
                )
            },
            skip={"GJLevelType"},
        )
        files, _ = emit_luau_bindings(root)

        self.assertIn(GEODE_ENUMS_MANIFEST, files)
        self.assertIn(GEODE_ENUMS_BINDING, files)
        self.assertIn("LUAX_GD_ENUM_GJLevelType", files[GEODE_ENUMS_MANIFEST])
        self.assertIn('lua_setfield(L, -2, "GJLevelType");', files[GEODE_ENUMS_BINDING])

    def test_collect_threads_gd_enum_members_into_runtime_emit(self) -> None:
        import os
        import shutil
        import tempfile

        repo = tempfile.mkdtemp()
        sdk = tempfile.mkdtemp()
        try:
            bindings = os.path.join(repo, "bindings", "2.2074")
            os.makedirs(bindings)
            with open(os.path.join(bindings, "Cocos2d.bro"), "w", encoding="utf-8") as f:
                f.write("[[link(win)]] class cocos2d::CCObject {};\n")
            include_dir = os.path.join(repo, "bindings", "include", "Geode")
            os.makedirs(include_dir)
            with open(os.path.join(include_dir, "Enums.hpp"), "w", encoding="utf-8") as f:
                f.write(
                    "enum class GJLevelType { Saved = 3, SearchResult = 4 };\n"
                    "enum class UniqueGeodeEnum { Alpha = 1 };\n"
                )

            root = collect_bindings_root(bindings, geode_sdk_path=sdk)
            assert root.codegen_ctx is not None
            manifest = emit_geode_enums_manifest(root.codegen_ctx)
            binding = emit_geode_enums_binding(root.codegen_ctx)

            self.assertEqual(
                root.codegen_ctx.gd_enum_members["GJLevelType"],
                (("Saved", 3), ("SearchResult", 4)),
            )
            self.assertEqual(
                root.codegen_ctx.geode_enum_members["UniqueGeodeEnum"],
                (("Alpha", 1),),
            )
            self.assertIn("X(Saved, 3)", manifest)
            self.assertIn("X(SearchResult, 4)", manifest)
            self.assertIn("X(Alpha, 1)", manifest)
            self.assertIn('lua_setfield(L, -2, "GJLevelType");', binding)
            self.assertIn('lua_setfield(L, -2, "UniqueGeodeEnum");', binding)
            self.assertIn("setIntField(L, entry.name, entry.value)", binding)
        finally:
            shutil.rmtree(repo)
            shutil.rmtree(sdk)


if __name__ == "__main__":
    unittest.main()
