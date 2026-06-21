from __future__ import annotations

import os
import tempfile
import unittest

from test_support import all_platforms, types_text
from luau_codegen.convert.marshalling import emit_stack_check  # type: ignore[import-unresolved]
from luau_codegen.convert.type_map import TypeInfo  # type: ignore[import-unresolved]
from luau_codegen.emit.bindings import emit_free_functions_file  # type: ignore[import-unresolved]
from luau_codegen.emit.bindings.class_file import _emit_class_file  # type: ignore[import-unresolved]
from luau_codegen.emit.bindings.common import _emit_common_file  # type: ignore[import-unresolved]
from luau_codegen.emit.cxx_templates import emit_hook_support, emit_internal_hpp  # type: ignore[import-unresolved]
from luau_codegen.emit.luau_types import emit as emit_luau_types  # type: ignore[import-unresolved]
from luau_codegen.emit.plan import collect_plan  # type: ignore[import-unresolved]
from luau_codegen.parse.broma import Arg, Class, Field, Function, Method, Root  # type: ignore[import-unresolved]
from luau_codegen.policy.filtering import group_supported  # type: ignore[import-unresolved]
from luau_codegen.policy.free_functions import group_supported_free_functions  # type: ignore[import-unresolved]
from luau_codegen.emit.metadata import emit_report  # type: ignore[import-unresolved]


