from __future__ import annotations

import re
from typing import TYPE_CHECKING, Dict, List, Sequence, Tuple

if TYPE_CHECKING:
    from luau_codegen.model.codegen_context import CodegenContext

from luau_codegen.parse.broma import Class, Field, Method
from luau_codegen.policy.fields import bindable_field
from luau_codegen.model.domain import short_name
from luau_codegen.convert.type_map import TypeInfo, classify_arg, classify_return
from luau_codegen.model import delegate_specs as _delegate_specs
from luau_codegen.model.value_types import (
    VALUE_STUB_BODY as _VALUE_STUB_BODY,
    VALUE_STUB_DEPS as _VALUE_STUB_DEPS,
    VALUE_STUB_ORDER as _VALUE_STUB_ORDER,
)

_OPAQUE_STUB_BODY: Dict[str, str] = {
    "FMODChannel": "--- @type-only: opaque FMOD handle\ndeclare class FMODChannel end\n\n",
    "FMODSoundHandle": (
        "--- @type-only: opaque FMOD handle\ndeclare class FMODSoundHandle end\n\n"
    ),
    "FMODSystem": "--- @type-only: opaque FMOD handle\ndeclare class FMODSystem end\n\n",
    "FMODDSP": "--- @type-only: opaque FMOD handle\ndeclare class FMODDSP end\n\n",
    "FMODChannelGroup": (
        "--- @type-only: opaque FMOD handle\ndeclare class FMODChannelGroup end\n\n"
    ),
    "CCEvent": "--- @type-only: opaque cocos2d handle\ndeclare class CCEvent end\n\n",
    "CCEditBox": ("--- @type-only: opaque cocos2d handle\ndeclare class CCEditBox end\n\n"),
    "GroupCommandObject2": (
        "--- @type-only: non-CCObject GD type\ndeclare class GroupCommandObject2 end\n\n"
    ),
    "DelayedSpawnNode": (
        "--- @type-only: non-CCObject GD type\ndeclare class DelayedSpawnNode end\n\n"
    ),
}

_OPAQUE_STUB_ORDER = (
    "FMODChannel",
    "FMODSoundHandle",
    "FMODSystem",
    "FMODDSP",
    "FMODChannelGroup",
    "CCEvent",
    "CCEditBox",
    "GroupCommandObject2",
    "DelayedSpawnNode",
)
_TYPE_STUB_ORDER = _VALUE_STUB_ORDER + _OPAQUE_STUB_ORDER


def _expand_value_refs(names: set[str]) -> set[str]:
    out: set[str] = set(names)
    queue = list(names)
    while queue:
        name = queue.pop()
        for dep in _VALUE_STUB_DEPS.get(name, ()):
            if dep not in out:
                out.add(dep)
                queue.append(dep)
    return {n for n in out if n in _VALUE_STUB_BODY}


def _value_type_object_refs(info: TypeInfo) -> set[str]:
    if info.kind != "value":
        return set()
    return set(_VALUE_STUB_DEPS.get(info.lua_type, ()))


def _all_type_stub_bodies() -> Dict[str, str]:
    return {**_VALUE_STUB_BODY, **_OPAQUE_STUB_BODY}


def _value_refs_in_text(text: str) -> set[str]:
    bodies = _all_type_stub_bodies()
    return {name for name in bodies if re.search(rf"\b{name}\b", text)}


def _emit_value_stub_block(names: set[str]) -> str:
    expanded = _expand_value_refs(names)
    expanded |= {n for n in names if n in _OPAQUE_STUB_BODY}
    if not expanded:
        return ""
    bodies = _all_type_stub_bodies()

    order = _VALUE_STUB_ORDER + _OPAQUE_STUB_ORDER

    ordered = list(order) + sorted(expanded - set(order))
    parts: list[str] = []
    prev_was_value = False
    for name in ordered:
        if name not in expanded:
            continue
        is_opaque = name in _OPAQUE_STUB_BODY
        if is_opaque and prev_was_value:
            parts.append("\n")
        parts.append(bodies[name])
        prev_was_value = name in _VALUE_STUB_BODY
    return "".join(parts)


def _object_type_name(info: TypeInfo) -> str:
    if info.class_name:
        return info.class_name
    return info.lua_type.removesuffix("?")


def _emit_delegate_stub_block() -> str:
    lines = ["-- Delegate table types\n\n"]
    seen: set[str] = set()
    for spec in _delegate_specs.DELEGATE_SPECS.values():
        if spec.lua_name in seen:
            continue
        seen.add(spec.lua_name)
        fields = []
        for m in spec.methods:
            params = ", ".join(f"arg{i}: {t}" for i, t in enumerate(m.args_lua, start=1))
            ret = m.ret_lua
            fn = f"({params}) -> ()" if ret == "()" else f"({params}) -> {ret}"
            fields.append(f"    {m.name}: ({fn})?")
        body = ",\n".join(fields)
        lines.append(f"export type {spec.lua_name} = {{\n{body}\n}}\n\n")
    return "".join(lines)


