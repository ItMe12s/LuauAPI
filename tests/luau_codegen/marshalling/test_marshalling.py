from __future__ import annotations

import os
import unittest

from test_support import ROOT
from luau_codegen.convert.marshalling import (  # type: ignore[import-unresolved]
    _push_impl,
    check_arg,
    emit_stack_check,
    push_return,
    push_value,
    sel_call_args,
    sel_menu_call_args,
    sel_selector_call_arg,
)
from luau_codegen.convert.type_map import TypeInfo, classify_arg, classify_return  # type: ignore[import-unresolved]
from luau_codegen.parse.broma import Arg  # type: ignore[import-unresolved]


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
            os.path.join(ROOT, "src", "framework", "stack", "Stack.hpp"),
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

    def test_seed_value_uses_int_marshalling(self) -> None:
        info = TypeInfo(
            kind="seed_value",
            lua_type="number",
            cxx_type="geode::SeedValueRSV",
        )
        arg = Arg(type="geode::SeedValueRSV", name="attempts")
        check_text = "".join(check_arg(arg, info, 1, "v", "test"))
        push_text = "".join(push_value(info, "self->m_attempts"))
        self.assertIn("check<int>", check_text)
        self.assertNotIn("check<geode::SeedValueRSV>", check_text)
        self.assertIn("static_cast<int>(self->m_attempts)", push_text)
        self.assertIn("lua_pushnumber", push_text)


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
        text = "".join(check_arg(Arg("SEL_MenuHandler", "sel"), info, 3, "sel", "CCMenuItem.init"))
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
        text = "".join(check_arg(Arg("SEL_SCHEDULE", "sel"), info, 2, "sel", "CCNode.schedule"))
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
            check_arg(Arg("Callback", "cb"), info, 2, "arg0", "LazySprite.setLoadCallback")
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

    def test_callback_object_return_allows_nil(self) -> None:
        sender = TypeInfo(
            kind="object",
            cxx_type="cocos2d::CCObject*",
            lua_type="CCObject?",
            class_name="CCObject",
        )
        ret = TypeInfo(
            kind="object",
            cxx_type="cocos2d::CCObject*",
            lua_type="CCObject?",
            class_name="CCObject",
        )
        info = TypeInfo(
            kind="callback",
            cxx_type="std::function<cocos2d::CCObject*(cocos2d::CCObject*)>",
            lua_type="(arg1: CCObject) -> CCObject?",
            callback_args=(sender,),
            callback_ret=ret,
        )
        text = "".join(
            check_arg(
                Arg("std::function<cocos2d::CCObject*(cocos2d::CCObject*)>", "cb"),
                info,
                2,
                "cb",
                "Test.method",
            )
        )
        self.assertIn("if (lua_isnil(L, -1))", text)
        self.assertIn("*slot = nullptr", text)

    def test_callback_string_return_emits_check(self) -> None:
        ret = TypeInfo(kind="string", cxx_type="gd::string", lua_type="string")
        info = TypeInfo(
            kind="callback",
            cxx_type="std::function<gd::string()>",
            lua_type="() -> string",
            callback_args=(),
            callback_ret=ret,
        )
        text = "".join(
            check_arg(
                Arg("std::function<gd::string()>", "cb"),
                info,
                2,
                "cb",
                "Test.method",
            )
        )
        self.assertIn("check<std::string>", text)
        self.assertIn("return std::string()", text)

    def test_unknown_sel_variant_fails_codegen(self) -> None:
        from luau_codegen.convert.marshalling import _sel_variant  # type: ignore[import-unresolved]

        info = TypeInfo(
            kind="sel",
            cxx_type="SEL_Unknown",
            lua_type="() -> ()",
            class_name="unknown",
        )
        with self.assertRaisesRegex(ValueError, "unknown SEL variant"):
            _sel_variant(info)

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


