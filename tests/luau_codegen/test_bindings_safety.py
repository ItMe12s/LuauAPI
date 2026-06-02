from __future__ import annotations

import unittest
from helpers import (
    Arg,  # type: ignore[import-unresolved]
    Class,  # type: ignore[import-unresolved]
    Field,  # type: ignore[import-unresolved]
    Function,  # type: ignore[import-unresolved]
    Method,  # type: ignore[import-unresolved]
    Root,  # type: ignore[import-unresolved]
    TypeInfo,  # type: ignore[import-unresolved]
    _emit_class_file,  # type: ignore[import-unresolved]
    _emit_common_file,  # type: ignore[import-unresolved]
    all_platforms,  # type: ignore[import-unresolved]
    collect_plan,  # type: ignore[import-unresolved]
    emit_free_functions_file,  # type: ignore[import-unresolved]
    emit_hook_support,  # type: ignore[import-unresolved]
    emit_internal_hpp,  # type: ignore[import-unresolved]
    emit_luau_types,  # type: ignore[import-unresolved]
    emit_stack_check,  # type: ignore[import-unresolved]
    group_supported,  # type: ignore[import-unresolved]
    group_supported_free_functions,  # type: ignore[import-unresolved]
    types_text,  # type: ignore[import-unresolved]
)


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

        self.assertIn("static_assert(1 < LUA_UTAG_LIMIT", text)

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

        self.assertIn("luax::evictMenuHandlersIfFinalRelease(self);", text)
        self.assertIn('#include "lua/bindings/framework/LuaMenuHandler.hpp"', text)
        self.assertIn("geode::Result<void> installFieldsReleaseHook()", text)
        self.assertIn(
            "if (auto hookResult = installFieldsReleaseHook(); hookResult.isErr())",
            text,
        )
        self.assertIn("return geode::Err(hookResult.unwrapErr());", text)
        self.assertNotIn("luau fields release hook failed", text)

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

    def test_generated_hooks_use_binding_host(self) -> None:
        text = emit_internal_hpp()

        self.assertIn("luax::BindingHost::getIfInitialized()", text)
        self.assertIn("luax::BindingHost::ResourcesRootScope", text)
        self.assertNotIn("luax::Runtime::getIfInitialized()", text)
        self.assertNotIn("lua/runtime/Runtime.hpp", text)
        self.assertIn('#include "lua/bindings/framework/BindingHost.hpp"', text)

    def test_common_bind_registers_shutdown_hooks_via_binding_host(self) -> None:
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

        self.assertIn("luax::BindingHost::fromState(L)", text)
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
        self.assertIn("override rejected", text)
        self.assertIn("continue;", text)
        self.assertNotIn('returned invalid skip value", targetId', text)

    def test_ui_button_config_decode_uses_check_specialization(self) -> None:
        info = TypeInfo(
            kind="value", cxx_type="UIButtonConfig", lua_type="UIButtonConfig"
        )
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
            "-- skipped m_pBatchNode: inaccessible-type:CCParticleBatchNode",
            types_text(files),
        )
