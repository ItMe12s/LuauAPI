from __future__ import annotations

import re
from dataclasses import dataclass, field
from typing import TYPE_CHECKING, Dict, Optional, Set, Tuple

from luau_codegen.parse.broma import Class, split_arg, split_top_level
from luau_codegen.model.domain import short_name

from luau_codegen.model.delegate_specs import lookup_delegate
from luau_codegen.model.cc_c_array import (
    CC_C_ARRAY_POINTER_TYPES,
    proven_cc_c_array_element,
)
from luau_codegen.model.nested_containers import (
    allow_nested_map_value,
    allow_nested_primitive_vector_outer,
)
from luau_codegen.model.pair_design import (
    PAIR_COMPONENT_KINDS,
    pair_key_map_entry_lua_type,
    pair_lua_type,
)
from luau_codegen.model.value_struct_gate import (
    GATED_VALUE_CHECK_CXX,
    GATED_VALUE_STRUCTS,
)

if TYPE_CHECKING:
    from luau_codegen.model.codegen_context import CodegenContext

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

UNSIGNED_NUMERIC_TYPES = frozenset(
    t for t in NUMERIC_TYPES if "unsigned" in t or t.startswith("uint")
)

STRING_TYPES = {
    "char const*",
    "const char*",
    "std::string",
    "gd::string",
    "std::string_view",
    "ZStringView",
    "geode::ZStringView",
}
VALUE_CHECK_CXX_TYPES: dict[str, str] = {
    "CCPoint": "cocos2d::CCPoint",
    "CCSize": "cocos2d::CCSize",
    "CCRect": "cocos2d::CCRect",
    "RGBColor": "cocos2d::ccColor3B",
    "RGBAColor": "cocos2d::ccColor4B",
    "RGBAFloatColor": "cocos2d::ccColor4F",
    "BlendFunc": "cocos2d::ccBlendFunc",
    "HSVValue": "cocos2d::ccHSVValue",
    "CCAffineTransform": "cocos2d::CCAffineTransform",
    "UIButtonConfig": "UIButtonConfig",
    **GATED_VALUE_CHECK_CXX,
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
    "cocos2d::ccColor4F": "RGBAFloatColor",
    "ccColor4F": "RGBAFloatColor",
    "cocos2d::ccBlendFunc": "BlendFunc",
    "ccBlendFunc": "BlendFunc",
    "cocos2d::ccHSVValue": "HSVValue",
    "ccHSVValue": "HSVValue",
    "cocos2d::CCAffineTransform": "CCAffineTransform",
    "CCAffineTransform": "CCAffineTransform",
    "UIButtonConfig": "UIButtonConfig",
    **GATED_VALUE_STRUCTS,
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
    "AudioSortType",
    "BoomListType",
    "ChestSpriteState",
    "CircleMode",
    "CommentKeyType",
    "CommentType",
    "CurrencyRewardType",
    "CurrencySpriteType",
    "DialogAnimationType",
    "EasingType",
    "FormatterType",
    "GameObjectClassType",
    "GameObjectType",
    "GameOptionsSetting",
    "GJActionCommand",
    "GJChallengeType",
    "GJDifficulty",
    "GJFeatureState",
    "GauntletType",
    "GJHttpType",
    "GJLevelType",
    "GJMusicAction",
    "GJRewardType",
    "GJSongError",
    "GJSongType",
    "GJTimedLevelType",
    "GhostType",
    "IconType",
    "InputValueType",
    "LeaderboardStat",
    "LeaderboardType",
    "LevelLeaderboardMode",
    "LevelLeaderboardType",
    "LikeItemType",
    "MenuAnimationType",
    "MoveTargetType",
    "PlaybackMode",
    "PlayerButton",
    "ScaleButtonType",
    "SearchType",
    "SelectArtType",
    "SelectSettingType",
    "ShipStreak",
    "ShopType",
    "SpecialRewardItem",
    "Speed",
    "TextStyleType",
    "TouchTriggerControl",
    "TouchTriggerType",
    "UndoCommand",
    "UnlockType",
    "UserListType",
    "ZLayer",
    "spriteMode",
}

COCOS_ENUM_TYPES = {
    "enumKeyCodes",
    "CCTextAlignment",
    "CCTextFormatFlags",
    "CCTexture2DPixelFormat",
    "CCImageFormat",
    "CCLabelBMFontAlignment",
    "TextureQuality",
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
    "cocos2d::CCEvent*": "CCEvent",
    "cocos2d::extension::CCEditBox*": "CCEditBox",
    "GroupCommandObject2*": "GroupCommandObject2",
    "DelayedSpawnNode*": "DelayedSpawnNode",
}

