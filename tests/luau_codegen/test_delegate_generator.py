from __future__ import annotations

import unittest

from test_support import DELEGATE_SPECS
from luau_codegen.emit.delegates import (  # type: ignore[import-unresolved]
    DelegateMethod,
    DelegateSpec,
    collect as collect_delegate_specs,
    cpp_emit_supported,
    emit_gen_cpp,
    emit_gen_hpp,
    emit_override,
)


class DelegateGeneratorTests(unittest.TestCase):
    def test_collect_delegate_specs_includes_cc_director_delegate(self) -> None:
        specs = collect_delegate_specs()
        self.assertIn("cocos2d::CCDirectorDelegate", specs)
        spec = specs["cocos2d::CCDirectorDelegate"]
        self.assertEqual(spec.lua_name, "CCDirectorDelegate")
        self.assertEqual(spec.cpp_class, "LuaCCDirectorDelegate")
        method_names = [m.name for m in spec.methods]
        self.assertIn("updateProjection", method_names)
        update = next(m for m in spec.methods if m.name == "updateProjection")
        self.assertEqual(update.ret, "void")
        self.assertEqual(update.args, [])

    def test_delegate_spec_cpp_parity(self) -> None:
        specs = collect_delegate_specs()
        hpp = emit_gen_hpp(specs)
        for spec in DELEGATE_SPECS.values():
            for method in spec.methods:
                self.assertIn(
                    f"{method.name}(",
                    hpp,
                    msg=f"missing override for {spec.lua_name}.{method.name}",
                )

    def test_generated_delegate_classes_declare_destructor(self) -> None:
        specs = collect_delegate_specs()
        hpp = emit_gen_hpp(specs)
        for spec in specs.values():
            if not any(cpp_emit_supported(spec, m) for m in spec.methods):
                continue
            self.assertIn(f"~{spec.cpp_class}()", hpp)
            self.assertIn("unregisterInterface", hpp)

    def test_get_song_file_name_emits_invoke_table_string(self) -> None:
        spec = DelegateSpec(
            cxx_type="CustomSongDelegate",
            lua_name="CustomSongDelegate",
            cpp_class="LuaCustomSongDelegate",
            methods=[DelegateMethod("getSongFileName", "gd::string", [])],
        )
        text = emit_override(spec, spec.methods[0])
        self.assertIn("invokeTableValue<std::string>", text)
        self.assertIn("getSongFileName", text)

    def test_object_pointer_return_emits_invoke_table_object(self) -> None:
        spec = DelegateSpec(
            cxx_type="CustomSongDelegate",
            lua_name="CustomSongDelegate",
            cpp_class="LuaCustomSongDelegate",
            methods=[DelegateMethod("getLevelSettings", "LevelSettingsObject*", [])],
        )
        text = emit_override(spec, spec.methods[0])
        self.assertIn("invokeTableObject<LevelSettingsObject>", text)
        self.assertIn("getLevelSettings", text)

    def test_override_emits_unique_parameter_names(self) -> None:
        spec = DelegateSpec(
            cxx_type="CCEGLViewProtocol",
            lua_name="CCEGLViewProtocol",
            cpp_class="LuaCCEGLViewProtocol",
            methods=[
                DelegateMethod(
                    "setFrameSize",
                    "void",
                    [("float", "arg"), ("float", "arg")],
                )
            ],
        )
        text = emit_override(spec, spec.methods[0])
        self.assertIn("void setFrameSize(float arg, float arg1) override", text)
        self.assertIn("Ctx ctx{ arg, arg1 }", text)

    def test_table_view_delegate_emits_ccindexpath_reference(self) -> None:
        specs = collect_delegate_specs()
        spec = specs["TableViewDelegate"]
        method = next(m for m in spec.methods if m.name == "didSelectRowAtIndexPath")
        self.assertEqual(method.args[0][0], "CCIndexPath&")
        text = emit_override(spec, method)
        self.assertIn(
            "void didSelectRowAtIndexPath(CCIndexPath& indexPath, TableView* tableView) override",
            text,
        )

    def test_synthetic_delegate_emits_ccindexpath_by_value(self) -> None:
        spec = DelegateSpec(
            cxx_type="TableViewDelegate",
            lua_name="TableViewDelegate",
            cpp_class="LuaTableViewDelegate",
            methods=[
                DelegateMethod(
                    "willCopyIndexPath",
                    "void",
                    [("CCIndexPath", "indexPath"), ("TableView*", "tableView")],
                )
            ],
        )
        text = emit_override(spec, spec.methods[0])
        self.assertIn(
            "void willCopyIndexPath(CCIndexPath indexPath, TableView* tableView) override",
            text,
        )
        self.assertNotIn("CCIndexPath& indexPath", text)

    def test_register_unregister_cast_use_qualified_cxx_type(self) -> None:
        specs = collect_delegate_specs()
        cxx = "cocos2d::extension::CCEditBoxDelegate"
        spec = specs[cxx]
        hpp = emit_gen_hpp({cxx: spec})
        cpp = emit_gen_cpp({cxx: spec})
        cast = f"static_cast<{cxx}*>"
        self.assertIn(f"unregisterInterface({cast}", hpp)
        self.assertIn(f"registerInterface({cast}", cpp)

    def test_float_return_emits_invoke_table_value(self) -> None:
        spec = DelegateSpec(
            cxx_type="TableViewDelegate",
            lua_name="TableViewDelegate",
            cpp_class="LuaTableViewDelegate",
            methods=[
                DelegateMethod(
                    "cellHeightForRowAtIndexPath",
                    "float",
                    [("CCIndexPath&", "indexPath"), ("TableView*", "tableView")],
                )
            ],
        )
        self.assertTrue(cpp_emit_supported(spec, spec.methods[0]))
        text = emit_override(spec, spec.methods[0])
        self.assertIn("invokeTableValue<float>", text)
        self.assertIn("0.f", text)


if __name__ == "__main__":
    unittest.main()
