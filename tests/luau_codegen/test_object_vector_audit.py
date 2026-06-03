from __future__ import annotations

import unittest
import warnings

from helpers import (  # type: ignore[import-unresolved]
    collect_bindings_root,  # type: ignore[import-unresolved]
    resolve_test_bindings_dir,  # type: ignore[import-unresolved]
)

from luau_codegen.convert.type_map import (  # type: ignore[import-unresolved]
    OPAQUE_HANDLE_TYPES,
    classify_arg,
)
from luau_codegen.model.domain import codegen_object_map  # type: ignore[import-unresolved]
from luau_codegen.model.object_discovery import (  # type: ignore[import-unresolved]
    undocumented_opaque_vector_elements,
    vector_pointer_element_types,
)
from luau_codegen.policy.fields import bindable_field  # type: ignore[import-unresolved]


class ObjectVectorAuditTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        bindings = resolve_test_bindings_dir(deps=("bindings-audit", "bindings-src"))
        if bindings is None:
            raise unittest.SkipTest(
                "Broma bindings not found, set LUAUAPI_BINDINGS_DIR or build once"
            )
        with warnings.catch_warnings():
            warnings.simplefilter("ignore")
            cls.root = collect_bindings_root(bindings, geode_sdk_path=None)
        cls.objects = codegen_object_map(cls.root)

    def test_group_command_and_delayed_spawn_vectors_classify(self) -> None:
        for cxx in (
            "gd::vector<GroupCommandObject2*>",
            "gd::vector<DelayedSpawnNode*>",
            "gd::vector<FMOD::Sound*>",
        ):
            info = classify_arg(cxx, self.objects)
            self.assertIsNotNone(info, cxx)
            assert info is not None
            self.assertEqual(info.kind, "vector_view", cxx)

    def test_ccmovecnode_group_objects_field_bindable(self) -> None:
        from luau_codegen.model.domain import build_class_lookup  # type: ignore[import-unresolved]

        lookup = build_class_lookup(self.root.classes)
        cls = lookup.get("CCMoveCNode")
        self.assertIsNotNone(cls)
        assert cls is not None
        field = next(f for f in cls.fields if f.name == "m_groupObjects")
        ok, reason, _, ret = bindable_field(field, self.objects, cls)
        self.assertTrue(ok, reason)
        assert ret is not None
        self.assertEqual(ret.kind, "vector_view")
        self.assertEqual(ret.lua_type, "{ GroupCommandObject2? }")

    def test_broma_non_ccobject_vector_elements_use_opaque_handles(self) -> None:
        elements = vector_pointer_element_types(self.root)
        self.assertIn("GroupCommandObject2*", elements)
        self.assertIn("DelayedSpawnNode*", elements)
        self.assertIn("FMOD::Sound*", elements)
        undocumented = undocumented_opaque_vector_elements(self.root)
        self.assertEqual(
            undocumented,
            set(),
            f"missing OPAQUE_HANDLE_TYPES for vector elements: {sorted(undocumented)}",
        )
        self.assertIn("GroupCommandObject2*", OPAQUE_HANDLE_TYPES)
        self.assertIn("DelayedSpawnNode*", OPAQUE_HANDLE_TYPES)


if __name__ == "__main__":
    unittest.main()
