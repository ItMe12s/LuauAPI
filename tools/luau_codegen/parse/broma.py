from __future__ import annotations

import dataclasses
import re
from typing import TYPE_CHECKING, Dict, List, Optional

from luau_codegen.parse.text import strip_comments
from luau_codegen.util.platforms import INTERSECTION_PLATFORMS

if TYPE_CHECKING:
    from luau_codegen.model.codegen_context import CodegenContext


@dataclasses.dataclass
class Arg:
    type: str
    name: str


@dataclasses.dataclass
class Method:
    name: str
    ret: str
    args: List[Arg]
    is_virtual: bool = False
    is_static: bool = False
    is_const: bool = False
    is_callback: bool = False
    is_ctor: bool = False
    is_dtor: bool = False
    is_bound_ctor: bool = False
    access: str = "public"
    platforms: Dict[str, str] = dataclasses.field(default_factory=dict)
    attributes: List[str] = dataclasses.field(default_factory=list)
    line: int = 0


@dataclasses.dataclass
class Field:
    name: str
    type: str
    count: int = 1
    line: int = 0
    platforms: frozenset[str] | None = None


@dataclasses.dataclass
class Class:
    name: str
    namespace: str = ""
    bases: List[str] = dataclasses.field(default_factory=list)
    methods: List[Method] = dataclasses.field(default_factory=list)
    fields: List[Field] = dataclasses.field(default_factory=list)
    attributes: List[str] = dataclasses.field(default_factory=list)
    source: str = ""
    line: int = 0

    @property
    def qualified_name(self) -> str:
        return f"{self.namespace}::{self.name}" if self.namespace else self.name


@dataclasses.dataclass
class Function:
    name: str
    namespace: str = ""
    ret: str = "void"
    args: List[Arg] = dataclasses.field(default_factory=list)
    attributes: List[str] = dataclasses.field(default_factory=list)
    line: int = 0

    @property
    def lua_path(self) -> str:
        return self.namespace.replace("::", ".") if self.namespace else "geode"


@dataclasses.dataclass
class Root:
    classes: List[Class] = dataclasses.field(default_factory=list)
    functions: List["Function"] = dataclasses.field(default_factory=list)
    codegen_ctx: CodegenContext | None = None


_PLATFORMS = ("win", "imac", "m1", "ios", "android", "android32", "android64", "mac")
_PLATFORM_BLOCK_TOKENS = frozenset(_PLATFORMS) | {"mac"}
_PLATFORM_SCOPE_CANDIDATES = frozenset(INTERSECTION_PLATFORMS) | {"imac"}
_PLATFORM_ALIAS_TOKENS: dict[str, frozenset[str]] = {
    "mac": frozenset({"mac", "imac", "m1"}),
    "imac": frozenset({"mac", "imac"}),
    "m1": frozenset({"mac", "m1"}),
    "android": frozenset({"android", "android32", "android64"}),
    "android32": frozenset({"android", "android32"}),
    "android64": frozenset({"android", "android64"}),
}
_QUALIFIERS = {"unsigned", "signed", "const", "volatile", "long", "short"}
_PRIMITIVES = {
    "int",
    "char",
    "short",
    "long",
    "float",
    "double",
    "bool",
    "void",
    "auto",
}


def split_top_level(value: str) -> List[str]:
    out: List[str] = []
    depth = 0
    cur: List[str] = []
    for ch in value:
        if ch in "<([{":
            depth += 1
        elif ch in ">)]}":
            depth -= 1
        if ch == "," and depth == 0:
            out.append("".join(cur).strip())
            cur = []
        else:
            cur.append(ch)
    tail = "".join(cur).strip()
    if tail:
        out.append(tail)
    return out


def split_arg(arg: str) -> Arg:
    arg = arg.strip()
    if not arg:
        return Arg("", "")
    if "=" in arg:
        arg = arg[: arg.find("=")].rstrip()
    arg = re.sub(r"\s+", " ", arg).strip()

    fp = re.match(
        r"^(?P<ret>.+?)\(\s*\*\s*(?P<name>[A-Za-z_]\w*)?\s*\)\s*\((?P<params>.*)\)\s*$",
        arg,
    )
    if fp:
        name = fp.group("name") or ""
        return Arg(f"{fp.group('ret').strip()}(*)({fp.group('params').strip()})", name)

    m = re.match(r"^(.*?)([A-Za-z_]\w*)\s*$", arg)
    if not m:
        return Arg(arg, "")

    head = m.group(1).rstrip()
    matched = m.group(2)
    if not head:
        return Arg(matched, "")
    if head.endswith("::"):
        return Arg(arg, "")
    if head.endswith("*") or head.endswith("&") or head.endswith(">"):
        return Arg(head, matched)
    if "::" in head:
        return Arg(head, matched)

    head_tokens = head.split()
    if all(t in _QUALIFIERS for t in head_tokens) and matched in (_QUALIFIERS | _PRIMITIVES):
        return Arg(arg, "")
    return Arg(head, matched)


