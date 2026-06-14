from __future__ import annotations

import logging
import os
import re
from typing import List

from luau_codegen.model.free_fn_sources import FREE_FUNCTION_SOURCES

_log = logging.getLogger("luau_codegen.geode_sdk")

_SCAN_WARNINGS: List[str] = []


def take_scan_warnings() -> List[str]:
    global _SCAN_WARNINGS
    warnings, _SCAN_WARNINGS = _SCAN_WARNINGS, []
    return warnings


def _record_scan_failure(message: str) -> None:
    _log.warning(message)
    _SCAN_WARNINGS.append(message)


from luau_codegen.model.geode_enums import EnumInfo, parse_enum_members
from luau_codegen.parse.broma import (
    Class,
    Function,
    Method,
    parse_method,
    split_arg,
    split_top_level,
)
from luau_codegen.parse.cpp_scan import (
    balanced_delimiter_end,
    find_unbalanced,
    scan_angle_block,
    template_preceded,
)
from luau_codegen.parse.text import strip_comments

_PREPROCESSOR = re.compile(r"^\s*#[^\n]*$", re.MULTILINE)

_SCANNED_LINK_ATTR = "link(win, android, android32, android64, imac, m1, ios)"
_SCANNED_LINK_PLATFORMS = (
    "win",
    "android",
    "android32",
    "android64",
    "imac",
    "m1",
    "ios",
)

_GL_TYPE_ALIASES = {
    "GLubyte": "unsigned char",
    "GLfloat": "float",
    "GLint": "int",
    "GLuint": "unsigned int",
}

_STRIP_KEYWORDS = re.compile(
    r"\b(override|noexcept|final|explicit|constexpr|consteval|constinit"
    r"|GEODE_DLL|GEODE_HIDDEN)\b"
)
_STRIP_ATTRS = re.compile(r"\[\[[^\]]*\]\]")
_STRIP_TRAILING = re.compile(r"\s*=\s*(0|default|delete)\s*$")

_CLASS_DECL = re.compile(
    r"class\s+GEODE_DLL\s+(\w+)"
    r"(?:\s+final)?"
    r"\s*(?::\s*((?:(?:public|private|protected|virtual)\s+)?[^{]+?))?"
    r"\s*\{",
    re.DOTALL,
)
_CCNODE_DECL = re.compile(
    r"class\s+CC_DLL\s+CCNode\s*:\s*public\s+CCObject\s*\{",
    re.DOTALL,
)
_ACCESS_LABEL = re.compile(r"(public|protected|private)\s*:")
_NESTED_DECL = re.compile(r"(class|struct|enum)\s+")
_TEMPLATE_DECL = re.compile(r"template\s*<")
_ENUM_DECL = re.compile(r"\benum\s+(?:class\s+|struct\s+)?(\w+)\b")
_NS_OPEN = re.compile(r"namespace\s+([\w:]+)\s*\{|namespace\s*\{")
_BLOCK_OPEN = re.compile(r"\b(?:class|struct|union|enum)\b[^;{}]*\{")
_FUNC_DECL = re.compile(
    r"GEODE_DLL\s+([A-Za-z_][\w:<>,\s\*&]*?)\s+([A-Za-z_]\w*)\s*\(([^{};]*)\)\s*;"
)

_FUNCTION_SOURCES = FREE_FUNCTION_SOURCES


def _included_headers(sdk_path: str) -> set[str]:
    ui_hpp = os.path.join(sdk_path, "loader", "include", "Geode", "UI.hpp")
    if not os.path.isfile(ui_hpp):
        return set()
    headers: set[str] = set()
    with open(ui_hpp, "r", encoding="utf-8") as f:
        for line in f:
            m = re.match(r'\s*#\s*include\s+"ui/(\w+\.hpp)"', line)
            if m:
                headers.add(m.group(1))
    return headers


def scan_geode_sdk(sdk_path: str) -> List[Class]:
    ui_dir = os.path.join(sdk_path, "loader", "include", "Geode", "ui")
    if not os.path.isdir(ui_dir):
        return []

    allowed = _included_headers(sdk_path)

    classes: List[Class] = []
    for filename in sorted(os.listdir(ui_dir)):
        if not filename.endswith(".hpp"):
            continue
        if allowed and filename not in allowed:
            continue
        try:
            classes.extend(_scan_header(os.path.join(ui_dir, filename)))
        except Exception as exc:
            _record_scan_failure(f"[luauapi] failed to scan {filename}: {exc}")
    return classes