def _refs_from_method(
    method: Method, objects: Dict[str, Class], ctx: CodegenContext | None = None
) -> set[str]:
    refs: set[str] = set()
    for arg in method.args:
        info = classify_arg(arg.type, objects, ctx=ctx)
        if info and info.kind == "value":
            refs.add(info.lua_type)
            refs.update(_value_type_object_refs(info))
        elif info and info.kind == "object":
            refs.add(_object_type_name(info))
        elif info and info.kind == "vector_view" and info.element_type:
            refs.add(_object_type_name(info.element_type))
        elif info and info.kind in ("map", "unordered_map") and info.value_type:
            if info.value_type.kind == "object":
                refs.add(_object_type_name(info.value_type))
            elif (
                info.value_type.kind == "vector_view"
                and info.value_type.element_type
                and info.value_type.element_type.kind == "object"
            ):
                refs.add(_object_type_name(info.value_type.element_type))
            else:
                refs.update(_value_type_object_refs(info.value_type))
        elif info and info.kind == "primitive_vector" and info.element_type:
            refs.update(_value_type_object_refs(info.element_type))
        elif info and info.kind == "std_array" and info.element_type:
            refs.update(_value_type_object_refs(info.element_type))
        elif info and info.kind in ("set", "unordered_set") and info.element_type:
            if info.element_type.kind == "object":
                refs.add(_object_type_name(info.element_type))
            else:
                refs.update(_value_type_object_refs(info.element_type))
    ret = classify_return(method.ret, objects, ctx=ctx)
    if ret and ret.kind == "value":
        refs.add(ret.lua_type)
        refs.update(_value_type_object_refs(ret))
    elif ret and ret.kind == "object":
        refs.add(_object_type_name(ret))
    elif ret and ret.kind == "vector_view" and ret.element_type:
        refs.add(_object_type_name(ret.element_type))
    elif ret and ret.kind in ("map", "unordered_map") and ret.value_type:
        if ret.value_type.kind == "object":
            refs.add(_object_type_name(ret.value_type))
        elif (
            ret.value_type.kind == "vector_view"
            and ret.value_type.element_type
            and ret.value_type.element_type.kind == "object"
        ):
            refs.add(_object_type_name(ret.value_type.element_type))
        else:
            refs.update(_value_type_object_refs(ret.value_type))
    elif ret and ret.kind == "primitive_vector" and ret.element_type:
        refs.update(_value_type_object_refs(ret.element_type))
    elif ret and ret.kind == "std_array" and ret.element_type:
        refs.update(_value_type_object_refs(ret.element_type))
    elif ret and ret.kind in ("set", "unordered_set") and ret.element_type:
        if ret.element_type.kind == "object":
            refs.add(_object_type_name(ret.element_type))
        else:
            refs.update(_value_type_object_refs(ret.element_type))
    return refs


def _refs_from_fields(
    cls: Class,
    fields: Sequence[Field],
    objects: Dict[str, Class],
    ctx: CodegenContext | None = None,
) -> set[str]:
    refs: set[str] = set()
    for field in fields:
        ok, _, _, ret = bindable_field(field, objects, cls, ctx=ctx)
        if ok and ret and ret.kind == "value":
            refs.add(ret.lua_type)
            refs.update(_value_type_object_refs(ret))
        elif ok and ret and ret.kind == "object":
            refs.add(_object_type_name(ret))
        elif ok and ret and ret.kind in ("vector_view", "cc_c_array_view") and ret.element_type:
            refs.add(_object_type_name(ret.element_type))
        elif ok and ret and ret.kind in ("map", "unordered_map") and ret.value_type:
            if ret.value_type.kind == "object":
                refs.add(_object_type_name(ret.value_type))
            elif (
                ret.value_type.kind == "vector_view"
                and ret.value_type.element_type
                and ret.value_type.element_type.kind == "object"
            ):
                refs.add(_object_type_name(ret.value_type.element_type))
            else:
                refs.update(_value_type_object_refs(ret.value_type))
        elif ok and ret and ret.kind == "primitive_vector" and ret.element_type:
            refs.update(_value_type_object_refs(ret.element_type))
        elif ok and ret and ret.kind == "std_array" and ret.element_type:
            refs.update(_value_type_object_refs(ret.element_type))
        elif ok and ret and ret.kind in ("set", "unordered_set") and ret.element_type:
            if ret.element_type.kind == "object":
                refs.add(_object_type_name(ret.element_type))
            else:
                refs.update(_value_type_object_refs(ret.element_type))
    return refs


def _base_type_refs(cls: Class, objects: Dict[str, Class], skipped_classes: set) -> set[str]:
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
    ctx: CodegenContext | None = None,
) -> set[str]:
    refs: set[str] = set()
    for name in class_names:
        cls = objects.get(name)
        if not cls or name in skipped_classes:
            continue
        refs.update(_base_type_refs(cls, objects, skipped_classes))
        refs.update(_refs_from_fields(cls, cls.fields, objects, ctx=ctx))
        for methods in grouped_by_class.get(name, {}).values():
            for method in methods:
                refs.update(_refs_from_method(method, objects, ctx=ctx))
    return refs


def _factory_object_refs(
    factories: Dict[str, Dict[str, List[Method]]],
    objects: Dict[str, Class],
    ctx: CodegenContext | None = None,
) -> set[str]:
    refs: set[str] = set()
    for methods in factories.values():
        for overloads in methods.values():
            for method in overloads:
                refs.update(_refs_from_method(method, objects, ctx=ctx))
    return refs


def _refs_from_text(content: str) -> set[str]:
    refs: set[str] = set()
    for match in re.finditer(r"extends (\w+)", content):
        refs.add(match.group(1))
    for match in re.finditer(r":\s*(\w+)\??(?:\s*[,&)]|\s*$|\s*->)", content, re.MULTILINE):
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