STATIC_ENUM_CXX_NAMES: dict[str, str] = {
    **{name: name for name in GD_ENUM_TYPES},
    **{name: f"cocos2d::{name}" for name in COCOS_ENUM_TYPES},
    **{f"cocos2d::{name}": f"cocos2d::{name}" for name in COCOS_ENUM_TYPES},
    **{name: name for name in FMOD_ENUM_TYPES},
}

# Back-compat alias for tests and callers that referenced the static table name.
ENUM_CXX_NAMES = STATIC_ENUM_CXX_NAMES


def _resolve_ctx(ctx: CodegenContext | None) -> CodegenContext:
    if ctx is not None:
        return ctx
    from luau_codegen.model.codegen_context import CodegenContext as Ctx

    return Ctx.static()


def register_geode_enums(
    names_to_cxx: Dict[str, str],
    skip: frozenset[str] | set[str] = frozenset(),
) -> CodegenContext:
    from luau_codegen.model.codegen_context import CodegenContext as Ctx

    return Ctx.with_geode_enums(names_to_cxx, skip=skip)


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
    key_type: Optional["TypeInfo"] = None
    value_type: Optional["TypeInfo"] = None
    array_size: int = 0


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
    s = re.sub(r"\s+", " ", t.strip())
    s = s.replace(" *", "*").replace("* ", "*")
    s = s.replace(" &", "&").replace("& ", "&")
    s = s.replace(" :: ", "::").replace(":: ", "::").replace(" ::", "::")
    if not s.endswith("&"):
        return False
    inner = s[:-1].strip()
    return inner.startswith("const ") or inner.endswith(" const")


def is_out_reference(t: str) -> bool:
    return is_reference_type(t) and not is_const_reference(t)


def without_pointer(t: str) -> str:
    s = strip_ref(t)
    return s[:-1].strip() if s.endswith("*") else s


def enum_cxx_type(n: str, base: str, ctx: CodegenContext | None = None) -> str:
    return _resolve_ctx(ctx).enum_cxx_type(n, base)


def enum_lua_names(namespace: str, ctx: CodegenContext | None = None) -> Set[str]:
    return _resolve_ctx(ctx).enum_lua_names(namespace)


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


_PRIMITIVE_VECTOR_ELEMENT_KINDS = frozenset(
    {"bool", "number", "wideint", "string", "enum", "value", "pair"}
)

_STD_ARRAY_ELEMENT_KINDS = _PRIMITIVE_VECTOR_ELEMENT_KINDS

STD_ARRAY_MAX_SIZE = 400

_MAP_KEY_KINDS = frozenset({"bool", "number", "wideint", "string", "enum"})

_MAP_VALUE_KINDS = _PRIMITIVE_VECTOR_ELEMENT_KINDS | {"object"}

_SET_ELEMENT_KINDS = _PRIMITIVE_VECTOR_ELEMENT_KINDS | {"object"}

_NESTED_CONTAINER_KINDS = frozenset(
    {
        "vector_view",
        "primitive_vector",
        "map",
        "unordered_map",
        "set",
        "unordered_set",
    }
)

_MAP_CONTAINER_PREFIXES = (
    ("gd::map", "map"),
    ("gd::unordered_map", "unordered_map"),
)

_SET_CONTAINER_PREFIXES = (
    ("gd::set", "set"),
    ("gd::unordered_set", "unordered_set"),
)


def _is_nested_container(info: TypeInfo) -> bool:
    return info.kind in _NESTED_CONTAINER_KINDS


def _map_value_nested_allowed(key: TypeInfo, value: TypeInfo) -> bool:
    if not _is_nested_container(value):
        return False
    return allow_nested_map_value(key, value)


def _pair_component_lua_type(info: TypeInfo) -> str:
    if info.kind == "object":
        return f"{info.class_name}?"
    if info.kind == "pair":
        if info.key_type is None or info.value_type is None:
            return info.lua_type
        return pair_lua_type(
            _pair_component_lua_type(info.key_type),
            _pair_component_lua_type(info.value_type),
        )
    return info.lua_type