def _bindings_enums_header_candidates(bindings_dir: str | None) -> list[str]:
    if not bindings_dir:
        return []
    return [
        os.path.join(bindings_dir, "include", "Geode", "Enums.hpp"),
        os.path.join(os.path.dirname(bindings_dir), "include", "Geode", "Enums.hpp"),
    ]


def scan_geode_enums(
    sdk_path: str,
    *,
    bindings_dir: str | None = None,
) -> dict[str, EnumInfo]:
    include_dir = os.path.join(sdk_path, "loader", "include", "Geode")
    ui_dir = os.path.join(include_dir, "ui")
    allowed = _included_headers(sdk_path)
    out: dict[str, EnumInfo] = {}

    if os.path.isdir(ui_dir):
        for filename in sorted(os.listdir(ui_dir)):
            if not filename.endswith(".hpp"):
                continue
            if allowed and filename not in allowed:
                continue
            try:
                out.update(_scan_header_enums(os.path.join(ui_dir, filename)))
            except Exception as exc:
                _record_scan_failure(f"[luauapi] failed to scan enums {filename}: {exc}")

    seen_headers: set[str] = set()
    for enums_hpp in [
        *_bindings_enums_header_candidates(bindings_dir),
        os.path.join(include_dir, "Enums.hpp"),
    ]:
        norm = os.path.normcase(os.path.abspath(enums_hpp))
        if norm in seen_headers:
            continue
        seen_headers.add(norm)
        if not os.path.isfile(enums_hpp):
            continue
        try:
            out.update(_scan_header_enums(enums_hpp))
        except Exception as exc:
            _record_scan_failure(f"[luauapi] failed to scan enums {enums_hpp}: {exc}")

    return out


def scan_geode_functions(sdk_path: str) -> List[Function]:
    include_dir = os.path.join(sdk_path, "loader", "include", "Geode")
    out: List[Function] = []
    for rel, namespaces, names in _FUNCTION_SOURCES:
        path = os.path.join(include_dir, *rel.split("/"))
        if not os.path.isfile(path):
            continue
        try:
            out.extend(_scan_header_functions(path, namespaces, names))
        except Exception as exc:
            _record_scan_failure(f"[luauapi] failed to scan functions {rel}: {exc}")
    return out


def scan_geode_ccnode_additions(sdk_path: str) -> Class | None:
    path = os.path.join(
        sdk_path,
        "loader",
        "include",
        "Geode",
        "cocos",
        "base_nodes",
        "CCNode.h",
    )
    if not os.path.isfile(path):
        return None

    with open(path, "r", encoding="utf-8") as f:
        text = f.read()
    text = strip_comments(text)
    text = _strip_preproc(text)

    match = _CCNODE_DECL.search(text)
    if not match:
        return None

    brace_start = match.end() - 1
    brace_end = balanced_delimiter_end(text, brace_start)
    body = text[brace_start + 1 : brace_end]
    line = text[: match.start()].count("\n") + 1
    methods = _extract_public_methods(
        "CCNode",
        body,
        line,
        geode_only=True,
        include_bodies=False,
    )
    if not methods:
        return None
    for method in methods:
        for platform in _SCANNED_LINK_PLATFORMS:
            method.platforms.setdefault(platform, "link")

    return Class(
        name="CCNode",
        namespace="cocos2d",
        bases=["CCObject"],
        methods=methods,
        source=path,
        line=line,
    )


