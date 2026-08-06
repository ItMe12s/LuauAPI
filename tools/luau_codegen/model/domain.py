from __future__ import annotations

from collections.abc import Iterable

from luau_codegen.parse.broma import Class, Root


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


def build_class_lookup(classes) -> dict[str, Class]:
    lookup: dict[str, Class] = {}
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


def resolve_base(lookup: dict[str, Class], base: str) -> "Class | None":
    return lookup.get(base) or lookup.get(short_name(base))


class ClassHierarchy:
    def __init__(self, classes: Iterable[Class]):
        by_name = {cls.qualified_name: cls for cls in classes}
        self._classes = list(by_name.values())
        self.lookup = build_class_lookup(self._classes)
        self._ccobject_memo: dict[str, bool] = {}
        self._depth_memo: dict[tuple[str, frozenset[str]], int] = {}

    @staticmethod
    def _is_root(cls: Class) -> bool:
        return cls.qualified_name == "cocos2d::CCObject" or cls.name == "CCObject"

    def is_ccobject_descendant(self, cls: Class | None) -> bool:
        found, _ = self._find_ccobject(cls, set())
        return found

    def _find_ccobject(self, cls: Class | None, visiting: set[str]) -> tuple[bool, bool]:
        if cls is None:
            return False, False
        key = cls.qualified_name
        if key in self._ccobject_memo:
            return self._ccobject_memo[key], False
        if self._is_root(cls):
            self._ccobject_memo[key] = True
            return True, False
        if key in visiting:
            return False, True

        visiting.add(key)
        saw_cycle = False
        for base in cls.bases:
            if short_name(base) == "CCObject":
                visiting.remove(key)
                self._ccobject_memo[key] = True
                return True, saw_cycle
            found, cyclic = self._find_ccobject(resolve_base(self.lookup, base), visiting)
            saw_cycle |= cyclic
            if found:
                visiting.remove(key)
                self._ccobject_memo[key] = True
                return True, saw_cycle
        visiting.remove(key)
        if not saw_cycle:
            self._ccobject_memo[key] = False
        return False, saw_cycle

    def depth(self, cls: Class, skipped_classes: set[str] | frozenset[str] = frozenset()) -> int:
        skipped = frozenset(skipped_classes)
        depth, _ = self._depth(cls, skipped, set())
        return depth if depth is not None else 1

    def _depth(
        self,
        cls: Class,
        skipped: frozenset[str],
        visiting: set[str],
    ) -> tuple[int | None, bool]:
        key = (cls.qualified_name, skipped)
        if key in self._depth_memo:
            return self._depth_memo[key], False
        if self._is_root(cls):
            self._depth_memo[key] = 0
            return 0, False
        if cls.qualified_name in visiting:
            return None, True

        visiting.add(cls.qualified_name)
        values: list[int] = []
        saw_cycle = False
        for base in cls.bases:
            base_cls = resolve_base(self.lookup, base)
            if (
                base_cls is None
                or base_cls.name in skipped
                or not self.is_ccobject_descendant(base_cls)
            ):
                continue
            base_depth, cyclic = self._depth(base_cls, skipped, visiting)
            saw_cycle |= cyclic
            if base_depth is not None:
                values.append(base_depth + 1)
        visiting.remove(cls.qualified_name)
        depth = max(values) if values else None
        if depth is not None or not saw_cycle:
            self._depth_memo[key] = depth if depth is not None else 1
        return (depth if depth is not None else (None if saw_cycle else 1)), saw_cycle

    def object_classes(self) -> list[Class]:
        classes = [
            cls
            for cls in self._classes
            if not is_namespace_class(cls) and self.is_ccobject_descendant(cls)
        ]
        return sorted(classes, key=lambda cls: (self.depth(cls), cls.namespace, cls.name))


def is_ccobject_descendant(cls: Class, lookup: dict[str, Class]) -> bool:
    classes = list({value.qualified_name: value for value in lookup.values()}.values())
    return ClassHierarchy(classes).is_ccobject_descendant(cls)


def object_classes(root: Root) -> list[Class]:
    return ClassHierarchy(root.classes).object_classes()


def codegen_object_map(root: Root) -> dict[str, Class]:
    classes = object_classes(root)
    return build_class_lookup(classes)


def status_for(platforms: dict[str, str]) -> str:
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
