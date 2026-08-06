from __future__ import annotations

import unittest
from unittest.mock import patch

from luau_codegen.convert import type_classification  # type: ignore[import-unresolved]
from luau_codegen.convert import type_primitives  # type: ignore[import-unresolved]
from luau_codegen.model.codegen_context import CodegenContext  # type: ignore[import-unresolved]
from luau_codegen.model.type_analysis import TypeAnalysis  # type: ignore[import-unresolved]
from luau_codegen.parse.broma import Arg, Method  # type: ignore[import-unresolved]


class TypeAnalysisModuleTests(unittest.TestCase):
    def test_normalization_lives_on_primitives(self) -> None:
        self.assertEqual(
            type_primitives.normalize_type("  const  int  &  "),
            "int&",
        )
        self.assertTrue(type_primitives.is_reference_type("int const&"))

    def test_strip_ref_delegate_classifier_samples(self) -> None:
        for raw, expected in (
            ("  const  int  &  ", "int"),
            ("char const*", "char const*"),
            ("const char*", "char const*"),
            ("cocos2d::CCNode *", "cocos2d::CCNode*"),
            ("gd::string &", "gd::string"),
        ):
            with self.subTest(raw=raw):
                self.assertEqual(type_primitives.strip_ref(raw), expected)

    def test_container_cap_on_classification_module(self) -> None:
        self.assertEqual(type_classification.STD_ARRAY_MAX_SIZE, 2000)

    def test_context_enum_derivations_are_precomputed_and_immutable(self) -> None:
        ctx = CodegenContext.static()
        self.assertIs(ctx.enum_types, ctx.enum_types)
        self.assertIsInstance(ctx.enum_types, frozenset)
        self.assertEqual(ctx.enum_cxx_type("CCTextAlignment", ""), "cocos2d::CCTextAlignment")

    def test_type_analysis_caches_supported_and_unsupported_signatures(self) -> None:
        analysis = TypeAnalysis({}, CodegenContext.static())
        method = Method(
            name="sample",
            ret="bool",
            args=[Arg(type="int", name="value"), Arg(type="UnknownType", name="unknown")],
            is_static=True,
        )
        with (
            patch(
                "luau_codegen.model.type_analysis.classify_arg",
                wraps=type_classification.classify_arg,
            ) as classify_arg,
            patch(
                "luau_codegen.model.type_analysis.classify_return",
                wraps=type_classification.classify_return,
            ) as classify_return,
        ):
            self.assertIsNone(analysis.signature(method))
            self.assertIsNone(analysis.signature(method))
            self.assertEqual(classify_arg.call_count, 2)
            self.assertEqual(classify_return.call_count, 1)