def _map_value_lua_type(value: TypeInfo) -> str:
    if value.kind in (
        "primitive_vector",
        "vector_view",
        "nested_primitive_vector_view",
    ):
        return value.lua_type
    if value.kind == "object":
        return f"{value.class_name}?"
    if value.kind == "pair":
        if value.key_type is None or value.value_type is None:
            return value.lua_type
        return pair_lua_type(
            _pair_component_lua_type(value.key_type),
            _pair_component_lua_type(value.value_type),
        )
    return value.lua_type


def _map_lua_type(key: TypeInfo, value: TypeInfo) -> str:
    if key.kind == "pair":
        if key.key_type is None or key.value_type is None:
            return key.lua_type
        entry = pair_key_map_entry_lua_type(
            _pair_component_lua_type(key.key_type),
            _pair_component_lua_type(key.value_type),
            _map_value_lua_type(value),
        )
        return f"{{ {entry} }}"
    return f"{{ [{key.lua_type}]: {_map_value_lua_type(value)} }}"


def _set_element_lua_type(element: TypeInfo) -> str:
    if element.kind == "object":
        return f"{element.class_name}?"
    if element.kind == "pair":
        if element.key_type is None or element.value_type is None:
            return element.lua_type
        return pair_lua_type(
            _pair_component_lua_type(element.key_type),
            _pair_component_lua_type(element.value_type),
        )
    return element.lua_type


def _parse_std_pair(
    n: str,
    object_classes: Dict[str, Class],
    ctx: CodegenContext | None = None,
) -> Optional[TypeInfo]:
    inner = _template_inner(n, "std::pair")
    if inner is None:
        return None
    parts = split_top_level(inner)
    if len(parts) != 2:
        return None
    first = classify_arg(parts[0], object_classes, ctx=ctx)
    second = classify_arg(parts[1], object_classes, ctx=ctx)
    if first is None or second is None:
        return None
    if (
        first.kind not in PAIR_COMPONENT_KINDS
        or second.kind not in PAIR_COMPONENT_KINDS
    ):
        return None
    if _is_nested_container(first) or _is_nested_container(second):
        return None
    return TypeInfo(
        "pair",
        f"std::pair<{first.cxx_type}, {second.cxx_type}>",
        pair_lua_type(
            _pair_component_lua_type(first),
            _pair_component_lua_type(second),
        ),
        key_type=first,
        value_type=second,
    )


def _parse_map_container(
    n: str,
    prefix: str,
    kind: str,
    object_classes: Dict[str, Class],
    ctx: CodegenContext | None = None,
) -> Optional[TypeInfo]:
    inner = _template_inner(n, prefix)
    if inner is None:
        return None
    parts = split_top_level(inner)
    if len(parts) != 2:
        return None
    key = classify_arg(parts[0], object_classes, ctx=ctx)
    value = classify_arg(parts[1], object_classes, ctx=ctx)
    if key is None or value is None:
        return None
    if key.kind == "pair":
        if key.key_type is None or key.value_type is None:
            return None
        if _is_nested_container(value):
            if not _map_value_nested_allowed(key, value):
                return None
        elif value.kind not in _MAP_VALUE_KINDS:
            return None
    else:
        if key.kind not in _MAP_KEY_KINDS or _is_nested_container(key):
            return None
        if _is_nested_container(value):
            if not _map_value_nested_allowed(key, value):
                return None
        elif value.kind not in _MAP_VALUE_KINDS:
            return None
    return TypeInfo(
        kind,
        f"{prefix}<{key.cxx_type}, {value.cxx_type}>",
        _map_lua_type(key, value),
        key_type=key,
        value_type=value,
    )


def _parse_set_container(
    n: str,
    prefix: str,
    kind: str,
    object_classes: Dict[str, Class],
    ctx: CodegenContext | None = None,
) -> Optional[TypeInfo]:
    inner = _template_inner(n, prefix)
    if inner is None:
        return None
    parts = split_top_level(inner)
    if len(parts) != 1:
        return None
    element = classify_arg(parts[0], object_classes, ctx=ctx)
    if element is None or element.kind not in _SET_ELEMENT_KINDS:
        return None
    if _is_nested_container(element):
        return None
    return TypeInfo(
        kind,
        f"{prefix}<{element.cxx_type}>",
        f"{{ {_set_element_lua_type(element)} }}",
        element_type=element,
    )


