from __future__ import annotations

import os
import unittest
from helpers import (
    Arg,  # type: ignore[import-unresolved]
    ROOT,  # type: ignore[import-unresolved]
    TypeInfo,  # type: ignore[import-unresolved]
    _push_impl,  # type: ignore[import-unresolved]
    check_arg,  # type: ignore[import-unresolved]
    classify_arg,  # type: ignore[import-unresolved]
    emit_stack_check,  # type: ignore[import-unresolved]
    push_return,  # type: ignore[import-unresolved]
    push_value,  # type: ignore[import-unresolved]
    sel_call_args,  # type: ignore[import-unresolved]
    sel_menu_call_args,  # type: ignore[import-unresolved]
    sel_selector_call_arg,  # type: ignore[import-unresolved]
)


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
        self.assertIn("if (!cb_cb->invoke(1, 0", text)
        self.assertIn("callback failed", text)
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

    def test_sel_schedule_handler_emits_trampoline(self) -> None:
        info = TypeInfo(
            kind="sel",
            cxx_type="SEL_SCHEDULE",
            lua_type="(dt: number) -> ()",
            class_name="schedule",
        )
        text = "".join(
            check_arg(Arg("SEL_SCHEDULE", "sel"), info, 2, "sel", "CCNode.schedule")
        )
        self.assertIn("LuaScheduleHandler::create", text)
        self.assertIn("sel_handler", text)

    def test_sel_schedule_call_args_handler_first(self) -> None:
        info = TypeInfo(
            kind="sel",
            cxx_type="SEL_SCHEDULE",
            lua_type="(dt: number) -> ()",
            class_name="schedule",
        )
        text = sel_call_args("sel", info, handler_first=True)
        self.assertEqual(
            text,
            ["sel_handler", "schedule_selector(luax::LuaScheduleHandler::onSchedule)"],
        )

    def test_sel_schedule_call_args_selector_first(self) -> None:
        info = TypeInfo(
            kind="sel",
            cxx_type="SEL_SCHEDULE",
            lua_type="(dt: number) -> ()",
            class_name="schedule",
        )
        text = sel_call_args("sel", info, handler_first=False)
        self.assertEqual(
            text,
            ["schedule_selector(luax::LuaScheduleHandler::onSchedule)", "sel_handler"],
        )

    def test_result_callback_arg_push(self) -> None:
        info = TypeInfo(
            kind="result",
            cxx_type="geode::Result<>",
            lua_type="boolean | string",
        )
        text = "".join(_push_impl(info, "c->result", False, indent=""))
        self.assertIn(".isOk()", text)
        self.assertIn(".unwrapErr()", text)
        self.assertIn("lua_pushboolean", text)

    def test_callback_with_result_arg_emits_lambda(self) -> None:
        info = TypeInfo(
            kind="callback",
            cxx_type="geode::Function<void(geode::Result<>)>",
            lua_type="(arg1: boolean | string) -> ()",
            callback_ret=TypeInfo("void", "void", "()"),
            callback_args=(
                TypeInfo(
                    kind="result",
                    cxx_type="geode::Result<>",
                    lua_type="boolean | string",
                ),
            ),
        )
        text = "".join(
            check_arg(
                Arg("Callback", "cb"), info, 2, "arg0", "LazySprite.setLoadCallback"
            )
        )
        self.assertIn("geode::Result<> arg0_p0", text)
        self.assertIn(".isOk()", text)
        self.assertIn("invoke(1, 0", text)

    def test_sel_selector_call_arg_orphan(self) -> None:
        info = TypeInfo(
            kind="sel",
            cxx_type="SEL_MenuHandler",
            lua_type="(sender: CCObject) -> ()",
            class_name="menu",
        )
        self.assertEqual(
            sel_selector_call_arg(info),
            "menu_selector(luax::LuaMenuHandler::onCallback)",
        )

    def test_callback_non_void_return_emits_result_reader(self) -> None:
        sender = TypeInfo(
            kind="object",
            cxx_type="cocos2d::CCObject*",
            lua_type="CCObject",
            class_name="CCObject",
        )
        ret = TypeInfo(kind="bool", cxx_type="bool", lua_type="boolean")
        info = TypeInfo(
            kind="callback",
            cxx_type="std::function<bool(cocos2d::CCObject*)>",
            lua_type="(arg1: CCObject) -> boolean",
            callback_args=(sender,),
            callback_ret=ret,
        )
        text = "".join(
            check_arg(
                Arg("std::function<bool(cocos2d::CCObject*)>", "cb"),
                info,
                2,
                "cb",
                "Test.method",
            )
        )
        self.assertIn("-> bool", text)
        self.assertIn("cb_ret", text)
        self.assertIn("check<bool>", text)
        self.assertIn("if (!cb_cb->invoke(1, 1", text)
        self.assertIn("callback failed", text)
        self.assertIn("return false", text)

    def test_cc_director_delegate_arg_emits_trampoline(self) -> None:
        info = TypeInfo(
            kind="delegate",
            cxx_type="cocos2d::CCDirectorDelegate*",
            lua_type="{ updateProjection: ... }",
            class_name="CCDirectorDelegate",
        )
        text = "".join(
            check_arg(
                Arg("cocos2d::CCDirectorDelegate*", "delegate"),
                info,
                2,
                "delegate",
                "CCDirector.setDelegate",
            )
        )
        self.assertIn("lua_istable", text)
        self.assertIn("luax::LuaCCDirectorDelegate::create", text)
        self.assertIn("delegate_trampoline", text)

    def test_delegate_arg_emits_trampoline(self) -> None:
        info = TypeInfo(
            kind="delegate",
            cxx_type="cocos2d::CCTouchDelegate*",
            lua_type="{ ccTouchBegan: ... }",
            class_name="CCTouchDelegate",
        )
        text = "".join(
            check_arg(
                Arg("cocos2d::CCTouchDelegate*", "delegate"),
                info,
                2,
                "delegate",
                "CCLayer.registerWithTouchDispatcher",
            )
        )
        self.assertIn("lua_istable", text)
        self.assertIn("luax::LuaCCTouchDelegate::create", text)
        self.assertIn("delegate_trampoline", text)

    def test_delegate_return_pushes_bound_table(self) -> None:
        info = TypeInfo(
            kind="delegate",
            cxx_type="cocos2d::CCTouchDelegate*",
            lua_type="CCTouchDelegate?",
            class_name="CCTouchDelegate",
        )
        text = "".join(push_return(info, "result", False))
        self.assertIn("tryPushBoundDelegateTable", text)
        self.assertIn("return 1", text)


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

    def test_vector_view_owned_push_uses_owned_helper(self) -> None:
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

        text = "".join(push_return(info, "result", False, vector_owned=True))

        self.assertIn("luax::pushOwnedReadOnlyVectorView<cocos2d::CCObject>", text)
        self.assertIn("result", text)

    def test_vector_view_check_uses_object_vector_helper(self) -> None:
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

        text = "".join(
            check_arg(
                Arg("gd::vector<cocos2d::CCObject*>", "children"),
                info,
                2,
                "arg0",
                "CCNode.setChildren",
            )
        )

        self.assertIn("luax::checkObjectVectorView<cocos2d::CCObject>", text)
        self.assertIn("arg0", text)


