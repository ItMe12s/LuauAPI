from __future__ import annotations

import os
import shutil
import tempfile
import unittest

import test_support  # noqa: F401 - installs codegen test fixtures
from luau_codegen.emit.types_binding import (
    emit_types_generated_containers_hpp,
    emit_types_generated_hpp,
    types_gen_containers_rel_path,
    types_gen_rel_path,
    write_types_generated,
)
from luau_codegen.model.cocos_value_types import COCOS_VALUE_STRUCTS, FieldDescriptor


class CocosValueDescriptorTests(unittest.TestCase):
    def test_standard_structs_exclude_hand_retained_types(self) -> None:
        cxx_types = {desc.cxx_type for desc in COCOS_VALUE_STRUCTS}
        self.assertIn("cocos2d::CCPoint", cxx_types)
        self.assertIn("cocos2d::CCSize", cxx_types)
        self.assertIn("cocos2d::ccColor3B", cxx_types)
        self.assertNotIn("UIButtonConfig", cxx_types)
        self.assertNotIn("SmartPrefabResult", cxx_types)

    def test_field_descriptor_schema_has_name_and_kind(self) -> None:
        point = next(desc for desc in COCOS_VALUE_STRUCTS if desc.cxx_type == "cocos2d::CCPoint")
        self.assertEqual(point.check_fields[0], FieldDescriptor("x", "number", "point.x"))
        self.assertEqual(point.check_fields[0].name, "x")
        self.assertEqual(point.check_fields[0].kind, "number")


class TypesGeneratedEmitterTests(unittest.TestCase):
    def test_emits_check_and_push_for_cocos_structs(self) -> None:
        text = emit_types_generated_hpp()
        self.assertIn("check<cocos2d::CCPoint>", text)
        self.assertIn("check<cocos2d::CCRect>", text)
        self.assertIn('fieldNumber(L, idx, "x", method)', text)
        self.assertIn('lua_setfield(L, -2, "origin")', text)
        self.assertIn("check<cocos2d::ccHSVValue>", text)
        self.assertIn("absoluteSaturation", text)
        self.assertNotIn("UIButtonConfig", text)
        self.assertNotIn("SmartPrefabResult", text)

    def test_ccrect_check_reads_flat_xywh(self) -> None:
        text = emit_types_generated_hpp()
        self.assertIn(
            '{fieldNumber(L, idx, "x", method), fieldNumber(L, idx, "y", method)},',
            text,
        )
        self.assertIn(
            '{fieldNumber(L, idx, "width", method), fieldNumber(L, idx, "height", method)},',
            text,
        )

    def test_blendfunc_push_casts_gl_enum(self) -> None:
        text = emit_types_generated_hpp()
        self.assertIn("static_cast<double>(blend.src)", text)
        self.assertIn("static_cast<double>(blend.dst)", text)

    def test_container_structs_emit_to_containers_header(self) -> None:
        base = emit_types_generated_hpp()
        containers = emit_types_generated_containers_hpp()
        self.assertNotIn("check<SequenceTriggerState>", base)
        self.assertIn("check<SequenceTriggerState>", containers)
        self.assertIn("checkContainerValue<gd::map", containers)
        self.assertNotIn("checkContainerValue<gd::map", base)
        self.assertIn("checkContainerValue<std::array", containers)
        self.assertNotIn("checkContainerValue<std::array", base)
        self.assertIn("inline cocos2d::CCPoint check<cocos2d::CCPoint>", base)
        self.assertNotIn("inline cocos2d::CCPoint check<cocos2d::CCPoint>", containers)
        self.assertNotIn("inline SavedObjectStateRef check<SavedObjectStateRef>", base)
        self.assertIn("Usertype<GameObject>", containers)
        self.assertNotIn("Usertype<GameObject>", base)
        self.assertNotIn("checkOpaqueHandle", base)
        self.assertIn("checkOpaqueHandle", containers)
        self.assertNotIn("check<FMODSoundState>", base)
        self.assertIn("check<FMODSoundState>", containers)
        self.assertIn("SequenceTriggerState value;", containers)
        self.assertNotIn("value{};", containers)
        self.assertNotIn("value();", containers)

    def test_write_types_generated_creates_expected_path(self) -> None:
        tmpdir = tempfile.mkdtemp()
        try:
            out_path = write_types_generated(tmpdir)
            rel = types_gen_rel_path()
            self.assertTrue(out_path.endswith(rel.replace("/", os.sep)))
            containers_path = os.path.join(
                tmpdir,
                types_gen_containers_rel_path().replace("/", os.sep),
            )
            self.assertTrue(os.path.isfile(containers_path))
            with open(out_path, encoding="utf-8") as handle:
                content = handle.read()
            self.assertIn("namespace luax", content)
            self.assertIn("check<cocos2d::CCAffineTransform>", content)
            with open(containers_path, encoding="utf-8") as handle:
                containers_content = handle.read()
            self.assertIn("check<SequenceTriggerState>", containers_content)
        finally:
            shutil.rmtree(tmpdir)


if __name__ == "__main__":
    unittest.main()
