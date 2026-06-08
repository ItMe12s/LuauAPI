from __future__ import annotations

import re
from pathlib import Path

from luau_codegen.parse.broma import parse_file
from luau_codegen.parse.text import strip_comments

_DELEGATE_SUFFIXES = ("Delegate", "Protocol")


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


def _method_to_delegate(method, norm) -> tuple[str, str, list[tuple[str, str]]] | None:
    from luau_codegen.emit.delegates import DelegateMethod, lua_for

    if not method.is_virtual:
        return None
    if lua_for(method.ret) is None:
        return None
    args = [(norm(a.type), a.name or "arg") for a in method.args]
    if any(lua_for(t) is None for t, _ in args):
        return None
    return method.name, method.ret, args


def parse_delegate_classes(
    bindings_dir: Path | str,
    *,
    norm,
    delegate_method_cls,
) -> dict[str, list]:
    out: dict[str, list] = {}
    root_dir = Path(bindings_dir)
    for bro in root_dir.glob("*.bro"):
        root = parse_file(str(bro))
        for cls in root.classes:
            qname = _qualified_name(cls.namespace, cls.name)
            if not qname.endswith(_DELEGATE_SUFFIXES):
                continue
            methods = []
            for method in cls.methods:
                parsed = _method_to_delegate(method, norm)
                if parsed is None:
                    continue
                name, ret, args = parsed
                methods.append(delegate_method_cls(name, ret, args))
            if methods:
                out[qname] = methods
    return out


def collect_delegate_ptrs(bindings_dir: Path | str, *, norm) -> set[str]:
    ptrs: set[str] = set()
    root_dir = Path(bindings_dir)
    for bro in root_dir.glob("*.bro"):
        root = parse_file(str(bro))
        for cls in root.classes:
            qname = _qualified_name(cls.namespace, cls.name)
            if qname.endswith(_DELEGATE_SUFFIXES):
                ptrs.add(qname)
            for method in cls.methods:
                for type_text in [method.ret, *(a.type for a in method.args)]:
                    base = _delegate_ptr_base(type_text, norm)
                    if base:
                        ptrs.add(base)
        text = strip_comments(bro.read_text(encoding="utf-8", errors="replace"))
        for match in re.finditer(r"([\w:]+(?:Delegate|Protocol))\*", text):
            ptrs.add(match.group(1))
    return ptrs
