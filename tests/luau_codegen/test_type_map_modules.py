from __future__ import annotations

import unittest

from luau_codegen.convert import type_map as facade  # type: ignore[import-unresolved]
from luau_codegen.convert import type_classification  # type: ignore[import-unresolved]
from luau_codegen.convert import type_containers  # type: ignore[import-unresolved]
from luau_codegen.convert import type_normalization  # type: ignore[import-unresolved]


class TypeMapModuleSplitTests(unittest.TestCase):
    def test_facade_reexports_classification_api(self) -> None:
        self.assertIs(facade.classify_arg, type_classification.classify_arg)
        self.assertIs(facade.classify_return, type_classification.classify_return)

    def test_normalization_module_own_normalize_type(self) -> None:
        self.assertEqual(
            type_normalization.normalize_type("  const  int  &  "),
            "int&",
        )
        self.assertTrue(type_normalization.is_reference_type("int const&"))

    def test_container_module_exports_std_array_cap(self) -> None:
        self.assertEqual(type_containers.STD_ARRAY_MAX_SIZE, facade.STD_ARRAY_MAX_SIZE)

    def test_classify_via_submodule_matches_facade(self) -> None:
        info = facade.classify_arg("bool", {})
        sub_info = type_classification.classify_arg("bool", {})
        self.assertEqual(info, sub_info)
