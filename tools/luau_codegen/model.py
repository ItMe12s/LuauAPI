from __future__ import annotations

from dataclasses import dataclass, field
from typing import Dict, List

from broma_parser import Class, Method, Root


BRO_FILES = ("Cocos2d.bro", "Extras.bro", "FMOD.bro", "Kazmath.bro", "GeometryDash.bro")
DENY_CLASSES = {"cocos2d", "pugi"}


@dataclass
class MethodDecision:
    method: Method
    supported: bool
    reason: str = ""


@dataclass
class ClassModel:
    source: Class
    cxx_name: str
    lua_name: str
    namespace: str
    base_names: List[str] = field(default_factory=list)
    methods: List[MethodDecision] = field(default_factory=list)


def collect_classes(root: Root) -> Dict[str, Class]:
    out: Dict[str, Class] = {}
    for cls in root.classes:
        out[cls.qualified_name] = cls
        out.setdefault(cls.name, cls)
    return out


def short_name(name: str) -> str:
    return name.strip().lstrip(":").split("::")[-1]


def cxx_name(cls: Class) -> str:
    return f"{cls.namespace}::{cls.name}" if cls.namespace else cls.name


def lua_namespace(cls: Class) -> str:
    if cls.namespace == "cocos2d" or cls.name.startswith("CC"):
        return "geode.cocos2d"
    return "geode.gd"


def is_namespace_class(cls: Class) -> bool:
    return cls.name in DENY_CLASSES


def object_classes(root: Root) -> List[Class]:
    by_short = {cls.name: cls for cls in root.classes}
    memo: Dict[str, bool] = {}

    def is_object(cls: Class) -> bool:
        if cls.name in memo:
            return memo[cls.name]
        if cls.qualified_name == "cocos2d::CCObject" or cls.name == "CCObject":
            memo[cls.name] = True
            return True
        for base in cls.bases:
            base_short = short_name(base)
            if base_short == "CCObject":
                memo[cls.name] = True
                return True
            base_cls = by_short.get(base_short)
            if base_cls and is_object(base_cls):
                memo[cls.name] = True
                return True
        memo[cls.name] = False
        return False

    classes = [
        cls
        for cls in root.classes
        if not is_namespace_class(cls) and is_object(cls)
    ]

    def depth(cls: Class) -> int:
        if cls.name == "CCObject":
            return 0
        values = []
        for base in cls.bases:
            base_cls = by_short.get(short_name(base))
            if base_cls and base_cls is not cls:
                values.append(depth(base_cls) + 1)
        return max(values) if values else 1

    return sorted(classes, key=lambda c: (depth(c), c.namespace, c.name))


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
