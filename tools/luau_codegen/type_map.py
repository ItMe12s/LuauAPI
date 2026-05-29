from __future__ import annotations

import re
from dataclasses import dataclass
from typing import Dict, Optional, Set

from broma_parser import Class
from model import short_name


WIDE_INTEGER_TYPES = {
    "long",
    "unsigned long",
    "long long",
    "unsigned long long",
    "size_t",
    "ptrdiff_t",
    "int64_t",
    "uint64_t",
}

NUMERIC_TYPES = {
    "char",
    "signed char",
    "unsigned char",
    "short",
    "unsigned short",
    "int",
    "unsigned int",
    "uint8_t",
    "uint16_t",
    "uint32_t",
    "int8_t",
    "int16_t",
    "int32_t",
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
    "UIButtonConfig": "UIButtonConfig",
}

GD_ENUM_TYPES = {
    "IconType",
    "UnlockType",
    "SearchType",
    "GJHttpType",
    "LikeItemType",
    "UserListType",
    "GJRewardType",
    "GJTimedLevelType",
    "GJMusicAction",
    "GJActionCommand",
    "GJSongError",
}

COCOS_ENUM_TYPES = {
    "enumKeyCodes",
    "CCTextAlignment",
    "CCTextFormatFlags",
    "CCTexture2DPixelFormat",
    "CCImageFormat",
    "CCLabelBMFontAlignment",
}

ENUM_CXX_NAMES: dict[str, str] = {
    **{name: name for name in GD_ENUM_TYPES},
    **{name: f"cocos2d::{name}" for name in COCOS_ENUM_TYPES},
    **{f"cocos2d::{name}": f"cocos2d::{name}" for name in COCOS_ENUM_TYPES},
}

ENUM_TYPES = frozenset(ENUM_CXX_NAMES.keys())


@dataclass(frozen=True)
class TypeInfo:
    kind: str
    cxx_type: str
    lua_type: str
    class_name: str = ""
    is_ref: bool = False
    is_out: bool = False


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


def is_reference_type(t: str) -> bool:
    return normalize_type(t).endswith("&")


def is_const_reference(t: str) -> bool:
    s = normalize_type(t)
    if not s.endswith("&"):
        return False
    inner = s[:-1].strip()
    return inner.startswith("const ") or inner.endswith(" const")


def is_out_reference(t: str) -> bool:
    return is_reference_type(t) and not is_const_reference(t)


def without_pointer(t: str) -> str:
    s = strip_ref(t)
    return s[:-1].strip() if s.endswith("*") else s


def enum_cxx_type(n: str, base: str) -> str:
    if n in ENUM_CXX_NAMES:
        return ENUM_CXX_NAMES[n]
    if base in ENUM_CXX_NAMES:
        return ENUM_CXX_NAMES[base]
    return "int"


def enum_lua_names(namespace: str) -> Set[str]:
    if namespace == "geode.cocos2d":
        return set(COCOS_ENUM_TYPES)
    return set(GD_ENUM_TYPES)


def cxx_class_name(cls: Class) -> str:
    return f"{cls.namespace}::{cls.name}" if cls.namespace else cls.name


def resolve_object_class(t: str, classes: Dict[str, Class]) -> Optional[Class]:
    base = without_pointer(t).lstrip(":")
    if base in classes:
        return classes[base]
    short = short_name(base)
    if short in classes:
        return classes[short]
    return None


def _classify_core(
    t: str, object_classes: Dict[str, Class], *, for_return: bool
) -> Optional[TypeInfo]:
    is_ref = is_reference_type(t)
    is_out = is_out_reference(t)
    s = strip_ref(t)
    n = normalize_type(s)
    base = short_name(without_pointer(n)) if n.endswith("*") else short_name(n)

    if n == "bool":
        return TypeInfo("bool", n, "boolean", is_ref=is_ref, is_out=is_out)
    if n in WIDE_INTEGER_TYPES:
        return TypeInfo("wideint", n, "string", is_ref=is_ref, is_out=is_out)
    if n in NUMERIC_TYPES:
        return TypeInfo("number", n, "number", is_ref=is_ref, is_out=is_out)
    if n in STRING_TYPES:
        return TypeInfo("string", n, "string", is_ref=is_ref, is_out=is_out)
    if n in VALUE_TYPES:
        return TypeInfo(
            "value",
            n,
            VALUE_TYPES.get(n, VALUE_TYPES.get(base, n)),
            is_ref=is_ref,
            is_out=is_out,
        )
    if base in ENUM_TYPES or n in ENUM_TYPES:
        cxx = enum_cxx_type(n, base)
        return TypeInfo("enum", cxx, "number", is_ref=is_ref, is_out=is_out)
    if n.endswith("*"):
        cls = resolve_object_class(n, object_classes)
        if cls:
            lua_suffix = "?" if for_return else ""
            return TypeInfo(
                "object",
                cxx_class_name(cls) + "*",
                f"{cls.name}{lua_suffix}",
                cls.name,
                is_ref=is_ref,
                is_out=is_out,
            )
    return None


def classify_arg(t: str, object_classes: Dict[str, Class]) -> Optional[TypeInfo]:
    return _classify_core(t, object_classes, for_return=False)


def classify_return(t: str, object_classes: Dict[str, Class]) -> Optional[TypeInfo]:
    n = strip_ref(t)
    if n in ("", "void"):
        return TypeInfo("void", "void", "()")
    info = _classify_core(t, object_classes, for_return=True)
    if info and info.kind != "void":
        if info.kind == "object" and not info.lua_type.endswith("?"):
            info = TypeInfo(
                info.kind,
                info.cxx_type,
                f"{info.class_name}?",
                info.class_name,
                info.is_ref,
                info.is_out,
            )
    return info


def require_classify_arg(t: str, object_classes: Dict[str, Class]) -> TypeInfo:
    info = classify_arg(t, object_classes)
    if info is None:
        raise ValueError(f"unsupported arg type: {t}")
    return info


def require_classify_return(t: str, object_classes: Dict[str, Class]) -> TypeInfo:
    info = classify_return(t, object_classes)
    if info is None:
        raise ValueError(f"unsupported return type: {t}")
    return info


def lua_arg_list(args: list[TypeInfo]) -> str:
    return ", ".join(f"arg{i}: {a.lua_type}" for i, a in enumerate(args, start=1))


def method_input_arg_count(method, object_classes: Dict[str, Class]) -> int:
    ret = classify_return(method.ret, object_classes)
    if ret is None:
        return len(method.args)
    count = 0
    for arg in method.args:
        info = classify_arg(arg.type, object_classes)
        if info and ret.kind == "void" and info.is_out:
            continue
        count += 1
    return count
