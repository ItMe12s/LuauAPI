from __future__ import annotations

import os
import re
from typing import List

from luau_codegen.parse.broma import (
    Class,
    Function,
    Method,
    parse_method,
    split_arg,
    split_top_level,
)
from luau_codegen.parse.text import strip_comments

_PREPROCESSOR = re.compile(r"^\s*#[^\n]*$", re.MULTILINE)

_SCANNED_LINK_ATTR = "link(win, android, android32, android64, imac, m1, ios)"

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
_ACCESS_LABEL = re.compile(r"(public|protected|private)\s*:")
_NESTED_DECL = re.compile(r"(class|struct|enum)\s+")
_TEMPLATE_DECL = re.compile(r"template\s*<")
_ENUM_DECL = re.compile(r"\benum\s+(?:class\s+|struct\s+)?(\w+)\b")
_NS_OPEN = re.compile(r"namespace\s+([\w:]+)\s*\{|namespace\s*\{")
_BLOCK_OPEN = re.compile(r"\b(?:class|struct|union|enum)\b[^;{}]*\{")
_FUNC_DECL = re.compile(
    r"GEODE_DLL\s+([A-Za-z_][\w:<>,\s\*&]*?)\s+([A-Za-z_]\w*)\s*\(([^{};]*)\)\s*;"
)

_FUNCTION_SOURCES = (
    (
        "utils/general.hpp",
        frozenset({"geode::utils::clipboard", "geode::utils::game"}),
        None,
    ),
    ("ui/Popup.hpp", frozenset({"geode"}), frozenset({"createQuickPopup"})),
)


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
            import warnings

            warnings.warn(f"[luauapi] failed to scan {filename}: {exc}")
    return classes


def scan_geode_enums(sdk_path: str) -> dict[str, str]:
    ui_dir = os.path.join(sdk_path, "loader", "include", "Geode", "ui")
    if not os.path.isdir(ui_dir):
        return {}
    allowed = _included_headers(sdk_path)
    out: dict[str, str] = {}
    for filename in sorted(os.listdir(ui_dir)):
        if not filename.endswith(".hpp"):
            continue
        if allowed and filename not in allowed:
            continue
        try:
            out.update(_scan_header_enums(os.path.join(ui_dir, filename)))
        except Exception as exc:
            import warnings

            warnings.warn(f"[luauapi] failed to scan enums {filename}: {exc}")
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
            import warnings

            warnings.warn(f"[luauapi] failed to scan functions {rel}: {exc}")
    return out


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


def _scan_header_enums(path: str) -> dict[str, str]:
    with open(path, "r", encoding="utf-8") as f:
        text = f.read()
    text = strip_comments(text)
    text = _strip_preproc(text)
    namespace = _find_namespace(text)

    class_ranges: list[tuple[int, int]] = []
    for match in _CLASS_DECL.finditer(text):
        brace_start = match.end() - 1
        brace_end = _balanced_end(text, brace_start)
        class_ranges.append((brace_start, brace_end))

    out: dict[str, str] = {}
    for m in _ENUM_DECL.finditer(text):
        pos = m.start()
        if any(start <= pos < end for start, end in class_ranges):
            continue
        name = m.group(1)
        out[name] = f"{namespace}::{name}" if namespace else name
    return out


def _strip_preproc(text: str) -> str:
    return _PREPROCESSOR.sub("", text)


def _balanced_end(text: str, start: int) -> int:
    depth = 0
    i = start
    while i < len(text):
        ch = text[i]
        if ch == "{":
            depth += 1
        elif ch == "}":
            depth -= 1
            if depth == 0:
                return i
        i += 1
    return len(text)


def _find_namespace(text: str) -> str:
    m = re.search(r"\bnamespace\s+(\w+)\s*\{", text)
    return m.group(1) if m else ""


def _parse_bases(raw: str) -> List[str]:
    bases: List[str] = []
    for part in split_top_level(raw):
        clean = re.sub(r"\b(public|private|protected|virtual)\b", "", part).strip()
        clean = re.sub(r"<[^>]*>", "", clean).strip()
        if clean:
            bases.append(clean)
    return bases


def _is_template_preceded(text: str, pos: int) -> bool:
    before = text[:pos].rstrip()
    return bool(re.search(r"template\s*<[^>]*>\s*$", before, re.DOTALL))


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


def _extract_public_methods(class_name: str, body: str, base_line: int) -> List[Method]:
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
            brace = body.find("{", i)
            semi = body.find(";", i)
            if brace != -1 and (semi == -1 or brace < semi):
                i = _balanced_end(body, brace) + 1
            else:
                i = semi + 1 if semi != -1 else length
            continue

        if body.startswith("template", i):
            tm = _TEMPLATE_DECL.match(body, i)
            if tm:
                depth = 1
                j = tm.end()
                while j < length and depth > 0:
                    if body[j] == "<":
                        depth += 1
                    elif body[j] == ">":
                        depth -= 1
                    j += 1
                brace = body.find("{", j)
                semi = body.find(";", j)
                if brace != -1 and (semi == -1 or brace < semi):
                    i = _balanced_end(body, brace) + 1
                else:
                    i = semi + 1 if semi != -1 else length
                continue

        brace = _find_unbalanced(body, i, "{")
        semi = _find_unbalanced(body, i, ";")

        if brace != -1 and (semi == -1 or brace < semi):
            stmt = body[i:brace].strip()
            if "(" in stmt and access == "public":
                cleaned = _clean_method_text(stmt)
                m = parse_method(class_name, cleaned, base_line, access)
                if m:
                    methods.append(m)
            i = _balanced_end(body, brace) + 1
            if i < length and body[i] == ";":
                i += 1
            continue

        if semi != -1:
            stmt = body[i:semi].strip()
            if "(" in stmt and ")" in stmt and access == "public":
                cleaned = _clean_method_text(stmt)
                m = parse_method(class_name, cleaned, base_line, access)
                if m:
                    methods.append(m)
            i = semi + 1
            continue

        i += 1

    return methods


def _find_unbalanced(text: str, start: int, target: str) -> int:
    depth_paren = 0
    depth_angle = 0
    i = start
    while i < len(text):
        ch = text[i]
        if ch == "(":
            depth_paren += 1
        elif ch == ")":
            depth_paren -= 1
        elif ch == "<":
            depth_angle += 1
        elif ch == ">":
            depth_angle -= 1
        if ch == target and depth_paren == 0 and depth_angle <= 0:
            return i
        i += 1
    return -1


def _scan_header(path: str) -> List[Class]:
    with open(path, "r", encoding="utf-8") as f:
        text = f.read()

    text = strip_comments(text)
    text = _strip_preproc(text)
    namespace = _find_namespace(text)

    classes: List[Class] = []

    for match in _CLASS_DECL.finditer(text):
        if _is_template_preceded(text, match.start()):
            continue

        name = match.group(1)
        bases_raw = match.group(2) or ""
        bases = _parse_bases(bases_raw)

        brace_start = match.end() - 1
        brace_end = _balanced_end(text, brace_start)
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