class GeneratedSafetyTests(unittest.TestCase):
    def test_register_type_error_is_propagated(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        foo = Class(name="Foo", bases=["CCObject"])

        text = _emit_class_file(
            foo,
            {},
            [],
            [],
            {"CCObject": ccobject, "Foo": foo},
            set(),
            1,
            "win",
        )

        self.assertIn("auto registerResult = luax::Usertype<Foo>::registerType", text)
        self.assertIn(
            "if (registerResult.isErr()) return geode::Err(registerResult.unwrapErr());",
            text,
        )

    def test_type_only_base_omits_unregistered_ccobject_tag(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        layout_options = Class(
            name="LayoutOptions",
            namespace="geode",
            bases=["CCObject"],
            methods=[
                Method(
                    name="create",
                    ret="LayoutOptions*",
                    args=[],
                    is_static=True,
                    platforms=all_platforms("0x1"),
                )
            ],
        )
        anchor_layout_options = Class(
            name="AnchorLayoutOptions",
            namespace="geode",
            bases=["LayoutOptions"],
            methods=[
                Method(
                    name="create",
                    ret="AnchorLayoutOptions*",
                    args=[],
                    is_static=True,
                    platforms=all_platforms("0x2"),
                )
            ],
        )
        objects = {
            "CCObject": ccobject,
            "LayoutOptions": layout_options,
            "AnchorLayoutOptions": anchor_layout_options,
        }
        emitted = {"LayoutOptions", "AnchorLayoutOptions"}

        layout_text = _emit_class_file(
            layout_options,
            {"create": layout_options.methods},
            [],
            [],
            objects,
            set(),
            1,
            "win",
            emitted_class_names=emitted,
        )
        anchor_text = _emit_class_file(
            anchor_layout_options,
            {"create": anchor_layout_options.methods},
            [],
            [],
            objects,
            set(),
            1,
            "win",
            emitted_class_names=emitted,
        )

        self.assertIn('registerType(L, "LayoutOptions", {})', layout_text)
        self.assertNotIn("CCObject>::tag()", layout_text)
        self.assertIn("LayoutOptions>::tag()", anchor_text)
        self.assertNotIn("CCObject>::tag()", anchor_text)

    def test_common_file_asserts_userdata_tag_budget(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        ccnode = Class(
            name="CCNode",
            namespace="cocos2d",
            bases=["CCObject"],
            methods=[
                Method(
                    name="getTag",
                    ret="int",
                    args=[],
                    platforms=all_platforms("0x1"),
                )
            ],
        )
        root = Root(classes=[ccobject, ccnode])
        plan = collect_plan(root, "win")
        text = _emit_common_file(plan.emitted_classes, plan, "win")

        self.assertIn(
            "static_assert(luax::detail::kFirstDynamicUsertypeTag + 1 < LUA_UTAG_LIMIT",
            text,
        )

    def test_common_file_release_hook_evicts_menu_handlers(self) -> None:
        ccobject = Class(
            name="CCObject",
            namespace="cocos2d",
            methods=[
                Method(
                    name="release",
                    ret="void",
                    args=[],
                    platforms=all_platforms("0x1"),
                )
            ],
        )
        ccnode = Class(
            name="CCNode",
            namespace="cocos2d",
            bases=["CCObject"],
            methods=[
                Method(
                    name="getTag",
                    ret="int",
                    args=[],
                    platforms=all_platforms("0x1"),
                )
            ],
        )
        root = Root(classes=[ccobject, ccnode])
        plan = collect_plan(root, "win")
        text = _emit_common_file(plan.emitted_classes, plan, "win")

        self.assertIn("luax::evictTrampolinesIfFinalRelease(self);", text)
        self.assertIn('#include "framework/callback/LuaCocosHandler.hpp"', text)
        self.assertIn("geode::Result<void> installFieldsReleaseHook()", text)
        self.assertIn(
            "if (auto hookResult = installFieldsReleaseHook(); hookResult.isErr())",
            text,
        )
        self.assertIn("CCObject::release hook setup skipped", text)
        self.assertNotIn("return geode::Err(hookResult.unwrapErr());", text)
        self.assertIn("if (!address)", text)
        self.assertIn("CCObject::release hook address unresolved", text)
        self.assertIn("installResult = geode::Ok();", text)
        self.assertNotIn("luau fields release hook failed", text)

    def test_codegen_report_documents_ccobject_release_hook(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        root = Root(classes=[ccobject])
        plan = collect_plan(root, "win")

        with tempfile.TemporaryDirectory() as tmp:
            report_path = os.path.join(tmp, "report.md")
            emit_report(
                root,
                report_path,
                [],
                "types.luau",
                [],
                "win",
                0,
                0,
                plan=plan,
            )
            with open(report_path, encoding="utf-8") as handle:
                text = handle.read()

        self.assertIn("CCObject::release hook", text)
        self.assertIn("non-fatal", text)

    def test_hook_shutdown_clears_refs_and_disables_hooks(self) -> None:
        text = emit_hook_support()

        self.assertIn("callback->removed = true;", text)
        self.assertIn("callback->ref.reset();", text)
        self.assertIn("state.hook->disable();", text)
        self.assertIn("!callback || callback->removed", text)

    def test_hook_runtime_uses_named_deadline(self) -> None:
        text = emit_internal_hpp()

        self.assertIn("luax::kHookScriptDeadlineMs", text)
        self.assertNotIn("targetId, 50", text)

    def test_generated_hooks_use_runtime(self) -> None:
        text = emit_internal_hpp()

        self.assertIn("luax::Runtime::getIfInitialized()", text)
        self.assertIn("luax::Runtime::ResourcesRootScope", text)
        self.assertIn('#include "core/Runtime.hpp"', text)

    def test_common_bind_registers_shutdown_hooks_via_runtime(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        ccnode = Class(
            name="CCNode",
            namespace="cocos2d",
            bases=["CCObject"],
            methods=[
                Method(
                    name="getTag",
                    ret="int",
                    args=[],
                    platforms=all_platforms("0x1"),
                )
            ],
        )
        root = Root(classes=[ccobject, ccnode])
        plan = collect_plan(root, "win")
        text = _emit_common_file(plan.emitted_classes, plan, "win")

        self.assertIn("luax::Runtime::fromState(L)", text)
        self.assertNotIn("static_cast<luax::Runtime*>", text)

    def test_hook_api_is_table_only_with_skip_fields(self) -> None:
        text = emit_hook_support()

        self.assertIn("geode.hook expected callback table", text)
        self.assertNotIn("geode.hook expected function at arg 2", text)
        self.assertIn("luaapi_geode_skip", text)
        self.assertIn("luaapi_geode_fields", text)

    def test_invalid_skip_value_rejects_skip_and_continues_chain(self) -> None:
        text = emit_internal_hpp()

        self.assertIn("applyHookOverride", text)
        self.assertIn("protectedCallWithTraceback", text)
        self.assertNotIn("lua_pcall(L, 1, 0, 0)", text)
        self.assertIn("continue;", text)
        self.assertNotIn('returned invalid skip value", targetId', text)

    def test_remove_hook_callbacks_keeps_sorted_list_tombstones(self) -> None:
        text = emit_hook_support()
        self.assertIn("callback->removed = true;", text)
        self.assertIn("rebuildSortedHookLists(state);", text)
        self.assertNotIn("removeFromSortedLists", text)
        self.assertIn("callback->removed ||", emit_internal_hpp())

    def test_invalid_post_hook_return_override_rejects_and_continues_chain(
        self,
    ) -> None:
        text = emit_internal_hpp()
        post_hooks = text.split("void runLuaPostHooks")[1].split("} // namespace luauapi_gen")[0]

        self.assertRegex(
            post_hooks,
            r"if \(!lua_isnil\(L, -1\)\) \{[\s\S]*if \(!applyReturn\(L, -1\)\) \{[\s\S]*lua_settop\(L, top\);[\s\S]*continue;",
        )
        self.assertNotIn('returned invalid return override", targetId', post_hooks)

    def test_ui_button_config_decode_uses_check_specialization(self) -> None:
        info = TypeInfo(kind="value", cxx_type="UIButtonConfig", lua_type="UIButtonConfig")
        text = "".join(emit_stack_check(info, "-1", "config", "hook args"))

        self.assertIn("luax::check<UIButtonConfig>", text)
        self.assertNotIn("luax::toUIButtonConfig", text)

    def test_generated_hook_thunk_uses_apply_trampoline(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        cls = Class(
            name="CCNode",
            namespace="cocos2d",
            bases=["CCObject"],
            methods=[
                Method(
                    name="setTag",
                    ret="void",
                    args=[Arg("int", "tag")],
                    platforms=all_platforms("0x1"),
                )
            ],
        )

        text = _emit_class_file(
            cls,
            {"setTag": cls.methods},
            [(cls, cls.methods[0])],
            [],
            {"CCObject": ccobject, "CCNode": cls},
            set(),
            1,
            "win",
        )

        self.assertIn("runLuaPreHooks", text)
        self.assertIn("skipOriginal", text)
        self.assertIn("ApplyArgsCtx_CCNode_setTag_1", text)
        self.assertIn("luaapi_apply_args_CCNode_setTag_1", text)
        self.assertIn("applyHookOverride", text)
        self.assertIn('lua_getfield(L, idx, "tag")', text)
        self.assertIn("luax::check<int>", text)
        self.assertIn("*ctx->arg0 = arg0Override", text)
        self.assertIn("if (!skipOriginal)", text)
        hook_body = text.split("void luaapi_hook_CCNode_setTag_1")[1].split(
            "geode::Result<geode::Hook*>"
        )[0]
        self.assertIn("luaapi_original_CCNode_setTag_1", text)
        self.assertIn("geode::hook::createWrapper", text)
        self.assertIn(
            "reinterpret_cast<void (*)(cocos2d::CCNode* self, int arg0)>(luaapi_original_CCNode_setTag_1)(self, arg0);",
            hook_body,
        )
        self.assertNotIn("self->setTag(arg0);", hook_body)
        self.assertNotIn("self->cocos2d::CCNode::setTag(arg0);", hook_body)

    def test_hook_char_const_ptr_arg_override_uses_storage(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        cls = Class(
            name="CCLabel",
            namespace="cocos2d",
            bases=["CCObject"],
            methods=[
                Method(
                    name="setString",
                    ret="void",
                    args=[Arg("char const*", "text")],
                    platforms=all_platforms("0x1"),
                )
            ],
        )

        text = _emit_class_file(
            cls,
            {"setString": cls.methods},
            [(cls, cls.methods[0])],
            [],
            {"CCObject": ccobject, "CCLabel": cls},
            set(),
            1,
            "win",
        )

        self.assertIn("std::string* arg0Storage;", text)
        self.assertIn("char const** arg0;", text)
        self.assertIn("std::string arg0Storage;", text)
        self.assertIn('lua_getfield(L, idx, "text")', text)
        self.assertIn("luax::check<std::string>", text)
        self.assertIn("*ctx->arg0Storage = arg0Override_storage;", text)
        self.assertIn("*ctx->arg0 = ctx->arg0Storage->c_str();", text)

    def test_hook_named_args_disabled_for_duplicate_names(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        cls = Class(
            name="CCNode",
            namespace="cocos2d",
            bases=["CCObject"],
            methods=[
                Method(
                    name="setPair",
                    ret="void",
                    args=[Arg("int", "value"), Arg("int", "value")],
                    platforms=all_platforms("0x1"),
                )
            ],
        )

        text = _emit_class_file(
            cls,
            {"setPair": cls.methods},
            [(cls, cls.methods[0])],
            [],
            {"CCObject": ccobject, "CCNode": cls},
            set(),
            1,
            "win",
        )

        self.assertIn("bool useNamedArgs = false;", text)
        self.assertIn("hook args table shape invalid", text)

    def test_luau_owned_attr_pushes_owned_instance_return(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        cls = Class(
            name="CCNode",
            namespace="cocos2d",
            bases=["CCObject"],
            methods=[
                Method(
                    name="newChild",
                    ret="cocos2d::CCNode*",
                    args=[],
                    attributes=["luau_owned"],
                    platforms=all_platforms("0x1"),
                )
            ],
        )

        text = _emit_class_file(
            cls,
            {"newChild": cls.methods},
            [],
            [],
            {"CCObject": ccobject, "CCNode": cls, "cocos2d::CCNode": cls},
            set(),
            1,
            "win",
        )

        self.assertIn("pushOwned", text)

    def test_unmarked_instance_return_stays_borrowed(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        cls = Class(
            name="CCNode",
            namespace="cocos2d",
            bases=["CCObject"],
            methods=[
                Method(
                    name="child",
                    ret="cocos2d::CCNode*",
                    args=[],
                    platforms=all_platforms("0x1"),
                )
            ],
        )

        text = _emit_class_file(
            cls,
            {"child": cls.methods},
            [],
            [],
            {"CCObject": ccobject, "CCNode": cls, "cocos2d::CCNode": cls},
            set(),
            1,
            "win",
        )

        self.assertIn("pushBorrowed", text)
        self.assertNotIn("pushOwned", text)

    def test_generated_field_accessors_are_registered(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        cls = Class(
            name="CCNode",
            namespace="cocos2d",
            bases=["CCObject"],
            fields=[Field("m_nTag", "int")],
        )

        text = _emit_class_file(
            cls,
            {},
            [],
            [(cls, cls.fields[0])],
            {"CCObject": ccobject, "CCNode": cls},
            set(),
            1,
            "win",
        )

        self.assertIn("luaapi_CCNode_field_get_m_nTag", text)
        self.assertIn("self->m_nTag", text)
        self.assertIn("luaapi_CCNode_field_register_m_nTag<cocos2d::CCNode>(L);", text)
        self.assertIn('luax::Usertype<T>::field(L, "m_nTag"', text)

    def test_generated_vector_field_accessor_is_readonly_view(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        cls = Class(
            name="CCNode",
            namespace="cocos2d",
            bases=["CCObject"],
            fields=[Field("m_children", "gd::vector<cocos2d::CCObject*>")],
        )
        objects = {"CCObject": ccobject, "cocos2d::CCObject": ccobject, "CCNode": cls}

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

        self.assertIn("luaapi_CCNode_field_get_m_children", text)
        self.assertIn("pushReadOnlyVectorView<cocos2d::CCObject>", text)
        self.assertIn('luax::Usertype<T>::readonlyField(L, "m_children"', text)
        self.assertNotIn("self->m_children =", text)

    def test_vector_field_plan_and_types_are_bound(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        ccnode = Class(
            name="CCNode",
            namespace="cocos2d",
            bases=["CCObject"],
            fields=[Field("m_children", "gd::vector<cocos2d::CCObject*>")],
        )
        root = Root(classes=[ccobject, ccnode])

        plan = collect_plan(root, "win")
        files = emit_luau_types(root, "win", plan=plan)

        self.assertEqual(len(plan.field_targets_by_class["CCNode"]), 1)
        text = types_text(files)
        self.assertIn("m_children: { CCObject? }", text)
        self.assertNotIn("-- skipped m_children", text)

    def test_instance_method_vector_return_uses_owner_backed_view(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        cls = Class(
            name="CCNode",
            namespace="cocos2d",
            bases=["CCObject"],
            methods=[
                Method(
                    name="getChildrenRef",
                    ret="gd::vector<cocos2d::CCObject*> const&",
                    args=[],
                    platforms=all_platforms("0x1"),
                )
            ],
        )
        objects = {"CCObject": ccobject, "cocos2d::CCObject": ccobject, "CCNode": cls}
        grouped, _ = group_supported(cls, objects, "win")

        text = _emit_class_file(cls, grouped, [], [], objects, set(), 1, "win")

        self.assertIn("pushReadOnlyVectorView<cocos2d::CCObject>", text)
        self.assertIn("result, self", text)
        self.assertNotIn("pushOwnedReadOnlyVectorView", text)

    def test_instance_method_vector_value_return_uses_owned_view(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        cls = Class(
            name="CCNode",
            namespace="cocos2d",
            bases=["CCObject"],
            methods=[
                Method(
                    name="getChildren",
                    ret="gd::vector<cocos2d::CCObject*>",
                    args=[],
                    platforms=all_platforms("0x1"),
                )
            ],
        )
        objects = {"CCObject": ccobject, "cocos2d::CCObject": ccobject, "CCNode": cls}
        grouped, _ = group_supported(cls, objects, "win")

        text = _emit_class_file(cls, grouped, [], [], objects, set(), 1, "win")

        self.assertIn("pushOwnedReadOnlyVectorView<cocos2d::CCObject>", text)
        self.assertIn("result", text)

    def test_method_vector_input_arg_checks_lua_table(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        cls = Class(
            name="CCNode",
            namespace="cocos2d",
            bases=["CCObject"],
            methods=[
                Method(
                    name="setChildren",
                    ret="void",
                    args=[Arg("gd::vector<cocos2d::CCObject*> const&", "children")],
                    platforms=all_platforms("0x1"),
                )
            ],
        )
        objects = {"CCObject": ccobject, "cocos2d::CCObject": ccobject, "CCNode": cls}
        grouped, _ = group_supported(cls, objects, "win")

        text = _emit_class_file(cls, grouped, [], [], objects, set(), 1, "win")

        self.assertIn("checkObjectVectorView<cocos2d::CCObject>", text)
        self.assertIn("self->setChildren(arg0)", text)

    def test_method_vector_out_arg_returns_owned_view(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        game_object = Class(name="GameObject", bases=["CCObject"])
        cls = Class(
            name="GameLevelManager",
            bases=["CCObject"],
            methods=[
                Method(
                    name="loadObjects",
                    ret="void",
                    args=[Arg("gd::vector<GameObject*>*", "objects")],
                    platforms=all_platforms("0x1"),
                )
            ],
        )
        objects = {
            "CCObject": ccobject,
            "GameObject": game_object,
            "GameLevelManager": cls,
        }
        grouped, _ = group_supported(cls, objects, "win")

        text = _emit_class_file(cls, grouped, [], [], objects, set(), 1, "win")

        self.assertIn("gd::vector<GameObject*> arg0{}", text)
        self.assertIn("self->loadObjects(&arg0)", text)
        self.assertIn("pushOwnedReadOnlyVectorView<GameObject>", text)

    def test_free_function_vector_return_uses_owned_view(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        fn = Function(
            name="children",
            namespace="geode::utils",
            ret="gd::vector<cocos2d::CCObject*>",
            args=[],
        )
        objects = {"CCObject": ccobject, "cocos2d::CCObject": ccobject}
        kept, _ = group_supported_free_functions([fn], objects, "win")
        text = emit_free_functions_file(kept, objects)

        self.assertIn("pushOwnedReadOnlyVectorView<cocos2d::CCObject>", text)
        self.assertIn("geode::utils::children()", text)

    def test_method_primitive_vector_input_checks_lua_table(self) -> None:
        cls = Class(
            name="Foo",
            methods=[
                Method(
                    name="take",
                    ret="void",
                    args=[Arg("gd::vector<int>", "values")],
                    platforms=all_platforms("0x1"),
                )
            ],
        )
        grouped, _ = group_supported(cls, {}, "win")

        text = _emit_class_file(cls, grouped, [], [], {}, set(), 1, "win")

        self.assertIn("checkPrimitiveVector<int>", text)
        self.assertIn("self->take(arg0)", text)

    def test_method_primitive_vector_return_pushes_table_copy(self) -> None:
        cls = Class(
            name="Foo",
            methods=[
                Method(
                    name="getValues",
                    ret="gd::vector<int>",
                    args=[],
                    platforms=all_platforms("0x1"),
                )
            ],
        )
        grouped, _ = group_supported(cls, {}, "win")

        text = _emit_class_file(cls, grouped, [], [], {}, set(), 1, "win")

        self.assertIn("pushPrimitiveVector<int>", text)
        self.assertIn("self->getValues()", text)

    def test_method_primitive_vector_out_arg_pushes_table_copy(self) -> None:
        cls = Class(
            name="Foo",
            methods=[
                Method(
                    name="fillValues",
                    ret="void",
                    args=[Arg("gd::vector<int>*", "values")],
                    platforms=all_platforms("0x1"),
                )
            ],
        )
        grouped, _ = group_supported(cls, {}, "win")

        text = _emit_class_file(cls, grouped, [], [], {}, set(), 1, "win")

        self.assertIn("gd::vector<int> arg0{}", text)
        self.assertIn("self->fillValues(&arg0)", text)
        self.assertIn("pushPrimitiveVector<int>", text)

    def test_free_function_primitive_vector_return_pushes_table_copy(self) -> None:
        fn = Function(
            name="values",
            namespace="geode::utils",
            ret="gd::vector<int>",
            args=[],
        )
        kept, _ = group_supported_free_functions([fn], {}, "win")
        text = emit_free_functions_file(kept, {})

        self.assertIn("pushPrimitiveVector<int>", text)
        self.assertIn("geode::utils::values()", text)

    def test_primitive_vector_pointer_field_get_dereferences_and_set_assigns(
        self,
    ) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        ccanimate = Class(
            name="CCAnimate",
            namespace="cocos2d",
            bases=["CCObject"],
            fields=[Field("m_pSplitTimes", "gd::vector<float>*")],
        )
        grouped, _ = group_supported(ccanimate, {}, "win")
        text = _emit_class_file(
            ccanimate,
            grouped,
            [],
            [(ccanimate, ccanimate.fields[0])],
            {"CCObject": ccobject, "cocos2d::CCObject": ccobject},
            set(),
            1,
            "win",
        )

        self.assertIn("pushPrimitiveVector<float>(L, self->m_pSplitTimes)", text)
        self.assertIn("luax::assignPrimitiveVector(*self->m_pSplitTimes, std::move(value))", text)
        self.assertNotIn("*self->m_pSplitTimes = std::move(value)", text)
        self.assertIn("field pointer is null", text)

    def test_primitive_vector_bool_field_setter_uses_assign_primitive_vector(
        self,
    ) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        foo = Class(
            name="Foo",
            namespace="cocos2d",
            bases=["CCObject"],
            fields=[Field("m_flags", "gd::vector<bool>")],
        )
        grouped, _ = group_supported(foo, {}, "win")
        text = _emit_class_file(
            foo,
            grouped,
            [],
            [(foo, foo.fields[0])],
            {"CCObject": ccobject, "cocos2d::CCObject": ccobject},
            set(),
            1,
            "win",
        )

        self.assertIn("checkPrimitiveVector<bool>", text)
        self.assertIn("luax::assignPrimitiveVector(self->m_flags, std::move(value))", text)
        self.assertNotIn("self->m_flags = std::move", text)

    def test_seed_value_field_setter_assigns_int(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        ccnode = Class(name="CCNode", namespace="cocos2d", bases=["CCObject"])
        level = Class(
            name="GJGameLevel",
            namespace="",
            bases=["CCNode"],
            fields=[Field("m_attempts", "geode::SeedValueRSV")],
        )
        objects = {
            "CCObject": ccobject,
            "CCNode": ccnode,
            "cocos2d::CCObject": ccobject,
            "cocos2d::CCNode": ccnode,
            "GJGameLevel": level,
        }
        grouped, _ = group_supported(level, objects, "win")
        text = _emit_class_file(
            level,
            grouped,
            [],
            [(level, level.fields[0])],
            objects,
            set(),
            1,
            "win",
        )

        self.assertIn("int value)", text)
        self.assertIn("self->m_attempts = value;", text)
        self.assertNotIn("static_cast<decltype(self->m_attempts)>(value)", text)
        self.assertIn("static_cast<int>(self->m_attempts)", text)

    def test_std_array_int_field_setter_uses_assign_std_array(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        foo = Class(
            name="Foo",
            namespace="cocos2d",
            bases=["CCObject"],
            fields=[Field("m_values", "std::array<int, 300>")],
        )
        grouped, _ = group_supported(foo, {}, "win")
        text = _emit_class_file(
            foo,
            grouped,
            [],
            [(foo, foo.fields[0])],
            {"CCObject": ccobject, "cocos2d::CCObject": ccobject},
            set(),
            1,
            "win",
        )

        self.assertIn("checkStdArray<int, 300>", text)
        self.assertIn("luax::assignStdArray<int, 300>(self->m_values, std::move(value))", text)
        self.assertNotIn("self->m_values = std::move", text)

    def test_std_array_short_pointer_field_get_and_set_use_std_array_helpers(
        self,
    ) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        foo = Class(
            name="Foo",
            namespace="cocos2d",
            bases=["CCObject"],
            fields=[Field("m_shorts", "std::array<short, 10>*")],
        )
        grouped, _ = group_supported(foo, {}, "win")
        text = _emit_class_file(
            foo,
            grouped,
            [],
            [(foo, foo.fields[0])],
            {"CCObject": ccobject, "cocos2d::CCObject": ccobject},
            set(),
            1,
            "win",
        )

        self.assertIn("pushStdArray<short, 10>(L, self->m_shorts)", text)
        self.assertIn("luax::assignStdArray<short, 10>(*self->m_shorts, std::move(value))", text)
        self.assertIn("field pointer is null", text)
        self.assertNotIn("*self->m_shorts = std::move(value)", text)

    def test_primitive_vector_int_pointer_field_setter_uses_assign_primitive_vector(
        self,
    ) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        foo = Class(
            name="Foo",
            namespace="cocos2d",
            bases=["CCObject"],
            fields=[Field("m_pInts", "gd::vector<int>*")],
        )
        grouped, _ = group_supported(foo, {}, "win")
        text = _emit_class_file(
            foo,
            grouped,
            [],
            [(foo, foo.fields[0])],
            {"CCObject": ccobject, "cocos2d::CCObject": ccobject},
            set(),
            1,
            "win",
        )

        self.assertIn("checkPrimitiveVector<int>", text)
        self.assertIn("luax::assignPrimitiveVector(*self->m_pInts, std::move(value))", text)
        self.assertNotIn("*self->m_pInts = std::move(value)", text)

    def test_map_field_setter_uses_assign_map(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        foo = Class(
            name="Foo",
            namespace="cocos2d",
            bases=["CCObject"],
            fields=[Field("m_values", "gd::map<int, bool>")],
        )
        grouped, _ = group_supported(foo, {}, "win")
        text = _emit_class_file(
            foo,
            grouped,
            [],
            [(foo, foo.fields[0])],
            {"CCObject": ccobject, "cocos2d::CCObject": ccobject},
            set(),
            1,
            "win",
        )

        self.assertIn("detail::checkAssociativeMap<int, bool, gd::map<int, bool>>", text)
        self.assertIn("luax::detail::assignAssociativeMap(self->m_values, std::move(value))", text)
        self.assertNotIn("self->m_values = std::move", text)

    def test_set_field_setter_uses_assign_set(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        foo = Class(
            name="Foo",
            namespace="cocos2d",
            bases=["CCObject"],
            fields=[Field("m_ids", "gd::set<int>")],
        )
        grouped, _ = group_supported(foo, {}, "win")
        text = _emit_class_file(
            foo,
            grouped,
            [],
            [(foo, foo.fields[0])],
            {"CCObject": ccobject, "cocos2d::CCObject": ccobject},
            set(),
            1,
            "win",
        )

        self.assertIn("detail::checkSetFromTable<int, gd::set<int>>", text)
        self.assertIn("luax::detail::assignSetContainer(self->m_ids, std::move(value))", text)
        self.assertNotIn("self->m_ids = std::move", text)

    def test_object_set_field_setter_uses_assign_set(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        foo = Class(
            name="Foo",
            namespace="cocos2d",
            bases=["CCObject"],
            fields=[Field("m_objects", "gd::set<cocos2d::CCObject*>")],
        )
        grouped, _ = group_supported(
            foo,
            {"CCObject": ccobject, "cocos2d::CCObject": ccobject},
            "win",
        )
        text = _emit_class_file(
            foo,
            grouped,
            [],
            [(foo, foo.fields[0])],
            {"CCObject": ccobject, "cocos2d::CCObject": ccobject},
            set(),
            1,
            "win",
        )

        self.assertIn(
            "detail::checkSetFromTable<cocos2d::CCObject*, gd::set<cocos2d::CCObject*>>",
            text,
        )
        self.assertIn("luax::detail::assignSetContainer(self->m_objects, std::move(value))", text)
        self.assertNotIn("self->m_objects = std::move", text)

    def test_object_set_pointer_field_setter_dereferences(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        ccset = Class(
            name="CCSet",
            namespace="cocos2d",
            bases=["CCObject"],
            fields=[Field("m_pSet", "gd::set<cocos2d::CCObject*>*")],
        )
        grouped, _ = group_supported(
            ccset,
            {"CCObject": ccobject, "cocos2d::CCObject": ccobject},
            "win",
        )
        text = _emit_class_file(
            ccset,
            grouped,
            [],
            [(ccset, ccset.fields[0])],
            {"CCObject": ccobject, "cocos2d::CCObject": ccobject},
            set(),
            1,
            "win",
        )

        self.assertIn(
            "detail::checkSetFromTable<cocos2d::CCObject*, gd::set<cocos2d::CCObject*>>",
            text,
        )
        self.assertIn("luax::detail::assignSetContainer(*self->m_pSet, std::move(value))", text)
        self.assertIn(
            "luax::detail::pushSetContainerPointer<gd::set<cocos2d::CCObject*>>(L, self->m_pSet)",
            text,
        )
        self.assertIn("is_pointer_v", text)
        self.assertIn("field pointer is null", text)

    def test_generated_bindings_include_container_tables_header(self) -> None:
        from luau_codegen.emit.cxx_templates import file_preamble  # type: ignore[import-unresolved]

        self.assertIn(
            '#include "framework/stack/ContainerTables.hpp"',
            file_preamble(),
        )

    def test_method_map_input_checks_lua_table(self) -> None:
        cls = Class(
            name="Foo",
            methods=[
                Method(
                    name="takeMap",
                    ret="void",
                    args=[Arg("gd::map<int, bool>", "values")],
                    platforms=all_platforms("0x1"),
                )
            ],
        )
        grouped, _ = group_supported(cls, {}, "win")

        text = _emit_class_file(cls, grouped, [], [], {}, set(), 1, "win")

        self.assertIn("detail::checkAssociativeMap<int, bool, gd::map<int, bool>>", text)
        self.assertIn("self->takeMap(arg0)", text)

    def test_method_map_return_pushes_table_copy(self) -> None:
        cls = Class(
            name="Foo",
            methods=[
                Method(
                    name="getMap",
                    ret="gd::map<int, bool>",
                    args=[],
                    platforms=all_platforms("0x1"),
                )
            ],
        )
        grouped, _ = group_supported(cls, {}, "win")

        text = _emit_class_file(cls, grouped, [], [], {}, set(), 1, "win")

        self.assertIn("detail::pushAssociativeMap<int, bool, gd::map<int, bool>>", text)
        self.assertIn("self->getMap()", text)

    def test_method_set_input_checks_lua_table(self) -> None:
        cls = Class(
            name="Foo",
            methods=[
                Method(
                    name="takeSet",
                    ret="void",
                    args=[Arg("gd::set<int>", "values")],
                    platforms=all_platforms("0x1"),
                )
            ],
        )
        grouped, _ = group_supported(cls, {}, "win")

        text = _emit_class_file(cls, grouped, [], [], {}, set(), 1, "win")

        self.assertIn("detail::checkSetFromTable<int, gd::set<int>>", text)
        self.assertIn("self->takeSet(arg0)", text)

    def test_free_function_unordered_map_return_pushes_table_copy(self) -> None:
        fn = Function(
            name="values",
            namespace="geode::utils",
            ret="gd::unordered_map<int, int>",
            args=[],
        )
        kept, _ = group_supported_free_functions([fn], {}, "win")
        text = emit_free_functions_file(kept, {})

        self.assertIn("detail::pushAssociativeMap<int, int, gd::unordered_map<int, int>>", text)
        self.assertIn("geode::utils::values()", text)

    def test_field_plan_and_types_include_ccnode_m_fields_only(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        ccnode = Class(
            name="CCNode",
            namespace="cocos2d",
            bases=["CCObject"],
            fields=[Field("m_obPosition", "cocos2d::CCPoint")],
        )
        root = Root(classes=[ccobject, ccnode])

        plan = collect_plan(root, "win")
        files = emit_luau_types(root, "win", plan=plan)

        self.assertEqual(len(plan.field_targets_by_class["CCNode"]), 1)
        cocos = types_text(files)
        self.assertIn("m_fields: { [string]: any }", cocos)
        self.assertIn("m_obPosition: CCPoint", cocos)
        self.assertIn("declare class CCNode", cocos)
        self.assertNotIn("declare class CCPoint end", cocos)

    def test_field_plan_emits_cocos_value_type_stubs(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        ccnode = Class(name="CCNode", namespace="cocos2d", bases=["CCObject"])
        sprite = Class(
            name="CCSprite",
            namespace="cocos2d",
            bases=["CCNode"],
            fields=[
                Field("m_sBlendFunc", "ccBlendFunc"),
                Field("m_transformToBatch", "CCAffineTransform"),
            ],
        )
        particle = Class(
            name="CCParticleSystem",
            namespace="cocos2d",
            bases=["CCNode"],
            fields=[Field("m_tStartColor", "cocos2d::ccColor4F")],
        )
        root = Root(classes=[ccobject, ccnode, sprite, particle])

        plan = collect_plan(root, "win")
        files = emit_luau_types(root, "win", plan=plan)

        cocos = types_text(files)
        self.assertIn("export type BlendFunc = { src: number, dst: number }", cocos)
        self.assertIn(
            "export type RGBAFloatColor = { r: number, g: number, b: number, a: number }",
            cocos,
        )
        self.assertIn(
            "export type CCAffineTransform = { a: number, b: number, c: number, d: number, tx: number, ty: number }",
            cocos,
        )
        self.assertIn("m_sBlendFunc: BlendFunc", cocos)
        self.assertIn("m_transformToBatch: CCAffineTransform", cocos)
        self.assertIn("m_tStartColor: RGBAFloatColor", cocos)
        self.assertNotIn("declare class BlendFunc end", cocos)

    def test_inaccessible_broma_field_is_not_bound(self) -> None:
        ccobject = Class(
            name="CCObject",
            namespace="cocos2d",
            fields=[Field("m_nChildIndex", "int")],
        )
        root = Root(classes=[ccobject])

        plan = collect_plan(root, "win")
        files = emit_luau_types(root, "win", plan=plan)

        self.assertEqual(plan.field_targets_by_class.get("CCObject", []), [])
        self.assertIn(
            "-- skipped m_nChildIndex: inaccessible-field",
            types_text(files),
        )

    def test_protected_broma_field_is_not_bound(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        cls = Class(
            name="Foo",
            attributes=["link(win)"],
            bases=["CCObject"],
            fields=[
                Field("m_secret", "int", access="protected"),
                Field("m_visible", "float"),
            ],
        )
        root = Root(classes=[ccobject, cls])

        plan = collect_plan(root, "win")
        files = emit_luau_types(root, "win", plan=plan)

        bound_names = [field.name for _, field in plan.field_targets_by_class.get("Foo", [])]
        self.assertEqual(bound_names, ["m_visible"])
        self.assertIn("-- skipped m_secret: inaccessible", types_text(files))

    def test_inaccessible_field_type_is_not_bound(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        ccnode = Class(name="CCNode", namespace="cocos2d", bases=["CCObject"])
        batch_node = Class(
            name="CCParticleBatchNode",
            namespace="cocos2d",
            bases=["CCNode"],
        )
        particle_system = Class(
            name="CCParticleSystem",
            namespace="cocos2d",
            bases=["CCNode"],
            fields=[Field("m_pBatchNode", "CCParticleBatchNode*")],
        )
        root = Root(classes=[ccobject, ccnode, batch_node, particle_system])

        plan = collect_plan(root, "win")
        files = emit_luau_types(root, "win", plan=plan)

        self.assertEqual(plan.field_targets_by_class.get("CCParticleSystem", []), [])
        self.assertIn(
            "-- skipped m_pBatchNode: not-callable-type:win:CCParticleBatchNode",
            types_text(files),
        )
