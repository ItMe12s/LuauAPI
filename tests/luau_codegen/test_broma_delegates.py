from __future__ import annotations

import os
import tempfile
import unittest

from luau_codegen.convert.type_normalization import strip_ref  # type: ignore[import-unresolved]
from luau_codegen.emit.delegates import (  # type: ignore[import-unresolved]
    parse_broma,
)
from luau_codegen.parse.broma_delegates import (  # type: ignore[import-unresolved]
    collect_delegate_ptrs,
)


class BromaDelegateParserTests(unittest.TestCase):
    def test_parse_nested_template_virtual_method(self) -> None:
        bro = """
class NestedDelegate {
    virtual std::function<void(std::pair<int, int>)> onPair(std::vector<int> const& ids) = inline;
};
"""
        with tempfile.TemporaryDirectory() as tmpdir:
            path = os.path.join(tmpdir, "Nested.bro")
            with open(path, "w", encoding="utf-8") as f:
                f.write(bro)
            parsed = parse_broma(tmpdir)
        self.assertNotIn("NestedDelegate", parsed)

    def test_parse_delegate_with_attributes_and_namespace(self) -> None:
        bro = """
class geode::ui::SampleDelegate {
    virtual void onEvent(int id) = inline;
    virtual bool shouldClose() = inline;
};
"""
        with tempfile.TemporaryDirectory() as tmpdir:
            path = os.path.join(tmpdir, "Sample.bro")
            with open(path, "w", encoding="utf-8") as f:
                f.write(bro)
            parsed = parse_broma(tmpdir)
        self.assertIn("geode::ui::SampleDelegate", parsed)
        methods = {m.name: m for m in parsed["geode::ui::SampleDelegate"]}
        self.assertEqual(methods["onEvent"].args, [("int", "id")])
        self.assertEqual(methods["shouldClose"].ret, "bool")

    def test_parse_delegate_ignores_destructor_and_names_unnamed_args(self) -> None:
        bro = """
class CCEGLViewProtocol {
    virtual ~CCEGLViewProtocol() = default;
    virtual void setFrameSize(float, float) = inline;
    virtual void setTouchDelegate(EGLTouchDelegate* delegate) = inline;
    virtual void handleTouchesBegin(int count, int* ids, float* xs, float* ys) = inline;
};
"""
        with tempfile.TemporaryDirectory() as tmpdir:
            path = os.path.join(tmpdir, "View.bro")
            with open(path, "w", encoding="utf-8") as f:
                f.write(bro)
            parsed = parse_broma(tmpdir)
        methods = {m.name: m for m in parsed["CCEGLViewProtocol"]}
        self.assertNotIn("~CCEGLViewProtocol", methods)
        self.assertEqual(methods["setFrameSize"].args, [("float", "arg0"), ("float", "arg1")])
        self.assertNotIn("setTouchDelegate", methods)
        self.assertNotIn("handleTouchesBegin", methods)

    def test_parse_delegate_preserves_ccindexpath_reference(self) -> None:
        bro = """
class TableViewDelegate {
    virtual void didSelectRowAtIndexPath(CCIndexPath& indexPath, TableView* tableView) = inline;
};
"""
        with tempfile.TemporaryDirectory() as tmpdir:
            path = os.path.join(tmpdir, "TableView.bro")
            with open(path, "w", encoding="utf-8") as f:
                f.write(bro)
            parsed = parse_broma(tmpdir)
        method = parsed["TableViewDelegate"][0]
        self.assertEqual(method.args[0][0], "CCIndexPath&")

    def test_collect_delegate_ptrs_from_setter_signatures(self) -> None:
        bro = """
class Holder {
    void set_CustomSongDelegate(CustomSongDelegate* delegate);
};
class CustomSongDelegate {
    virtual int getActiveSongID() = inline;
};
"""
        with tempfile.TemporaryDirectory() as tmpdir:
            path = os.path.join(tmpdir, "Holder.bro")
            with open(path, "w", encoding="utf-8") as f:
                f.write(bro)
            ptrs = collect_delegate_ptrs(tmpdir, norm=strip_ref)
        self.assertIn("CustomSongDelegate", ptrs)

    def test_collect_matches_fixture_delegate_count(self) -> None:
        from luau_codegen.emit.delegates import (  # type: ignore[import-unresolved]
            collect as collect_delegate_specs,
            fallback_bindings_dir,
        )

        specs = collect_delegate_specs(fallback_bindings_dir())
        self.assertIn("cocos2d::CCDirectorDelegate", specs)
        self.assertIn("CustomSongDelegate", specs)

    def test_unsupported_delegate_method_not_parsed(self) -> None:
        bro = """
class SampleDelegate {
    virtual std::function<void(std::pair<int, int>)> onPair(std::vector<int> const& ids) = inline;
};
"""
        with tempfile.TemporaryDirectory() as tmpdir:
            path = os.path.join(tmpdir, "Sample.bro")
            with open(path, "w", encoding="utf-8") as f:
                f.write(bro)
            parsed = parse_broma(tmpdir)
        self.assertNotIn("SampleDelegate", parsed)

    def test_parse_delegate_warns_skipped_methods(self) -> None:
        bro = """
class NestedDelegate {
    virtual std::vector<int> getValues() = inline;
    virtual void onEvent(int id) = inline;
};
"""
        with tempfile.TemporaryDirectory() as tmpdir:
            path = os.path.join(tmpdir, "Nested.bro")
            with open(path, "w", encoding="utf-8") as f:
                f.write(bro)
            import contextlib
            import io

            stderr = io.StringIO()
            with contextlib.redirect_stderr(stderr):
                parsed = parse_broma(tmpdir)
            self.assertIn("NestedDelegate", parsed)
            self.assertEqual([m.name for m in parsed["NestedDelegate"]], ["onEvent"])
            err = stderr.getvalue()
            self.assertIn("skipped Broma delegate method NestedDelegate.getValues", err)
            self.assertIn("unsupported-return", err)
