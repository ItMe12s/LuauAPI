from __future__ import annotations

from typing import TYPE_CHECKING, Dict, List, Tuple

from luau_codegen.parse.broma import Class, Field, Method

if TYPE_CHECKING:
    from luau_codegen.model.codegen_context import CodegenContext

from luau_codegen.policy.fields import bindable_field, field_applies_on_platform
from luau_codegen.model.domain import short_name

from luau_codegen.emit.luau_types.method_types import (
    LUAU_KEYWORDS,
    _classify_input_args,
    _method_return_type,
    _widened_method_type,
)


def _should_emit_type_class(
    cls: Class,
    objects: Dict[str, Class],
    field_targets: list[tuple[Class, Field]],
    skipped_classes: set,
    target_platform: str = "win",
    ctx: CodegenContext | None = None,
) -> bool:
    if cls.name not in skipped_classes:
        return True
    bound = {field.name for _, field in field_targets}
    for field in cls.fields:
        if not field_applies_on_platform(field, target_platform):
            continue
        ok, reason, _, ret = bindable_field(field, objects, cls, ctx=ctx)
        if ok and field.name in bound and ret:
            return True
        if reason:
            return True
    return False


def _emitted_base_name(
    cls: Class, objects: Dict[str, Class], skipped_classes: set
) -> str | None:
    for b in cls.bases:
        b_cls = objects.get(short_name(b))
        if b_cls and b_cls.name not in skipped_classes:
            return b_cls.name
    return None


def _emit_class(
    cls: Class,
    grouped: Dict[str, List[Method]],
    field_targets: list[tuple[Class, Field]],
    objects: Dict[str, Class],
    skipped_classes: set,
    target_platform: str = "win",
    ctx: CodegenContext | None = None,
) -> List[str]:
    lines: List[str] = []
    base_name = _emitted_base_name(cls, objects, skipped_classes)
    base = f" extends {base_name}" if base_name else ""
    instance_methods = {
        k: v
        for k, v in grouped.items()
        if not v[0].is_static and k not in LUAU_KEYWORDS
    }
    bound_field_names = {field.name for _, field in field_targets}
    field_lines: List[str] = []
    if _is_ccnode_descendant(cls, objects, skipped_classes):
        field_lines.append("    m_fields: { [string]: any }\n")
    for field in cls.fields:
        if not field_applies_on_platform(field, target_platform):
            continue
        ok, reason, _, ret = bindable_field(field, objects, cls, ctx=ctx)
        if ok and field.name in bound_field_names and ret:
            field_lines.append(f"    {field.name}: {ret.lua_type}\n")
        elif reason:
            field_lines.append(f"    -- skipped {field.name}: {reason}\n")

    if not instance_methods and not field_lines:
        lines.append(f"declare class {cls.name}{base} end\n\n")
    else:
        lines.append(f"declare class {cls.name}{base}\n")
        lines.extend(field_lines)
        for name, methods in sorted(instance_methods.items()):
            if len(methods) == 1:
                m = methods[0]
                args = _classify_input_args(cls, m, objects, ctx=ctx)
                ret_type = _method_return_type(cls, m, objects, ctx=ctx)
                arg_text = ", ".join(
                    f"arg{i}: {arg.lua_type}" for i, arg in enumerate(args, start=1)
                )
                joiner = ", " if arg_text else ""
                self_prefix = "" if m.is_static else f"self{joiner}"
                if m.is_static:
                    lines.append(
                        f"    function {name}({arg_text}){': ' + ret_type if ret_type != '()' else ''}\n"
                    )
                else:
                    lines.append(
                        f"    function {name}({self_prefix}{arg_text}){': ' + ret_type if ret_type != '()' else ''}\n"
                    )
            else:
                widened = _widened_method_type(
                    cls, methods, objects, static=False, ctx=ctx
                )
                lines.append(f"    {name}: {widened}\n")
        lines.append("end\n\n")
    return lines


def _is_ccnode_descendant(
    cls: Class,
    objects: Dict[str, Class],
    skipped_classes: set,
    seen: set[str] | None = None,
) -> bool:
    if cls.name == "CCNode":
        return True
    if cls.name in skipped_classes:
        return False
    seen = seen or set()
    if cls.name in seen:
        return False
    seen.add(cls.name)
    for base in cls.bases:
        base_cls = objects.get(short_name(base))
        if base_cls and _is_ccnode_descendant(base_cls, objects, skipped_classes, seen):
            return True
    return False


def _topo_sort_chunks(
    chunks: List[Tuple[str, str]], base_of: Dict[str, str | None]
) -> List[Tuple[str, str]]:
    chunk_map = dict(chunks)
    present = set(chunk_map)
    placed: set[str] = set()
    ordered: List[str] = []

    def visit(name: str, stack: frozenset[str]) -> None:
        if name in placed:
            return
        base = base_of.get(name)
        if base in present and base not in placed and name not in stack:
            visit(base, stack | {name})
        placed.add(name)
        ordered.append(name)

    for name in sorted(chunk_map):
        visit(name, frozenset())
    return [(n, chunk_map[n]) for n in ordered]
