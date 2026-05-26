from __future__ import annotations

import os
import re
from typing import List, Optional

from broma_parser import Arg, Class, Method, _parse_method, split_top_level

_LINE_COMMENT = re.compile(r"//[^\n]*")
_BLOCK_COMMENT = re.compile(r"/\*.*?\*/", re.DOTALL)
_PREPROCESSOR = re.compile(r"^\s*#[^\n]*$", re.MULTILINE)

_ALL_LINK_ATTR = "link(win, android, android32, android64, imac, m1, ios)"

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
        except Exception:
            pass
    return classes


def _strip_comments(text: str) -> str:
    text = _BLOCK_COMMENT.sub("", text)
    return _LINE_COMMENT.sub("", text)


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

        rest = body[i:]

        access_m = re.match(r"(public|protected|private)\s*:", rest)
        if access_m:
            access = access_m.group(1)
            i += access_m.end()
            continue

        if rest.startswith("using ") or rest.startswith("friend "):
            semi = body.find(";", i)
            i = semi + 1 if semi != -1 else length
            continue

        if re.match(r"(class|struct|enum)\s+", rest):
            brace = body.find("{", i)
            semi = body.find(";", i)
            if brace != -1 and (semi == -1 or brace < semi):
                i = _balanced_end(body, brace) + 1
            else:
                i = semi + 1 if semi != -1 else length
            continue

        if rest.startswith("template"):
            tm = re.match(r"template\s*<", rest)
            if tm:
                depth = 1
                j = i + tm.end()
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
                m = _parse_method(class_name, cleaned, base_line, access)
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
                m = _parse_method(class_name, cleaned, base_line, access)
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

    text = _strip_comments(text)
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
            attributes=[_ALL_LINK_ATTR],
            source=path,
            line=line,
        )
        classes.append(cls)

    return classes
