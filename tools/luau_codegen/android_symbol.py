from __future__ import annotations

import re

from broma_parser import Class, Method, split_top_level
from model import cxx_name


def _mangle_ident(value: str, nested: bool = True) -> str:
    if "::" not in value:
        return f"{len(value)}{value}"
    body = "".join(f"{len(part)}{part}" for part in value.split("::"))
    return f"N{body}E" if nested else body


def _int_to_base36(value: int) -> str:
    digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    out = ""
    while True:
        out = digits[value % 36] + out
        value //= 36
        if value == 0:
            return out


def _seen_ref(seen: list[str], expanded: str) -> str:
    try:
        idx = seen.index(expanded)
    except ValueError:
        return ""
    if idx == 0:
        return "S_"
    return f"S{_int_to_base36(idx - 1)}_"


def _subs_seen(seen: list[str], mangled: str, subs: bool, expanded: str) -> str:
    if not subs or not mangled:
        return mangled
    ref = _seen_ref(seen, expanded)
    if ref:
        return ref
    seen.append(expanded)
    return mangled


def _split_template_parts(value: str) -> list[str]:
    return [part.strip() for part in split_top_level(value)]


def _clean_type(value: str) -> str:
    out = re.sub(r"\s+", " ", value.strip())
    out = out.replace(" *", "*").replace("* ", "*")
    out = out.replace(" &", "&").replace("& ", "&")
    out = out.replace(" :: ", "::").replace(":: ", "::").replace(" ::", "::")
    return out


def _mangle_type(
    seen: list[str], name: str, subs: bool = True, is_template: bool = False
) -> str:
    name = _clean_type(name)
    primitives = {
        "void": "v",
        "bool": "b",
        "char": "c",
        "short": "s",
        "int": "i",
        "long": "l",
        "long long": "x",
        "unsigned": "j",
        "unsigned char": "h",
        "unsigned short": "t",
        "unsigned int": "j",
        "unsigned long": "m",
        "unsigned long long": "y",
        "float": "f",
        "double": "d",
        "gd::string": "Ss",
        "std::allocator": "Sa",
    }
    if name in primitives:
        return primitives[name]
    if name == "cocos2d::ccColor3B":
        return _mangle_type(seen, "cocos2d::_ccColor3B", subs)
    if name.endswith("*"):
        inner = name[:-1].strip()
        unsub = _mangle_type(seen, inner, False)
        if not subs:
            return f"P{unsub}"
        ref = _seen_ref(seen, f"P{unsub}")
        if ref:
            return ref
        result = _mangle_type(seen, inner, subs)
        return _subs_seen(seen, f"P{result}", subs, f"P{unsub}")
    if name.endswith("&"):
        inner = name[:-1].strip()
        unsub = _mangle_type(seen, inner, False)
        if not subs:
            return f"R{unsub}"
        ref = _seen_ref(seen, f"R{unsub}")
        if ref:
            return ref
        result = _mangle_type(seen, inner, subs)
        return _subs_seen(seen, f"R{result}", subs, f"R{unsub}")
    if name.startswith("const "):
        inner = name[6:].strip()
        unsub = _mangle_type(seen, inner, False)
        if not subs:
            return f"K{unsub}"
        ref = _seen_ref(seen, f"K{unsub}")
        if ref:
            return ref
        result = _mangle_type(seen, inner, subs)
        return _subs_seen(seen, f"K{result}", subs, f"K{unsub}")
    if name.endswith(" const"):
        inner = name[:-6].strip()
        unsub = _mangle_type(seen, inner, False)
        if not subs:
            return f"K{unsub}"
        ref = _seen_ref(seen, f"K{unsub}")
        if ref:
            return ref
        result = _mangle_type(seen, inner, subs)
        return _subs_seen(seen, f"K{result}", subs, f"K{unsub}")
    if "<" in name and name.endswith(">"):
        base, _, tail = name.partition("<")
        parts = _split_template_parts(tail[:-1])
        unsub = _mangle_template(seen, base, parts, False)
        if not subs:
            return unsub
        ref = _seen_ref(seen, unsub)
        if ref:
            return ref
        result = _mangle_template(seen, base, parts, subs)
        return _subs_seen(seen, result, subs, unsub)
    if "::" in name:
        result = ""
        substituted = ""
        for part in name.split("::"):
            item = f"{len(part)}{part}"
            if part in ("gd", "std"):
                substituted = "St"
            elif not subs:
                substituted += item
            else:
                ref = _seen_ref(seen, result + item)
                substituted = (
                    ref
                    if ref
                    else _subs_seen(seen, substituted + item, subs, result + item)
                )
            result += item
        if len(substituted) == 3 and substituted.startswith("S"):
            return substituted
        if is_template:
            return substituted
        return f"N{substituted}E"
    return _subs_seen(seen, _mangle_ident(name), subs, _mangle_ident(name))


def _mangle_template(seen: list[str], base: str, parts: list[str], subs: bool) -> str:
    outer = _mangle_type(seen, base, subs, True)
    result = "".join(_mangle_type(seen, part, subs, True) for part in parts)
    if base == "gd::vector":
        result += _mangle_type(seen, f"std::allocator<{parts[0]}>", subs, True)
    elif base == "gd::map":
        result += _mangle_type(seen, f"std::less<{parts[0]}>", subs, True)
        result += _mangle_type(
            seen,
            f"std::allocator<std::pair<const {parts[0]}, {parts[1]}>>",
            subs,
            True,
        )
    elif base == "gd::set":
        result += _mangle_type(seen, f"std::less<{parts[0]}>", subs, True)
        result += _mangle_type(seen, f"std::allocator<{parts[0]}>", subs, True)
    elif base == "gd::unordered_map":
        result += _mangle_type(seen, f"std::hash<{parts[0]}>", subs, True)
        result += _mangle_type(seen, f"std::equal_to<{parts[0]}>", subs, True)
        result += _mangle_type(
            seen,
            f"std::allocator<std::pair<const {parts[0]}, {parts[1]}>>",
            subs,
            True,
        )
    elif base == "gd::unordered_set":
        result += _mangle_type(seen, f"std::hash<{parts[0]}>", subs, True)
        result += _mangle_type(seen, f"std::equal_to<{parts[0]}>", subs, True)
        result += _mangle_type(seen, f"std::allocator<{parts[0]}>", subs, True)
    return f"{outer}I{result}E"


def android_symbol(cls: Class, method: Method) -> str:
    qualified = cxx_name(cls)
    symbol = f"_Z{_mangle_ident(f'{qualified}::{method.name}')}"
    if not method.args:
        return f"{symbol}v"
    seen = [_mangle_ident(qualified.split("::", 1)[0])]
    return symbol + "".join(_mangle_type(seen, arg.type) for arg in method.args)
