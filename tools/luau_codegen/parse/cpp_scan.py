from __future__ import annotations


_LITERAL_PREFIXES = ('u8R"', 'LR"', 'uR"', 'UR"', 'R"', 'u8"', 'L"', 'u"', 'U"')


def _advance_past_literal(text: str, i: int) -> int:
    quote = text[i]
    i += 1
    while i < len(text):
        ch = text[i]
        if ch == "\\":
            i += 2
            continue
        if ch == quote:
            return i + 1
        i += 1
    return len(text)


def _skip_raw_literal(text: str, i: int) -> int:
    marker = text.find('R"', i, i + 4)
    if marker == -1:
        return i
    open_paren = text.find("(", marker + 2)
    if open_paren == -1:
        return len(text)
    delim = text[marker + 2 : open_paren]
    close = f'){delim}"'
    found = text.find(close, open_paren + 1)
    return len(text) if found == -1 else found + len(close)


def _literal_end(text: str, i: int) -> int | None:
    if text[i] == "'":
        return _advance_past_literal(text, i)
    if text[i] == '"':
        return _advance_past_literal(text, i)
    for prefix in _LITERAL_PREFIXES:
        if not text.startswith(prefix, i):
            continue
        if 'R"' in prefix:
            return _skip_raw_literal(text, i)
        return _advance_past_literal(text, i + len(prefix) - 1)
    return None


def skip_non_code(text: str, i: int) -> int:
    while i < len(text):
        literal_end = _literal_end(text, i)
        if literal_end is not None:
            i = literal_end
            continue
        if text.startswith("//", i):
            newline = text.find("\n", i + 2)
            i = len(text) if newline == -1 else newline
            continue
        if text.startswith("/*", i):
            close = text.find("*/", i + 2)
            i = len(text) if close == -1 else close + 2
            continue
        return i
    return i


def strip_comments(text: str) -> str:
    """Replace C++ comments with whitespace while preserving offsets and newlines."""
    out = list(text)
    i = 0
    while i < len(text):
        literal_end = _literal_end(text, i)
        if literal_end is not None:
            i = literal_end
            continue
        if text.startswith("//", i):
            end = text.find("\n", i + 2)
            end = len(text) if end == -1 else end
            for pos in range(i, end):
                if out[pos] not in "\r\n":
                    out[pos] = " "
            i = end
            continue
        if text.startswith("/*", i):
            close = text.find("*/", i + 2)
            end = len(text) if close == -1 else close + 2
            for pos in range(i, end):
                if out[pos] not in "\r\n":
                    out[pos] = " "
            i = end
            continue
        i += 1
    return "".join(out)


def balanced_delimiter_end(
    text: str,
    start: int,
    *,
    open_ch: str = "{",
    close_ch: str = "}",
) -> int:
    depth = 0
    i = start
    n = len(text)
    while i < n:
        i = skip_non_code(text, i)
        if i >= n:
            break
        ch = text[i]
        if ch == open_ch:
            depth += 1
        elif ch == close_ch:
            depth -= 1
            if depth == 0:
                return i
        i += 1
    return n


def find_unbalanced(
    text: str,
    start: int,
    target: str,
    *,
    track_angle: bool = True,
) -> int:
    depth_paren = 0
    depth_angle = 0
    i = start
    n = len(text)
    while i < n:
        i = skip_non_code(text, i)
        if i >= n:
            break
        ch = text[i]
        if ch == "(":
            depth_paren += 1
        elif ch == ")":
            depth_paren -= 1
        elif track_angle and ch == "<":
            depth_angle += 1
        elif track_angle and ch == ">":
            depth_angle -= 1
        if ch == target and depth_paren == 0 and depth_angle <= 0:
            return i
        i += 1
    return -1


def template_preceded(text: str, pos: int) -> bool:
    before = text[:pos].rstrip()
    if not before.endswith(">"):
        return False
    depth = 0
    for i in range(len(before) - 1, -1, -1):
        ch = before[i]
        if ch == ">":
            depth += 1
        elif ch == "<":
            depth -= 1
            if depth == 0:
                head = before[:i].rstrip()
                return head.endswith("template") or head.endswith("template<")
    return False


def scan_angle_block(text: str, start: int) -> int:
    if start >= len(text) or text[start] != "<":
        return start
    depth = 1
    i = start + 1
    n = len(text)
    while i < n and depth > 0:
        i = skip_non_code(text, i)
        if i >= n:
            break
        ch = text[i]
        if ch == "<":
            depth += 1
        elif ch == ">":
            depth -= 1
        i += 1
    return i
