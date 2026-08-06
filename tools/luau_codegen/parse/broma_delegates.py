from __future__ import annotations

import re
import sys
from pathlib import Path
from typing import TYPE_CHECKING

from luau_codegen.convert.type_primitives import normalize_type
from luau_codegen.parse.broma import Root, parse_text
from luau_codegen.parse.cpp_scan import strip_comments

if TYPE_CHECKING:
    from luau_codegen.model.codegen_context import CodegenContext

_DELEGATE_SUFFIXES = ("Delegate", "Protocol")


def _load_bindings(bindings_dir: Path | str) -> list[tuple[Root, str]]:
    loaded: list[tuple[Root, str]] = []
    for bro in Path(bindings_dir).glob("*.bro"):
        text = bro.read_text(encoding="utf-8", errors="replace")
        loaded.append((parse_text(text, str(bro)), text))
    return loaded


def _qualified_name(namespace: str, name: str) -> str:
    return f"{namespace}::{name}" if namespace else name


def _delegate_ptr_base(type_text: str, norm) -> str | None:
    n = norm(type_text)
    if not n.endswith("*"):
        return None
    base = n[:-1].strip()
    if base.endswith(_DELEGATE_SUFFIXES):
        return base
    return None


def _delegate_bindability(
    method,
    ctx: CodegenContext,
    norm=normalize_type,
) -> tuple[bool, str | None]:
    from luau_codegen.emit.delegates import lua_for

    if not method.is_virtual or method.is_ctor or method.is_dtor or method.name.startswith("~"):
        return False, None
    if lua_for(method.ret, ctx) is None:
        return False, f"unsupported-return:{method.ret}"
    for arg in method.args:
        if lua_for(normalize_type(arg.type), ctx) is None:
            return False, f"unsupported-arg:{arg.type}"
    return True, None


def parse_delegate_classes(
    bindings_dir: Path | str,
    *,
    norm,
    delegate_method_cls,
    ctx: CodegenContext,
    warn_skipped: bool = True,
    loaded: list[tuple[Root, str]] | None = None,
) -> dict[str, list]:
    out: dict[str, list] = {}
    for root, _ in loaded if loaded is not None else _load_bindings(bindings_dir):
        for cls in root.classes:
            qname = _qualified_name(cls.namespace, cls.name)
            if not qname.endswith(_DELEGATE_SUFFIXES):
                continue
            methods = []
            for method in cls.methods:
                ok, reason = _delegate_bindability(method, ctx, norm)
                if not ok:
                    if warn_skipped and reason:
                        print(
                            f"warning: skipped Broma delegate method {qname}.{method.name}: {reason}",
                            file=sys.stderr,
                        )
                    continue
                args = [
                    (normalize_type(a.type), a.name or f"arg{i}") for i, a in enumerate(method.args)
                ]
                methods.append(delegate_method_cls(method.name, method.ret, args))
            if methods:
                out[qname] = methods
    return out


def collect_delegate_ptrs(
    bindings_dir: Path | str,
    *,
    norm,
    loaded: list[tuple[Root, str]] | None = None,
) -> set[str]:
    ptrs: set[str] = set()
    for root, raw_text in loaded if loaded is not None else _load_bindings(bindings_dir):
        for cls in root.classes:
            qname = _qualified_name(cls.namespace, cls.name)
            if qname.endswith(_DELEGATE_SUFFIXES):
                ptrs.add(qname)
            for method in cls.methods:
                for type_text in [method.ret, *(a.type for a in method.args)]:
                    base = _delegate_ptr_base(type_text, norm)
                    if base:
                        ptrs.add(base)
        text = strip_comments(raw_text)
        for match in re.finditer(r"([\w:]+(?:Delegate|Protocol))\*", text):
            ptrs.add(match.group(1))
    return ptrs


def scan_delegate_bindings(
    bindings_dir: Path | str,
    *,
    norm,
    delegate_method_cls,
    ctx: CodegenContext,
) -> tuple[dict[str, list], set[str]]:
    loaded = _load_bindings(bindings_dir)
    return (
        parse_delegate_classes(
            bindings_dir,
            norm=norm,
            delegate_method_cls=delegate_method_cls,
            ctx=ctx,
            loaded=loaded,
        ),
        collect_delegate_ptrs(bindings_dir, norm=norm, loaded=loaded),
    )
