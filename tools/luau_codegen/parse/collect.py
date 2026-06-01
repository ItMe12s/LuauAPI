from __future__ import annotations

import os
import warnings

from luau_codegen.parse import broma
from luau_codegen.model.domain import BRO_FILES, object_classes


def collect_bindings_root(
    bindings_dir: str, geode_sdk_path: str | None = None
) -> broma.Root:
    root = broma.Root()
    for name in BRO_FILES:
        path = os.path.join(bindings_dir, name)
        if not os.path.exists(path):
            warnings.warn(f"[luauapi] missing Broma file: {path}")
            continue
        parsed = broma.parse_file(path)
        root.classes.extend(parsed.classes)
    geode_enums: dict[str, str] | None = None
    if geode_sdk_path:
        from luau_codegen.parse.geode_sdk import (
            scan_geode_sdk,
            scan_geode_enums,
            scan_geode_functions,
        )

        geode_enums = scan_geode_enums(geode_sdk_path)
        root.classes.extend(scan_geode_sdk(geode_sdk_path))
        root.functions.extend(scan_geode_functions(geode_sdk_path))
    from luau_codegen.policy.filtering import method_key

    seen: dict[str, broma.Class] = {}
    for cls in root.classes:
        if cls.qualified_name in seen:
            existing = seen[cls.qualified_name]
            for attr in cls.attributes:
                if attr not in existing.attributes:
                    existing.attributes.append(attr)
            if cls.methods:
                if not existing.methods:
                    existing.methods = list(cls.methods)
                else:
                    known = {method_key(existing, m) for m in existing.methods}
                    for method in cls.methods:
                        key = method_key(cls, method)
                        if key not in known:
                            existing.methods.append(method)
                            known.add(key)
        else:
            seen[cls.qualified_name] = cls
    root.classes = list(seen.values())
    root.classes.sort(key=lambda c: (c.namespace, c.name))
    if geode_enums is not None:
        from luau_codegen.convert.type_map import (
            COCOS_ENUM_TYPES,
            GD_ENUM_TYPES,
            register_geode_enums,
        )

        skip = GD_ENUM_TYPES | COCOS_ENUM_TYPES | {c.name for c in object_classes(root)}
        register_geode_enums(geode_enums, skip=skip)
    return root
