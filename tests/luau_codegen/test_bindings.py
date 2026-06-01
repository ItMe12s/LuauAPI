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

    def test_hook_api_is_table_only_with_skip_fields(self) -> None:
        text = emit_hook_support()

        self.assertIn("geode.hook expected callback table", text)
        self.assertNotIn("geode.hook expected function at arg 2", text)
        self.assertIn("luaapi_geode_skip", text)
        self.assertIn("luaapi_geode_fields", text)

    def test_invalid_skip_value_continues_before_chain(self) -> None:
        text = emit_internal_hpp()

        self.assertIn('returned invalid skip value", targetId', text)
        self.assertIn("continue;", text)
        self.assertIn("return true;", text)

    def test_ui_button_config_decode_supported_for_hook_overrides(self) -> None:
        info = TypeInfo(
            kind="value", cxx_type="UIButtonConfig", lua_type="UIButtonConfig"
        )
        text = "".join(emit_value_decode(info, "config", "-1", "hook args"))

        self.assertIn("luax::toUIButtonConfig", text)
        self.assertIn("config = ", text)

    def test_generated_hook_thunk_runs_pre_branch(self) -> None:
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
        self.assertIn("lua_objlen", text)
        self.assertIn("useArrayArgs", text)
        self.assertIn('lua_getfield(L, idx, "tag")', text)
        self.assertIn("arg0 = arg0Override", text)
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