class Cursor:
    __slots__ = ("text", "pos", "line")

    def __init__(self, text: str):
        self.text = text
        self.pos = 0
        self.line = 1

    def starts_with(self, value: str) -> bool:
        return self.text.startswith(value, self.pos)

    def advance(self, count: int) -> str:
        s = self.text[self.pos : self.pos + count]
        self.line += s.count("\n")
        self.pos += count
        return s

    def skip_ws(self) -> None:
        while self.pos < len(self.text) and self.text[self.pos] in " \t\r\n":
            if self.text[self.pos] == "\n":
                self.line += 1
            self.pos += 1

    def at_end(self) -> bool:
        return self.pos >= len(self.text)


def parse_file(path: str) -> Root:
    with open(path, "r", encoding="utf-8") as f:
        return parse_text(f.read(), source=path)


def parse_text(raw: str, source: str = "") -> Root:
    cur = Cursor(strip_comments(raw))
    root = Root()
    pending_attrs: List[str] = []

    while not cur.at_end():
        cur.skip_ws()
        if cur.at_end():
            break
        if cur.starts_with("[["):
            end = cur.text.find("]]", cur.pos)
            if end == -1:
                cur.advance(2)
                continue
            attr_text = cur.text[cur.pos + 2 : end].strip()
            for part in split_top_level(attr_text):
                part = part.strip()
                if part:
                    pending_attrs.append(part)
            cur.line += cur.text[cur.pos : end + 2].count("\n")
            cur.pos = end + 2
            continue
        if cur.starts_with("#"):
            nl = cur.text.find("\n", cur.pos)
            if nl == -1:
                cur.pos = len(cur.text)
            else:
                cur.line += 1
                cur.pos = nl + 1
            continue

        m = re.match(r"class\s+([A-Za-z_][\w:]*)\s*(:[^{]*)?\{", cur.text[cur.pos :])
        if not m:
            cur.advance(1)
            continue

        full_name = m.group(1)
        bases = _parse_bases((m.group(2) or "").lstrip(":").strip())
        line = cur.line
        cur.advance(m.end())
        namespace = ""
        name = full_name
        if "::" in full_name:
            namespace, _, name = full_name.rpartition("::")
        cls = Class(
            name=name,
            namespace=namespace,
            bases=bases,
            attributes=pending_attrs[:],
            source=source,
            line=line,
        )
        pending_attrs.clear()
        parse_class_body(cur, cls)
        root.classes.append(cls)

    return root


def _parse_bases(value: str) -> List[str]:
    bases: List[str] = []
    for base in split_top_level(value):
        clean = re.sub(r"\b(public|private|protected|virtual)\b", "", base).strip()
        if clean:
            bases.append(clean)
    return bases


def parse_class_body(
    cur: Cursor,
    cls: Class,
    *,
    field_platforms: frozenset[str] | None = None,
) -> None:
    pending_method_attrs: List[str] = []
    current_access = "public"
    while not cur.at_end():
        cur.skip_ws()
        if cur.at_end():
            return
        if cur.text[cur.pos] == "}":
            cur.advance(1)
            return
        if cur.starts_with("[["):
            end = cur.text.find("]]", cur.pos)
            if end == -1:
                cur.advance(2)
            else:
                attr_text = cur.text[cur.pos + 2 : end].strip()
                for part in split_top_level(attr_text):
                    part = part.strip()
                    if part:
                        pending_method_attrs.append(part)
                cur.line += cur.text[cur.pos : end + 2].count("\n")
                cur.pos = end + 2
            continue

        start = cur.pos
        line = cur.line
        depth = 0
        i = cur.pos
        while i < len(cur.text):
            ch = cur.text[i]
            if ch in "<([":
                depth += 1
            elif ch in ">)]":
                depth -= 1
            elif ch == ":" and depth == 0:
                candidate = cur.text[start : i + 1].strip()
                section = re.match(r"^(public|protected|private)\s*:\s*$", candidate)
                if section:
                    cur.line += cur.text[start : i + 1].count("\n")
                    cur.pos = i + 1
                    current_access = section.group(1)
                    break
            elif ch == "{" and depth == 0:
                head = cur.text[start:i].strip()
                end = _skip_balanced_brace(cur.text, i)
                block_platforms = _parse_platform_block_header(head)
                if block_platforms is not None:
                    inner_start = i + 1
                    inner_end = end - 1
                    inner_cur = Cursor(cur.text[inner_start:inner_end])
                    inner_cur.line = line + cur.text[start:inner_start].count("\n")
                    parse_class_body(inner_cur, cls, field_platforms=block_platforms)
                    cur.line += cur.text[start:end].count("\n")
                    cur.pos = end
                else:
                    cur.line += cur.text[start:end].count("\n")
                    cur.pos = end
                    _parse_member(
                        cls,
                        head,
                        line,
                        pending_method_attrs,
                        current_access,
                        field_platforms=field_platforms,
                    )
                pending_method_attrs.clear()
                break
            elif ch == ";" and depth == 0:
                text = cur.text[start:i].strip()
                cur.line += cur.text[start : i + 1].count("\n")
                cur.pos = i + 1
                new_access = _parse_member(
                    cls,
                    text,
                    line,
                    pending_method_attrs,
                    current_access,
                    field_platforms=field_platforms,
                )
                if new_access:
                    current_access = new_access
                pending_method_attrs.clear()
                break
            elif ch == "}" and depth == 0:
                cur.pos = i
                return
            i += 1
        else:
            cur.pos = len(cur.text)
            return


