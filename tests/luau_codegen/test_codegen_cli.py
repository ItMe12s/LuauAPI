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
        from luau_codegen.parse.collect import collect_bindings_root  # type: ignore[import-unresolved]

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
        tmpdir = tempfile.mkdtemp()
        try:
            bindings = os.path.join(tmpdir, "bindings")
            os.makedirs(bindings)
            bro_a = os.path.join(bindings, "Cocos2d.bro")
            bro_b = os.path.join(bindings, "Extras.bro")
            with open(bro_a, "w", encoding="utf-8") as f:
                f.write(
                    "[[link(win)]]\n"
                    "class test::Foo {\n"
                    "    void first() = win 0x1;\n"
                    "}\n"
                )
            with open(bro_b, "w", encoding="utf-8") as f:
                f.write(
                    "[[link(android)]]\n"
                    "class test::Foo {\n"
                    "    void second() = android 0x2;\n"
                    "}\n"
                )
            for name in ("FMOD.bro", "Kazmath.bro", "GeometryDash.bro"):
                open(os.path.join(bindings, name), "w", encoding="utf-8").close()
            root = collect_bindings_root(bindings)
            merged = next(c for c in root.classes if c.name == "Foo")
            self.assertIn("link(win)", merged.attributes)
            self.assertIn("link(android)", merged.attributes)
            self.assertEqual({m.name for m in merged.methods}, {"first", "second"})
        finally:
            shutil.rmtree(tmpdir)
