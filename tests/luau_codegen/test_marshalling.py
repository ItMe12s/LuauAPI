from __future__ import annotations

from conftest import *


class F6NumericMarshallingTests(unittest.TestCase):
    def test_unsigned_int_uses_check_unsigned(self) -> None:
        info = TypeInfo(kind="number", lua_type="number", cxx_type="unsigned int")
        arg = Arg(type="unsigned int", name="x")
        lines = check_arg(arg, info, 1, "v", "test")
        text = "".join(lines)
        self.assertIn("check<unsigned>", text)
        self.assertNotIn("check<int>", text)

    def test_double_uses_check_double(self) -> None:
        info = TypeInfo(kind="number", lua_type="number", cxx_type="double")
        arg = Arg(type="double", name="x")
        lines = check_arg(arg, info, 1, "v", "test")
        text = "".join(lines)
        self.assertIn("check<double>", text)

    def test_wide_integer_uses_string_marshalling(self) -> None:
        info = classify_arg("uint64_t", {})
        self.assertIsNotNone(info)
        assert info is not None
        self.assertEqual(info.kind, "wideint")
        self.assertEqual(info.lua_type, "string")
        check_text = "".join(check_arg(Arg("uint64_t", "x"), info, 1, "v", "test"))
        push_text = "".join(push_value(info, "v"))
        self.assertIn("checkIntegerString<uint64_t>", check_text)
        self.assertIn("pushIntegerString", push_text)

    def test_unsigned_push_does_not_narrow_through_int(self) -> None:
        with open(
            os.path.join(ROOT, "src", "lua", "bindings", "framework", "Stack.hpp"),
            "r",
            encoding="utf-8",
        ) as f:
            text = f.read()
        self.assertIn("inline void push(lua_State* L, unsigned v)", text)
        self.assertNotIn("static_cast<int>(v)", text)
        self.assertIn("static_cast<double>(v)", text)

    def test_regular_int_unchanged(self) -> None:
        info = TypeInfo(kind="number", lua_type="number", cxx_type="int")
        arg = Arg(type="int", name="x")
        lines = check_arg(arg, info, 1, "v", "test")
        text = "".join(lines)
        self.assertIn("check<int>", text)