class GeodeTaskHandleMarshallingTests(unittest.TestCase):
    def test_task_handle_return_pushes_bridge_handle(self) -> None:
        info = classify_return("arc::TaskHandle<bool>", {})
        self.assertIsNotNone(info)
        assert info is not None
        text = "".join(push_return(info, "result", False))
        self.assertIn("pushGeodeTaskHandle<bool>", text)
        self.assertIn("std::move(result)", text)
        self.assertIn("static_cast<bool const*>", text)
        self.assertIn("return 1", text)

    def test_optional_task_handle_return_pushes_optional_bridge_handle(self) -> None:
        info = classify_return("std::optional<arc::TaskHandle<bool>>", {})
        self.assertIsNotNone(info)
        assert info is not None
        text = "".join(push_return(info, "result", False))
        self.assertIn("pushOptionalGeodeTaskHandle<bool>", text)
        self.assertIn("std::move(result)", text)

    def test_void_task_handle_return_uses_null_pusher(self) -> None:
        info = classify_return("arc::TaskHandle<void>", {})
        self.assertIsNotNone(info)
        assert info is not None
        text = "".join(push_return(info, "result", False))
        self.assertIn("pushGeodeTaskHandle<void>", text)
        self.assertIn("nullptr", text)


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

    def test_smart_prefab_result_uses_check_specialization(self) -> None:
        info = TypeInfo(kind="value", cxx_type="SmartPrefabResult", lua_type="SmartPrefabResult")
        text = "".join(emit_stack_check(info, 1, "result", "test"))
        self.assertIn("luax::check<SmartPrefabResult>", text)

    def test_smart_prefab_result_push_uses_push_overload(self) -> None:
        info = TypeInfo(kind="value", cxx_type="SmartPrefabResult", lua_type="SmartPrefabResult")
        text = "".join(push_value(info, "result"))
        self.assertIn("luax::push(L, result)", text)

    def test_ccobject_push_uses_dynamic_borrowed(self) -> None:
        info = TypeInfo(
            kind="object",
            cxx_type="cocos2d::CCObject*",
            lua_type="CCObject?",
            class_name="CCObject",
        )
        text = "".join(push_value(info, "result"))
        self.assertIn("luax::Usertype<cocos2d::CCObject>::pushBorrowedDynamic(L, result);", text)
        self.assertNotIn("pushBorrowed(L, result)", text)

    def test_ccobject_push_uses_dynamic_owned(self) -> None:
        info = TypeInfo(
            kind="object",
            cxx_type="cocos2d::CCObject*",
            lua_type="CCObject?",
            class_name="CCObject",
        )
        text = "".join(push_return(info, "result", True))
        self.assertIn("luax::Usertype<cocos2d::CCObject>::pushOwnedDynamic(L, result);", text)
        self.assertNotIn("pushOwned(L, result)", text)

    def test_ccnode_push_uses_dynamic_borrowed(self) -> None:
        info = TypeInfo(
            kind="object",
            cxx_type="cocos2d::CCNode*",
            lua_type="CCNode?",
            class_name="CCNode",
        )
        text = "".join(push_value(info, "result"))
        self.assertIn("luax::Usertype<cocos2d::CCNode>::pushBorrowedDynamic(L, result);", text)
        self.assertNotIn("Usertype<cocos2d::CCObject>::pushBorrowedDynamic(L, result);", text)

    def test_ccnode_push_uses_dynamic_owned(self) -> None:
        info = TypeInfo(
            kind="object",
            cxx_type="cocos2d::CCNode*",
            lua_type="CCNode?",
            class_name="CCNode",
        )
        text = "".join(push_return(info, "result", True))
        self.assertIn("luax::Usertype<cocos2d::CCNode>::pushOwnedDynamic(L, result);", text)
        self.assertNotIn("Usertype<cocos2d::CCObject>::pushOwnedDynamic(L, result);", text)

    def test_specific_object_push_unchanged(self) -> None:
        info = TypeInfo(
            kind="object",
            cxx_type="CCMenuItemSpriteExtra*",
            lua_type="CCMenuItemSpriteExtra?",
            class_name="CCMenuItemSpriteExtra",
        )
        text = "".join(push_value(info, "result"))
        self.assertIn("luax::Usertype<CCMenuItemSpriteExtra>::pushBorrowed(L, result);", text)
        self.assertNotIn("pushBorrowedDynamic", text)

    def test_map_string_smart_prefab_result_marshalling(self) -> None:
        value = TypeInfo(kind="value", cxx_type="SmartPrefabResult", lua_type="SmartPrefabResult")
        key = TypeInfo(kind="string", cxx_type="gd::string", lua_type="string")
        info = TypeInfo(
            kind="map",
            cxx_type="gd::map<gd::string, SmartPrefabResult>",
            lua_type="{ [string]: SmartPrefabResult }",
            key_type=key,
            value_type=value,
        )
        check_text = "".join(emit_stack_check(info, 1, "map", "test"))
        self.assertIn(
            "luax::detail::checkAssociativeMap<gd::string, SmartPrefabResult, gd::map<gd::string, SmartPrefabResult>>",
            check_text,
        )

    def test_ccblendfunc_uses_check_specialization(self) -> None:
        info = TypeInfo(kind="value", cxx_type="cocos2d::ccBlendFunc", lua_type="BlendFunc")
        text = "".join(emit_stack_check(info, 1, "blend", "test"))
        self.assertIn("luax::check<cocos2d::ccBlendFunc>", text)

    def test_cchsvvalue_uses_check_specialization(self) -> None:
        info = TypeInfo(kind="value", cxx_type="cocos2d::ccHSVValue", lua_type="HSVValue")
        text = "".join(emit_stack_check(info, 1, "hsv", "test"))
        self.assertIn("luax::check<cocos2d::ccHSVValue>", text)

    def test_cccolor4f_push_uses_push_overload(self) -> None:
        info = TypeInfo(kind="value", cxx_type="cocos2d::ccColor4F", lua_type="RGBAFloatColor")
        text = "".join(push_value(info, "color"))
        self.assertIn("luax::push(L, color)", text)

    def test_ccaffinetransform_uses_check_specialization(self) -> None:
        info = TypeInfo(
            kind="value",
            cxx_type="cocos2d::CCAffineTransform",
            lua_type="CCAffineTransform",
        )
        text = "".join(emit_stack_check(info, 1, "t", "test"))
        self.assertIn("luax::check<cocos2d::CCAffineTransform>", text)

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

    def test_vector_view_opaque_push_uses_opaque_readonly_helper(self) -> None:
        element = TypeInfo(
            kind="opaque_handle",
            cxx_type="GroupCommandObject2*",
            lua_type="GroupCommandObject2",
        )
        info = TypeInfo(
            kind="vector_view",
            cxx_type="gd::vector<GroupCommandObject2*>",
            lua_type="{ GroupCommandObject2? }",
            class_name="GroupCommandObject2",
            element_type=element,
        )

        text = "".join(push_value(info, "self->m_groupObjects", owner_expr="self"))

        self.assertIn("luax::pushReadOnlyOpaqueVectorView<GroupCommandObject2>", text)
        self.assertIn("self->m_groupObjects", text)

    def test_vector_view_opaque_check_uses_opaque_vector_helper(self) -> None:
        element = TypeInfo(
            kind="opaque_handle",
            cxx_type="FMOD::Sound*",
            lua_type="FMODSoundHandle",
        )
        info = TypeInfo(
            kind="vector_view",
            cxx_type="gd::vector<FMOD::Sound*>",
            lua_type="{ FMODSoundHandle? }",
            class_name="FMODSoundHandle",
            element_type=element,
        )

        text = "".join(
            check_arg(
                Arg("gd::vector<FMOD::Sound*>", "sounds"),
                info,
                2,
                "arg0",
                "FMODAudioEngine.setSounds",
            )
        )

        self.assertIn("luax::checkOpaqueVectorView<FMOD::Sound>", text)

    def test_primitive_vector_check_uses_table_helper(self) -> None:
        element = TypeInfo(kind="number", cxx_type="int", lua_type="number")
        info = TypeInfo(
            kind="primitive_vector",
            cxx_type="gd::vector<int>",
            lua_type="{ number }",
            element_type=element,
        )

        text = "".join(
            check_arg(
                Arg("gd::vector<int>", "values"),
                info,
                2,
                "arg0",
                "Foo.take",
            )
        )

        self.assertIn("luax::checkPrimitiveVector<int>", text)
        self.assertIn("arg0", text)

    def test_primitive_vector_push_uses_table_helper(self) -> None:
        element = TypeInfo(kind="number", cxx_type="int", lua_type="number")
        info = TypeInfo(
            kind="primitive_vector",
            cxx_type="gd::vector<int>",
            lua_type="{ number }",
            element_type=element,
        )

        text = "".join(push_return(info, "result", False))

        self.assertIn("luax::pushPrimitiveVector<int>", text)
        self.assertIn("result", text)

    def test_primitive_vector_bool_check_and_push(self) -> None:
        element = TypeInfo(kind="bool", cxx_type="bool", lua_type="boolean")
        info = TypeInfo(
            kind="primitive_vector",
            cxx_type="gd::vector<bool>",
            lua_type="{ boolean }",
            element_type=element,
        )

        check_text = "".join(check_arg(Arg("gd::vector<bool>", "flags"), info, 1, "arg0", "test"))
        push_text = "".join(push_value(info, "flags"))

        self.assertIn("luax::checkPrimitiveVector<bool>", check_text)
        self.assertIn("luax::pushPrimitiveVector<bool>", push_text)

    def test_primitive_vector_wideint_check_and_push(self) -> None:
        element = TypeInfo(kind="wideint", cxx_type="uint64_t", lua_type="string")
        info = TypeInfo(
            kind="primitive_vector",
            cxx_type="gd::vector<uint64_t>",
            lua_type="{ string }",
            element_type=element,
        )

        check_text = "".join(check_arg(Arg("gd::vector<uint64_t>", "ids"), info, 1, "arg0", "test"))
        push_text = "".join(push_value(info, "ids"))

        self.assertIn("luax::checkPrimitiveVector<uint64_t>", check_text)
        self.assertIn("luax::pushPrimitiveVector<uint64_t>", push_text)

    def test_std_array_check_uses_fixed_length_helper(self) -> None:
        element = TypeInfo(kind="number", cxx_type="int", lua_type="number")
        info = TypeInfo(
            kind="std_array",
            cxx_type="std::array<int, 300>",
            lua_type="{ number }",
            element_type=element,
            array_size=300,
        )

        text = "".join(
            check_arg(
                Arg("std::array<int, 300>", "values"),
                info,
                2,
                "arg0",
                "Foo.take",
            )
        )

        self.assertIn("luax::checkStdArray<int, 300>", text)

    def test_std_array_push_uses_fixed_length_helper(self) -> None:
        element = TypeInfo(
            kind="value",
            cxx_type="cocos2d::CCPoint",
            lua_type="CCPoint",
        )
        info = TypeInfo(
            kind="std_array",
            cxx_type="std::array<cocos2d::CCPoint, 4>",
            lua_type="{ CCPoint }",
            element_type=element,
            array_size=4,
        )

        text = "".join(push_value(info, "points"))

        self.assertIn("luax::pushStdArray<cocos2d::CCPoint, 4>", text)


