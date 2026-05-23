from __future__ import annotations

import re
from dataclasses import dataclass
from typing import Dict, Optional

from broma_parser import Class
from model import short_name


NUMERIC_TYPES = {
    "char",
    "signed char",
    "unsigned char",
    "short",
    "unsigned short",
    "int",
    "unsigned int",
    "long",
    "unsigned long",
    "long long",
    "unsigned long long",
    "size_t",
    "uint8_t",
    "uint16_t",
    "uint32_t",
    "uint64_t",
    "int8_t",
    "int16_t",
    "int32_t",
    "int64_t",
    "float",
    "double",
}

STRING_TYPES = {"char const*", "const char*", "std::string", "gd::string"}
VALUE_TYPES = {
    "cocos2d::CCPoint": "CCPoint",
    "CCPoint": "CCPoint",
    "cocos2d::CCSize": "CCSize",
    "CCSize": "CCSize",
    "cocos2d::CCRect": "CCRect",
    "CCRect": "CCRect",
    "cocos2d::ccColor3B": "RGBColor",
    "ccColor3B": "RGBColor",
}


@dataclass(frozen=True)
class TypeInfo:
    kind: str
    cxx_type: str
    lua_type: str
    class_name: str = ""


def normalize_type(t: str) -> str:
    s = re.sub(r"\s+", " ", t.strip())
    s = s.replace(" *", "*").replace("* ", "*")
    s = s.replace(" &", "&").replace("& ", "&")
    s = s.replace(" :: ", "::").replace(":: ", "::").replace(" ::", "::")
    if s.startswith("const ") and s.endswith("*"):
        s = s[6:].strip()
        return f"{s[:-1].strip()} const*"
    while s.startswith("const "):
        s = s[6:].strip()
    while s.endswith(" const"):
        s = s[:-6].strip()
    return s


def strip_ref(t: str) -> str:
    s = normalize_type(t)
    if s.endswith("&"):
        s = s[:-1].strip()
    while s.startswith("const "):
        s = s[6:].strip()
    while s.endswith(" const"):
        s = s[:-6].strip()
    return s


def without_pointer(t: str) -> str:
    s = strip_ref(t)
    return s[:-1].strip() if s.endswith("*") else s


def cxx_class_name(cls: Class) -> str:
    return f"{cls.namespace}::{cls.name}" if cls.namespace else cls.name


def resolve_object_class(t: str, classes: Dict[str, Class]) -> Optional[Class]:
    base = without_pointer(t).lstrip(":")
    if base in classes:
        return classes[base]
    return classes.get(short_name(base))


def classify_arg(t: str, object_classes: Dict[str, Class]) -> Optional[TypeInfo]:
    s = strip_ref(t)
    n = normalize_type(s)
    if n == "bool":
        return TypeInfo("bool", n, "boolean")
    if n in NUMERIC_TYPES:
        return TypeInfo("number", n, "number")
    if n in STRING_TYPES:
        return TypeInfo("string", n, "string")
    if n in VALUE_TYPES:
        return TypeInfo("value", n, VALUE_TYPES[n])
    if n.endswith("*"):
        cls = resolve_object_class(n, object_classes)
        if cls:
            return TypeInfo("object", cxx_class_name(cls) + "*", cls.name, cls.name)
    return None


def classify_return(t: str, object_classes: Dict[str, Class]) -> Optional[TypeInfo]:
    n = strip_ref(t)
    if n in ("", "void"):
        return TypeInfo("void", "void", "()")
    if n == "bool":
        return TypeInfo("bool", n, "boolean")
    if n in NUMERIC_TYPES:
        return TypeInfo("number", n, "number")
    if n in STRING_TYPES:
        return TypeInfo("string", n, "string")
    if n in VALUE_TYPES:
        return TypeInfo("value", n, VALUE_TYPES[n])
    if n.endswith("*"):
        cls = resolve_object_class(n, object_classes)
        if cls:
            return TypeInfo("object", cxx_class_name(cls) + "*", f"{cls.name}?", cls.name)
    return None


def lua_arg_list(args: list[TypeInfo]) -> str:
    return ", ".join(f"arg{i}: {a.lua_type}" for i, a in enumerate(args, start=1))
