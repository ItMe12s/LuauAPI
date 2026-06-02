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


class CallbackMarshallingTests(unittest.TestCase):
    def test_callback_arg_emits_lua_callback_helper(self) -> None:
        sender = TypeInfo(
            kind="object",
            cxx_type="cocos2d::CCObject*",
            lua_type="CCObject",
            class_name="CCObject",
        )
        info = TypeInfo(
            kind="callback",
            cxx_type="std::function<void(cocos2d::CCObject*)>",
            lua_type="(sender: CCObject) -> ()",
            callback_args=(sender,),
        )
        text = "".join(
            check_arg(
                Arg("std::function<void(cocos2d::CCObject*)>", "cb"),
                info,
                2,
                "cb",
                "Test.method",
            )
        )
        self.assertIn("std::make_shared<luax::LuaCallback>", text)
        self.assertIn("cb_cb->invoke(1, 0", text)
        self.assertIn("pushBorrowed", text)
        self.assertNotIn("protectedCall", text)

    def test_callback_ctx_struct_uses_semicolon_fields(self) -> None:
        sender = TypeInfo(
            kind="object",
            cxx_type="FLAlertLayer*",
            lua_type="FLAlertLayer",
            class_name="FLAlertLayer",
        )
        ok = TypeInfo(kind="bool", cxx_type="bool", lua_type="boolean")
        info = TypeInfo(
            kind="callback",
            cxx_type="std::function<void(FLAlertLayer*, bool)>",
            lua_type="(arg1: FLAlertLayer, arg2: boolean) -> ()",
            callback_args=(sender, ok),
        )
        text = "".join(
            check_arg(
                Arg("std::function<void(FLAlertLayer*, bool)>", "cb"),
                info,
                5,
                "arg4",
                "geode.createQuickPopup",
            )
        )
        self.assertIn("FLAlertLayer* p0; bool p1;", text)
        self.assertNotIn("p0, bool p1", text)

    def test_sel_menu_handler_emits_trampoline(self) -> None:
        info = TypeInfo(
            kind="sel",
            cxx_type="SEL_MenuHandler",
            lua_type="(sender: CCObject) -> ()",
        )
        text = "".join(
            check_arg(Arg("SEL_MenuHandler", "sel"), info, 3, "sel", "CCMenuItem.init")
        )
        self.assertIn("LuaMenuHandler::create", text)
        self.assertIn("sel_handler", text)

    def test_sel_menu_call_args(self) -> None:
        text = sel_menu_call_args("sel2")
        self.assertEqual(
            text,
            ["sel2_handler", "menu_selector(luax::LuaMenuHandler::onCallback)"],
        )


class EmitStackCheckTests(unittest.TestCase):
    def test_ccpoint_uses_check_specialization(self) -> None:
        info = TypeInfo(kind="value", cxx_type="cocos2d::CCPoint", lua_type="CCPoint")
        text = "".join(emit_stack_check(info, 1, "pt", "test"))
        self.assertIn("luax::check<cocos2d::CCPoint>", text)
        self.assertNotIn("toPoint", text)

    def test_ccrect_uses_check_specialization(self) -> None:
        info = TypeInfo(kind="value", cxx_type="cocos2d::CCRect", lua_type="CCRect")
        text = "".join(emit_stack_check(info, 1, "rect", "test"))
        self.assertIn("luax::check<cocos2d::CCRect>", text)

    def test_assign_mode_emits_assignment(self) -> None:
        info = TypeInfo(kind="number", lua_type="number", cxx_type="int")
        text = "".join(emit_stack_check(info, 1, "v", "test", declare=False))
        self.assertIn("v = static_cast<int>", text)
        self.assertNotIn("auto v", text)

    def test_allow_nil_object_branch(self) -> None:
        info = TypeInfo(
            kind="object",
            cxx_type="cocos2d::CCNode*",
            lua_type="CCNode",
            class_name="CCNode",
        )
        text = "".join(emit_stack_check(info, 1, "node", "test", allow_nil_object=True))
        self.assertIn("lua_isnil(L, 1)", text)
        self.assertIn("node = nullptr", text)
        self.assertIn("Usertype<cocos2d::CCNode>::check", text)

    def test_char_ptr_storage_assign_mode(self) -> None:
        info = TypeInfo(kind="string", lua_type="string", cxx_type="char const*")
        text = "".join(emit_stack_check(info, 1, "text", "test", declare=False))
        self.assertIn("text = text_storage.c_str()", text)

    def test_value_push_uses_push_overload(self) -> None:
        info = TypeInfo(kind="value", cxx_type="cocos2d::CCPoint", lua_type="CCPoint")
        text = "".join(push_value(info, "pt"))
        self.assertIn("luax::push(L, pt)", text)
        self.assertNotIn("pushPoint", text)

    def test_vector_view_push_uses_readonly_helper(self) -> None:
        element = TypeInfo(
            kind="object",
            cxx_type="cocos2d::CCObject*",
            lua_type="CCObject",
            class_name="CCObject",
        )
        info = TypeInfo(
            kind="vector_view",
            cxx_type="gd::vector<cocos2d::CCObject*>",
            lua_type="{ CCObject? }",
            class_name="CCObject",
            element_type=element,
        )

        text = "".join(push_value(info, "self->m_children", owner_expr="self"))

        self.assertIn("luax::pushReadOnlyVectorView<cocos2d::CCObject>", text)
        self.assertIn("self->m_children", text)
        self.assertIn("self", text)