def _parse_container(
    n: str,
    object_classes: Dict[str, Class],
    ctx: CodegenContext | None = None,
) -> Optional[TypeInfo]:
    for prefix, kind in _MAP_CONTAINER_PREFIXES:
        parsed = _parse_map_container(n, prefix, kind, object_classes, ctx=ctx)
        if parsed is not None:
            return parsed
    for prefix, kind in _SET_CONTAINER_PREFIXES:
        parsed = _parse_set_container(n, prefix, kind, object_classes, ctx=ctx)
        if parsed is not None:
            return parsed
    return None


def _with_container_ref_flags(
    info: TypeInfo,
    *,
    is_ref: bool,
    is_out: bool,
    is_vector_ptr: bool = False,
) -> TypeInfo:
    return TypeInfo(
        info.kind,
        info.cxx_type,
        info.lua_type,
        info.class_name,
        is_ref,
        is_out,
        is_vector_ptr,
        info.callback_ret,
        info.callback_args,
        element_type=info.element_type,
        key_type=info.key_type,
        value_type=info.value_type,
        array_size=info.array_size,
    )


def _parse_std_array(
    n: str,
    object_classes: Dict[str, Class],
    ctx: CodegenContext | None = None,
) -> Optional[TypeInfo]:
    inner = _template_inner(n, "std::array")
    if inner is None:
        return None
    parts = split_top_level(inner)
    if len(parts) != 2:
        return None
    size_str = parts[1].strip()
    if not size_str.isdigit():
        return None
    size = int(size_str)
    if size <= 0 or size > STD_ARRAY_MAX_SIZE:
        return None
    element = classify_arg(parts[0].strip(), object_classes, ctx=ctx)
    if element is None or element.kind not in _STD_ARRAY_ELEMENT_KINDS:
        return None
    if _is_nested_container(element):
        return None
    return TypeInfo(
        "std_array",
        f"std::array<{element.cxx_type}, {size}>",
        f"{{ {element.lua_type} }}",
        element_type=element,
        array_size=size,
    )


def _parse_primitive_vector(
    n: str,
    object_classes: Dict[str, Class],
    ctx: CodegenContext | None = None,
) -> Optional[TypeInfo]:
    inner = _template_inner(n, "gd::vector")
    if inner is None:
        return None
    parts = split_top_level(inner)
    if len(parts) != 1:
        return None
    element = classify_arg(parts[0], object_classes, ctx=ctx)
    if element is None or element.kind not in _PRIMITIVE_VECTOR_ELEMENT_KINDS:
        return None
    return TypeInfo(
        "primitive_vector",
        f"gd::vector<{element.cxx_type}>",
        f"{{ {element.lua_type} }}",
        element_type=element,
    )


def _parse_cc_c_array_view(
    n: str,
    owner_class: str,
    field_name: str,
    object_classes: Dict[str, Class],
    ctx: CodegenContext | None = None,
) -> Optional[TypeInfo]:
    if normalize_type(n) not in CC_C_ARRAY_POINTER_TYPES:
        return None
    element_short = proven_cc_c_array_element(owner_class, field_name)
    if element_short is None:
        return None
    element = classify_arg(f"{element_short}*", object_classes, ctx=ctx)
    if element is None or element.kind != "object":
        return None
    return TypeInfo(
        "cc_c_array_view",
        "cocos2d::ccCArray*",
        f"{{ {element.class_name}? }}",
        element.class_name,
        element_type=element,
    )


def _parse_vector_view(
    n: str,
    object_classes: Dict[str, Class],
    ctx: CodegenContext | None = None,
) -> Optional[TypeInfo]:
    inner = _template_inner(n, "gd::vector")
    if inner is None:
        return None
    parts = split_top_level(inner)
    if len(parts) != 1:
        return None
    element = classify_arg(parts[0], object_classes, ctx=ctx)
    if element is None or element.kind not in ("object", "opaque_handle"):
        return None
    if element.kind == "object":
        lua_type = f"{{ {element.class_name}? }}"
        class_name = element.class_name
    else:
        lua_type = f"{{ {element.lua_type}? }}"
        class_name = element.lua_type
    return TypeInfo(
        "vector_view",
        f"gd::vector<{element.cxx_type}>",
        lua_type,
        class_name,
        element_type=element,
    )


