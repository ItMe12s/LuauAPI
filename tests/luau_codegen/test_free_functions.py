from __future__ import annotations

import unittest
from helpers import (
    Arg,  # type: ignore[import-unresolved]
    Class,  # type: ignore[import-unresolved]
    Function,  # type: ignore[import-unresolved]
    Method,  # type: ignore[import-unresolved]
    Root,  # type: ignore[import-unresolved]
    collect_plan,  # type: ignore[import-unresolved]
    emit_free_functions_file,  # type: ignore[import-unresolved]
    emit_luau_bindings,  # type: ignore[import-unresolved]
    emit_luau_types,  # type: ignore[import-unresolved]
    free_fn_allowed,  # type: ignore[import-unresolved]
    free_function_key,  # type: ignore[import-unresolved]
    free_function_supported,  # type: ignore[import-unresolved]
    free_function_unsupported_reason,  # type: ignore[import-unresolved]
    group_supported_free_functions,  # type: ignore[import-unresolved]
    types_text,  # type: ignore[import-unresolved]
)


class FreeFunctionOverrideTests(unittest.TestCase):
    def _restart(self, arity: int) -> Function:
        args = [Arg("bool", f"a{i}") for i in range(arity)]
        return Function(
            name="restart", namespace="geode::utils::game", ret="void", args=args
        )

    def test_free_fn_allowed_drops_arity2_on_mobile(self) -> None:
        two = self._restart(2)
        one = self._restart(1)
        mobile = ("android32", "android64", "ios", "m1", "imac", "mac")
        for plat in mobile:
            self.assertFalse(free_fn_allowed(two, plat))
            self.assertTrue(free_fn_allowed(one, plat))
        self.assertTrue(free_fn_allowed(two, "win"))
        self.assertTrue(free_fn_allowed(one, "win"))

    def test_free_fn_allowed_passes_unlisted_function(self) -> None:
        other = Function(
            name="other",
            namespace="geode::utils::game",
            ret="void",
            args=[Arg("bool", "a"), Arg("bool", "b")],
        )
        for plat in ("win", "android32", "android64", "ios", "m1"):
            self.assertTrue(free_fn_allowed(other, plat))

    def test_emit_drops_restart_arity2_on_android(self) -> None:
        functions = [self._restart(1), self._restart(2)]
        kept, skipped = group_supported_free_functions(functions, {}, "android64")
        out = emit_free_functions_file(kept, {})
        self.assertIn("geode::utils::game::restart(arg0)", out)
        self.assertNotIn("geode::utils::game::restart(arg0, arg1)", out)
        self.assertNotIn("switch (lua_gettop(L))", out)
        self.assertIn(
            (
                free_function_key(functions[1]),
                "geode.utils.game",
                "restart",
                "free-function-override-arity:android64",
            ),
            skipped,
        )

    def test_emit_drops_restart_arity2_on_ios(self) -> None:
        functions = [self._restart(1), self._restart(2)]
        kept, skipped = group_supported_free_functions(functions, {}, "ios")
        out = emit_free_functions_file(kept, {})
        self.assertIn("geode::utils::game::restart(arg0)", out)
        self.assertNotIn("geode::utils::game::restart(arg0, arg1)", out)
        self.assertNotIn("switch (lua_gettop(L))", out)
        self.assertIn(
            (
                free_function_key(functions[1]),
                "geode.utils.game",
                "restart",
                "free-function-override-arity:ios",
            ),
            skipped,
        )

    def test_emit_keeps_both_overloads_on_win(self) -> None:
        functions = [self._restart(1), self._restart(2)]
        kept, skipped = group_supported_free_functions(functions, {}, "win")
        out = emit_free_functions_file(kept, {})
        self.assertIn("geode::utils::game::restart(arg0)", out)
        self.assertIn("geode::utils::game::restart(arg0, arg1)", out)
        self.assertIn("switch (lua_gettop(L))", out)
        self.assertEqual(skipped, [])

    def test_types_emit_drops_restart_arity2_on_ios(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        root = Root(
            classes=[ccobject],
            functions=[self._restart(1), self._restart(2)],
        )
        out = types_text(emit_luau_types(root, "ios"))
        self.assertIn("restart:", out)
        self.assertNotIn("restart: (self: any, a0: boolean, a1: boolean)", out)


class FreeFunctionContainerTests(unittest.TestCase):
    def test_vector_return_is_supported(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        fn = Function(
            name="children",
            namespace="geode::utils",
            ret="gd::vector<cocos2d::CCObject*>",
            args=[],
        )
        objects = {"CCObject": ccobject, "cocos2d::CCObject": ccobject}

        reason = free_function_unsupported_reason(fn, objects)

        self.assertIsNone(reason)

    def test_vector_input_arg_is_supported(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        fn = Function(
            name="applyChildren",
            namespace="geode::utils",
            ret="void",
            args=[Arg("gd::vector<cocos2d::CCObject*> const&", "children")],
        )
        objects = {"CCObject": ccobject, "cocos2d::CCObject": ccobject}

        reason = free_function_unsupported_reason(fn, objects)

        self.assertIsNone(reason)

    def test_primitive_vector_input_arg_is_supported(self) -> None:
        fn = Function(
            name="take",
            namespace="geode::utils",
            ret="void",
            args=[Arg("gd::vector<int>", "values")],
        )

        reason = free_function_unsupported_reason(fn, {})

        self.assertIsNone(reason)

    def test_primitive_vector_return_is_supported(self) -> None:
        fn = Function(
            name="values",
            namespace="geode::utils",
            ret="gd::vector<int>",
            args=[],
        )

        reason = free_function_unsupported_reason(fn, {})

        self.assertIsNone(reason)

    def test_map_input_arg_is_supported(self) -> None:
        fn = Function(
            name="takeMap",
            namespace="geode::utils",
            ret="void",
            args=[Arg("gd::map<int, bool>", "values")],
        )

        reason = free_function_unsupported_reason(fn, {})

        self.assertIsNone(reason)

    def test_set_return_is_supported(self) -> None:
        fn = Function(
            name="values",
            namespace="geode::utils",
            ret="gd::set<int>",
            args=[],
        )

        reason = free_function_unsupported_reason(fn, {})

        self.assertIsNone(reason)


class GetEnvironmentVariableExclusionTests(unittest.TestCase):
    def _get_env(self) -> Function:
        return Function(
            name="getEnvironmentVariable",
            namespace="geode::utils",
            ret="std::string",
            args=[Arg("ZStringView", "name")],
        )

    def test_free_fn_excluded_on_ios(self) -> None:
        fn = self._get_env()
        self.assertFalse(free_fn_allowed(fn, "ios"))
        for plat in ("win", "android64", "m1"):
            self.assertTrue(free_fn_allowed(fn, plat))

    def test_emit_drops_get_environment_variable_on_ios(self) -> None:
        functions = [self._get_env()]
        kept, skipped = group_supported_free_functions(functions, {}, "ios")
        out = emit_free_functions_file(kept, {})
        self.assertNotIn("geode::utils::getEnvironmentVariable", out)
        self.assertIn(
            (
                free_function_key(functions[0]),
                "geode.utils",
                "getEnvironmentVariable",
                "free-function-excluded:ios",
            ),
            skipped,
        )

    def test_emit_keeps_get_environment_variable_on_win(self) -> None:
        functions = [self._get_env()]
        kept, skipped = group_supported_free_functions(functions, {}, "win")
        out = emit_free_functions_file(kept, {})
        self.assertIn("geode::utils::getEnvironmentVariable(arg0)", out)
        self.assertEqual(skipped, [])

    def test_emit_keeps_get_environment_variable_on_android64(self) -> None:
        functions = [self._get_env()]
        kept, skipped = group_supported_free_functions(functions, {}, "android64")
        out = emit_free_functions_file(kept, {})
        self.assertIn("geode::utils::getEnvironmentVariable(arg0)", out)
        self.assertEqual(skipped, [])


class FreeFunctionIntersectionTests(unittest.TestCase):
    def _restart(self, arity: int) -> Function:
        args = [Arg("bool", f"a{i}") for i in range(arity)]
        return Function(
            name="restart", namespace="geode::utils::game", ret="void", args=args
        )

    def _get_env(self) -> Function:
        return Function(
            name="getEnvironmentVariable",
            namespace="geode::utils",
            ret="std::string",
            args=[Arg("ZStringView", "name")],
        )

    def test_collect_plan_drops_free_function_missing_one_platform(self) -> None:
        fn = self._get_env()
        root = Root(
            classes=[Class(name="CCObject", namespace="cocos2d")], functions=[fn]
        )

        plan = collect_plan(root, "win")

        self.assertEqual(plan.supported_free_functions, [])
        self.assertIn(
            (
                free_function_key(fn),
                "geode.utils",
                "getEnvironmentVariable",
                "intersection-missing-platform:ios",
            ),
            plan.skipped_free_functions,
        )

    def test_collect_plan_keeps_only_intersected_restart_arity(self) -> None:
        one = self._restart(1)
        two = self._restart(2)
        root = Root(
            classes=[Class(name="CCObject", namespace="cocos2d")],
            functions=[one, two],
        )

        plan = collect_plan(root, "win")

        self.assertEqual(
            [free_function_key(fn) for fn in plan.supported_free_functions],
            [free_function_key(one)],
        )
        self.assertIn(
            (
                free_function_key(two),
                "geode.utils.game",
                "restart",
                "intersection-missing-platform:m1,ios,android32,android64",
            ),
            plan.skipped_free_functions,
        )

    def test_binding_emit_has_same_free_function_surface_on_win_and_ios(self) -> None:
        root = Root(
            classes=[Class(name="CCObject", namespace="cocos2d")],
            functions=[self._restart(1), self._restart(2)],
        )

        win = emit_luau_bindings(root, "win")[0]["bindings_free_functions.cpp"]
        ios = emit_luau_bindings(root, "ios")[0]["bindings_free_functions.cpp"]

        self.assertEqual(win, ios)
        self.assertIn("geode::utils::game::restart(arg0)", win)
        self.assertNotIn("geode::utils::game::restart(arg0, arg1)", win)

    def test_final_class_prune_drops_free_function_object_ref(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        weak = Class(
            name="WeakFreeFnType",
            bases=["CCObject"],
            methods=[
                Method(name="winOnly", ret="void", args=[], platforms={"win": "0x1"}),
                Method(name="m1Only", ret="void", args=[], platforms={"m1": "0x1"}),
                Method(name="iosOnly", ret="void", args=[], platforms={"ios": "0x1"}),
                Method(
                    name="android32Only",
                    ret="void",
                    args=[],
                    platforms={"android32": "0x1"},
                ),
                Method(
                    name="android64Only",
                    ret="void",
                    args=[],
                    platforms={"android64": "0x1"},
                ),
            ],
        )
        fn = Function(
            name="makeWeak",
            namespace="geode::utils",
            ret="WeakFreeFnType*",
            args=[],
        )
        root = Root(classes=[ccobject, weak], functions=[fn])

        plan = collect_plan(root, "win")

        self.assertEqual(plan.supported_free_functions, [])
        self.assertIn("WeakFreeFnType", plan.skipped_classes)
        self.assertIn(
            (
                free_function_key(fn),
                "geode.utils",
                "makeWeak",
                "not-callable-type:win:WeakFreeFnType",
            ),
            plan.skipped_free_functions,
        )


class FreeFunctionSupportedTests(unittest.TestCase):
    def test_supported_marshallable_signature(self) -> None:
        fn = Function(
            name="foo",
            namespace="geode::utils",
            ret="void",
            args=[Arg("int", "x"), Arg("bool", "flag")],
        )
        self.assertTrue(free_function_supported(fn, {}))

    def test_supported_object_pointer_return(self) -> None:
        objects = {"CCNode": Class(name="CCNode", namespace="cocos2d")}
        fn = Function(
            name="getNode",
            namespace="geode::utils",
            ret="cocos2d::CCNode*",
            args=[],
        )
        self.assertTrue(free_function_supported(fn, objects))

    def test_unsupported_return_type(self) -> None:
        fn = Function(
            name="foo",
            namespace="geode::utils",
            ret="UnknownType*",
            args=[],
        )
        self.assertFalse(free_function_supported(fn, {}))

    def test_unsupported_arg_type(self) -> None:
        fn = Function(
            name="foo",
            namespace="geode::utils",
            ret="void",
            args=[Arg("SomeUnknownClass*", "obj")],
        )
        self.assertFalse(free_function_supported(fn, {}))

    def test_out_reference_arg_unsupported(self) -> None:
        mutate = Function(
            name="toLowerIP",
            namespace="geode::utils::string",
            ret="void",
            args=[Arg("std::string&", "s")],
        )
        self.assertFalse(free_function_supported(mutate, {}))
        read = Function(
            name="contains",
            namespace="geode::utils::string",
            ret="bool",
            args=[Arg("std::string const&", "s")],
        )
        self.assertTrue(free_function_supported(read, {}))


class FreeFunctionOverloadDedupTests(unittest.TestCase):
    def _contains(self, second: str) -> Function:
        return Function(
            name="contains",
            namespace="geode::utils::string",
            ret="bool",
            args=[Arg("std::string", "s"), Arg(second, "x")],
        )

    def test_same_arity_overloads_deduped(self) -> None:
        functions = [self._contains("std::string"), self._contains("char")]
        kept, skipped = group_supported_free_functions(functions, {}, "win")
        out = emit_free_functions_file(kept, {})
        self.assertNotIn("switch (lua_gettop(L))", out)
        self.assertIn(
            (
                free_function_key(functions[1]),
                "geode.utils.string",
                "contains",
                "free-function-ambiguous-arity:2",
            ),
            skipped,
        )
