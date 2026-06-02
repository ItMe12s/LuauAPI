from __future__ import annotations

import unittest

from conftest import *


class DelegateGeneratorTests(unittest.TestCase):
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
        self.assertIn("invokeTableString", text)
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


if __name__ == "__main__":
    unittest.main()
