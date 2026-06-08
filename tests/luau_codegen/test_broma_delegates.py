from __future__ import annotations

import os
import tempfile
import unittest

from luau_codegen.emit.delegates import (  # type: ignore[import-unresolved]
    norm,
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
            ptrs = collect_delegate_ptrs(tmpdir, norm=norm)
        self.assertIn("CustomSongDelegate", ptrs)

    def test_collect_matches_fixture_delegate_count(self) -> None:
        from luau_codegen.emit.delegates import (  # type: ignore[import-unresolved]
            collect as collect_delegate_specs,
            fallback_bindings_dir,
        )

        specs = collect_delegate_specs(fallback_bindings_dir())
        self.assertIn("cocos2d::CCDirectorDelegate", specs)
        self.assertIn("CustomSongDelegate", specs)
