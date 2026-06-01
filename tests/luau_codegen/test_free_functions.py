from __future__ import annotations

from conftest import *


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
        skipped: list[tuple[str, str, str]] = []
        out = emit_free_functions_file(functions, {}, "android64", skipped)
        self.assertIn("geode::utils::game::restart(arg0)", out)
        self.assertNotIn("geode::utils::game::restart(arg0, arg1)", out)
        self.assertNotIn("switch (lua_gettop(L))", out)
        self.assertIn(
            ("geode.utils.game", "restart", "free-function-override-arity:android64"),
            skipped,
        )

    def test_emit_drops_restart_arity2_on_ios(self) -> None:
        functions = [self._restart(1), self._restart(2)]
        skipped: list[tuple[str, str, str]] = []
        out = emit_free_functions_file(functions, {}, "ios", skipped)
        self.assertIn("geode::utils::game::restart(arg0)", out)
        self.assertNotIn("geode::utils::game::restart(arg0, arg1)", out)
        self.assertNotIn("switch (lua_gettop(L))", out)
        self.assertIn(
            ("geode.utils.game", "restart", "free-function-override-arity:ios"),
            skipped,
        )

    def test_emit_keeps_both_overloads_on_win(self) -> None:
        functions = [self._restart(1), self._restart(2)]
        skipped: list[tuple[str, str, str]] = []
        out = emit_free_functions_file(functions, {}, "win", skipped)
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
