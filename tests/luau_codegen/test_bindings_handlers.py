from __future__ import annotations

import unittest
from helpers import (
    Arg,  # type: ignore[import-unresolved]
    Class,  # type: ignore[import-unresolved]
    Function,  # type: ignore[import-unresolved]
    Method,  # type: ignore[import-unresolved]
    _emit_class_file,  # type: ignore[import-unresolved]
    all_platforms,  # type: ignore[import-unresolved]
    emit_free_functions_file,  # type: ignore[import-unresolved]
)


class SelMenuHandlerBindingTests(unittest.TestCase):
    def test_sel_pair_collapses_target_and_anchors_on_result(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        ccsprite = Class(name="CCSprite", namespace="cocos2d", bases=["CCObject"])
        menu_item = Class(
            name="CCMenuItem",
            namespace="cocos2d",
            bases=["CCNode"],
            methods=[
                Method(
                    name="initWithNormalSprite",
                    ret="bool",
                    args=[
                        Arg("cocos2d::CCSprite*", "normal"),
                        Arg("cocos2d::CCSprite*", "selected"),
                        Arg("cocos2d::CCObject*", "target"),
                        Arg("SEL_MenuHandler", "selector"),
                    ],
                    platforms=all_platforms("0x1"),
                )
            ],
        )
        ccnode = Class(name="CCNode", namespace="cocos2d", bases=["CCObject"])
        objects = {
            "CCObject": ccobject,
            "CCSprite": ccsprite,
            "CCMenuItem": menu_item,
            "CCNode": ccnode,
            "cocos2d::CCObject": ccobject,
            "cocos2d::CCSprite": ccsprite,
            "cocos2d::CCMenuItem": menu_item,
            "cocos2d::CCNode": ccnode,
        }

        text = _emit_class_file(
            menu_item,
            {"initWithNormalSprite": menu_item.methods},
            [],
            [],
            objects,
            set(),
            1,
            "win",
        )

        self.assertIn("expected 4 args", text)
        self.assertIn("LuaMenuHandler::create", text)
        self.assertIn("menu_selector(luax::LuaMenuHandler::onCallback)", text)
        self.assertNotIn("arg2 = luax::Usertype<cocos2d::CCObject>", text)
        self.assertIn("anchorMenuHandler(self, sel3_handler)", text)

    def test_orphan_sel_mid_args_uses_handler_in_call(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        ccnode = Class(name="CCNode", namespace="cocos2d", bases=["CCObject"])
        ccmenu = Class(name="CCMenu", namespace="cocos2d", bases=["CCNode"])
        editor = Class(
            name="EditorUI",
            bases=["CCNode"],
            methods=[
                Method(
                    name="getButton",
                    ret="CCMenuItemSpriteExtra*",
                    args=[
                        Arg("char const*", "text"),
                        Arg("int", "tag"),
                        Arg("SEL_MenuHandler", "selector"),
                        Arg("cocos2d::CCMenu*", "menu"),
                    ],
                    platforms=all_platforms("0x1"),
                )
            ],
        )
        menu_item_extra = Class(name="CCMenuItemSpriteExtra", bases=["CCMenuItem"])
        objects = {
            "CCObject": ccobject,
            "CCNode": ccnode,
            "CCMenu": ccmenu,
            "EditorUI": editor,
            "CCMenuItemSpriteExtra": menu_item_extra,
            "cocos2d::CCObject": ccobject,
            "cocos2d::CCNode": ccnode,
            "cocos2d::CCMenu": ccmenu,
        }

        text = _emit_class_file(
            editor,
            {"getButton": editor.methods},
            [],
            [],
            objects,
            set(),
            1,
            "win",
        )

        self.assertIn("sel2_handler", text)
        self.assertIn(
            "getButton(arg0, arg1, menu_selector(luax::LuaMenuHandler::onCallback), arg3)",
            text,
        )
        self.assertNotIn("getButton(arg0, arg1, arg2, arg3)", text)
        self.assertIn("anchorMenuHandler(result, sel2_handler)", text)

    def test_static_create_orphan_sel_uses_handler_in_call(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        ccnode = Class(name="CCNode", namespace="cocos2d", bases=["CCObject"])
        slider = Class(
            name="Slider",
            bases=["CCNode"],
            methods=[
                Method(
                    name="create",
                    ret="Slider*",
                    args=[
                        Arg("cocos2d::CCNode*", "target"),
                        Arg("SEL_MenuHandler", "handler"),
                    ],
                    is_static=True,
                    platforms=all_platforms("0x1"),
                )
            ],
        )
        objects = {
            "CCObject": ccobject,
            "CCNode": ccnode,
            "Slider": slider,
            "cocos2d::CCObject": ccobject,
            "cocos2d::CCNode": ccnode,
        }

        text = _emit_class_file(
            slider,
            {"create": slider.methods},
            [],
            [],
            objects,
            set(),
            1,
            "win",
        )

        self.assertIn(
            "Slider::create(arg0, menu_selector(luax::LuaMenuHandler::onCallback))",
            text,
        )
        self.assertIn("anchorMenuHandler(result, sel1_handler)", text)


class CCNodeScheduleBindingTests(unittest.TestCase):
    def test_ccnode_schedule_emits_schedule_selector_and_anchor_trampoline(
        self,
    ) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        ccnode = Class(
            name="CCNode",
            namespace="cocos2d",
            bases=["CCObject"],
            methods=[
                Method(
                    name="schedule",
                    ret="void",
                    args=[
                        Arg("SEL_SCHEDULE", "selector"),
                        Arg("float", "interval"),
                    ],
                    platforms=all_platforms("0x1"),
                )
            ],
        )
        objects = {
            "CCObject": ccobject,
            "CCNode": ccnode,
            "cocos2d::CCObject": ccobject,
            "cocos2d::CCNode": ccnode,
        }

        text = _emit_class_file(
            ccnode,
            {m.name: [m] for m in ccnode.methods},
            [],
            [],
            objects,
            set(),
            1,
            "win",
        )

        self.assertIn(
            "schedule_selector(luax::LuaScheduleHandler::onSchedule)",
            text,
        )
        self.assertIn("anchorTrampoline(self, sel0_handler)", text)
        self.assertIn("scheduleSelector(", text)

    def test_ccnode_unschedule_passes_literal_schedule_selector(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        ccnode = Class(
            name="CCNode",
            namespace="cocos2d",
            bases=["CCObject"],
            methods=[
                Method(
                    name="unschedule",
                    ret="void",
                    args=[Arg("SEL_SCHEDULE", "selector")],
                    platforms=all_platforms("0x1"),
                )
            ],
        )
        objects = {
            "CCObject": ccobject,
            "CCNode": ccnode,
            "cocos2d::CCObject": ccobject,
            "cocos2d::CCNode": ccnode,
        }

        text = _emit_class_file(
            ccnode,
            {m.name: [m] for m in ccnode.methods},
            [],
            [],
            objects,
            set(),
            1,
            "win",
        )

        self.assertIn(
            "self->unschedule(schedule_selector(luax::LuaScheduleHandler::onSchedule))",
            text,
        )

    def test_sel_then_target_pair_uses_selector_first_in_call(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        ccnode = Class(name="CCNode", namespace="cocos2d", bases=["CCObject"])
        menu_item = Class(
            name="CCMenuItem",
            namespace="cocos2d",
            bases=["CCNode"],
            methods=[
                Method(
                    name="initWithTarget",
                    ret="bool",
                    args=[
                        Arg("SEL_MenuHandler", "selector"),
                        Arg("cocos2d::CCObject*", "target"),
                    ],
                    platforms=all_platforms("0x1"),
                )
            ],
        )
        objects = {
            "CCObject": ccobject,
            "CCNode": ccnode,
            "CCMenuItem": menu_item,
            "cocos2d::CCObject": ccobject,
            "cocos2d::CCNode": ccnode,
            "cocos2d::CCMenuItem": menu_item,
        }

        text = _emit_class_file(
            menu_item,
            {m.name: [m] for m in menu_item.methods},
            [],
            [],
            objects,
            set(),
            1,
            "win",
        )

        self.assertIn(
            "menu_selector(luax::LuaMenuHandler::onCallback), sel0_handler",
            text,
        )


class LazySpriteCallbackBindingTests(unittest.TestCase):
    def test_set_load_callback_emits_result_callback_lambda(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        lazy_sprite = Class(
            name="LazySprite",
            bases=["CCObject"],
            methods=[
                Method(
                    name="setLoadCallback",
                    ret="void",
                    args=[Arg("Callback", "cb")],
                    platforms=all_platforms("0x1"),
                )
            ],
        )
        objects = {"CCObject": ccobject, "LazySprite": lazy_sprite}

        text = _emit_class_file(
            lazy_sprite,
            {m.name: [m] for m in lazy_sprite.methods},
            [],
            [],
            objects,
            set(),
            1,
            "win",
        )

        self.assertIn("geode::Result<> arg0_p0", text)
        self.assertIn(".isOk()", text)
        self.assertIn("self->setLoadCallback(std::move(arg0))", text)
        self.assertIn("expected 2 args", text)


class FreeFunctionSelMenuHandlerBindingTests(unittest.TestCase):
    def test_sel_pair_collapses_and_anchors_on_result(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        ccnode = Class(name="CCNode", namespace="cocos2d", bases=["CCObject"])
        menu_item = Class(name="CCMenuItem", namespace="cocos2d", bases=["CCNode"])
        fn = Function(
            name="makeItem",
            namespace="geode::utils",
            ret="cocos2d::CCMenuItem*",
            args=[
                Arg("cocos2d::CCObject*", "target"),
                Arg("SEL_MenuHandler", "selector"),
            ],
        )
        objects = {
            "CCObject": ccobject,
            "CCNode": ccnode,
            "CCMenuItem": menu_item,
            "cocos2d::CCObject": ccobject,
            "cocos2d::CCNode": ccnode,
            "cocos2d::CCMenuItem": menu_item,
        }

        text = emit_free_functions_file([fn], objects)

        self.assertIn("expected 1 args", text)
        self.assertIn("LuaMenuHandler::create", text)
        self.assertIn("menu_selector(luax::LuaMenuHandler::onCallback)", text)
        self.assertNotIn("arg0 = luax::Usertype<cocos2d::CCObject>", text)
        self.assertIn("anchorMenuHandler(result, sel1_handler)", text)