class FmodMarshallingTests(unittest.TestCase):
    def test_fmod_enum_arg_uses_numeric_check(self) -> None:
        info = TypeInfo(kind="enum", cxx_type="FMOD_RESULT", lua_type="number")
        text = "".join(check_arg(Arg("FMOD_RESULT", "result"), info, 2, "arg0", "test"))
        self.assertIn("static_cast<FMOD_RESULT>", text)
        self.assertIn("check<int>", text)

    def test_fmod_enum_return_pushes_number(self) -> None:
        info = TypeInfo(kind="enum", cxx_type="FMOD_OPENSTATE", lua_type="number")
        text = "".join(push_return(info, "state", False))
        self.assertIn("static_cast<int>(state)", text)

    def test_fmod_channel_arg_uses_lightuserdata(self) -> None:
        info = TypeInfo(
            kind="opaque_handle",
            cxx_type="FMOD::Channel*",
            lua_type="FMODChannel",
        )
        text = "".join(
            check_arg(Arg("FMOD::Channel*", "channel"), info, 2, "arg0", "test")
        )
        self.assertIn("lua_islightuserdata", text)
        self.assertIn("static_cast<FMOD::Channel*>", text)
        self.assertNotIn("Usertype<FMOD::Channel>", text)

    def test_fmod_channel_return_pushes_lightuserdata(self) -> None:
        info = TypeInfo(
            kind="opaque_handle",
            cxx_type="FMOD::Channel*",
            lua_type="FMODChannel?",
        )
        text = "".join(push_return(info, "result", False))
        self.assertIn("lua_pushlightuserdata(L, result)", text)
        self.assertIn("lua_pushnil(L)", text)
        self.assertNotIn("Usertype<FMOD::Channel>", text)

    def test_fmod_sound_alias_arg_uses_lightuserdata(self) -> None:
        info = TypeInfo(
            kind="opaque_handle",
            cxx_type="FMODSound*",
            lua_type="FMODSound",
        )
        text = "".join(check_arg(Arg("FMODSound*", "sound"), info, 2, "arg0", "test"))
        self.assertIn("lua_islightuserdata", text)
        self.assertIn("static_cast<FMODSound*>", text)
        self.assertNotIn("Usertype<FMODSound>", text)

    def test_fmod_speaker_mode_enum_arg_uses_numeric_check(self) -> None:
        info = TypeInfo(kind="enum", cxx_type="FMOD_SPEAKERMODE", lua_type="number")
        text = "".join(
            check_arg(Arg("FMOD_SPEAKERMODE", "mode"), info, 2, "arg0", "test")
        )
        self.assertIn("static_cast<FMOD_SPEAKERMODE>", text)
        self.assertIn("check<int>", text)


class ResultPushTests(unittest.TestCase):
    def test_result_push_ok_branch(self) -> None:
        info = TypeInfo("result", "geode::Result<>", "boolean | string")
        text = "".join(_push_impl(info, "value", False))
        self.assertIn("value.isOk()", text)
        self.assertIn("lua_pushboolean(L, true)", text)

    def test_result_push_err_branch(self) -> None:
        info = TypeInfo("result", "geode::Result<>", "boolean | string")
        text = "".join(_push_impl(info, "value", False))
        self.assertIn("value.unwrapErr()", text)
        self.assertIn("luax::push(L, value.unwrapErr())", text)