class ContainerTableMarshallingTests(unittest.TestCase):
    def test_map_check_and_push_use_table_helpers(self) -> None:
        key = TypeInfo(kind="number", cxx_type="int", lua_type="number")
        value = TypeInfo(kind="bool", cxx_type="bool", lua_type="boolean")
        info = TypeInfo(
            kind="map",
            cxx_type="gd::map<int, bool>",
            lua_type="{ [number]: boolean }",
            key_type=key,
            value_type=value,
        )

        check_text = "".join(
            check_arg(Arg("gd::map<int, bool>", "values"), info, 1, "arg0", "test")
        )
        push_text = "".join(push_value(info, "values"))

        self.assertIn(
            "luax::detail::checkAssociativeMap<int, bool, gd::map<int, bool>>", check_text
        )
        self.assertIn("luax::detail::pushAssociativeMap<int, bool, gd::map<int, bool>>", push_text)

    def test_unordered_map_check_and_push_use_table_helpers(self) -> None:
        key = TypeInfo(kind="string", cxx_type="std::string", lua_type="string")
        value = TypeInfo(kind="number", cxx_type="int", lua_type="number")
        info = TypeInfo(
            kind="unordered_map",
            cxx_type="gd::unordered_map<std::string, int>",
            lua_type="{ [string]: number }",
            key_type=key,
            value_type=value,
        )

        check_text = "".join(
            check_arg(
                Arg("gd::unordered_map<std::string, int>", "values"),
                info,
                1,
                "arg0",
                "test",
            )
        )
        push_text = "".join(push_value(info, "values"))

        self.assertIn(
            "luax::detail::checkAssociativeMap<std::string, int, gd::unordered_map<std::string, int>>",
            check_text,
        )
        self.assertIn(
            "luax::detail::pushAssociativeMap<std::string, int, gd::unordered_map<std::string, int>>",
            push_text,
        )

    def test_set_check_and_push_use_table_helpers(self) -> None:
        element = TypeInfo(kind="number", cxx_type="int", lua_type="number")
        info = TypeInfo(
            kind="set",
            cxx_type="gd::set<int>",
            lua_type="{ number }",
            element_type=element,
        )

        check_text = "".join(check_arg(Arg("gd::set<int>", "values"), info, 1, "arg0", "test"))
        push_text = "".join(push_value(info, "values"))

        self.assertIn("luax::detail::checkSetFromTable<int, gd::set<int>>", check_text)
        self.assertIn("luax::detail::pushSetAsTable<int, gd::set<int>>", push_text)

    def test_unordered_set_check_and_push_use_table_helpers(self) -> None:
        element = TypeInfo(kind="number", cxx_type="int", lua_type="number")
        info = TypeInfo(
            kind="unordered_set",
            cxx_type="gd::unordered_set<int>",
            lua_type="{ number }",
            element_type=element,
        )

        check_text = "".join(
            check_arg(
                Arg("gd::unordered_set<int>", "values"),
                info,
                1,
                "arg0",
                "test",
            )
        )
        push_text = "".join(push_value(info, "values"))

        self.assertIn("luax::detail::checkSetFromTable<int, gd::unordered_set<int>>", check_text)
        self.assertIn("luax::detail::pushSetAsTable<int, gd::unordered_set<int>>", push_text)

    def test_object_set_check_and_push_use_table_helpers(self) -> None:
        element = TypeInfo(
            kind="object",
            cxx_type="cocos2d::CCObject*",
            lua_type="CCObject",
            class_name="CCObject",
        )
        info = TypeInfo(
            kind="set",
            cxx_type="gd::set<cocos2d::CCObject*>",
            lua_type="{ CCObject? }",
            element_type=element,
        )

        check_text = "".join(
            check_arg(Arg("gd::set<cocos2d::CCObject*>", "values"), info, 1, "arg0", "test")
        )
        push_text = "".join(push_value(info, "values"))

        self.assertIn(
            "luax::detail::checkSetFromTable<cocos2d::CCObject*, gd::set<cocos2d::CCObject*>>",
            check_text,
        )
        self.assertIn(
            "luax::detail::pushSetAsTable<cocos2d::CCObject*, gd::set<cocos2d::CCObject*>>",
            push_text,
        )

    def test_object_unordered_set_check_and_push_use_table_helpers(self) -> None:
        element = TypeInfo(
            kind="object",
            cxx_type="cocos2d::CCObject*",
            lua_type="CCObject",
            class_name="CCObject",
        )
        info = TypeInfo(
            kind="unordered_set",
            cxx_type="gd::unordered_set<cocos2d::CCObject*>",
            lua_type="{ CCObject? }",
            element_type=element,
        )

        check_text = "".join(
            check_arg(
                Arg("gd::unordered_set<cocos2d::CCObject*>", "values"),
                info,
                1,
                "arg0",
                "test",
            )
        )
        push_text = "".join(push_value(info, "values"))

        self.assertIn(
            "luax::detail::checkSetFromTable<cocos2d::CCObject*, gd::unordered_set<cocos2d::CCObject*>>",
            check_text,
        )
        self.assertIn(
            "luax::detail::pushSetAsTable<cocos2d::CCObject*, gd::unordered_set<cocos2d::CCObject*>>",
            push_text,
        )

    def test_pair_check_and_push_use_record_helpers(self) -> None:
        first = TypeInfo(kind="number", cxx_type="int", lua_type="number")
        second = TypeInfo(kind="number", cxx_type="int", lua_type="number")
        info = TypeInfo(
            kind="pair",
            cxx_type="std::pair<int, int>",
            lua_type="{ first: number, second: number }",
            key_type=first,
            value_type=second,
        )

        check_text = "".join(
            check_arg(Arg("std::pair<int, int>", "entry"), info, 1, "arg0", "test")
        )
        push_text = "".join(push_value(info, "entry"))

        self.assertIn("luax::checkPair<int, int>", check_text)
        self.assertIn("luax::pushPair<int, int>", push_text)

    def test_map_pair_value_check_and_push(self) -> None:
        key = TypeInfo(kind="number", cxx_type="int", lua_type="number")
        first = TypeInfo(kind="number", cxx_type="int", lua_type="number")
        second = TypeInfo(kind="number", cxx_type="int", lua_type="number")
        pair_val = TypeInfo(
            kind="pair",
            cxx_type="std::pair<int, int>",
            lua_type="{ first: number, second: number }",
            key_type=first,
            value_type=second,
        )
        info = TypeInfo(
            kind="unordered_map",
            cxx_type="gd::unordered_map<int, std::pair<int, int>>",
            lua_type="{ [number]: { first: number, second: number } }",
            key_type=key,
            value_type=pair_val,
        )

        check_text = "".join(
            check_arg(
                Arg("gd::unordered_map<int, std::pair<int, int>>", "groups"),
                info,
                1,
                "arg0",
                "test",
            )
        )
        push_text = "".join(push_value(info, "groups"))

        self.assertIn(
            "luax::detail::checkAssociativeMap<int, std::pair<int, int>, gd::unordered_map<int, std::pair<int, int>>>",
            check_text,
        )
        self.assertIn(
            "luax::detail::pushAssociativeMap<int, std::pair<int, int>, gd::unordered_map<int, std::pair<int, int>>>",
            push_text,
        )

    def test_map_pair_key_check_and_push_use_entry_list_helpers(self) -> None:
        first = TypeInfo(kind="number", cxx_type="int", lua_type="number")
        second = TypeInfo(kind="number", cxx_type="int", lua_type="number")
        pair_key = TypeInfo(
            kind="pair",
            cxx_type="std::pair<int, int>",
            lua_type="{ first: number, second: number }",
            key_type=first,
            value_type=second,
        )
        value = TypeInfo(kind="number", cxx_type="int", lua_type="number")
        info = TypeInfo(
            kind="map",
            cxx_type="gd::map<std::pair<int, int>, int>",
            lua_type="{ { first: number, second: number, value: number } }",
            key_type=pair_key,
            value_type=value,
        )

        check_text = "".join(
            check_arg(
                Arg("gd::map<std::pair<int, int>, int>", "accounts"),
                info,
                1,
                "arg0",
                "test",
            )
        )
        push_text = "".join(push_value(info, "accounts"))

        self.assertIn(
            "luax::detail::checkPairKeyAssociativeMap<int, int, int, gd::map<std::pair<int, int>, int>>",
            check_text,
        )
        self.assertIn(
            "luax::detail::pushPairKeyAssociativeMap<int, int, int, gd::map<std::pair<int, int>, int>>",
            push_text,
        )


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

    def test_fmod_channel_arg_uses_opaque_handle_check(self) -> None:
        info = TypeInfo(
            kind="opaque_handle",
            cxx_type="FMOD::Channel*",
            lua_type="FMODChannel",
        )
        text = "".join(check_arg(Arg("FMOD::Channel*", "channel"), info, 2, "arg0", "test"))
        self.assertIn("checkOpaqueHandle<FMOD::Channel>", text)
        self.assertNotIn("lua_islightuserdata", text)
        self.assertNotIn("Usertype<FMOD::Channel>", text)

    def test_fmod_channel_return_pushes_opaque_handle(self) -> None:
        info = TypeInfo(
            kind="opaque_handle",
            cxx_type="FMOD::Channel*",
            lua_type="FMODChannel?",
        )
        text = "".join(push_return(info, "result", False))
        self.assertIn("pushOpaqueHandle(L, result)", text)
        self.assertNotIn("lua_pushlightuserdata", text)
        self.assertNotIn("Usertype<FMOD::Channel>", text)

    def test_fmod_sound_alias_arg_uses_opaque_handle_check(self) -> None:
        info = TypeInfo(
            kind="opaque_handle",
            cxx_type="FMODSound*",
            lua_type="FMODSoundHandle",
        )
        text = "".join(check_arg(Arg("FMODSound*", "sound"), info, 2, "arg0", "test"))
        self.assertIn("checkOpaqueHandle<FMODSound>", text)
        self.assertNotIn("lua_islightuserdata", text)
        self.assertNotIn("Usertype<FMODSound>", text)

    def test_fmod_speaker_mode_enum_arg_uses_numeric_check(self) -> None:
        info = TypeInfo(kind="enum", cxx_type="FMOD_SPEAKERMODE", lua_type="number")
        text = "".join(check_arg(Arg("FMOD_SPEAKERMODE", "mode"), info, 2, "arg0", "test"))
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
