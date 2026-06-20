from __future__ import annotations

import unittest

from luau_codegen.convert import type_map as facade  # type: ignore[import-unresolved]
from luau_codegen.convert import type_classification  # type: ignore[import-unresolved]


class TypeMapModuleSplitTests(unittest.TestCase):
    def test_facade_reexports_classification_api(self) -> None:
        self.assertIs(facade.classify_arg, type_classification.classify_arg)
        self.assertIs(facade.classify_return, type_classification.classify_return)

    def test_normalization_lives_on_type_map(self) -> None:
        self.assertEqual(
            facade.normalize_type("  const  int  &  "),
            "int&",
        )
        self.assertTrue(facade.is_reference_type("int const&"))

    def test_strip_ref_delegate_classifier_samples(self) -> None:
        for raw, expected in (
            ("  const  int  &  ", "int"),
            ("char const*", "char const*"),
            ("const char*", "char const*"),
            ("cocos2d::CCNode *", "cocos2d::CCNode*"),
            ("gd::string &", "gd::string"),
        ):
            with self.subTest(raw=raw):
                self.assertEqual(facade.strip_ref(raw), expected)

    def test_container_cap_on_classification_module(self) -> None:
        self.assertEqual(type_classification.STD_ARRAY_MAX_SIZE, facade.STD_ARRAY_MAX_SIZE)

    def test_classify_via_submodule_matches_facade(self) -> None:
        info = facade.classify_arg("bool", {})
        sub_info = type_classification.classify_arg("bool", {})
        self.assertEqual(info, sub_info)
