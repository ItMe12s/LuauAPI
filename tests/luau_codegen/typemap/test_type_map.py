from __future__ import annotations

import os
import shutil
import tempfile
import unittest

import test_support
from luau_codegen.convert.type_map import (  # type: ignore[import-unresolved]
    COCOS_ENUM_TYPES,
    GD_ENUM_TYPES,
    STD_ARRAY_MAX_SIZE,
    classify_arg,
    classify_return,
    method_input_arg_count as _input_arg_count,
    register_geode_enums,
)
from luau_codegen.model.domain import codegen_object_map, object_classes  # type: ignore[import-unresolved]
from luau_codegen.parse.broma import Arg, Class, Method, Root  # type: ignore[import-unresolved]
from luau_codegen.model.codegen_context import CodegenContext  # type: ignore[import-unresolved]
from luau_codegen.model.geode_enums import EnumInfo, EnumMember  # type: ignore[import-unresolved]
from luau_codegen.parse.collect import collect_bindings_root  # type: ignore[import-unresolved]


class GeodeEnumRegistrationTests(unittest.TestCase):
    def test_register_geode_enums_skips_reserved(self) -> None:
        skip = GD_ENUM_TYPES | COCOS_ENUM_TYPES | {"CollidingClass"}
        ctx = register_geode_enums(
            {
                "IconType": "IconType",
                "enumKeyCodes": "cocos2d::enumKeyCodes",
                "CollidingClass": "geode::CollidingClass",
                "UniqueGeodeEnum": "geode::UniqueGeodeEnum",
            },
            skip=skip,
        )
        self.assertNotIn("IconType", ctx.geode_enum_names)
        self.assertNotIn("enumKeyCodes", ctx.geode_enum_names)
        self.assertNotIn("CollidingClass", ctx.geode_enum_names)
        self.assertIn("UniqueGeodeEnum", ctx.geode_enum_names)

    def test_register_geode_enums_keeps_unique(self) -> None:
        ctx = register_geode_enums({"BaseType": "geode::BaseType"})
        self.assertEqual(ctx.geode_enum_names, frozenset({"BaseType"}))
        self.assertIn("BaseType", ctx.enum_cxx_names())

    def test_with_geode_enums_stores_members(self) -> None:
        ctx = CodegenContext.with_geode_enums(
            {
                "GJLevelType": EnumInfo(
                    name="GJLevelType",
                    cxx_name="GJLevelType",
                    members=(
                        EnumMember("Saved", 3),
                        EnumMember("SearchResult", 4),
                    ),
                )
            }
        )
        self.assertEqual(
            ctx.geode_enum_members["GJLevelType"],
            (("Saved", 3), ("SearchResult", 4)),
        )

    def test_with_geode_enums_routes_skipped_gd_enum_to_gd_members(self) -> None:
        ctx = CodegenContext.with_geode_enums(
            {
                "GJLevelType": EnumInfo(
                    name="GJLevelType",
                    cxx_name="GJLevelType",
                    members=(
                        EnumMember("Saved", 3),
                        EnumMember("SearchResult", 4),
                    ),
                )
            },
            skip={"GJLevelType"},
        )
        self.assertEqual(
            ctx.gd_enum_members["GJLevelType"],
            (("Saved", 3), ("SearchResult", 4)),
        )
        self.assertNotIn("GJLevelType", ctx.geode_enum_members)
        self.assertNotIn("GJLevelType", ctx.geode_enum_names)

    def test_with_geode_enums_skips_reserved_members(self) -> None:
        skip = GD_ENUM_TYPES | {"CollidingClass"}
        ctx = CodegenContext.with_geode_enums(
            {
                "UniqueGeodeEnum": EnumInfo(
                    name="UniqueGeodeEnum",
                    cxx_name="geode::UniqueGeodeEnum",
                    members=(EnumMember("Alpha", 1),),
                ),
                "IconType": EnumInfo(
                    name="IconType",
                    cxx_name="IconType",
                    members=(EnumMember("A", 0),),
                ),
            },
            skip=skip,
        )
        self.assertIn("UniqueGeodeEnum", ctx.geode_enum_members)
        self.assertNotIn("IconType", ctx.geode_enum_members)
        self.assertIn("IconType", ctx.gd_enum_members)

    def test_collect_bindings_root_threads_enum_members(self) -> None:
        bindings = tempfile.mkdtemp()
        sdk = tempfile.mkdtemp()
        try:
            os.makedirs(os.path.join(bindings), exist_ok=True)
            with open(os.path.join(bindings, "Cocos2d.bro"), "w", encoding="utf-8") as f:
                f.write("[[link(win)]] class cocos2d::CCObject {};\n")
            include_dir = os.path.join(sdk, "loader", "include", "Geode")
            os.makedirs(include_dir)
            with open(os.path.join(include_dir, "Enums.hpp"), "w", encoding="utf-8") as f:
                f.write("enum class UniqueGeodeEnum { Alpha = 1, Beta = 2 };\n")

            root = collect_bindings_root(bindings, geode_sdk_path=sdk)
            assert root.codegen_ctx is not None
            self.assertEqual(
                root.codegen_ctx.geode_enum_members["UniqueGeodeEnum"],
                (("Alpha", 1), ("Beta", 2)),
            )
        finally:
            shutil.rmtree(bindings)
            shutil.rmtree(sdk)

    def test_classify_arg_no_enum_shadow(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        colliding = Class(name="CollidingClass", namespace="geode", bases=["CCObject"])
        root = Root(classes=[ccobject, colliding])
        objects = codegen_object_map(root)
        skip = GD_ENUM_TYPES | COCOS_ENUM_TYPES | {c.name for c in object_classes(root)}
        ctx = register_geode_enums({"CollidingClass": "geode::CollidingClass"}, skip=skip)
        info = classify_arg("CollidingClass*", objects, ctx=ctx)
        self.assertIsNotNone(info)
        assert info is not None
        self.assertEqual(info.kind, "object")
        self.assertEqual(info.class_name, "CollidingClass")

    def test_classify_sel_menu_handler(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        objects = {"CCObject": ccobject}
        info = classify_arg("SEL_MenuHandler", objects)
        self.assertIsNotNone(info)
        assert info is not None
        self.assertEqual(info.kind, "sel")
        self.assertEqual(info.lua_type, "(sender: CCObject) -> ()")

    def test_classify_std_function_callback(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        objects = {"CCObject": ccobject}
        info = classify_arg("std::function<void(cocos2d::CCObject*)>", objects)
        self.assertIsNotNone(info)
        assert info is not None
        self.assertEqual(info.kind, "callback")
        self.assertEqual(info.lua_type, "(arg1: CCObject) -> ()")

    def test_classify_std_function_non_void_return(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        objects = {"CCObject": ccobject}
        info = classify_arg("std::function<bool(cocos2d::CCObject*)>", objects)
        self.assertIsNotNone(info)
        assert info is not None
        self.assertEqual(info.kind, "callback")
        self.assertEqual(info.lua_type, "(arg1: CCObject) -> boolean")
        assert info.callback_ret is not None
        self.assertEqual(info.callback_ret.kind, "bool")

    def test_classify_callback_alias(self) -> None:
        info = classify_arg("Callback", {})
        self.assertIsNotNone(info)
        assert info is not None
        self.assertEqual(info.kind, "callback")
        self.assertEqual(info.lua_type, "() -> ()")

    def test_classify_lazy_sprite_callback_alias(self) -> None:
        info = classify_arg("Callback", {}, owner_class="LazySprite")
        self.assertIsNotNone(info)
        assert info is not None
        self.assertEqual(info.kind, "callback")
        self.assertEqual(info.lua_type, "(arg1: boolean | string) -> ()")
        assert info.callback_args
        self.assertEqual(info.callback_args[0].kind, "result")

    def test_classify_geode_result_void(self) -> None:
        info = classify_arg("geode::Result<>", {})
        self.assertIsNotNone(info)
        assert info is not None
        self.assertEqual(info.kind, "result")
        self.assertEqual(info.lua_type, "boolean | string")

    def test_classify_sel_schedule(self) -> None:
        info = classify_arg("SEL_SCHEDULE", {})
        self.assertIsNotNone(info)
        assert info is not None
        self.assertEqual(info.kind, "sel")
        self.assertEqual(info.lua_type, "(dt: number) -> ()")
        self.assertEqual(info.class_name, "schedule")

    def test_classify_cctouch_delegate(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        objects = {"CCObject": ccobject}
        info = classify_arg("cocos2d::CCTouchDelegate*", objects)
        self.assertIsNotNone(info)
        assert info is not None
        self.assertEqual(info.kind, "delegate")
        self.assertEqual(info.class_name, "CCTouchDelegate")
        self.assertIn("ccTouchBegan", info.lua_type)

    def test_classify_cc_director_delegate(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        objects = {"CCObject": ccobject}
        info = classify_arg("cocos2d::CCDirectorDelegate*", objects)
        self.assertIsNotNone(info)
        assert info is not None
        self.assertEqual(info.kind, "delegate")
        self.assertEqual(info.class_name, "CCDirectorDelegate")
        self.assertIn("updateProjection", info.lua_type)

    def test_classify_cccolor4b_value(self) -> None:
        info = classify_arg("cocos2d::ccColor4B const&", {})
        self.assertIsNotNone(info)
        assert info is not None
        self.assertEqual(info.kind, "value")
        self.assertEqual(info.lua_type, "RGBAColor")

    def test_classify_cccolor4f_value(self) -> None:
        info = classify_arg("cocos2d::ccColor4F const&", {})
        self.assertIsNotNone(info)
        assert info is not None
        self.assertEqual(info.kind, "value")
        self.assertEqual(info.lua_type, "RGBAFloatColor")

    def test_classify_ccblendfunc_value(self) -> None:
        info = classify_arg("cocos2d::ccBlendFunc", {})
        self.assertIsNotNone(info)
        assert info is not None
        self.assertEqual(info.kind, "value")
        self.assertEqual(info.lua_type, "BlendFunc")

    def test_classify_cchsvvalue_value(self) -> None:
        info = classify_arg("cocos2d::ccHSVValue", {})
        self.assertIsNotNone(info)
        assert info is not None
        self.assertEqual(info.kind, "value")
        self.assertEqual(info.lua_type, "HSVValue")

    def test_classify_ccaffinetransform_value(self) -> None:
        info = classify_arg("cocos2d::CCAffineTransform const&", {})
        self.assertIsNotNone(info)
        assert info is not None
        self.assertEqual(info.kind, "value")
        self.assertEqual(info.lua_type, "CCAffineTransform")

    def test_method_input_arg_count_collapses_sel_pair(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        ccnode = Class(name="CCNode", namespace="cocos2d", bases=["CCObject"])
        ccsprite = Class(name="CCSprite", namespace="cocos2d", bases=["CCObject"])
        menu_item = Class(name="CCMenuItem", namespace="cocos2d", bases=["CCNode"])
        method = Method(
            name="initWithNormalSprite",
            ret="bool",
            args=[
                Arg("cocos2d::CCSprite*", "normal"),
                Arg("cocos2d::CCSprite*", "selected"),
                Arg("cocos2d::CCObject*", "target"),
                Arg("SEL_MenuHandler", "selector"),
            ],
        )
        objects = {
            "CCObject": ccobject,
            "CCNode": ccnode,
            "CCSprite": ccsprite,
            "CCMenuItem": menu_item,
            "cocos2d::CCObject": ccobject,
            "cocos2d::CCNode": ccnode,
            "cocos2d::CCSprite": ccsprite,
            "cocos2d::CCMenuItem": menu_item,
        }

        self.assertEqual(_input_arg_count(method, objects), 3)

    def test_method_input_arg_count_lazy_sprite_callback_uses_owner_class(
        self,
    ) -> None:
        method = Method(
            name="setLoadCallback",
            ret="void",
            args=[Arg("Callback", "cb")],
        )
        objects: dict[str, Class] = {}

        self.assertEqual(_input_arg_count(method, objects, owner_class="LazySprite"), 1)
        info = classify_arg("Callback", objects, owner_class="LazySprite")
        assert info is not None
        self.assertEqual(len(info.callback_args), 1)


class GeodeTaskHandleTypeMapTests(unittest.TestCase):
    def test_classify_task_handle_return(self) -> None:
        info = classify_return("arc::TaskHandle<bool>", {})
        self.assertIsNotNone(info)
        assert info is not None
        self.assertEqual(info.kind, "task_handle")
        self.assertEqual(info.cxx_type, "arc::TaskHandle<bool>")
        self.assertEqual(info.lua_type, "GeodeTaskHandle<boolean>")
        self.assertIsNotNone(info.element_type)
        assert info.element_type is not None
        self.assertEqual(info.element_type.kind, "bool")

    def test_classify_optional_task_handle_return(self) -> None:
        info = classify_return("std::optional<arc::TaskHandle<bool>>", {})
        self.assertIsNotNone(info)
        assert info is not None
        self.assertEqual(info.kind, "optional_task_handle")
        self.assertEqual(info.lua_type, "GeodeTaskHandle<boolean>?")
        self.assertEqual(info.cxx_type, "std::optional<arc::TaskHandle<bool>>")

    def test_classify_void_task_handle_return(self) -> None:
        info = classify_return("arc::TaskHandle<void>", {})
        self.assertIsNotNone(info)
        assert info is not None
        self.assertEqual(info.kind, "task_handle")
        self.assertEqual(info.lua_type, "GeodeTaskHandle<nil>")

    def test_task_handle_args_are_consume_only(self) -> None:
        self.assertIsNotNone(classify_arg("arc::TaskHandle<bool>", {}))
        self.assertIsNotNone(classify_arg("arc::TaskHandle<bool>&&", {}))
        self.assertIsNone(classify_arg("arc::TaskHandle<bool>&", {}))
        self.assertIsNone(classify_arg("arc::TaskHandle<bool> const&", {}))
        self.assertIsNone(classify_arg("arc::TaskHandle<bool>*", {}))

    def test_optional_task_handle_arg(self) -> None:
        info = classify_arg("std::optional<arc::TaskHandle<void>>", {})
        self.assertIsNotNone(info)
        assert info is not None
        self.assertEqual(info.kind, "optional_task_handle")
        self.assertEqual(info.lua_type, "GeodeTaskHandle<nil>?")


class ContainerTypeMapTests(unittest.TestCase):
    def test_classify_gd_vector_object_pointer_as_readonly_view(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        objects = {"CCObject": ccobject, "cocos2d::CCObject": ccobject}

        info = classify_arg("gd::vector<cocos2d::CCObject*>", objects)

        self.assertIsNotNone(info)
        assert info is not None
        self.assertEqual(info.kind, "vector_view")
        self.assertEqual(info.class_name, "CCObject")
        self.assertEqual(info.lua_type, "{ CCObject? }")
        self.assertFalse(info.is_ref)
        self.assertFalse(info.is_out)
        self.assertFalse(info.is_vector_ptr)

    def test_classify_gd_vector_const_ref(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        objects = {"CCObject": ccobject, "cocos2d::CCObject": ccobject}

        info = classify_arg("gd::vector<cocos2d::CCObject*> const&", objects)

        assert info is not None
        self.assertEqual(info.kind, "vector_view")
        self.assertTrue(info.is_ref)
        self.assertFalse(info.is_out)
        self.assertFalse(info.is_vector_ptr)

    def test_classify_gd_vector_out_ref(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        objects = {"CCObject": ccobject, "cocos2d::CCObject": ccobject}

        info = classify_arg("gd::vector<cocos2d::CCObject*>&", objects)

        assert info is not None
        self.assertEqual(info.kind, "vector_view")
        self.assertTrue(info.is_ref)
        self.assertTrue(info.is_out)
        self.assertFalse(info.is_vector_ptr)

    def test_classify_gd_vector_pointer_out(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        game_object = Class(name="GameObject", bases=["CCObject"])
        objects = {
            "CCObject": ccobject,
            "GameObject": game_object,
            "cocos2d::CCObject": ccobject,
        }

        info = classify_arg("gd::vector<GameObject*>*", objects)

        assert info is not None
        self.assertEqual(info.kind, "vector_view")
        self.assertFalse(info.is_ref)
        self.assertTrue(info.is_out)
        self.assertTrue(info.is_vector_ptr)
        self.assertEqual(info.class_name, "GameObject")

    def test_classify_gd_vector_int_as_primitive_vector(self) -> None:
        info = classify_arg("gd::vector<int>", {})

        self.assertIsNotNone(info)
        assert info is not None
        self.assertEqual(info.kind, "primitive_vector")
        self.assertEqual(info.cxx_type, "gd::vector<int>")
        self.assertEqual(info.lua_type, "{ number }")
        self.assertIsNotNone(info.element_type)
        assert info.element_type is not None
        self.assertEqual(info.element_type.kind, "number")
        self.assertFalse(info.is_ref)
        self.assertFalse(info.is_out)

    def test_classify_gd_vector_int_const_ref(self) -> None:
        info = classify_arg("gd::vector<int> const&", {})

        assert info is not None
        self.assertEqual(info.kind, "primitive_vector")
        self.assertTrue(info.is_ref)
        self.assertFalse(info.is_out)

    def test_classify_gd_vector_bool_as_primitive_vector(self) -> None:
        info = classify_arg("gd::vector<bool>", {})

        assert info is not None
        self.assertEqual(info.kind, "primitive_vector")
        self.assertEqual(info.lua_type, "{ boolean }")
        assert info.element_type is not None
        self.assertEqual(info.element_type.kind, "bool")

    def test_classify_gd_vector_string_as_primitive_vector(self) -> None:
        info = classify_arg("gd::vector<std::string>", {})

        assert info is not None
        self.assertEqual(info.kind, "primitive_vector")
        self.assertEqual(info.lua_type, "{ string }")
        assert info.element_type is not None
        self.assertEqual(info.element_type.kind, "string")

    def test_classify_gd_vector_wideint_as_primitive_vector(self) -> None:
        info = classify_arg("gd::vector<uint64_t>", {})

        assert info is not None
        self.assertEqual(info.kind, "primitive_vector")
        self.assertEqual(info.lua_type, "{ string }")
        assert info.element_type is not None
        self.assertEqual(info.element_type.kind, "wideint")

    def test_classify_gd_vector_enum_as_primitive_vector(self) -> None:
        info = classify_arg("gd::vector<FMOD_RESULT>", {})

        assert info is not None
        self.assertEqual(info.kind, "primitive_vector")
        self.assertEqual(info.lua_type, "{ number }")
        assert info.element_type is not None
        self.assertEqual(info.element_type.kind, "enum")
        self.assertEqual(info.element_type.cxx_type, "FMOD_RESULT")

    def test_classify_std_array_int(self) -> None:
        info = classify_arg("std::array<int, 300>", {})

        assert info is not None
        self.assertEqual(info.kind, "std_array")
        self.assertEqual(info.cxx_type, "std::array<int, 300>")
        self.assertEqual(info.lua_type, "{ number }")
        self.assertEqual(info.array_size, 300)
        assert info.element_type is not None
        self.assertEqual(info.element_type.kind, "number")

    def test_classify_std_array_float_2000(self) -> None:
        info = classify_arg("std::array<float, 2000>", {})

        assert info is not None
        self.assertEqual(info.kind, "std_array")
        self.assertEqual(info.array_size, 2000)

    def test_classify_std_array_ccpoint(self) -> None:
        info = classify_arg("std::array<cocos2d::CCPoint, 4>", {})

        assert info is not None
        self.assertEqual(info.kind, "std_array")
        self.assertEqual(info.lua_type, "{ CCPoint }")
        self.assertEqual(info.array_size, 4)
        assert info.element_type is not None
        self.assertEqual(info.element_type.kind, "value")
        self.assertEqual(info.element_type.lua_type, "CCPoint")

    def test_classify_std_array_short_pointer(self) -> None:
        info = classify_arg("std::array<short, 10>*", {})

        assert info is not None
        self.assertEqual(info.kind, "std_array")
        self.assertEqual(info.cxx_type, "std::array<short, 10>")
        self.assertTrue(info.is_vector_ptr)
        self.assertFalse(info.is_ref)
        self.assertTrue(info.is_out)
        self.assertEqual(info.array_size, 10)

    def test_classify_std_array_ccpoint_pointer(self) -> None:
        info = classify_arg("std::array<cocos2d::CCPoint, 400>*", {})

        assert info is not None
        self.assertEqual(info.kind, "std_array")
        self.assertEqual(info.array_size, 400)
        self.assertTrue(info.is_vector_ptr)

    def test_classify_std_array_rejects_oversize(self) -> None:
        self.assertIsNone(classify_arg(f"std::array<int, {STD_ARRAY_MAX_SIZE + 1}>", {}))

    def test_classify_std_array_rejects_unmapped_value_element(self) -> None:
        self.assertIsNone(classify_arg("std::array<ccVertex2F, 4>", {}))

    def test_classify_std_array_rejects_nested_container_element(self) -> None:
        self.assertIsNone(classify_arg("std::array<gd::vector<int>, 4>", {}))

    def test_classify_gd_vector_object_pointer_stays_vector_view(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        objects = {"CCObject": ccobject, "cocos2d::CCObject": ccobject}

        info = classify_arg("gd::vector<cocos2d::CCObject*>", objects)

        assert info is not None
        self.assertEqual(info.kind, "vector_view")
        self.assertEqual(info.lua_type, "{ CCObject? }")

    def test_classify_gd_vector_opaque_handle_pointer_as_vector_view(self) -> None:
        info = classify_arg("gd::vector<FMOD::Sound*>", {})

        assert info is not None
        self.assertEqual(info.kind, "vector_view")
        self.assertEqual(info.lua_type, "{ FMODSoundHandle? }")
        assert info.element_type is not None
        self.assertEqual(info.element_type.kind, "opaque_handle")

    def test_classify_gd_vector_group_command_object2_pointer_as_vector_view(
        self,
    ) -> None:
        info = classify_arg("gd::vector<GroupCommandObject2*>", {})

        assert info is not None
        self.assertEqual(info.kind, "vector_view")
        self.assertEqual(info.lua_type, "{ GroupCommandObject2? }")

    def test_classify_gd_vector_rejects_nested_containers(self) -> None:
        self.assertIsNone(classify_arg("gd::vector<gd::vector<int>>", {}))

    def test_classify_gd_map_int_object_pointer(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        objects = {"CCObject": ccobject, "cocos2d::CCObject": ccobject}

        info = classify_arg("gd::map<int, cocos2d::CCObject*>", objects)

        self.assertIsNotNone(info)
        assert info is not None
        self.assertEqual(info.kind, "map")
        self.assertEqual(info.cxx_type, "gd::map<int, cocos2d::CCObject*>")
        self.assertEqual(info.lua_type, "{ [number]: CCObject? }")
        assert info.key_type is not None
        assert info.value_type is not None
        self.assertEqual(info.key_type.kind, "number")
        self.assertEqual(info.value_type.kind, "object")
        self.assertEqual(info.value_type.class_name, "CCObject")
        self.assertFalse(info.is_ref)
        self.assertFalse(info.is_out)

    def test_classify_gd_map_primitive_value_pair(self) -> None:
        info = classify_arg("gd::map<int, bool>", {})

        self.assertIsNotNone(info)
        assert info is not None
        self.assertEqual(info.kind, "map")
        self.assertEqual(info.lua_type, "{ [number]: boolean }")
        assert info.key_type is not None
        assert info.value_type is not None
        self.assertEqual(info.key_type.kind, "number")
        self.assertEqual(info.value_type.kind, "bool")

    def test_classify_gd_map_string_key(self) -> None:
        info = classify_arg("gd::map<std::string, int>", {})

        self.assertIsNotNone(info)
        assert info is not None
        self.assertEqual(info.kind, "map")
        self.assertEqual(info.lua_type, "{ [string]: number }")
        assert info.key_type is not None
        self.assertEqual(info.key_type.kind, "string")

    def test_classify_gd_map_const_ref(self) -> None:
        info = classify_arg("gd::map<int, int> const&", {})

        assert info is not None
        self.assertEqual(info.kind, "map")
        self.assertTrue(info.is_ref)
        self.assertFalse(info.is_out)

    def test_classify_gd_unordered_map_primitive_pair(self) -> None:
        info = classify_arg("gd::unordered_map<int, int>", {})

        self.assertIsNotNone(info)
        assert info is not None
        self.assertEqual(info.kind, "unordered_map")
        self.assertEqual(info.cxx_type, "gd::unordered_map<int, int>")
        self.assertEqual(info.lua_type, "{ [number]: number }")
        assert info.key_type is not None
        assert info.value_type is not None
        self.assertEqual(info.key_type.kind, "number")
        self.assertEqual(info.value_type.kind, "number")

    def test_classify_gd_set_int(self) -> None:
        info = classify_arg("gd::set<int>", {})

        self.assertIsNotNone(info)
        assert info is not None
        self.assertEqual(info.kind, "set")
        self.assertEqual(info.cxx_type, "gd::set<int>")
        self.assertEqual(info.lua_type, "{ number }")
        assert info.element_type is not None
        self.assertEqual(info.element_type.kind, "number")
        self.assertFalse(info.is_ref)
        self.assertFalse(info.is_out)

    def test_classify_gd_set_string(self) -> None:
        info = classify_arg("gd::set<std::string>", {})

        assert info is not None
        self.assertEqual(info.kind, "set")
        self.assertEqual(info.lua_type, "{ string }")
        assert info.element_type is not None
        self.assertEqual(info.element_type.kind, "string")

    def test_classify_gd_unordered_set_int(self) -> None:
        info = classify_arg("gd::unordered_set<int>", {})

        self.assertIsNotNone(info)
        assert info is not None
        self.assertEqual(info.kind, "unordered_set")
        self.assertEqual(info.cxx_type, "gd::unordered_set<int>")
        self.assertEqual(info.lua_type, "{ number }")
        assert info.element_type is not None
        self.assertEqual(info.element_type.kind, "number")

    def test_classify_leading_const_container_ref_is_not_out(self) -> None:
        info = classify_arg("const gd::unordered_set<int>&", {})

        self.assertIsNotNone(info)
        assert info is not None
        self.assertEqual(info.kind, "unordered_set")
        self.assertTrue(info.is_ref)
        self.assertFalse(info.is_out)

    def test_classify_gd_map_rejects_object_keys(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        objects = {"CCObject": ccobject, "cocos2d::CCObject": ccobject}

        self.assertIsNone(classify_arg("gd::map<cocos2d::CCObject*, int>", objects))

    def test_classify_gd_map_rejects_nested_containers(self) -> None:
        self.assertIsNone(classify_arg("gd::map<int, gd::vector<int>>", {}))

    def test_classify_gd_unordered_map_nested_object_vector_value(self) -> None:
        label = Class(name="LabelGameObject", namespace="")
        objects = {"LabelGameObject": label}

        info = classify_arg("gd::unordered_map<int, gd::vector<LabelGameObject*>>", objects)

        self.assertIsNotNone(info)
        assert info is not None
        assert info.value_type is not None
        self.assertEqual(info.value_type.kind, "vector_view")
        self.assertEqual(info.lua_type, "{ [number]: { LabelGameObject? } }")

    def test_classify_gd_map_pair_key_nested_opaque_vector_value(self) -> None:
        info = classify_arg("gd::map<std::pair<int, int>, gd::vector<GroupCommandObject2*>>", {})

        self.assertIsNotNone(info)
        assert info is not None
        assert info.value_type is not None
        self.assertEqual(info.value_type.kind, "vector_view")
        self.assertIn("value:", info.lua_type)

    def test_classify_nested_primitive_vector_int_pointers(self) -> None:
        info = classify_arg("gd::vector<gd::vector<int>*>", {})

        self.assertIsNotNone(info)
        assert info is not None
        self.assertEqual(info.kind, "nested_primitive_vector_view")
        self.assertEqual(info.lua_type, "{ { number } }")

    def test_classify_gd_set_rejects_nested_containers(self) -> None:
        self.assertIsNone(classify_arg("gd::set<gd::vector<int>>", {}))

    def test_classify_gd_set_ccobject_pointer(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        objects = {"CCObject": ccobject, "cocos2d::CCObject": ccobject}

        info = classify_arg("gd::set<cocos2d::CCObject*>", objects)

        self.assertIsNotNone(info)
        assert info is not None
        self.assertEqual(info.kind, "set")
        self.assertEqual(info.cxx_type, "gd::set<cocos2d::CCObject*>")
        self.assertEqual(info.lua_type, "{ CCObject? }")
        assert info.element_type is not None
        self.assertEqual(info.element_type.kind, "object")
        self.assertEqual(info.element_type.class_name, "CCObject")

    def test_classify_gd_unordered_set_ccobject_pointer(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        objects = {"CCObject": ccobject, "cocos2d::CCObject": ccobject}

        info = classify_arg("gd::unordered_set<cocos2d::CCObject*>", objects)

        self.assertIsNotNone(info)
        assert info is not None
        self.assertEqual(info.kind, "unordered_set")
        self.assertEqual(info.cxx_type, "gd::unordered_set<cocos2d::CCObject*>")
        self.assertEqual(info.lua_type, "{ CCObject? }")
        assert info.element_type is not None
        self.assertEqual(info.element_type.kind, "object")
        self.assertEqual(info.element_type.class_name, "CCObject")

    def test_classify_std_pair_int_int(self) -> None:
        info = classify_arg("std::pair<int, int>", {})

        self.assertIsNotNone(info)
        assert info is not None
        self.assertEqual(info.kind, "pair")
        self.assertEqual(info.lua_type, "{ first: number, second: number }")

    def test_classify_gd_map_pair_value(self) -> None:
        info = classify_arg("gd::unordered_map<int, std::pair<int, int>>", {})

        self.assertIsNotNone(info)
        assert info is not None
        self.assertEqual(info.kind, "unordered_map")
        self.assertEqual(info.lua_type, "{ [number]: { first: number, second: number } }")

    def test_classify_gd_map_pair_key_entry_list(self) -> None:
        info = classify_arg("gd::map<std::pair<int, int>, std::pair<float, float>>", {})

        self.assertIsNotNone(info)
        assert info is not None
        self.assertEqual(info.kind, "map")
        self.assertIn("value:", info.lua_type)
        self.assertNotIn("[", info.lua_type)

    def test_classify_gd_vector_pair(self) -> None:
        info = classify_arg("gd::vector<std::pair<int, int>>", {})

        self.assertIsNotNone(info)
        assert info is not None
        self.assertEqual(info.kind, "primitive_vector")
        self.assertEqual(info.lua_type, "{ { first: number, second: number } }")

    def test_classify_gd_set_pair(self) -> None:
        info = classify_arg("gd::set<std::pair<int, int>>", {})

        self.assertIsNotNone(info)
        assert info is not None
        self.assertEqual(info.kind, "set")
        self.assertEqual(info.lua_type, "{ { first: number, second: number } }")


class GdEnumTypeMapTests(unittest.TestCase):
    _RESIDUAL_GD_ENUMS = (
        "BoomListType",
        "ChestSpriteState",
        "CircleMode",
        "CommentKeyType",
        "CommentType",
        "CurrencyRewardType",
        "CurrencySpriteType",
        "DialogAnimationType",
        "FormatterType",
        "GameObjectClassType",
        "GhostType",
        "InputValueType",
        "LeaderboardType",
        "MenuAnimationType",
        "MoveTargetType",
        "PlaybackMode",
        "PlayerButton",
        "ScaleButtonType",
        "SelectSettingType",
        "ShipStreak",
        "SpecialRewardItem",
        "TextStyleType",
        "TouchTriggerControl",
        "TouchTriggerType",
        "UndoCommand",
        "spriteMode",
    )

    _STATE_TYPES_NOT_ENUMS = ("CCIndexPath",)

    _REGISTERED_STATE_VALUE_STRUCTS = (
        "GJGameState",
        "GJShaderState",
        "FMODAudioState",
        "GJTransformState",
        "EffectManagerState",
        "SequenceTriggerState",
    )

    def test_classify_gd_enums(self) -> None:
        for name in (
            "GameOptionsSetting",
            "EasingType",
            "GJLevelType",
            "LeaderboardStat",
            "LevelLeaderboardMode",
            "LevelLeaderboardType",
            "ShopType",
            "Speed",
            "GJDifficulty",
            "ZLayer",
            "AudioSortType",
            "GameObjectType",
            "GJFeatureState",
            "GJChallengeType",
            "GauntletType",
            "GJSongType",
            "SelectArtType",
            *self._RESIDUAL_GD_ENUMS,
        ):
            info = classify_arg(name, {})
            self.assertIsNotNone(info)
            assert info is not None
            self.assertEqual(info.kind, "enum")
            self.assertEqual(info.lua_type, "number")
            self.assertEqual(info.cxx_type, name)

    def test_residual_state_types_stay_unsupported(self) -> None:
        for name in self._STATE_TYPES_NOT_ENUMS:
            info = classify_arg(name, {})
            self.assertIsNone(info, name)

    def test_registered_state_structs_classify_as_value(self) -> None:
        for name in self._REGISTERED_STATE_VALUE_STRUCTS:
            info = classify_arg(name, {})
            self.assertIsNotNone(info, name)
            assert info is not None
            self.assertEqual(info.kind, "value", name)
            self.assertNotEqual(info.kind, "enum", name)

    def test_bound_runtime_state_class_is_value_not_enum(self) -> None:
        objects = codegen_object_map(Root(classes=[Class(name="GJGameState")]))
        info = classify_arg("GJGameState", objects)
        self.assertIsNotNone(info)
        assert info is not None
        self.assertEqual(info.kind, "value")

    def test_classify_gd_enum_const_ref(self) -> None:
        info = classify_arg("EasingType const&", {})
        self.assertIsNotNone(info)
        assert info is not None
        self.assertEqual(info.kind, "enum")
        self.assertEqual(info.cxx_type, "EasingType")
        self.assertTrue(info.is_ref)
        self.assertFalse(info.is_out)

    def test_classify_gd_vector_enum(self) -> None:
        info = classify_arg("gd::vector<GJLevelType>", {})
        self.assertIsNotNone(info)
        assert info is not None
        self.assertEqual(info.kind, "primitive_vector")
        self.assertEqual(info.lua_type, "{ number }")
        assert info.element_type is not None
        self.assertEqual(info.element_type.kind, "enum")
        self.assertEqual(info.element_type.cxx_type, "GJLevelType")


class CocosEnumTypeMapTests(unittest.TestCase):
    _MOD_FACING_COCOS_ENUMS = (
        "CCObjectType",
        "CCVerticalTextAlignment",
        "ccGLServerState",
        "ccScriptType",
        "ccTouchesMode",
        "tCCMenuState",
        "tCCPositionType",
    )

    def test_classify_cocos_enums(self) -> None:
        for name, cxx in (
            ("enumKeyCodes", "cocos2d::enumKeyCodes"),
            ("TextureQuality", "cocos2d::TextureQuality"),
            *((n, f"cocos2d::{n}") for n in self._MOD_FACING_COCOS_ENUMS),
        ):
            for arg in (name, cxx):
                info = classify_arg(arg, {})
                self.assertIsNotNone(info, arg)
                assert info is not None
                self.assertEqual(info.kind, "enum", arg)
                self.assertEqual(info.lua_type, "number", arg)
                self.assertEqual(info.cxx_type, cxx, arg)

    def test_classify_cocos_enum_const_ref(self) -> None:
        info = classify_arg("cocos2d::ccTouchesMode const&", {})
        self.assertIsNotNone(info)
        assert info is not None
        self.assertEqual(info.kind, "enum")
        self.assertEqual(info.cxx_type, "cocos2d::ccTouchesMode")
        self.assertTrue(info.is_ref)
        self.assertFalse(info.is_out)

    def test_cc_timeval_not_enum(self) -> None:
        info = classify_arg("cocos2d::cc_timeval", {})
        self.assertIsNone(info)


class FmodTypeMapTests(unittest.TestCase):
    def test_classify_fmod_enums(self) -> None:
        for name in ("FMOD_RESULT", "FMOD_OPENSTATE", "FMOD_SPEAKERMODE"):
            info = classify_arg(name, {})
            self.assertIsNotNone(info)
            assert info is not None
            self.assertEqual(info.kind, "enum")
            self.assertEqual(info.lua_type, "number")
            self.assertEqual(info.cxx_type, name)

    def test_classify_fmod_opaque_handles(self) -> None:
        cases = {
            "FMOD::Channel*": "FMODChannel",
            "FMOD::Sound*": "FMODSoundHandle",
            "FMOD::System*": "FMODSystem",
            "FMOD::DSP*": "FMODDSP",
            "FMOD::ChannelGroup*": "FMODChannelGroup",
            "FMODSound*": "FMODSoundHandle",
        }
        for cxx, lua in cases.items():
            info = classify_arg(cxx, {})
            self.assertIsNotNone(info)
            assert info is not None
            self.assertEqual(info.kind, "opaque_handle")
            self.assertEqual(info.cxx_type, cxx)
            self.assertEqual(info.lua_type, lua)

    def test_classify_fmod_channel_return_is_optional(self) -> None:
        info = classify_return("FMOD::Channel*", {})
        self.assertIsNotNone(info)
        assert info is not None
        self.assertEqual(info.kind, "opaque_handle")
        self.assertEqual(info.lua_type, "FMODChannel?")

    def test_classify_fmod_sound_alias_return_is_optional(self) -> None:
        info = classify_return("FMODSound*", {})
        self.assertIsNotNone(info)
        assert info is not None
        self.assertEqual(info.kind, "opaque_handle")
        self.assertEqual(info.cxx_type, "FMODSound*")
        self.assertEqual(info.lua_type, "FMODSoundHandle?")

    def test_classify_unlisted_fmod_pointer_stays_unsupported(self) -> None:
        self.assertIsNone(classify_arg("FMOD::Geometry*", {}))


class CocosOpaqueHandleTests(unittest.TestCase):
    def test_classify_cocos_delegate_opaque_handles(self) -> None:
        cases = {
            "cocos2d::CCEvent*": "CCEvent",
            "cocos2d::extension::CCEditBox*": "CCEditBox",
        }
        for cxx, lua in cases.items():
            info = classify_arg(cxx, {})
            self.assertIsNotNone(info)
            assert info is not None
            self.assertEqual(info.kind, "opaque_handle")
            self.assertEqual(info.cxx_type, cxx)
            self.assertEqual(info.lua_type, lua)

    def test_classify_cc_event_return_is_optional(self) -> None:
        info = classify_return("cocos2d::CCEvent*", {})
        self.assertIsNotNone(info)
        assert info is not None
        self.assertEqual(info.kind, "opaque_handle")
        self.assertEqual(info.lua_type, "CCEvent?")


class ValueStructGateTests(unittest.TestCase):
    def test_classify_smart_prefab_result_value(self) -> None:
        info = classify_arg("SmartPrefabResult const&", {})
        self.assertIsNotNone(info)
        assert info is not None
        self.assertEqual(info.kind, "value")
        self.assertEqual(info.lua_type, "SmartPrefabResult")

    def test_classify_map_string_smart_prefab_result(self) -> None:
        info = classify_arg("gd::map<gd::string, SmartPrefabResult>", {})
        self.assertIsNotNone(info)
        assert info is not None
        self.assertEqual(info.kind, "map")
        self.assertEqual(info.lua_type, "{ [string]: SmartPrefabResult }")
        assert info.value_type is not None
        self.assertEqual(info.value_type.kind, "value")
        self.assertEqual(info.value_type.lua_type, "SmartPrefabResult")

    def test_classify_chance_object_vector_is_value_primitive_vector(self) -> None:
        info = classify_arg("gd::vector<ChanceObject>", {})
        self.assertIsNotNone(info)
        assert info is not None
        self.assertEqual(info.kind, "primitive_vector")
        assert info.element_type is not None
        self.assertEqual(info.element_type.kind, "value")
        self.assertEqual(info.element_type.lua_type, "ChanceObject")
