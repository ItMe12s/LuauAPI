from __future__ import annotations

import re
from dataclasses import dataclass, field
from typing import AbstractSet, Dict, Optional, Set, Tuple

from luau_codegen.parse.broma import Class, split_arg, split_top_level
from luau_codegen.model.domain import short_name

SEL_MENU_HANDLER_TYPES = frozenset(
    {
        "SEL_MenuHandler",
        "cocos2d::SEL_MenuHandler",
    }
)


def is_sel_menu_handler(t: str) -> bool:
    return normalize_type(t) in SEL_MENU_HANDLER_TYPES


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

STRING_TYPES = {
    "char const*",
    "const char*",
    "std::string",
    "gd::string",
    "std::string_view",
    "ZStringView",
    "geode::ZStringView",
}
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

GEODE_ENUM_TYPES: set[str] = set()

ENUM_CXX_NAMES: dict[str, str] = {
    **{name: name for name in GD_ENUM_TYPES},
    **{name: f"cocos2d::{name}" for name in COCOS_ENUM_TYPES},
    **{f"cocos2d::{name}": f"cocos2d::{name}" for name in COCOS_ENUM_TYPES},
}

ENUM_TYPES = frozenset(ENUM_CXX_NAMES.keys())


def register_geode_enums(
    names_to_cxx: Dict[str, str],
    skip: AbstractSet[str] = frozenset(),
) -> None:
    global ENUM_TYPES
    GEODE_ENUM_TYPES.clear()
    for name, cxx in names_to_cxx.items():
        if name in skip:
            continue
        GEODE_ENUM_TYPES.add(name)
        ENUM_CXX_NAMES.setdefault(name, cxx)
    ENUM_TYPES = frozenset(ENUM_CXX_NAMES.keys())


@dataclass(frozen=True)
class TypeInfo:
    kind: str
    cxx_type: str
    lua_type: str
    class_name: str = ""
    is_ref: bool = False
    is_out: bool = False
    is_vector_ptr: bool = False
    callback_ret: Optional["TypeInfo"] = None
    callback_args: Tuple["TypeInfo", ...] = field(default_factory=tuple)
    element_type: Optional["TypeInfo"] = None


_CALLBACK_PREFIXES = (
    "geode::Function<",
    "Function<",
    "std::function<",
    "geode::utils::MiniFunction<",
    "utils::MiniFunction<",
    "MiniFunction<",
    "std::move_only_function<",
)


def _callback_inner(n: str) -> Optional[str]:
    start = n.find("<")
    if start == -1 or (n[:start] + "<") not in _CALLBACK_PREFIXES:
        return None
    depth = 0
    for i in range(start, len(n)):
        c = n[i]
        if c == "<":
            depth += 1
        elif c == ">":
            depth -= 1
            if depth == 0:
                return n[start + 1 : i]
    return None


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
    if namespace == "geode":
        return set(GEODE_ENUM_TYPES)
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


def _template_inner(n: str, prefix: str) -> Optional[str]:
    marker = f"{prefix}<"
    if not n.startswith(marker) or not n.endswith(">"):
        return None
    depth = 0
    for i, c in enumerate(n[len(prefix) :], start=len(prefix)):
        if c == "<":
            depth += 1
        elif c == ">":
            depth -= 1
            if depth == 0 and i == len(n) - 1:
                return n[len(marker) : i]
    return None


def _parse_vector_view(n: str, object_classes: Dict[str, Class]) -> Optional[TypeInfo]:
    inner = _template_inner(n, "gd::vector")
    if inner is None:
        return None
    parts = split_top_level(inner)
    if len(parts) != 1:
        return None
    element = classify_arg(parts[0], object_classes)
    if element is None or element.kind != "object":
        return None
    return TypeInfo(
        "vector_view",
        f"gd::vector<{element.cxx_type}>",
        f"{{ {element.class_name}? }}",
        element.class_name,
        element_type=element,
    )


def _parse_callback(n: str, object_classes: Dict[str, Class]) -> Optional[TypeInfo]:
    inner = _callback_inner(n)
    if inner is None:
        return None
    depth = 0
    paren = -1
    for i, c in enumerate(inner):
        if c == "<":
            depth += 1
        elif c == ">":
            depth -= 1
        elif c == "(" and depth == 0:
            paren = i
            break
    if paren == -1:
        return None
    ret_str = inner[:paren].strip()
    pdepth = 0
    close = -1
    for i in range(paren, len(inner)):
        if inner[i] == "(":
            pdepth += 1
        elif inner[i] == ")":
            pdepth -= 1
            if pdepth == 0:
                close = i
                break
    if close == -1:
        return None
    ret_info = classify_return(ret_str, object_classes)
    if ret_info is None or ret_info.kind != "void":
        return None
    arg_infos = []
    for raw in split_top_level(inner[paren + 1 : close]):
        raw = raw.strip()
        if not raw:
            continue
        parsed = split_arg(raw)
        info = classify_arg(parsed.type, object_classes)
        if info is None or info.kind == "callback":
            return None
        arg_infos.append(info)
    lua_params = ", ".join(
        f"arg{i}: {ai.lua_type}" for i, ai in enumerate(arg_infos, start=1)
    )
    return TypeInfo(
        "callback",
        n,
        f"({lua_params}) -> ()",
        callback_ret=ret_info,
        callback_args=tuple(arg_infos),
    )


def _classify_core(
    t: str, object_classes: Dict[str, Class], *, for_return: bool
) -> Optional[TypeInfo]:
    is_ref = is_reference_type(t)
    is_out = is_out_reference(t)
    s = strip_ref(t)
    n = normalize_type(s)
    base = short_name(without_pointer(n)) if n.endswith("*") else short_name(n)

    if n.endswith("*"):
        base = n[:-1].strip()
        ptr_vector = _parse_vector_view(base, object_classes)
        if ptr_vector is not None:
            return TypeInfo(
                ptr_vector.kind,
                ptr_vector.cxx_type,
                ptr_vector.lua_type,
                ptr_vector.class_name,
                is_ref=False,
                is_out=True,
                is_vector_ptr=True,
                element_type=ptr_vector.element_type,
            )

    vector_view = _parse_vector_view(n, object_classes)
    if vector_view is not None:
        return TypeInfo(
            vector_view.kind,
            vector_view.cxx_type,
            vector_view.lua_type,
            vector_view.class_name,
            is_ref,
            is_out,
            element_type=vector_view.element_type,
        )
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
    if not for_return:
        callback = _parse_callback(n, object_classes)
        if callback is not None:
            return callback
        if is_sel_menu_handler(n):
            sender = classify_arg("cocos2d::CCObject*", object_classes)
            if sender is None:
                return None
            return TypeInfo(
                "sel",
                n,
                "(sender: CCObject) -> ()",
                callback_args=(sender,),
            )
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
                info.is_vector_ptr,
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


def method_input_arg_count(method, object_classes: Dict[str, Class]) -> int:
    from luau_codegen.convert.sel_args import count_lua_method_args

    ret = classify_return(method.ret, object_classes)
    if ret is None:
        return len(method.args)
    return count_lua_method_args(method, object_classes, ret.kind)
