from __future__ import annotations

import re
from typing import Dict, List, Sequence, Tuple

from luau_codegen.parse.broma import Class, Field, Method
from luau_codegen.policy.fields import bindable_field
from luau_codegen.model.domain import short_name
from luau_codegen.convert.type_map import TypeInfo, classify_arg, classify_return

_VALUE_STUB_BODY: Dict[str, str] = {
    "RGBColor": "export type RGBColor = { r: number, g: number, b: number }\n",
    "CCSize": "export type CCSize = { width: number, height: number }\n",
    "CCPoint": "export type CCPoint = { x: number, y: number }\n",
    "CCRect": "export type CCRect = { origin: CCPoint, size: CCSize }\n",
    "UIButtonConfig": (
        "export type UIButtonConfig = {\n"
        "    width: number,\n"
        "    height: number,\n"
        "    deadzone: number,\n"
        "    scale: number,\n"
        "    opacity: number,\n"
        "    radius: number,\n"
        "    modeB: boolean,\n"
        "    snap: boolean,\n"
        "    position: CCPoint,\n"
        "    oneButton: boolean,\n"
        "    player2: boolean,\n"
        "    split: boolean,\n"
        "}\n"
    ),
}

_VALUE_STUB_ORDER = ("CCPoint", "CCSize", "RGBColor", "CCRect", "UIButtonConfig")

_VALUE_STUB_DEPS: Dict[str, Tuple[str, ...]] = {
    "CCRect": ("CCPoint", "CCSize"),
    "UIButtonConfig": ("CCPoint",),
}


def _expand_value_refs(names: set[str]) -> set[str]:
    out = set(names)
    for name in names:
        out.update(_VALUE_STUB_DEPS.get(name, ()))
    return {n for n in out if n in _VALUE_STUB_BODY}


def _value_refs_in_text(text: str) -> set[str]:
    return {name for name in _VALUE_STUB_BODY if re.search(rf"\b{name}\b", text)}


def _emit_value_stub_block(names: set[str]) -> str:
    expanded = _expand_value_refs(names)
    if not expanded:
        return ""
    return "".join(_VALUE_STUB_BODY[n] for n in _VALUE_STUB_ORDER if n in expanded)


def _object_type_name(info: TypeInfo) -> str:
    if info.class_name:
        return info.class_name
    return info.lua_type.removesuffix("?")


def _refs_from_method(method: Method, objects: Dict[str, Class]) -> set[str]:
    refs: set[str] = set()
    for arg in method.args:
        info = classify_arg(arg.type, objects)
        if info and info.kind == "object":
            refs.add(_object_type_name(info))
        elif info and info.kind == "vector_view" and info.element_type:
            refs.add(_object_type_name(info.element_type))
    ret = classify_return(method.ret, objects)
    if ret and ret.kind == "object":
        refs.add(_object_type_name(ret))
    elif ret and ret.kind == "vector_view" and ret.element_type:
        refs.add(_object_type_name(ret.element_type))
    return refs


def _refs_from_fields(
    cls: Class, fields: Sequence[Field], objects: Dict[str, Class]
) -> set[str]:
    refs: set[str] = set()
    for field in fields:
        ok, _, _, ret = bindable_field(field, objects, cls)
        if ok and ret and ret.kind == "object":
            refs.add(_object_type_name(ret))
        elif ok and ret and ret.kind == "vector_view" and ret.element_type:
            refs.add(_object_type_name(ret.element_type))
    return refs


def _base_type_refs(
    cls: Class, objects: Dict[str, Class], skipped_classes: set
) -> set[str]:
    refs: set[str] = set()
    for base in cls.bases:
        base_cls = objects.get(short_name(base))
        if base_cls and base_cls.name not in skipped_classes:
            refs.add(base_cls.name)
    return refs


def _refs_from_classes(
    class_names: set[str],
    grouped_by_class: Dict[str, Dict[str, List[Method]]],
    objects: Dict[str, Class],
    skipped_classes: set,
) -> set[str]:
    refs: set[str] = set()
    for name in class_names:
        cls = objects.get(name)
        if not cls or name in skipped_classes:
            continue
        refs.update(_base_type_refs(cls, objects, skipped_classes))
        refs.update(_refs_from_fields(cls, cls.fields, objects))
        for methods in grouped_by_class.get(name, {}).values():
            for method in methods:
                refs.update(_refs_from_method(method, objects))
    return refs


def _factory_object_refs(
    factories: Dict[str, Dict[str, List[Method]]],
    objects: Dict[str, Class],
) -> set[str]:
    refs: set[str] = set()
    for methods in factories.values():
        for overloads in methods.values():
            for method in overloads:
                refs.update(_refs_from_method(method, objects))
    return refs


def _refs_from_text(content: str) -> set[str]:
    refs: set[str] = set()
    for match in re.finditer(r"extends (\w+)", content):
        refs.add(match.group(1))
    for match in re.finditer(
        r":\s*(\w+)\??(?:\s*[,&)]|\s*$|\s*->)", content, re.MULTILINE
    ):
        name = match.group(1)
        if name[0].isupper():
            refs.add(name)
    return refs


def _emit_orphan_stubs(names: set[str]) -> str:
    if not names:
        return ""
    lines = [
        "-- Forward declarations for referenced classes without bindable members\n",
        "-- @type-only: opaque handles, no members bound on this platform\n",
        "\n",
    ]
    for name in sorted(names):
        lines.append("--- @type-only\n")
        lines.append(f"declare class {name} end\n\n")
    return "".join(lines)
