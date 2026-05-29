from __future__ import annotations

import re

from broma_parser import Class, Method, split_top_level
from model import cxx_name


_PRIMITIVES = {
    "void": "v",
    "bool": "b",
    "char": "c",
    "signed char": "a",
    "unsigned char": "h",
    "short": "s",
    "unsigned short": "t",
    "int": "i",
    "unsigned": "j",
    "unsigned int": "j",
    "long": "l",
    "unsigned long": "m",
    "long long": "x",
    "unsigned long long": "y",
    "float": "f",
    "double": "d",
    "size_t": "m",
    "uint8_t": "h",
    "uint16_t": "t",
    "uint32_t": "j",
    "uint64_t": "y",
    "int8_t": "a",
    "int16_t": "s",
    "int32_t": "i",
    "int64_t": "x",
}

_SPECIAL_NAMES = {
    "std": "St",
    "std::allocator": "Sa",
    "std::string": "Ss",
    "gd::string": "Ss",
}

_ALIASES = {
    "cocos2d::ccColor3B": "cocos2d::_ccColor3B",
}


def _clean_type(value: str) -> str:
    out = re.sub(r"\s+", " ", value.strip())
    out = out.replace(" *", "*").replace("* ", "*")
    out = out.replace(" &", "&").replace("& ", "&")
    out = out.replace(" :: ", "::").replace(":: ", "::").replace(" ::", "::")
    return _ALIASES.get(out, out)


def _split_template(value: str) -> tuple[str, list[str]] | None:
    if "<" not in value or not value.endswith(">"):
        return None
    base, _, tail = value.partition("<")
    return base.strip(), [part.strip() for part in split_top_level(tail[:-1])]


def _source_name(value: str) -> str:
    return f"{len(value)}{value}"


class ItaniumMangler:
    def __init__(self) -> None:
        self.substitutions: list[tuple[str, str]] = []

    def _substitution(self, key: str) -> str:
        for idx, (seen_key, _) in enumerate(self.substitutions):
            if seen_key == key:
                if idx == 0:
                    return "S_"
                return f"S{self._base36(idx - 1)}_"
        return ""

    def _remember(self, key: str, encoded: str) -> str:
        if key not in (seen_key for seen_key, _ in self.substitutions):
            self.substitutions.append((key, encoded))
        return encoded

    @staticmethod
    def _base36(value: int) -> str:
        digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        out = ""
        while True:
            out = digits[value % 36] + out
            value //= 36
            if value == 0:
                return out

    def nested_name(self, qualified: str, const_method: bool = False) -> str:
        parts = qualified.split("::")
        cv = "K" if const_method else ""
        return "N" + cv + "".join(_source_name(part) for part in parts) + "E"

    def type(self, value: str) -> str:
        name = _clean_type(value)
        if name in _PRIMITIVES:
            return _PRIMITIVES[name]
        if name in _SPECIAL_NAMES:
            return _SPECIAL_NAMES[name]
        if name.endswith("*"):
            return self._qualified("P", name[:-1].strip())
        if name.endswith("&"):
            return self._qualified("R", name[:-1].strip())
        if name.startswith("const "):
            return self._qualified("K", name[6:].strip())
        if name.endswith(" const"):
            return self._qualified("K", name[:-6].strip())
        tmpl = _split_template(name)
        if tmpl:
            return self._template(tmpl[0], tmpl[1])
        return self._name_type(name)

    def _qualified(self, qualifier: str, inner: str) -> str:
        normalized_inner = _clean_type(inner)
        key = f"{qualifier}{normalized_inner}"
        ref = self._substitution(key)
        if ref:
            return ref
        encoded = qualifier + self.type(normalized_inner)
        return self._remember(key, encoded)

    def _name_type(self, name: str) -> str:
        name = _ALIASES.get(name, name)
        ref = self._substitution(name)
        if ref:
            return ref
        if "::" not in name:
            return self._remember(name, _source_name(name))
        encoded = self._nested_type(name)
        return self._remember(name, encoded)

    def _nested_type(self, name: str) -> str:
        parts = name.split("::")
        if parts[0] == "std":
            encoded = "St" + "".join(_source_name(part) for part in parts[1:])
        else:
            encoded = "N" + "".join(_source_name(part) for part in parts) + "E"
        return encoded

    def _template(self, base: str, parts: list[str]) -> str:
        key = f"{base}<{', '.join(parts)}>"
        ref = self._substitution(key)
        if ref:
            return ref
        encoded_args = "".join(self.type(part) for part in parts)
        encoded_args += self._default_template_args(base, parts)
        base = _clean_type(base)
        if base in _SPECIAL_NAMES:
            encoded = f"{_SPECIAL_NAMES[base]}I{encoded_args}E"
        elif "::" not in base:
            encoded = f"{_source_name(base)}I{encoded_args}E"
        else:
            base_parts = base.split("::")
            if base_parts[0] == "std":
                encoded = (
                    "St"
                    + "".join(_source_name(part) for part in base_parts[1:])
                    + f"I{encoded_args}E"
                )
            else:
                encoded = (
                    "N"
                    + "".join(_source_name(part) for part in base_parts)
                    + f"I{encoded_args}EE"
                )
        return self._remember(key, encoded)

    def _default_template_args(self, base: str, parts: list[str]) -> str:
        if not parts:
            return ""
        if base == "gd::vector":
            return self.type(f"std::allocator<{parts[0]}>")
        if base == "gd::set":
            return self.type(f"std::less<{parts[0]}>") + self.type(
                f"std::allocator<{parts[0]}>"
            )
        if base == "gd::map" and len(parts) >= 2:
            return self.type(f"std::less<{parts[0]}>") + self.type(
                f"std::allocator<std::pair<const {parts[0]}, {parts[1]}>>"
            )
        if base == "gd::unordered_set":
            return (
                self.type(f"std::hash<{parts[0]}>")
                + self.type(f"std::equal_to<{parts[0]}>")
                + self.type(f"std::allocator<{parts[0]}>")
            )
        if base == "gd::unordered_map" and len(parts) >= 2:
            return (
                self.type(f"std::hash<{parts[0]}>")
                + self.type(f"std::equal_to<{parts[0]}>")
                + self.type(f"std::allocator<std::pair<const {parts[0]}, {parts[1]}>>")
            )
        return ""


def android_symbol(cls: Class, method: Method) -> str:
    mangler = ItaniumMangler()
    qualified = f"{cxx_name(cls)}::{method.name}"
    symbol = f"_Z{mangler.nested_name(qualified, method.is_const)}"
    if not method.args:
        return f"{symbol}v"
    return symbol + "".join(mangler.type(arg.type) for arg in method.args)
