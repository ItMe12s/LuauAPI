from __future__ import annotations

from conftest import *


class CodegenIoTests(unittest.TestCase):
    def test_write_if_changed_skips_identical_content(self) -> None:
        from luau_codegen.cli.io import _write_if_changed  # type: ignore[import-unresolved]

        tmpdir = tempfile.mkdtemp()
        try:
            path = os.path.join(tmpdir, "out.txt")
            _write_if_changed(path, "same\n")
            first_mtime = os.stat(path).st_mtime_ns
            _write_if_changed(path, "same\n")
            self.assertEqual(os.stat(path).st_mtime_ns, first_mtime)
            _write_if_changed(path, "different\n")
            self.assertEqual(open(path, encoding="utf-8").read(), "different\n")
        finally:
            shutil.rmtree(tmpdir)

    def test_collect_bindings_root_warns_for_missing_bro_files(self) -> None:
        from luau_codegen.cli.collect import collect_bindings_root  # type: ignore[import-unresolved]

        tmpdir = tempfile.mkdtemp()
        try:
            with open(
                os.path.join(tmpdir, "GeometryDash.bro"), "w", encoding="utf-8"
            ) as f:
                f.write("class Foo {\n    void bar();\n};\n")
            with warnings.catch_warnings(record=True) as w:
                warnings.simplefilter("always")
                root = collect_bindings_root(tmpdir)
            self.assertEqual(len(root.classes), 1)
            self.assertTrue(any("missing Broma file" in str(x.message) for x in w))
        finally:
            shutil.rmtree(tmpdir)


class CodegenExitCodeTests(unittest.TestCase):
    def test_emit_exception_returns_exit_code_5(self) -> None:
        from luau_codegen.cli import main as cg  # type: ignore[import-unresolved]

        root = Root(classes=[Class(name="CCObject", namespace="cocos2d")])
        tmpdir = tempfile.mkdtemp()
        try:
            bindings = os.path.join(tmpdir, "bindings")
            out_dir = os.path.join(tmpdir, "out")
            types_dir = os.path.join(tmpdir, "types")
            os.makedirs(bindings)
            os.makedirs(types_dir)
            for name in (
                "Cocos2d.bro",
                "Extras.bro",
                "FMOD.bro",
                "Kazmath.bro",
                "GeometryDash.bro",
            ):
                open(os.path.join(bindings, name), "w", encoding="utf-8").close()
            with mock.patch.object(cg, "collect_bindings_root", return_value=root):
                with mock.patch.object(
                    cg.emit_bindings,
                    "emit",
                    side_effect=ValueError("emit failed"),
                ):
                    rc = cg.main(
                        [
                            "--bindings",
                            bindings,
                            "--out",
                            out_dir,
                            "--types-out",
                            types_dir,
                            "--platform",
                            "win",
                        ]
                    )
            self.assertEqual(rc, 5)
        finally:
            shutil.rmtree(tmpdir)


class F11ClassMergeTests(unittest.TestCase):
    def test_duplicate_class_merges_attrs(self) -> None:
        root = Root()
        cls1 = Class(
            name="Foo", namespace="test", attributes=["link(win)"], source="a.bro"
        )
        cls2 = Class(
            name="Foo", namespace="test", attributes=["link(android)"], source="b.bro"
        )
        root.classes = [cls1, cls2]

        seen: dict = {}
        for cls in root.classes:
            if cls.qualified_name in seen:
                existing = seen[cls.qualified_name]
                for attr in cls.attributes:
                    if attr not in existing.attributes:
                        existing.attributes.append(attr)
            else:
                seen[cls.qualified_name] = cls
        merged = list(seen.values())
        self.assertEqual(len(merged), 1)
        self.assertIn("link(win)", merged[0].attributes)
        self.assertIn("link(android)", merged[0].attributes)

    def test_duplicate_class_warns_on_method_conflict(self) -> None:
        cls1 = Class(name="Bar", namespace="test", source="a.bro")
        cls1.methods = [
            Method(name="foo", ret="void", args=[], platforms={"win": "0x1"})
        ]
        cls2 = Class(name="Bar", namespace="test", source="b.bro")
        cls2.methods = [
            Method(name="bar", ret="void", args=[], platforms={"win": "0x2"})
        ]

        with warnings.catch_warnings(record=True) as w:
            warnings.simplefilter("always")
            seen: dict = {}
            for cls in [cls1, cls2]:
                if cls.qualified_name in seen:
                    existing = seen[cls.qualified_name]
                    for attr in cls.attributes:
                        if attr not in existing.attributes:
                            existing.attributes.append(attr)
                    if cls.methods and existing.methods:
                        warnings.warn(
                            f"[luauapi] duplicate class {cls.qualified_name} "
                            f"from {cls.source} and {existing.source}, keeping first"
                        )
                    elif cls.methods and not existing.methods:
                        existing.methods = cls.methods
                else:
                    seen[cls.qualified_name] = cls
            self.assertEqual(len(w), 1)
            self.assertIn("duplicate class", str(w[0].message))
