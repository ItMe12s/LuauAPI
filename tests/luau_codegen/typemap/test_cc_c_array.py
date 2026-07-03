from __future__ import annotations

import unittest

from luau_codegen.convert.marshalling import push_value  # type: ignore[import-unresolved]
from luau_codegen.convert.type_map import TypeInfo, classify_arg, classify_return  # type: ignore[import-unresolved]
from luau_codegen.model.cc_c_array import (  # type: ignore[import-unresolved]
    CC_C_ARRAY_FIELD_ELEMENTS,
    proven_cc_c_array_element,
)
from luau_codegen.parse.broma import Class, Field  # type: ignore[import-unresolved]
from luau_codegen.emit.bindings.class_file import _emit_class_file  # type: ignore[import-unresolved]
from luau_codegen.policy.fields import bindable_field  # type: ignore[import-unresolved]


class CcCArrayPolicyTests(unittest.TestCase):
    def test_allowlist_covers_dispatcher_handler_queues(self) -> None:
        self.assertIn(("CCKeyboardDispatcher", "m_pHandlersToAdd"), CC_C_ARRAY_FIELD_ELEMENTS)
        self.assertIn(("CCTouchDispatcher", "m_pHandlersToRemove"), CC_C_ARRAY_FIELD_ELEMENTS)

    def test_proven_element_lookup(self) -> None:
        self.assertEqual(
            proven_cc_c_array_element("CCKeypadDispatcher", "m_pHandlersToRemove"),
            "CCKeypadHandler",
        )
        self.assertIsNone(proven_cc_c_array_element("CCArray", "data"))


class CcCArrayTypeMapTests(unittest.TestCase):
    def _keyboard_handler_objects(self) -> dict[str, Class]:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        handler = Class(name="CCKeyboardHandler", namespace="cocos2d", bases=["CCObject"])
        dispatcher = Class(
            name="CCKeyboardDispatcher",
            namespace="cocos2d",
            bases=["CCObject"],
        )
        return {
            "CCObject": ccobject,
            "cocos2d::CCObject": ccobject,
            "CCKeyboardHandler": handler,
            "CCKeyboardDispatcher": dispatcher,
        }

    def test_classify_proven_field_as_cc_c_array_view(self) -> None:
        objects = self._keyboard_handler_objects()
        info = classify_arg(
            "cocos2d::ccCArray*",
            objects,
            owner_class="CCKeyboardDispatcher",
            field_name="m_pHandlersToAdd",
        )

        self.assertIsNotNone(info)
        assert info is not None
        self.assertEqual(info.kind, "cc_c_array_view")
        self.assertEqual(info.lua_type, "{ CCKeyboardHandler? }")
        assert info.element_type is not None
        self.assertEqual(info.element_type.class_name, "CCKeyboardHandler")

    def test_classify_return_for_proven_field(self) -> None:
        objects = self._keyboard_handler_objects()
        info = classify_return(
            "cocos2d::ccCArray*",
            objects,
            owner_class="CCKeyboardDispatcher",
            field_name="m_pHandlersToRemove",
        )

        self.assertIsNotNone(info)
        assert info is not None
        self.assertEqual(info.kind, "cc_c_array_view")
        self.assertEqual(info.lua_type, "{ CCKeyboardHandler? }")

    def test_unproven_cc_c_array_stays_unsupported(self) -> None:
        self.assertIsNone(classify_arg("cocos2d::ccCArray*", {}))
        self.assertIsNone(
            classify_arg(
                "cocos2d::ccCArray*",
                {},
                owner_class="CCKeyboardDispatcher",
                field_name="m_unknown",
            )
        )

    def test_raw_cc_array_pointer_stays_unsupported(self) -> None:
        self.assertIsNone(classify_arg("cocos2d::ccArray*", {}))
        self.assertIsNone(
            classify_arg(
                "cocos2d::ccArray*",
                {},
                owner_class="CCArray",
                field_name="data",
            )
        )


class CcCArrayMarshallingTests(unittest.TestCase):
    def test_push_uses_readonly_cc_array_view_helper(self) -> None:
        element = TypeInfo(
            kind="object",
            cxx_type="cocos2d::CCKeyboardHandler*",
            lua_type="CCKeyboardHandler",
            class_name="CCKeyboardHandler",
        )
        info = TypeInfo(
            kind="cc_c_array_view",
            cxx_type="cocos2d::ccCArray*",
            lua_type="{ CCKeyboardHandler? }",
            class_name="CCKeyboardHandler",
            element_type=element,
        )

        text = "".join(push_value(info, "self->m_pHandlersToAdd", False, owner_expr="self"))

        self.assertIn(
            "luax::pushReadOnlyCCArrayView<cocos2d::CCKeyboardHandler>",
            text,
        )
        self.assertIn("self->m_pHandlersToAdd", text)
        self.assertIn(", self", text)


class CcCArrayFieldBindingTests(unittest.TestCase):
    def test_generated_field_is_readonly_cc_c_array_view(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        handler = Class(name="CCKeyboardHandler", namespace="cocos2d", bases=["CCObject"])
        cls = Class(
            name="CCKeyboardDispatcher",
            namespace="cocos2d",
            bases=["CCObject"],
            fields=[Field("m_pHandlersToAdd", "cocos2d::ccCArray*")],
        )
        objects = {
            "CCObject": ccobject,
            "cocos2d::CCObject": ccobject,
            "CCKeyboardHandler": handler,
            "CCKeyboardDispatcher": cls,
        }

        ok, reason, _, ret = bindable_field(cls.fields[0], objects, cls)
        self.assertTrue(ok, reason)
        assert ret is not None
        self.assertEqual(ret.kind, "cc_c_array_view")

        text = _emit_class_file(
            cls,
            {},
            [],
            [(cls, cls.fields[0])],
            objects,
            set(),
            1,
            "win",
        )

        self.assertIn("pushReadOnlyCCArrayView<cocos2d::CCKeyboardHandler>", text)
        self.assertIn('readonlyField(L, "m_pHandlersToAdd"', text)
        self.assertNotIn("field_set_m_pHandlersToAdd", text)
