from __future__ import annotations

import re
import unittest
from helpers import (
    Arg,  # type: ignore[import-unresolved]
    Class,  # type: ignore[import-unresolved]
    Field,  # type: ignore[import-unresolved]
    Method,  # type: ignore[import-unresolved]
    Root,  # type: ignore[import-unresolved]
    TYPES_FILE,  # type: ignore[import-unresolved]
    _emit_dispatcher,  # type: ignore[import-unresolved]
    _input_arg_count,  # type: ignore[import-unresolved]
    all_platforms,  # type: ignore[import-unresolved]
    classify_arg,  # type: ignore[import-unresolved]
    emit_luau_types,  # type: ignore[import-unresolved]
    is_const_reference,  # type: ignore[import-unresolved]
    is_out_reference,  # type: ignore[import-unresolved]
    types_text,  # type: ignore[import-unresolved]
)


class LuauTypeEmissionTests(unittest.TestCase):
    def test_enum_prelude_uses_valid_luau_names(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        ccnode = Class(
            name="CCNode",
            namespace="cocos2d",
            bases=["CCObject"],
            methods=[
                Method(
                    name="getPosition",
                    ret="cocos2d::CCPoint",
                    args=[],
                    platforms=all_platforms("0x1"),
                )
            ],
        )
        gm = Class(
            name="GameManager",
            bases=["GManager"],
            methods=[
                Method(
                    name="colorForIdx",
                    ret="cocos2d::ccColor3B",
                    args=[Arg("int", "idx")],
                    platforms=all_platforms("0x2"),
                )
            ],
        )
        root = Root(
            classes=[
                ccobject,
                ccnode,
                Class(
                    name="GManager",
                    bases=["CCNode"],
                    methods=[
                        Method(
                            name="init",
                            ret="bool",
                            args=[],
                            platforms=all_platforms("0x3"),
                        )
                    ],
                ),
                gm,
            ]
        )
        files = emit_luau_types(root)
        text = types_text(files)

        self.assertNotIn("cocos2d::", text)
        self.assertIn("export type enumKeyCodes = number", text)
        self.assertIn(
            "export type RGBColor = { r: number, g: number, b: number }", text
        )
        self.assertNotIn("declare class CCNode end", text)

    def test_factories_and_namespace_types(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        director = Class(
            name="CCDirector",
            namespace="cocos2d",
            bases=["CCObject"],
            methods=[
                Method(
                    name="getWinSize",
                    ret="cocos2d::CCSize",
                    args=[],
                    platforms=all_platforms("0x1"),
                ),
                Method(
                    name="sharedDirector",
                    ret="cocos2d::CCDirector*",
                    args=[],
                    is_static=True,
                    platforms=all_platforms("0x2"),
                ),
            ],
            attributes=["link(win)"],
        )
        root = Root(classes=[ccobject, director])
        files = emit_luau_types(root)
        text = types_text(files)

        self.assertIn("export type CCDirectorFactory", text)
        self.assertIn("export type Cocos2dNamespace", text)
        self.assertIn("sharedDirector: (() -> CCDirector?)", text)
        self.assertIn("function getWinSize(self): CCSize", text)
        self.assertIn("cocos2d: Cocos2dNamespace", text)

    def test_factory_after_class_decl(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        widget = Class(
            name="WidgetLayer",
            bases=["CCObject"],
            methods=[
                Method(
                    name="create",
                    ret="WidgetLayer*",
                    args=[],
                    is_static=True,
                    platforms=all_platforms("0x1"),
                ),
                Method(
                    name="init",
                    ret="bool",
                    args=[],
                    platforms=all_platforms("0x2"),
                ),
            ],
            attributes=["link(win)"],
        )
        root = Root(classes=[ccobject, widget])
        files = emit_luau_types(root)
        text = types_text(files)
        class_pos = text.find("declare class WidgetLayer")
        factory_pos = text.find("export type WidgetLayerFactory")
        self.assertGreaterEqual(class_pos, 0)
        self.assertGreater(factory_pos, class_pos)
        self.assertIn("export type WidgetLayerFactory", text)

    def test_defined_classes_have_no_empty_stub(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        ccsprite = Class(
            name="CCSprite",
            namespace="cocos2d",
            bases=["CCNode"],
            methods=[
                Method(
                    name="setPosition",
                    ret="void",
                    args=[Arg("cocos2d::CCPoint", "pos")],
                    platforms=all_platforms("0x1"),
                )
            ],
        )
        root = Root(
            classes=[
                ccobject,
                Class(name="CCNode", namespace="cocos2d", bases=["CCObject"]),
                ccsprite,
            ]
        )
        files = emit_luau_types(root)
        text = types_text(files)
        self.assertNotIn("declare class CCSprite end", text)
        self.assertNotIn("declare class CCNode end", text)
        self.assertIn("function setPosition(self", text)

    def test_emit_outputs_single_file(self) -> None:
        ccobject = Class(
            name="CCObject",
            namespace="cocos2d",
            methods=[
                Method(
                    name="retain", ret="void", args=[], platforms=all_platforms("0x1")
                )
            ],
        )
        widget = Class(
            name="Widget",
            bases=["CCObject"],
            methods=[
                Method(name="init", ret="bool", args=[], platforms=all_platforms("0x1"))
            ],
        )
        files = emit_luau_types(Root(classes=[ccobject, widget]))
        self.assertEqual(set(files.keys()), {TYPES_FILE})

    def test_no_backward_class_stub(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        ccsprite = Class(
            name="CCSprite",
            namespace="cocos2d",
            bases=["CCNode"],
            methods=[
                Method(
                    name="setPosition",
                    ret="void",
                    args=[Arg("cocos2d::CCPoint", "pos")],
                    platforms=all_platforms("0x1"),
                )
            ],
        )
        text_area = Class(
            name="TextArea",
            bases=["CCSprite"],
            methods=[
                Method(
                    name="init",
                    ret="bool",
                    args=[],
                    platforms=all_platforms("0x2"),
                )
            ],
        )
        root = Root(
            classes=[
                ccobject,
                Class(name="CCNode", namespace="cocos2d", bases=["CCObject"]),
                ccsprite,
                text_area,
            ]
        )
        files = emit_luau_types(root)
        combined = types_text(files)
        decl_re = re.compile(r"^declare class (\w+)\b(.*)$")
        fully_defined: set[str] = set()
        for line in combined.splitlines():
            match = decl_re.match(line.strip())
            if not match:
                continue
            name, rest = match.group(1), match.group(2).strip()
            if rest == "end":
                self.assertNotIn(
                    name,
                    fully_defined,
                    msg=f"backward stub for {name} after its definition",
                )
            else:
                fully_defined.add(name)

    def test_field_object_type_is_declared(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        ccgrid_base = Class(name="CCGridBase", namespace="cocos2d", bases=["CCObject"])
        ccnode = Class(
            name="CCNode",
            namespace="cocos2d",
            bases=["CCObject"],
            fields=[Field("m_pGrid", "cocos2d::CCGridBase*")],
        )
        root = Root(classes=[ccobject, ccgrid_base, ccnode])
        files = emit_luau_types(root)
        cocos = types_text(files)
        self.assertIn("declare class CCGridBase", cocos)
        self.assertIn("declare class CCNode", cocos)
        self.assertIn("m_pGrid: CCGridBase?", cocos)

    def test_vector_field_element_type_is_declared(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        ccnode = Class(name="CCNode", namespace="cocos2d", bases=["CCObject"])
        child = Class(name="ChildNode", namespace="cocos2d", bases=["CCNode"])
        owner = Class(
            name="OwnerNode",
            namespace="cocos2d",
            bases=["CCNode"],
            fields=[Field("m_children", "gd::vector<cocos2d::ChildNode*>")],
        )
        root = Root(classes=[ccobject, ccnode, child, owner])

        cocos = types_text(emit_luau_types(root))

        self.assertIn("declare class ChildNode", cocos)
        self.assertIn("m_children: { ChildNode? }", cocos)
        self.assertNotIn("-- skipped m_children", cocos)

    def test_vector_method_element_type_is_declared(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        ccnode = Class(name="CCNode", namespace="cocos2d", bases=["CCObject"])
        child = Class(name="ChildNode", namespace="cocos2d", bases=["CCNode"])
        owner = Class(
            name="OwnerNode",
            namespace="cocos2d",
            bases=["CCNode"],
            methods=[
                Method(
                    name="getChildren",
                    ret="gd::vector<cocos2d::ChildNode*>",
                    args=[],
                    platforms=all_platforms("0x1"),
                )
            ],
        )
        root = Root(classes=[ccobject, ccnode, child, owner])

        cocos = types_text(emit_luau_types(root))

        self.assertIn("declare class ChildNode", cocos)
        self.assertIn("function getChildren(self): { ChildNode? }", cocos)

    def test_primitive_vector_method_types_use_table_arrays(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        foo = Class(
            name="Foo",
            bases=["CCObject"],
            methods=[
                Method(
                    name="take",
                    ret="void",
                    args=[Arg("gd::vector<int>", "values")],
                    platforms=all_platforms("0x1"),
                ),
                Method(
                    name="getValues",
                    ret="gd::vector<int>",
                    args=[],
                    platforms=all_platforms("0x1"),
                ),
            ],
        )
        root = Root(classes=[ccobject, foo])

        text = types_text(emit_luau_types(root))

        self.assertIn("function take(self, arg1: { number })", text)
        self.assertIn("function getValues(self): { number }", text)

    def test_map_and_set_method_types_use_table_shapes(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        child = Class(name="ChildNode", namespace="cocos2d", bases=["CCObject"])
        foo = Class(
            name="Foo",
            bases=["CCObject"],
            methods=[
                Method(
                    name="takeMap",
                    ret="void",
                    args=[Arg("gd::map<int, bool>", "values")],
                    platforms=all_platforms("0x1"),
                ),
                Method(
                    name="getChildrenById",
                    ret="gd::map<int, cocos2d::ChildNode*>",
                    args=[],
                    platforms=all_platforms("0x1"),
                ),
                Method(
                    name="takeSet",
                    ret="void",
                    args=[Arg("gd::set<int>", "values")],
                    platforms=all_platforms("0x1"),
                ),
            ],
        )
        root = Root(classes=[ccobject, child, foo])

        text = types_text(emit_luau_types(root))

        self.assertIn("declare class ChildNode", text)
        self.assertIn("function takeMap(self, arg1: { [number]: boolean })", text)
        self.assertIn("function getChildrenById(self): { [number]: ChildNode? }", text)
        self.assertIn("function takeSet(self, arg1: { number })", text)

    def test_orphan_forward_decls_exclude_value_types(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        ccnode = Class(
            name="CCNode",
            namespace="cocos2d",
            bases=["CCObject"],
            fields=[Field("m_obPosition", "cocos2d::CCPoint")],
        )
        root = Root(classes=[ccobject, ccnode])
        files = emit_luau_types(root)
        cocos = types_text(files)
        self.assertIn("m_obPosition: CCPoint", cocos)
        self.assertNotIn("declare class CCPoint end", cocos)

    def test_value_stub_order(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        node = Class(
            name="CCNode",
            namespace="cocos2d",
            bases=["CCObject"],
            methods=[
                Method(
                    name="getRect",
                    ret="cocos2d::CCRect",
                    args=[],
                    platforms=all_platforms("0x1"),
                )
            ],
        )
        gm = Class(
            name="GameManager",
            bases=["CCNode"],
            methods=[
                Method(
                    name="getSize",
                    ret="cocos2d::CCSize",
                    args=[],
                    platforms=all_platforms("0x2"),
                ),
                Method(
                    name="getRect",
                    ret="cocos2d::CCRect",
                    args=[],
                    platforms=all_platforms("0x3"),
                ),
            ],
        )
        root = Root(classes=[ccobject, node, gm])
        files = emit_luau_types(root)
        gd = types_text(files)
        ccsize_pos = gd.find("export type CCSize =")
        ccrect_pos = gd.find("export type CCRect =")
        self.assertGreaterEqual(ccsize_pos, 0)
        self.assertGreater(ccrect_pos, ccsize_pos)

    def test_fmod_opaque_handle_type_stubs(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        engine = Class(
            name="FMODAudioEngine",
            bases=["CCObject"],
            attributes=["link(win)"],
            methods=[
                Method(
                    name="getActiveMusicChannel",
                    ret="FMOD::Channel*",
                    args=[],
                    platforms=all_platforms("0x1"),
                ),
                Method(
                    name="isSoundReady",
                    ret="bool",
                    args=[Arg("FMOD::Sound*", "sound")],
                    platforms=all_platforms("0x2"),
                ),
                Method(
                    name="channelLinkSound",
                    ret="void",
                    args=[Arg("int", "id"), Arg("FMODSound*", "sound")],
                    platforms=all_platforms("0x2a"),
                ),
                Method(
                    name="preloadEffect",
                    ret="FMODSound*",
                    args=[Arg("gd::string", "path")],
                    platforms=all_platforms("0x2b"),
                ),
                Method(
                    name="storeEffect",
                    ret="FMODSound*",
                    args=[Arg("FMOD::Sound*", "sound"), Arg("gd::string", "path")],
                    platforms=all_platforms("0x2c"),
                ),
                Method(
                    name="printResult",
                    ret="void",
                    args=[Arg("FMOD_RESULT", "result")],
                    platforms=all_platforms("0x3"),
                ),
                Method(
                    name="applyConfig",
                    ret="void",
                    args=[Arg("UIButtonConfig", "config")],
                    platforms=all_platforms("0x4"),
                ),
            ],
        )
        root = Root(classes=[ccobject, engine])
        text = types_text(emit_luau_types(root))
        self.assertIn("declare class FMODChannel end", text)
        self.assertIn("declare class FMODSound end", text)
        self.assertIn("}\n\n--- @type-only: opaque FMOD handle", text)
        channel_pos = text.find("declare class FMODChannel end")
        sig_pos = text.find("function getActiveMusicChannel(self): FMODChannel?")
        self.assertGreaterEqual(channel_pos, 0)
        self.assertGreater(sig_pos, channel_pos)
        self.assertIn("function isSoundReady(self, arg1: FMODSound): boolean", text)
        self.assertIn(
            "function channelLinkSound(self, arg1: number, arg2: FMODSound)",
            text,
        )
        self.assertIn(
            "function preloadEffect(self, arg1: string): FMODSound?",
            text,
        )
        self.assertIn(
            "function storeEffect(self, arg1: FMODSound, arg2: string): FMODSound?",
            text,
        )
        self.assertIn("function printResult(self, arg1: number)", text)

    def test_cocos_opaque_delegate_type_stubs(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        dispatcher = Class(
            name="CCTouchDispatcher",
            namespace="cocos2d",
            bases=["CCObject"],
            methods=[
                Method(
                    name="addTargetedDelegate",
                    ret="void",
                    args=[
                        Arg("cocos2d::CCTouchDelegate*", "pDelegate"),
                        Arg("int", "priority"),
                        Arg("bool", "bSwallowsTouches"),
                    ],
                    platforms=all_platforms("0x1"),
                ),
            ],
        )
        root = Root(classes=[ccobject, dispatcher])
        text = types_text(emit_luau_types(root))
        self.assertIn("declare class CCEvent end", text)
        self.assertIn("declare class CCEditBox end", text)
        event_pos = text.find("declare class CCEvent end")
        edit_box_pos = text.find("declare class CCEditBox end")
        touch_delegate_pos = text.find("export type CCTouchDelegate =")
        edit_box_delegate_pos = text.find("export type CCEditBoxDelegate =")
        self.assertGreaterEqual(event_pos, 0)
        self.assertGreaterEqual(edit_box_pos, 0)
        self.assertGreater(touch_delegate_pos, event_pos)
        self.assertGreater(edit_box_delegate_pos, edit_box_pos)

    @staticmethod
    def _stub_method(name: str = "init", addr: str = "0x1") -> Method:
        return Method(name=name, ret="bool", args=[], platforms=all_platforms(addr))

    def test_base_class_declared_before_derived(self) -> None:
        ccobject = Class(
            name="CCObject", namespace="cocos2d", methods=[self._stub_method()]
        )
        ccaction = Class(
            name="CCAction",
            namespace="cocos2d",
            bases=["CCObject"],
            methods=[self._stub_method("step", "0x2")],
        )
        root = Root(classes=[ccaction, ccobject])
        out = types_text(emit_luau_types(root))
        obj_pos = out.index("declare class CCObject")
        action_pos = out.index("declare class CCAction extends CCObject")
        self.assertLess(obj_pos, action_pos)

    def test_no_extends_references_class_declared_later(self) -> None:
        ccobject = Class(
            name="CCObject", namespace="cocos2d", methods=[self._stub_method()]
        )
        finite = Class(
            name="CCFiniteTimeAction",
            namespace="cocos2d",
            bases=["CCObject"],
            methods=[self._stub_method("step", "0x2")],
        )
        interval = Class(
            name="CCActionInterval",
            namespace="cocos2d",
            bases=["CCFiniteTimeAction"],
            methods=[self._stub_method("isDone", "0x3")],
        )
        ease = Class(
            name="CCActionEase",
            namespace="cocos2d",
            bases=["CCActionInterval"],
            methods=[self._stub_method("reverse", "0x4")],
        )
        gm = Class(
            name="GameManager",
            bases=["CCObject"],
            methods=[self._stub_method("update", "0x5")],
        )
        root = Root(classes=[ease, interval, finite, ccobject, gm])
        out = types_text(emit_luau_types(root))
        for m in re.finditer(r"declare class (\w+) extends (\w+)", out):
            derived, base = m.group(1), m.group(2)
            base_pos = out.find(f"declare class {base}")
            self.assertGreaterEqual(
                base_pos, 0, f"base {base} of {derived} not declared"
            )
            self.assertLess(
                base_pos,
                m.start(),
                f"base {base} declared after derived {derived}",
            )

    def test_geode_root_namespace_stubs(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        root = Root(classes=[ccobject])
        main = types_text(emit_luau_types(root))
        self.assertIn("export type Cocos2dNamespace", main)
        self.assertIn("export type GDNamespace", main)
        self.assertIn("cocos2d: Cocos2dNamespace", main)
        self.assertIn("before: ((...any) -> any?)?,", main)
        self.assertIn("after: ((...any) -> any?)?,", main)

    def test_emit_includes_extra_bindings_dluau(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        text = types_text(emit_luau_types(Root(classes=[ccobject])))
        self.assertIn(
            "-- Custom definitions from tools/luau_codegen/extra_bindings/",
            text,
        )
        self.assertIn("declare task: TaskNamespace", text)
        self.assertIn("declare imgui: ImGuiNamespace", text)

    def test_dispatcher_uses_input_arg_count_for_out_refs(self) -> None:
        method = Method(
            name="getUnlockForAchievement",
            ret="void",
            args=[
                Arg("char const*", "achievement"),
                Arg("UnlockType&", "type"),
            ],
        )
        objects = {"UnlockType": Class(name="UnlockType")}

        self.assertEqual(_input_arg_count(method, objects), 1)

    def test_const_ref_is_input_not_out(self) -> None:
        self.assertTrue(is_const_reference("cocos2d::ccColor3B const&"))
        self.assertFalse(is_out_reference("cocos2d::ccColor3B const&"))
        self.assertTrue(is_out_reference("UnlockType&"))

        info = classify_arg("cocos2d::ccColor3B const&", {})
        assert info is not None
        self.assertTrue(info.is_ref)
        self.assertFalse(info.is_out)

    def test_const_ref_overload_dispatcher_has_distinct_cases(self) -> None:
        cls = Class(name="GameObject", bases=["CCSprite"])
        methods = [
            Method(
                name="updateMainColor",
                ret="void",
                args=[Arg("cocos2d::ccColor3B const&", "color")],
                platforms={"win": "0x1"},
            ),
            Method(
                name="updateMainColor",
                ret="void",
                args=[],
                platforms={"win": "inline"},
            ),
        ]
        objects = {
            "GameObject": cls,
            "CCSprite": Class(name="CCSprite", namespace="cocos2d"),
        }
        text = _emit_dispatcher(cls, "updateMainColor", methods, objects)
        self.assertIn("case 0:", text)
        self.assertIn("case 1:", text)
        self.assertNotIn(
            "case 0: return luaapi_GameObject_updateMainColor_0(L);\n            case 0:",
            text,
        )


class LuauOverloadWideningTests(unittest.TestCase):
    @staticmethod
    def _factory_slice(text: str, name: str) -> str:
        start = text.index(f"export type {name}Factory")
        return text[start : text.index("}", start)]

    @staticmethod
    def _class_slice(text: str, name: str) -> str:
        start = text.index(f"declare class {name}")
        return text[start : text.index("end", start)]

    def _factory(self, *methods: Method) -> str:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        instance = Method(
            name="getValue", ret="int", args=[], platforms=all_platforms("0xff")
        )
        foo = Class(
            name="Foo",
            bases=["CCObject"],
            attributes=["link(win)"],
            methods=[*methods, instance],
        )
        return types_text(emit_luau_types(Root(classes=[ccobject, foo])))

    def _instance(self, *methods: Method) -> str:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        bar = Class(
            name="Bar",
            bases=["CCObject"],
            attributes=["link(win)"],
            methods=list(methods),
        )
        return types_text(emit_luau_types(Root(classes=[ccobject, bar])))

    def test_factory_multi_overload_no_intersection(self) -> None:
        text = self._factory(
            Method(
                name="create",
                ret="Foo*",
                args=[Arg("int", "a")],
                is_static=True,
                platforms=all_platforms("0x1"),
            ),
            Method(
                name="create",
                ret="Foo*",
                args=[Arg("int", "a"), Arg("int", "b")],
                is_static=True,
                platforms=all_platforms("0x2"),
            ),
        )
        factory = self._factory_slice(text, "Foo")
        self.assertNotIn(" & (", factory)

    def test_factory_multi_overload_keeps_shared_prefix(self) -> None:
        text = self._factory(
            Method(
                name="create",
                ret="Foo*",
                args=[Arg("int", "a")],
                is_static=True,
                platforms=all_platforms("0x1"),
            ),
            Method(
                name="create",
                ret="Foo*",
                args=[Arg("int", "a"), Arg("int", "b")],
                is_static=True,
                platforms=all_platforms("0x2"),
            ),
        )
        self.assertIn(
            "create: (arg1: number, ...any) -> Foo?",
            self._factory_slice(text, "Foo"),
        )

    def test_factory_divergent_arg1_widens_to_varargs(self) -> None:
        text = self._factory(
            Method(
                name="create",
                ret="Foo*",
                args=[Arg("int", "a")],
                is_static=True,
                platforms=all_platforms("0x1"),
            ),
            Method(
                name="create",
                ret="Foo*",
                args=[Arg("bool", "a"), Arg("int", "b")],
                is_static=True,
                platforms=all_platforms("0x2"),
            ),
        )
        factory = self._factory_slice(text, "Foo")
        self.assertNotIn(" & (", factory)
        self.assertIn("create: (...any) -> Foo?", factory)

    def test_instance_multi_overload_no_intersection(self) -> None:
        text = self._instance(
            Method(
                name="foo",
                ret="void",
                args=[Arg("int", "a")],
                platforms=all_platforms("0x1"),
            ),
            Method(
                name="foo",
                ret="void",
                args=[Arg("int", "a"), Arg("int", "b")],
                platforms=all_platforms("0x2"),
            ),
        )
        body = self._class_slice(text, "Bar")
        self.assertNotIn(" & (", body)
        self.assertIn("foo: (self: Bar, arg1: number, ...any) -> ()", body)

    def test_single_overload_still_function_form(self) -> None:
        text = self._instance(
            Method(
                name="foo",
                ret="void",
                args=[Arg("int", "a")],
                platforms=all_platforms("0x1"),
            ),
        )
        body = self._class_slice(text, "Bar")
        self.assertIn("function foo(self", body)
        self.assertNotIn("...any", body)

    def test_widened_return_uses_common_type(self) -> None:
        text = self._factory(
            Method(
                name="create",
                ret="Foo*",
                args=[Arg("int", "a")],
                is_static=True,
                platforms=all_platforms("0x1"),
            ),
            Method(
                name="create",
                ret="Foo*",
                args=[Arg("int", "a"), Arg("int", "b")],
                is_static=True,
                platforms=all_platforms("0x2"),
            ),
        )
        factory = self._factory_slice(text, "Foo")
        self.assertIn("-> Foo?", factory)
        self.assertNotIn("-> any?", factory)

    def test_widened_return_falls_back_to_any_on_divergence(self) -> None:
        text = self._instance(
            Method(
                name="foo",
                ret="void",
                args=[Arg("int", "a")],
                platforms=all_platforms("0x1"),
            ),
            Method(
                name="foo",
                ret="int",
                args=[Arg("int", "a"), Arg("int", "b")],
                platforms=all_platforms("0x2"),
            ),
        )
        self.assertIn(
            "foo: (self: Bar, arg1: number, ...any) -> any?",
            self._class_slice(text, "Bar"),
        )

    def test_lazy_sprite_callback_luau_type(self) -> None:
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
        root = Root(classes=[ccobject, lazy_sprite])
        text = types_text(emit_luau_types(root))
        self.assertIn(
            "arg1: (arg1: boolean | string) -> ()",
            self._class_slice(text, "LazySprite"),
        )


class F9SingleFileTests(unittest.TestCase):
    def test_each_class_appears_in_single_file(self) -> None:
        ccobject = Class(
            name="CCObject",
            namespace="cocos2d",
            methods=[
                Method(
                    name="retain", ret="void", args=[], platforms=all_platforms("0x1")
                )
            ],
        )
        ccnode = Class(
            name="CCNode",
            namespace="cocos2d",
            bases=["CCObject"],
            methods=[
                Method(
                    name="getTag", ret="int", args=[], platforms=all_platforms("0x2")
                )
            ],
        )
        player = Class(
            name="PlayerObject",
            bases=["CCObject"],
            methods=[
                Method(name="init", ret="bool", args=[], platforms=all_platforms("0x3"))
            ],
        )
        files = emit_luau_types(Root(classes=[ccobject, ccnode, player]))
        text = types_text(files)
        for name in ("CCObject", "CCNode", "PlayerObject"):
            self.assertIn(f"declare class {name}", text)
