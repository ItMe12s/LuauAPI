from __future__ import annotations

import json

from conftest import *

from luau_codegen.model.denylist import (  # type: ignore[import-unresolved]
    BINDABLE_CONSTRUCTORS,
    INACCESSIBLE_CLASSES,
    INACCESSIBLE_METHODS,
    PREFERRED_OVERLOADS,
)
from luau_codegen.convert.type_map import normalize_type  # type: ignore[import-unresolved]


def _bindings_dir() -> str | None:
    env = os.environ.get("LUAUAPI_BINDINGS_DIR")
    if env and os.path.isfile(os.path.join(env, "GeometryDash.bro")):
        return env

    version = "2.2081"
    mod_json = os.path.join(ROOT, "mod.json")
    try:
        with open(mod_json, encoding="utf-8") as f:
            version = json.load(f).get("gd", {}).get("win", version)
    except (OSError, ValueError):
        pass

    candidate = os.path.join(
        ROOT, "build", "_deps", "bindings-src", "bindings", version
    )
    if os.path.isfile(os.path.join(candidate, "GeometryDash.bro")):
        return candidate
    return None


def _load_root():
    bindings = _bindings_dir()
    if bindings is None:
        return None
    sdk = os.environ.get("GEODE_SDK")
    sdk = sdk if sdk and os.path.isdir(sdk) else None
    with warnings.catch_warnings():
        warnings.simplefilter("ignore")
        return collect_bindings_root(bindings, geode_sdk_path=sdk)


class DenylistStaleEntryTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.root = _load_root()
        if cls.root is None:
            raise unittest.SkipTest(
                "Broma bindings not found, set LUAUAPI_BINDINGS_DIR or build once"
            )
        cls.methods_by_class: dict[str, set[str]] = {}
        for c in cls.root.classes:
            cls.methods_by_class.setdefault(c.name, set()).update(
                m.name for m in c.methods
            )
        cls.overloads: dict[tuple[str, str], list[tuple[str, ...]]] = {}
        for c in cls.root.classes:
            for m in c.methods:
                sig = tuple(normalize_type(a.type) for a in m.args)
                cls.overloads.setdefault((c.name, m.name), []).append(sig)

    def test_inaccessible_classes_exist(self) -> None:
        stale = sorted(
            name for name in INACCESSIBLE_CLASSES if name not in self.methods_by_class
        )
        self.assertEqual(stale, [], f"stale INACCESSIBLE_CLASSES: {stale}")

    def test_inaccessible_methods_exist(self) -> None:
        stale = []
        for cls_name, method in sorted(INACCESSIBLE_METHODS):
            methods = self.methods_by_class.get(cls_name)
            if methods is None or method not in methods:
                stale.append((cls_name, method))
        self.assertEqual(stale, [], f"stale INACCESSIBLE_METHODS: {stale}")

    def test_preferred_overload_keys_exist(self) -> None:
        stale = []
        for cls_name, method in sorted(PREFERRED_OVERLOADS):
            methods = self.methods_by_class.get(cls_name)
            if methods is None or method not in methods:
                stale.append((cls_name, method))
        self.assertEqual(stale, [], f"stale PREFERRED_OVERLOADS keys: {stale}")

    def test_preferred_overload_signatures_match(self) -> None:
        mismatched = []
        for (cls_name, method), sigs in sorted(PREFERRED_OVERLOADS.items()):
            parsed = self.overloads.get((cls_name, method), [])
            if not any(sig in parsed for sig in sigs):
                mismatched.append((cls_name, method))
        self.assertEqual(
            mismatched,
            [],
            f"PREFERRED_OVERLOADS signatures match nothing: {mismatched}",
        )

    def test_bindable_constructor_keys_exist(self) -> None:
        stale = sorted(
            name for name in BINDABLE_CONSTRUCTORS if name not in self.methods_by_class
        )
        self.assertEqual(stale, [], f"stale BINDABLE_CONSTRUCTORS: {stale}")


if __name__ == "__main__":
    unittest.main()