def _parse_nested_primitive_vector_view(
    n: str,
    object_classes: Dict[str, Class],
    ctx: CodegenContext | None = None,
) -> Optional[TypeInfo]:
    if not allow_nested_primitive_vector_outer(n):
        return None
    inner = _template_inner(n, "gd::vector")
    if inner is None:
        return None
    parts = split_top_level(inner)
    if len(parts) != 1:
        return None
    elem = parts[0].strip()
    if not elem.endswith("*"):
        return None
    inner_vec = _parse_primitive_vector(elem[:-1].strip(), object_classes, ctx=ctx)
    if inner_vec is None or inner_vec.element_type is None:
        return None
    if inner_vec.element_type.kind != "number":
        return None
    return TypeInfo(
        "nested_primitive_vector_view",
        n,
        f"{{ {inner_vec.lua_type} }}",
        element_type=inner_vec,
    )


def _parse_callback(
    n: str, object_classes: Dict[str, Class], ctx: CodegenContext | None = None
) -> Optional[TypeInfo]:
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
    ret_info = classify_return(ret_str, object_classes, ctx=ctx)
    if ret_info is None:
        return None
    arg_infos = []
    for raw in split_top_level(inner[paren + 1 : close]):
        raw = raw.strip()
        if not raw:
            continue
        parsed = split_arg(raw)
        info = classify_arg(parsed.type, object_classes, ctx=ctx)
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
    t: str,
    object_classes: Dict[str, Class],
    *,
    for_return: bool,
    ctx: CodegenContext | None = None,
    owner_class: str = "",
    field_name: str = "",
) -> Optional[TypeInfo]:
    resolved = _resolve_ctx(ctx)
    is_ref = is_reference_type(t)
    is_out = is_out_reference(t)
    s = strip_ref(t)
    n = normalize_type(s)
    base = short_name(without_pointer(n)) if n.endswith("*") else short_name(n)

    if n.endswith("*"):
        base = n[:-1].strip()
        ptr_container = _parse_container(base, object_classes, ctx=ctx)
        if ptr_container is not None:
            return _with_container_ref_flags(
                ptr_container,
                is_ref=False,
                is_out=True,
                is_vector_ptr=True,
            )
        ptr_primitive = _parse_primitive_vector(base, object_classes, ctx=ctx)
        if ptr_primitive is not None:
            return TypeInfo(
                ptr_primitive.kind,
                ptr_primitive.cxx_type,
                ptr_primitive.lua_type,
                ptr_primitive.class_name,
                is_ref=False,
                is_out=True,
                is_vector_ptr=True,
                element_type=ptr_primitive.element_type,
            )
        ptr_std_array = _parse_std_array(base, object_classes, ctx=ctx)
        if ptr_std_array is not None:
            return TypeInfo(
                ptr_std_array.kind,
                ptr_std_array.cxx_type,
                ptr_std_array.lua_type,
                ptr_std_array.class_name,
                is_ref=False,
                is_out=True,
                is_vector_ptr=True,
                element_type=ptr_std_array.element_type,
                array_size=ptr_std_array.array_size,
            )
        ptr_nested = _parse_nested_primitive_vector_view(base, object_classes, ctx=ctx)
        if ptr_nested is not None:
            return TypeInfo(
                ptr_nested.kind,
                ptr_nested.cxx_type,
                ptr_nested.lua_type,
                ptr_nested.class_name,
                is_ref=False,
                is_out=True,
                is_vector_ptr=True,
                element_type=ptr_nested.element_type,
            )
        ptr_vector = _parse_vector_view(base, object_classes, ctx=ctx)
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

    std_array = _parse_std_array(n, object_classes, ctx=ctx)
    if std_array is not None:
        return TypeInfo(
            std_array.kind,
            std_array.cxx_type,
            std_array.lua_type,
            std_array.class_name,
            is_ref,
            is_out,
            element_type=std_array.element_type,
            array_size=std_array.array_size,
        )

    nested_primitive_vector_view = _parse_nested_primitive_vector_view(
        n, object_classes, ctx=ctx
    )
    if nested_primitive_vector_view is not None:
        return TypeInfo(
            nested_primitive_vector_view.kind,
            nested_primitive_vector_view.cxx_type,
            nested_primitive_vector_view.lua_type,
            nested_primitive_vector_view.class_name,
            is_ref,
            is_out,
            element_type=nested_primitive_vector_view.element_type,
        )

    primitive_vector = _parse_primitive_vector(n, object_classes, ctx=ctx)
    if primitive_vector is not None:
        return TypeInfo(
            primitive_vector.kind,
            primitive_vector.cxx_type,
            primitive_vector.lua_type,
            primitive_vector.class_name,
            is_ref,
            is_out,
            element_type=primitive_vector.element_type,
        )

    vector_view = _parse_vector_view(n, object_classes, ctx=ctx)
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

    cc_c_array_view = _parse_cc_c_array_view(
        n, owner_class, field_name, object_classes, ctx=ctx
    )
    if cc_c_array_view is not None:
        return TypeInfo(
            cc_c_array_view.kind,
            cc_c_array_view.cxx_type,
            cc_c_array_view.lua_type,
            cc_c_array_view.class_name,
            is_ref,
            is_out,
            element_type=cc_c_array_view.element_type,
        )

    container = _parse_container(n, object_classes, ctx=ctx)
    if container is not None:
        return _with_container_ref_flags(
            container,
            is_ref=is_ref,
            is_out=is_out,
        )
    pair_type = _parse_std_pair(n, object_classes, ctx=ctx)
    if pair_type is not None:
        return TypeInfo(
            pair_type.kind,
            pair_type.cxx_type,
            pair_type.lua_type,
            pair_type.class_name,
            is_ref=is_ref,
            is_out=is_out,
            key_type=pair_type.key_type,
            value_type=pair_type.value_type,
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
    if (
        base in resolved.enum_types or n in resolved.enum_types
    ) and resolve_object_class(base, object_classes) is None:
        cxx = resolved.enum_cxx_type(n, base)
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
        callback = _parse_callback(n, object_classes, ctx=ctx)
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
    t: str,
    object_classes: Dict[str, Class],
    *,
    owner_class: str = "",
    field_name: str = "",
    ctx: CodegenContext | None = None,
) -> Optional[TypeInfo]:
    n = normalize_type(t)
    if owner_class:
        class_aliases = CLASS_CALLBACK_ALIASES.get(owner_class, {})
        alias = class_aliases.get(n) or class_aliases.get(short_name(n))
        if alias:
            return classify_arg(
                alias,
                object_classes,
                owner_class=owner_class,
                field_name=field_name,
                ctx=ctx,
            )
    alias = CALLBACK_ALIASES.get(n) or CALLBACK_ALIASES.get(short_name(n))
    if alias:
        return classify_arg(
            alias,
            object_classes,
            owner_class=owner_class,
            field_name=field_name,
            ctx=ctx,
        )
    return _classify_core(
        t,
        object_classes,
        for_return=False,
        ctx=ctx,
        owner_class=owner_class,
        field_name=field_name,
    )


