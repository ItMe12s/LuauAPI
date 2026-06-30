from __future__ import annotations

import sys
import unittest

from luau_codegen.model.value_types import (  # type: ignore[import-unresolved]
    COCOS_VALUE_STRUCTS,
    VALUE_CHECK_CXX_TYPES,
    VALUE_TYPE_SPECS,
    VALUE_TYPES,
    _VALUE_TYPE_PROPAGATION,
    _rebuild_value_type_maps,
)

import test_support  # type: ignore[import-unresolved]


class ValueTypePropagationTests(unittest.TestCase):
    def setUp(self) -> None:
        if not VALUE_TYPE_SPECS:
            test_support.reinstall_fixture_value_struct_specs()
        if not VALUE_TYPE_SPECS:
            self.skipTest("value-struct specs unavailable (bindings dir not built)")

    def test_specs_are_populated_after_fixture_install(self) -> None:
        self.assertGreater(len(VALUE_TYPES), 0)
        self.assertGreater(len(VALUE_CHECK_CXX_TYPES), 0)
        self.assertGreater(len(COCOS_VALUE_STRUCTS), 0)

    def test_rebuild_leaves_all_targets_in_sync(self) -> None:
        for mod_name in _VALUE_TYPE_PROPAGATION:
            if mod_name not in sys.modules:
                __import__(mod_name)

        _rebuild_value_type_maps()

        for mod_name, attrs in _VALUE_TYPE_PROPAGATION.items():
            mod = sys.modules[mod_name]
            for attr in attrs:
                self.assertTrue(
                    hasattr(mod, attr),
                    f"{mod_name}.{attr} missing after rebuild",
                )
                value = getattr(mod, attr)
                if attr in ("VALUE_STUB_ORDER",):
                    self.assertGreater(len(value), 0, f"{mod_name}.{attr} empty after rebuild")
                elif isinstance(value, (dict, tuple, frozenset, set, list)):
                    self.assertGreater(len(value), 0, f"{mod_name}.{attr} empty after rebuild")


if __name__ == "__main__":
    unittest.main()
