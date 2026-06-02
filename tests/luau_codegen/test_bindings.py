from __future__ import annotations

from conftest import *


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


class F5OverloadFirstDeclaredWinsTests(unittest.TestCase):
    def test_first_overload_kept_on_arity_collision(self) -> None:
        cls = Class(
            name="TestObj",
            namespace="cocos2d",
            bases=["CCObject"],
            attributes=["link(win)"],
        )
        m1 = Method(
            name="foo",
            ret="void",
            args=[Arg(type="int", name="a")],
            platforms={"win": "0x1"},
        )
        m2 = Method(
            name="foo",
            ret="void",
            args=[Arg(type="float", name="b")],
            platforms={"win": "0x2"},
        )
        cls.methods = [m1, m2]
        objects = {"TestObj": cls, "cocos2d::TestObj": cls}
        grouped, skipped = group_supported(cls, objects, "win")
        self.assertIn("foo", grouped)
        self.assertEqual(len(grouped["foo"]), 1)
        self.assertIs(grouped["foo"][0], m1)
        self.assertEqual(len(skipped), 1)
        self.assertIn("ambiguous-overload-arity", skipped[0][1])

    def test_fail_on_ambiguous_overload_flag_exits_nonzero(self) -> None:
        tmpdir = tempfile.mkdtemp()
        try:
            with open(
                os.path.join(tmpdir, "GeometryDash.bro"), "w", encoding="utf-8"
            ) as f:
                f.write(
                    "class cocos2d::CCObject {};"
                    "class gd::TestObj : cocos2d::CCObject {"
                    "void foo(int a) = win 0x1;"
                    "void foo(float b) = win 0x2;"
                    "};"
                )
            code = codegen_main(
                [
                    "--bindings",
                    tmpdir,
                    "--platform",
                    "win",
                    "--list-outputs",
                    "--fail-on-ambiguous-overload",
                ]
            )
            self.assertEqual(code, 6)
        finally:
            shutil.rmtree(tmpdir)

    def test_ambiguous_overload_default_does_not_fail(self) -> None:
        tmpdir = tempfile.mkdtemp()
        try:
            with open(
                os.path.join(tmpdir, "GeometryDash.bro"), "w", encoding="utf-8"
            ) as f:
                f.write(
                    "class cocos2d::CCObject {};"
                    "class gd::TestObj : cocos2d::CCObject {"
                    "void foo(int a) = win 0x1;"
                    "void foo(float b) = win 0x2;"
                    "};"
                )
            code = codegen_main(
                ["--bindings", tmpdir, "--platform", "win", "--list-outputs"]
            )
            self.assertEqual(code, 0)
        finally:
            shutil.rmtree(tmpdir)

    def test_preferred_overload_selected_over_first_declared(self) -> None:
        cls = Class(
            name="CCScale9Sprite",
            namespace="cocos2d",
            bases=["CCObject"],
            attributes=["link(win)"],
        )
        loser = Method(
            name="create",
            ret="bool",
            args=[
                Arg(type="cocos2d::CCRect", name="rect"),
                Arg(type="char const*", name="file"),
            ],
            is_static=True,
            platforms={"win": "0x1"},
        )
        winner = Method(
            name="create",
            ret="bool",
            args=[
                Arg(type="char const*", name="file"),
                Arg(type="cocos2d::CCRect", name="rect"),
            ],
            is_static=True,
            platforms={"win": "0x2"},
        )
        cls.methods = [loser, winner]
        objects = {"CCScale9Sprite": cls, "cocos2d::CCScale9Sprite": cls}
        grouped, skipped = group_supported(cls, objects, "win")
        self.assertEqual(len(grouped["create"]), 1)
        self.assertIs(grouped["create"][0], winner)
        reasons = [reason for _, reason in skipped]
        self.assertIn("overload-superseded:2", reasons)
        self.assertFalse(
            any(r.startswith("ambiguous-overload-arity") for r in reasons),
            f"resolved overload should not be flagged ambiguous: {reasons}",
        )

    def test_unmatched_preference_falls_back_to_ambiguous(self) -> None:
        cls = Class(
            name="CCScale9Sprite",
            namespace="cocos2d",
            bases=["CCObject"],
            attributes=["link(win)"],
        )
        first = Method(
            name="create",
            ret="bool",
            args=[Arg(type="int", name="a"), Arg(type="int", name="b")],
            is_static=True,
            platforms={"win": "0x1"},
        )
        second = Method(
            name="create",
            ret="bool",
            args=[Arg(type="float", name="a"), Arg(type="float", name="b")],
            is_static=True,
            platforms={"win": "0x2"},
        )
        cls.methods = [first, second]
        objects = {"CCScale9Sprite": cls, "cocos2d::CCScale9Sprite": cls}
        grouped, skipped = group_supported(cls, objects, "win")
        self.assertEqual(len(grouped["create"]), 1)
        self.assertIs(grouped["create"][0], first)
        reasons = [reason for _, reason in skipped]
        self.assertIn("ambiguous-overload-arity:2", reasons)


