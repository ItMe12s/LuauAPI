from __future__ import annotations

from conftest import *

_ALLOWLIST_PATH = "luau_codegen.model.denylist.BINDABLE_CONSTRUCTORS"


def _fixture():
    ccobject = Class(name="CCObject", namespace="cocos2d")
    cls = Class(name="Fixture", namespace="geode", bases=["CCObject"])
    ctor = Method(
        name="Fixture",
        ret="void",
        args=[Arg("int", "x")],
        is_ctor=True,
        platforms=all_platforms("0x1"),
    )
    dtor = Method(
        name="~Fixture",
        ret="void",
        args=[],
        is_dtor=True,
        platforms=all_platforms("0x2"),
    )
    cls.methods = [ctor, dtor]
    objects = {"CCObject": ccobject, "Fixture": cls}
    return ccobject, cls, ctor, dtor, objects


class BindableConstructorTests(unittest.TestCase):
    def test_constructor_blocked_by_default(self) -> None:
        _, cls, ctor, _, objects = _fixture()
        ok, reason = supported(cls, ctor, objects, "win")
        self.assertFalse(ok)
        self.assertEqual(reason, "constructor")

    def test_allowlisted_constructor_supported(self) -> None:
        _, cls, ctor, dtor, objects = _fixture()
        with mock.patch.dict(_ALLOWLIST_PATH, {"Fixture": {("int",)}}):
            ok, reason = supported(cls, ctor, objects, "win")
            ok_dtor, reason_dtor = supported(cls, dtor, objects, "win")
        self.assertTrue(ok)
        self.assertEqual(reason, "")
        self.assertFalse(ok_dtor)
        self.assertEqual(reason_dtor, "destructor")

    def test_constructor_signature_must_match(self) -> None:
        _, cls, ctor, _, objects = _fixture()
        with mock.patch.dict(_ALLOWLIST_PATH, {"Fixture": {("float",)}}):
            ok, reason = supported(cls, ctor, objects, "win")
        self.assertFalse(ok)
        self.assertEqual(reason, "constructor")

    def test_group_supported_emits_new_factory(self) -> None:
        _, cls, _, _, objects = _fixture()
        with mock.patch.dict(_ALLOWLIST_PATH, {"Fixture": {("int",)}}):
            grouped, skipped = group_supported(cls, objects, "win")
        self.assertIn("new", grouped)
        self.assertNotIn("Fixture", grouped)
        syn = grouped["new"][0]
        self.assertTrue(syn.is_static)
        self.assertTrue(syn.is_bound_ctor)
        self.assertFalse(syn.is_ctor)
        self.assertEqual(syn.ret, "Fixture*")
        self.assertTrue(any(reason == "destructor" for _, reason in skipped))

    def test_emitted_cpp_uses_new_and_autorelease(self) -> None:
        _, cls, _, _, objects = _fixture()
        with mock.patch.dict(_ALLOWLIST_PATH, {"Fixture": {("int",)}}):
            grouped, _ = group_supported(cls, objects, "win")
            cpp = _emit_class_file(cls, grouped, [], [], objects, set(), 1, "win")
        self.assertIn("new geode::Fixture(", cpp)
        self.assertIn("result->autorelease();", cpp)
        self.assertIn("Fixture.new", cpp)

    def test_type_stub_exposes_new_factory(self) -> None:
        ccobject, cls, _, _, _ = _fixture()
        root = Root(classes=[ccobject, cls])
        with mock.patch.dict(_ALLOWLIST_PATH, {"Fixture": {("int",)}}):
            files = emit_luau_types(root, "win")
        text = types_text(files)
        self.assertIn("FixtureFactory", text)
        self.assertRegex(text, r"\bnew:\s*\(")


if __name__ == "__main__":
    unittest.main()