def _scan_header_functions(path: str, namespaces, names) -> List[Function]:
    with open(path, "r", encoding="utf-8") as f:
        text = f.read()
    text = strip_comments(text)
    text = _strip_preproc(text)

    out: List[Function] = []
    ns_stack: List[str] = []
    frames: List[str] = []
    i = 0
    n = len(text)
    while i < n:
        c = text[i]
        if c in " \t\r\n":
            i += 1
            continue
        m = _NS_OPEN.match(text, i)
        if m:
            ns_stack.append(m.group(1) or "")
            frames.append("ns")
            i = m.end()
            continue
        m = _BLOCK_OPEN.match(text, i)
        if m:
            frames.append("block")
            i = m.end()
            continue
        if c == "{":
            frames.append("block")
            i += 1
            continue
        if c == "}":
            if frames and frames.pop() == "ns" and ns_stack:
                ns_stack.pop()
            i += 1
            continue
        if "block" not in frames:
            m = _FUNC_DECL.match(text, i)
            if m:
                ns = "::".join(seg for seg in ns_stack if seg)
                name = m.group(2)
                if ns in namespaces and (names is None or name in names):
                    args = [
                        a
                        for a in (split_arg(a) for a in split_top_level(m.group(3)))
                        if a.type or a.name
                    ]
                    out.append(
                        Function(
                            name=name,
                            namespace=ns,
                            ret=m.group(1).strip(),
                            args=args,
                            line=text.count("\n", 0, i) + 1,
                        )
                    )
                i = m.end()
                continue
        i += 1
    return out


def _enum_body_slice(text: str, decl_end: int) -> str | None:
    i = decl_end
    n = len(text)
    while i < n and text[i] in " \t\r\n":
        i += 1
    if i < n and text[i] == ":":
        brace = text.find("{", i)
        if brace == -1:
            return None
        i = brace
    if i >= n or text[i] != "{":
        return None
    brace_end = balanced_delimiter_end(text, i)
    return text[i + 1 : brace_end]


def _class_body_ranges(text: str) -> list[tuple[int, int]]:
    ranges: list[tuple[int, int]] = []
    for match in _CLASS_DECL.finditer(text):
        brace_start = match.end() - 1
        brace_end = balanced_delimiter_end(text, brace_start)
        ranges.append((brace_start, brace_end))
    return ranges


def _namespace_at(text: str, pos: int, class_ranges: list[tuple[int, int]]) -> str:
    ns_stack: list[str] = []
    frames: list[str] = []
    i = 0
    while i < pos:
        c = text[i]
        if c in " \t\r\n":
            i += 1
            continue
        for start, end in class_ranges:
            if start <= i < end:
                i = end
                break
        else:
            m = _NS_OPEN.match(text, i)
            if m:
                ns_stack.append(m.group(1) or "")
                frames.append("ns")
                i = m.end()
                continue
            m = _BLOCK_OPEN.match(text, i)
            if m:
                frames.append("block")
                i = m.end()
                continue
            if c == "{":
                frames.append("block")
                i += 1
                continue
            if c == "}":
                if frames and frames.pop() == "ns" and ns_stack:
                    ns_stack.pop()
                i += 1
                continue
            i += 1
    return "::".join(seg for seg in ns_stack if seg)


def parse_geode_enums(text: str, *, source: str = "") -> dict[str, EnumInfo]:
    text = strip_comments(text)
    text = _strip_preproc(text)

    class_ranges = _class_body_ranges(text)

    out: dict[str, EnumInfo] = {}
    for m in _ENUM_DECL.finditer(text):
        pos = m.start()
        if any(start <= pos < end for start, end in class_ranges):
            continue
        name = m.group(1)
        body = _enum_body_slice(text, m.end())
        if body is None:
            continue
        namespace = _namespace_at(text, pos, class_ranges)
        cxx_name = f"{namespace}::{name}" if namespace else name
        members, warning = parse_enum_members(body)
        if warning:
            label = source or name
            _record_scan_failure(
                f"[luauapi] failed to parse enum members {name} in {label}: {warning}"
            )
            members = ()
        out[name] = EnumInfo(name=name, cxx_name=cxx_name, members=members)
    return out


def _scan_header_enums(path: str) -> dict[str, EnumInfo]:
    with open(path, "r", encoding="utf-8") as f:
        text = f.read()
    return parse_geode_enums(text, source=path)


def _strip_preproc(text: str) -> str:
    return _PREPROCESSOR.sub("", text)


def _find_namespace(text: str) -> str:
    m = re.search(r"\bnamespace\s+([\w:]+)\s*\{", text)
    return m.group(1) if m else ""


def _parse_bases(raw: str) -> List[str]:
    bases: List[str] = []
    for part in split_top_level(raw):
        clean = re.sub(r"\b(public|private|protected|virtual)\b", "", part).strip()
        clean = re.sub(r"<[^>]*>", "", clean).strip()
        if clean:
            bases.append(clean)
    return bases