class F8ConstMethodManglingTests(unittest.TestCase):
    def test_const_method_has_K_qualifier(self) -> None:
        cls = Class(name="CCObject", namespace="cocos2d")
        m = Method(
            name="getTag",
            ret="int",
            args=[],
            is_const=True,
            platforms={"android64": "0x1"},
        )
        sym = android_symbol(cls, m)
        self.assertTrue(sym.startswith("_ZNK"), f"Expected _ZNK prefix, got {sym}")
        self.assertIn("6getTag", sym)

    def test_non_const_method_no_K_qualifier(self) -> None:
        cls = Class(name="CCObject", namespace="cocos2d")
        m = Method(
            name="setTag",
            ret="void",
            args=[Arg(type="int", name="t")],
            platforms={"android64": "0x1"},
        )
        sym = android_symbol(cls, m)
        self.assertTrue(sym.startswith("_ZN"), f"Expected _ZN prefix, got {sym}")
        self.assertFalse(sym.startswith("_ZNK"))

    def test_const_vs_non_const_distinct(self) -> None:
        cls = Class(name="CCObject", namespace="cocos2d")
        m_const = Method(
            name="getTag",
            ret="int",
            args=[],
            is_const=True,
            platforms={"android64": "0x1"},
        )
        m_non = Method(
            name="getTag",
            ret="int",
            args=[],
            is_const=False,
            platforms={"android64": "0x1"},
        )
        self.assertNotEqual(android_symbol(cls, m_const), android_symbol(cls, m_non))

    def test_reference_and_const_type_mangling(self) -> None:
        cls = Class(name="Foo")
        method = Method(name="take", ret="void", args=[Arg("int const&", "value")])
        self.assertEqual(android_symbol(cls, method), "_ZN3Foo4takeERKi")

    def test_repeated_pointer_uses_substitution(self) -> None:
        cls = Class(name="Bar")
        method = Method(
            name="take",
            ret="void",
            args=[Arg("Foo*", "a"), Arg("Foo*", "b")],
        )
        self.assertEqual(android_symbol(cls, method), "_ZN3Bar4takeEP3FooS0_")

    def test_template_type_mangling_includes_defaults(self) -> None:
        cls = Class(name="Foo")
        method = Method(
            name="take",
            ret="void",
            args=[Arg("gd::vector<int>", "values")],
        )
        self.assertEqual(
            android_symbol(cls, method), "_ZN3Foo4takeEN2gd6vectorIiSaIiEEE"
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


class FmodBindingTests(unittest.TestCase):
    def test_fmod_opaque_handle_binding_emits_lightuserdata(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        cls = Class(
            name="FMODAudioEngine",
            bases=["CCObject"],
            attributes=["link(win)"],
            methods=[
                Method(
                    name="stopChannel",
                    ret="void",
                    args=[Arg("FMOD::Channel*", "channel")],
                    platforms=all_platforms("0x1"),
                ),
                Method(
                    name="getActiveMusicChannel",
                    ret="FMOD::Channel*",
                    args=[],
                    platforms=all_platforms("0x2"),
                ),
                Method(
                    name="printResult",
                    ret="void",
                    args=[Arg("FMOD_RESULT", "result")],
                    platforms=all_platforms("0x3"),
                ),
                Method(
                    name="channelLinkSound",
                    ret="void",
                    args=[Arg("int", "id"), Arg("FMODSound*", "sound")],
                    platforms=all_platforms("0x4"),
                ),
                Method(
                    name="preloadEffect",
                    ret="FMODSound*",
                    args=[Arg("gd::string", "path")],
                    platforms=all_platforms("0x5"),
                ),
            ],
        )
        objects = {"CCObject": ccobject, "FMODAudioEngine": cls}

        text = _emit_class_file(
            cls,
            {m.name: [m] for m in cls.methods},
            [],
            [],
            objects,
            set(),
            1,
            "win",
        )

        self.assertIn("self->stopChannel(arg0)", text)
        self.assertIn("static_cast<FMOD::Channel*>(lua_touserdata", text)
        self.assertIn("self->channelLinkSound(arg0, arg1)", text)
        self.assertIn("static_cast<FMODSound*>(lua_touserdata", text)
        self.assertIn("auto result = self->getActiveMusicChannel();", text)
        self.assertIn("lua_pushlightuserdata(L, result)", text)
        self.assertIn("static_cast<FMOD_RESULT>", text)
        self.assertIn("auto result = self->preloadEffect(arg0);", text)
        self.assertNotIn("Usertype<FMOD::Channel>", text)
        self.assertNotIn("pushBorrowed", text)


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
