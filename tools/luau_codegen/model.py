from __future__ import annotations

from typing import Dict, List

from broma_parser import Class, Root


BRO_FILES = ("Cocos2d.bro", "Extras.bro", "FMOD.bro", "Kazmath.bro", "GeometryDash.bro")
DENY_CLASSES = {"cocos2d", "pugi"}


def short_name(name: str) -> str:
    return name.strip().lstrip(":").split("::")[-1]


def cxx_name(cls: Class) -> str:
    return f"{cls.namespace}::{cls.name}" if cls.namespace else cls.name


def lua_namespace(cls: Class) -> str:
    if cls.namespace == "cocos2d" or cls.name.startswith("CC"):
        return "geode.cocos2d"
    if cls.namespace == "geode":
        return "geode"
    return "geode.gd"


def is_namespace_class(cls: Class) -> bool:
    return cls.name in DENY_CLASSES


def build_class_lookup(classes) -> Dict[str, Class]:
    lookup: Dict[str, Class] = {}
    ambiguous: set[str] = set()
    for cls in classes:
        lookup[cls.qualified_name] = cls
        if cls.name in ambiguous:
            continue
        if cls.name in lookup and lookup[cls.name].qualified_name != cls.qualified_name:
            ambiguous.add(cls.name)
            del lookup[cls.name]
        else:
            lookup[cls.name] = cls
    return lookup


def resolve_base(lookup: Dict[str, Class], base: str) -> "Class | None":
    return lookup.get(base) or lookup.get(short_name(base))


def object_classes(root: Root) -> List[Class]:
    lookup = build_class_lookup(root.classes)
    memo: Dict[str, bool] = {}

    def is_object(cls: Class) -> bool:
        key = cls.qualified_name
        if key in memo:
            return memo[key]
        if cls.qualified_name == "cocos2d::CCObject" or cls.name == "CCObject":
            memo[key] = True
            return True
        for base in cls.bases:
            base_short = short_name(base)
            if base_short == "CCObject":
                memo[key] = True
                return True
            base_cls = resolve_base(lookup, base)
            if base_cls and is_object(base_cls):
                memo[key] = True
                return True
        memo[key] = False
        return False

    classes = [
        cls for cls in root.classes if not is_namespace_class(cls) and is_object(cls)
    ]

    def depth(cls: Class) -> int:
        if cls.name == "CCObject":
            return 0
        values = []
        for base in cls.bases:
            base_cls = resolve_base(lookup, base)
            if base_cls and base_cls is not cls:
                values.append(depth(base_cls) + 1)
        return max(values) if values else 1

    return sorted(classes, key=lambda c: (depth(c), c.namespace, c.name))


def codegen_object_map(root: Root) -> Dict[str, Class]:
    classes = object_classes(root)
    return build_class_lookup(classes)


def status_for(platforms: Dict[str, str]) -> str:
    if not platforms:
        return "Missing"
    if any(v.startswith("inline") for v in platforms.values()):
        return "Inlined"
    vals = {v.split()[0] if v else "" for v in platforms.values()}
    if vals and all(v == "link" for v in vals):
        return "Linked"
    if vals and all(v in ("rebind", "Rebinded") for v in vals):
        return "Rebinded"
    return "Bindable"
