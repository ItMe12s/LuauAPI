from __future__ import annotations

import re
from dataclasses import dataclass, field
from typing import AbstractSet, Dict, Optional, Set, Tuple

from luau_codegen.parse.broma import Class, split_arg, split_top_level
from luau_codegen.model.domain import short_name

from luau_codegen.model.delegate_specs import lookup_delegate

SEL_TYPES: dict[str, tuple[str, str]] = {
    "SEL_MenuHandler": ("menu", "(sender: CCObject) -> ()"),
    "cocos2d::SEL_MenuHandler": ("menu", "(sender: CCObject) -> ()"),
    "SEL_SCHEDULE": ("schedule", "(dt: number) -> ()"),
    "cocos2d::SEL_SCHEDULE": ("schedule", "(dt: number) -> ()"),
    "SEL_CallFunc": ("callfunc", "() -> ()"),
    "cocos2d::SEL_CallFunc": ("callfunc", "() -> ()"),
    "SEL_CallFuncN": ("callfuncn", "(node: CCNode) -> ()"),
    "cocos2d::SEL_CallFuncN": ("callfuncn", "(node: CCNode) -> ()"),
    "SEL_CallFuncND": ("callfuncnd", "(node: CCNode, data: userdata) -> ()"),
    "cocos2d::SEL_CallFuncND": ("callfuncnd", "(node: CCNode, data: userdata) -> ()"),
    "SEL_CallFuncO": ("callfunco", "(obj: CCObject) -> ()"),
    "cocos2d::SEL_CallFuncO": ("callfunco", "(obj: CCObject) -> ()"),
}


def is_sel_type(t: str) -> bool:
    return normalize_type(t) in SEL_TYPES


def sel_variant(t: str) -> str:
    return SEL_TYPES[normalize_type(t)][0]


def sel_lua_type(t: str) -> str:
    return SEL_TYPES[normalize_type(t)][1]


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
    "cocos2d::ccColor4B": "RGBAColor",
    "ccColor4B": "RGBAColor",
    "UIButtonConfig": "UIButtonConfig",
}

CALLBACK_ALIASES: dict[str, str] = {
    "Callback": "std::function<void()>",
}

CLASS_CALLBACK_ALIASES: dict[str, dict[str, str]] = {
    "LazySprite": {
        "Callback": "geode::Function<void(geode::Result<>)>",
    },
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

FMOD_ENUM_TYPES = {
    "FMOD_RESULT",
    "FMOD_OPENSTATE",
    "FMOD_SPEAKERMODE",
}

OPAQUE_HANDLE_TYPES: dict[str, str] = {
    "FMOD::Channel*": "FMODChannel",
    "FMOD::Sound*": "FMODSound",
    "FMOD::ChannelGroup*": "FMODChannelGroup",
    "FMODSound*": "FMODSound",
}

GEODE_ENUM_TYPES: set[str] = set()

ENUM_CXX_NAMES: dict[str, str] = {
    **{name: name for name in GD_ENUM_TYPES},
    **{name: f"cocos2d::{name}" for name in COCOS_ENUM_TYPES},
    **{f"cocos2d::{name}": f"cocos2d::{name}" for name in COCOS_ENUM_TYPES},
    **{name: name for name in FMOD_ENUM_TYPES},
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


def _parse_result_type(n: str) -> Optional[TypeInfo]:
    s = strip_ref(n)
    for prefix in ("geode::Result<", "Result<"):
        if s.startswith(prefix) and s.endswith(">"):
            inner = s[len(prefix) : -1].strip()
            if inner not in ("", "void"):
                return None
            return TypeInfo("result", "geode::Result<>", "boolean | string")
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
    if ret_info is None:
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
    if ret_info.kind == "void":
        lua_ret = "()"
    else:
        lua_ret = ret_info.lua_type
    return TypeInfo(
        "callback",
        n,
        f"({lua_params}) -> {lua_ret}",
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
    if n in OPAQUE_HANDLE_TYPES:
        return TypeInfo(
            "opaque_handle",
            n,
            OPAQUE_HANDLE_TYPES[n],
            is_ref=is_ref,
            is_out=is_out,
        )
    result = _parse_result_type(n)
    if result is not None:
        return TypeInfo(
            result.kind,
            result.cxx_type,
            result.lua_type,
            is_ref=is_ref,
            is_out=is_out,
        )
    if not for_return:
        callback = _parse_callback(n, object_classes)
        if callback is not None:
            return callback
        if is_sel_type(n):
            return TypeInfo(
                "sel",
                n,
                sel_lua_type(n),
                class_name=sel_variant(n),
            )
    if n.endswith("*"):
        spec = lookup_delegate(n)
        if spec is not None:
            lua_table = _delegate_lua_type(spec)
            return TypeInfo(
                "delegate",
                n,
                lua_table,
                class_name=spec.lua_name,
                is_ref=is_ref,
                is_out=is_out,
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


def classify_arg(
    t: str, object_classes: Dict[str, Class], *, owner_class: str = ""
) -> Optional[TypeInfo]:
    n = normalize_type(t)
    if owner_class:
        class_aliases = CLASS_CALLBACK_ALIASES.get(owner_class, {})
        alias = class_aliases.get(n) or class_aliases.get(short_name(n))
        if alias:
            return classify_arg(alias, object_classes, owner_class=owner_class)
    alias = CALLBACK_ALIASES.get(n) or CALLBACK_ALIASES.get(short_name(n))
    if alias:
        return classify_arg(alias, object_classes, owner_class=owner_class)
    return _classify_core(t, object_classes, for_return=False)


def _delegate_lua_type(spec) -> str:
    fields = []
    for m in spec.methods:
        params = ", ".join(f"arg{i}: {t}" for i, t in enumerate(m.args_lua, start=1))
        ret = m.ret_lua
        fn = f"({params}) -> ()" if ret == "()" else f"({params}) -> {ret}"
        fields.append(f"{m.name}: ({fn})?")
    return "{ " + ", ".join(fields) + " }"


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
        elif info.kind == "opaque_handle" and not info.lua_type.endswith("?"):
            info = TypeInfo(
                info.kind,
                info.cxx_type,
                f"{info.lua_type}?",
                is_ref=info.is_ref,
                is_out=info.is_out,
                is_vector_ptr=info.is_vector_ptr,
            )
        elif info.kind == "delegate":
            info = TypeInfo(
                info.kind,
                info.cxx_type,
                f"{info.lua_type}?",
                info.class_name,
                info.is_ref,
                info.is_out,
                info.is_vector_ptr,
            )
    return info


def require_classify_arg(
    t: str, object_classes: Dict[str, Class], *, owner_class: str = ""
) -> TypeInfo:
    info = classify_arg(t, object_classes, owner_class=owner_class)
    if info is None:
        raise ValueError(f"unsupported arg type: {t}")
    return info


def require_classify_return(t: str, object_classes: Dict[str, Class]) -> TypeInfo:
    info = classify_return(t, object_classes)
    if info is None:
        raise ValueError(f"unsupported return type: {t}")
    return info


def method_input_arg_count(
    method, object_classes: Dict[str, Class], *, owner_class: str = ""
) -> int:
    from luau_codegen.convert.sel_args import count_lua_method_args

    ret = classify_return(method.ret, object_classes)
    if ret is None:
        return len(method.args)
    return count_lua_method_args(
        method, object_classes, ret.kind, owner_class=owner_class
    )