def _skip_balanced_brace(text: str, start: int) -> int:
    depth = 0
    i = start
    while i < len(text):
        if text[i] == "{":
            depth += 1
        elif text[i] == "}":
            depth -= 1
            if depth == 0:
                return i + 1
        i += 1
    return len(text)


def _expand_platform_scope(tokens: List[str]) -> frozenset[str]:
    out: set[str] = set()
    for token in tokens:
        for platform in _PLATFORM_SCOPE_CANDIDATES:
            aliases = _PLATFORM_ALIAS_TOKENS.get(platform, frozenset({platform}))
            if token == platform or token in aliases:
                out.add(platform)
    return frozenset(out)


def _parse_platform_block_header(head: str) -> frozenset[str] | None:
    if "(" in head:
        return None
    tokens = [part.strip() for part in head.split(",")]
    if not tokens or not all(token in _PLATFORM_BLOCK_TOKENS for token in tokens):
        return None
    return _expand_platform_scope(tokens)


def _parse_member(
    cls: Class,
    decl: str,
    line: int,
    pending_method_attrs: List[str],
    current_access: str,
    *,
    field_platforms: frozenset[str] | None = None,
) -> Optional[str]:
    if not decl:
        return None
    head, addr_tail = _split_decl_and_addr(decl)
    head = head.strip()
    section = re.match(r"^(public|protected|private)\s*:\s*$", head)
    if section:
        return section.group(1)
    if "(" in head and ")" in head:
        method = parse_method(cls.name, head, line, current_access)
        if method:
            method.platforms = _parse_platform_list(addr_tail)
            method.attributes = pending_method_attrs[:]
            cls.methods.append(method)
        return None
    field = _parse_field(head, line)
    if field:
        field.platforms = field_platforms
        cls.fields.append(field)
    return None


def _split_decl_and_addr(decl: str) -> tuple[str, str]:
    depth = 0
    for i, ch in enumerate(decl):
        if ch in "<([{":
            depth += 1
        elif ch in ">)]}":
            depth -= 1
        elif ch == "=" and depth == 0:
            return decl[:i], decl[i + 1 :]
    return decl, ""


def _parse_platform_list(tail: str) -> Dict[str, str]:
    out: Dict[str, str] = {}
    stripped = tail.strip()
    if stripped == "inline":
        out["inline"] = "inline"
        return out
    for part in split_top_level(tail):
        toks = part.strip().split(None, 1)
        if not toks or toks[0] not in _PLATFORMS:
            continue
        out[toks[0]] = toks[1].strip() if len(toks) > 1 else ""
    return out


def parse_method(
    class_name: str, head: str, line: int, default_access: str = "public"
) -> Optional[Method]:
    flags = {"virtual": False, "static": False, "inline": False, "callback": False}
    access = default_access
    rest = head
    while True:
        m = re.match(
            r"\s*(virtual|static|inline|callback|public|protected|private)\s+",
            rest,
        )
        if not m:
            break
        kw = m.group(1)
        if kw in ("public", "protected", "private"):
            access = kw
        else:
            flags[kw] = True
        rest = rest[m.end() :]

    paren_start = rest.find("(")
    if paren_start == -1:
        return None
    depth = 0
    end = paren_start
    while end < len(rest):
        ch = rest[end]
        if ch == "(":
            depth += 1
        elif ch == ")":
            depth -= 1
            if depth == 0:
                break
        end += 1
    if end >= len(rest):
        return None

    lhs = rest[:paren_start].strip()
    trailer = rest[end + 1 :].strip()
    name_match = re.search(r"([~A-Za-z_]\w*)\s*$", lhs)
    if not name_match:
        return None
    name = name_match.group(1)
    ret = lhs[: name_match.start()].strip()
    is_dtor = name.startswith("~")
    is_ctor = not is_dtor and name == class_name and ret == ""
    if is_ctor or is_dtor:
        ret = "void"
    args = [split_arg(a) for a in split_top_level(rest[paren_start + 1 : end])]
    args = [a for a in args if a.type or a.name]
    return Method(
        name=name,
        ret=ret,
        args=args,
        is_virtual=flags["virtual"],
        is_static=flags["static"],
        is_const="const" in trailer.split(),
        is_callback=flags["callback"],
        is_ctor=is_ctor,
        is_dtor=is_dtor,
        access=access,
        line=line,
    )


def _parse_field(head: str, line: int) -> Optional[Field]:
    m = re.match(
        r"^(?P<type>.+?)\s+(?P<name>[A-Za-z_]\w*)(?:\s*\[\s*(?P<count>\d+)\s*\])?\s*$",
        head,
    )
    if not m:
        return None
    return Field(
        name=m.group("name"),
        type=m.group("type").strip(),
        count=int(m.group("count")) if m.group("count") else 1,
        line=line,
    )