def _delegate_lua_type(spec) -> str:
    fields = []
    for m in spec.methods:
        params = ", ".join(f"arg{i}: {t}" for i, t in enumerate(m.args_lua, start=1))
        ret = m.ret_lua
        fn = f"({params}) -> ()" if ret == "()" else f"({params}) -> {ret}"
        fields.append(f"{m.name}: ({fn})?")
    return "{ " + ", ".join(fields) + " }"


def classify_return(
    t: str,
    object_classes: Dict[str, Class],
    *,
    owner_class: str = "",
    field_name: str = "",
    ctx: CodegenContext | None = None,
) -> Optional[TypeInfo]:
    n = strip_ref(t)
    if n in ("", "void"):
        return TypeInfo("void", "void", "()")
    info = _classify_core(
        t,
        object_classes,
        for_return=True,
        ctx=ctx,
        owner_class=owner_class,
        field_name=field_name,
    )
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
    t: str,
    object_classes: Dict[str, Class],
    *,
    owner_class: str = "",
    ctx: CodegenContext | None = None,
) -> TypeInfo:
    info = classify_arg(t, object_classes, owner_class=owner_class, ctx=ctx)
    if info is None:
        raise ValueError(f"unsupported arg type: {t}")
    return info


def require_classify_return(
    t: str, object_classes: Dict[str, Class], *, ctx: CodegenContext | None = None
) -> TypeInfo:
    info = classify_return(t, object_classes, ctx=ctx)
    if info is None:
        raise ValueError(f"unsupported return type: {t}")
    return info


def method_input_arg_count(
    method,
    object_classes: Dict[str, Class],
    *,
    owner_class: str = "",
    ctx: CodegenContext | None = None,
) -> int:
    from luau_codegen.convert.sel_args import count_lua_method_args

    ret = classify_return(method.ret, object_classes, ctx=ctx)
    if ret is None:
        return len(method.args)
    return count_lua_method_args(
        method, object_classes, ret.kind, owner_class=owner_class, ctx=ctx
    )