def _normalize_types(text: str) -> str:
    for alias, replacement in _GL_TYPE_ALIASES.items():
        text = re.sub(rf"\b{alias}\b", replacement, text)
    return text


def _clean_method_text(text: str) -> str:
    text = _STRIP_ATTRS.sub("", text)
    text = _STRIP_KEYWORDS.sub("", text)
    text = _STRIP_TRAILING.sub("", text)
    text = _normalize_types(text)
    text = re.sub(r"\s+", " ", text).strip()
    return text


def _extract_public_methods(
    class_name: str,
    body: str,
    base_line: int,
    *,
    geode_only: bool = False,
    include_bodies: bool = True,
) -> List[Method]:
    methods: List[Method] = []
    access = "private"

    i = 0
    length = len(body)
    while i < length:
        ch = body[i]

        if ch in " \t\r\n":
            i += 1
            continue

        access_m = _ACCESS_LABEL.match(body, i)
        if access_m:
            access = access_m.group(1)
            i = access_m.end()
            continue

        if body.startswith("using ", i) or body.startswith("friend ", i):
            semi = body.find(";", i)
            i = semi + 1 if semi != -1 else length
            continue

        if _NESTED_DECL.match(body, i):
            brace = find_unbalanced(body, i, "{")
            semi = find_unbalanced(body, i, ";")
            if brace != -1 and (semi == -1 or brace < semi):
                i = balanced_delimiter_end(body, brace) + 1
            else:
                i = semi + 1 if semi != -1 else length
            continue

        if body.startswith("template", i):
            tm = _TEMPLATE_DECL.match(body, i)
            if tm:
                j = scan_angle_block(body, tm.end() - 1)
                brace = find_unbalanced(body, j, "{")
                semi = find_unbalanced(body, j, ";")
                if brace != -1 and (semi == -1 or brace < semi):
                    i = balanced_delimiter_end(body, brace) + 1
                else:
                    i = semi + 1 if semi != -1 else length
                continue

        brace = find_unbalanced(body, i, "{")
        semi = find_unbalanced(body, i, ";")

        if brace != -1 and (semi == -1 or brace < semi):
            stmt = body[i:brace].strip()
            if (
                include_bodies
                and "(" in stmt
                and access == "public"
                and _method_stmt_allowed(stmt, geode_only)
            ):
                cleaned = _clean_method_text(stmt)
                m = parse_method(class_name, cleaned, base_line, access)
                if m:
                    methods.append(m)
            i = balanced_delimiter_end(body, brace) + 1
            if i < length and body[i] == ";":
                i += 1
            continue

        if semi != -1:
            stmt = body[i:semi].strip()
            if (
                "(" in stmt
                and ")" in stmt
                and access == "public"
                and _method_stmt_allowed(stmt, geode_only)
            ):
                cleaned = _clean_method_text(stmt)
                m = parse_method(class_name, cleaned, base_line, access)
                if m:
                    methods.append(m)
            i = semi + 1
            continue

        i += 1

    return methods


def _method_stmt_allowed(stmt: str, geode_only: bool) -> bool:
    return not geode_only or "GEODE_DLL" in stmt


def _scan_header(path: str) -> List[Class]:
    with open(path, "r", encoding="utf-8") as f:
        text = f.read()

    text = strip_comments(text)
    text = _strip_preproc(text)
    namespace = _find_namespace(text)

    classes: List[Class] = []

    for match in _CLASS_DECL.finditer(text):
        if template_preceded(text, match.start()):
            continue

        name = match.group(1)
        bases_raw = match.group(2) or ""
        bases = _parse_bases(bases_raw)

        brace_start = match.end() - 1
        brace_end = balanced_delimiter_end(text, brace_start)
        body = text[brace_start + 1 : brace_end]

        line = text[: match.start()].count("\n") + 1
        methods = _extract_public_methods(name, body, line)

        cls = Class(
            name=name,
            namespace=namespace,
            bases=bases,
            methods=methods,
            attributes=[_SCANNED_LINK_ATTR],
            source=path,
            line=line,
        )
        classes.append(cls)

    return classes
