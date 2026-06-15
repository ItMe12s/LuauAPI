from __future__ import annotations

import unittest

from helpers import (
    DELEGATE_SPECS,  # type: ignore[import-unresolved]
    DelegateMethod,  # type: ignore[import-unresolved]
    DelegateSpec,  # type: ignore[import-unresolved]
    collect_delegate_specs,  # type: ignore[import-unresolved]
    cpp_emit_supported,  # type: ignore[import-unresolved]
    emit_gen_hpp,  # type: ignore[import-unresolved]
    emit_override,  # type: ignore[import-unresolved]
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


if __name__ == "__main__":
    unittest.main()
