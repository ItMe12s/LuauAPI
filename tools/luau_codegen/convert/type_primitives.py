from __future__ import annotations

import re
from dataclasses import dataclass, field
from collections.abc import Iterator

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
CALLBACK_ALIASES: dict[str, str] = {"Callback": "std::function<void()>"}
CLASS_CALLBACK_ALIASES: dict[str, dict[str, str]] = {
    "LazySprite": {"Callback": "geode::Function<void(geode::Result<>)>"},
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
    "EditCommand",
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
    "CCObjectType",
    "CCVerticalTextAlignment",
    "ccGLServerState",
    "ccScriptType",
    "ccTouchesMode",
    "tCCMenuState",
    "tCCPositionType",
    "TextureQuality",
}
FMOD_ENUM_TYPES = {"FMOD_RESULT", "FMOD_OPENSTATE", "FMOD_SPEAKERMODE"}
OPAQUE_HANDLE_TYPES: dict[str, str] = {
    "FMOD::Channel*": "FMODChannel",
    "FMOD::Sound*": "FMODSoundHandle",
    "FMOD::System*": "FMODSystem",
    "FMOD::DSP*": "FMODDSP",
    "FMOD::ChannelGroup*": "FMODChannelGroup",
    "FMODSound*": "FMODSoundHandle",
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
COMPOSITE_KINDS = frozenset(
    {
        "vector_view",
        "vector",
        "std_array",
        "map",
        "unordered_map",
        "set",
        "unordered_set",
        "pair",
        "tuple",
    }
)


@dataclass(frozen=True)
class TypeInfo:
    kind: str
    cxx_type: str
    lua_type: str
    class_name: str = ""
    is_ref: bool = False
    is_out: bool = False
    is_vector_ptr: bool = False
    callback_ret: TypeInfo | None = None
    callback_args: tuple[TypeInfo, ...] = field(default_factory=tuple)
    element_type: TypeInfo | None = None
    key_type: TypeInfo | None = None
    value_type: TypeInfo | None = None
    array_size: int = 0
    tuple_types: tuple[TypeInfo, ...] = field(default_factory=tuple)
    value_deferred_init: bool = False
    delegate_create_fn: str = ""


def iter_type_tree(info: TypeInfo) -> Iterator[TypeInfo]:
    stack = [info]
    while stack:
        current = stack.pop()
        yield current
        children = [
            child
            for child in (
                current.element_type,
                current.key_type,
                current.value_type,
                current.callback_ret,
            )
            if child is not None
        ]
        children.extend(current.tuple_types)
        children.extend(current.callback_args)
        stack.extend(reversed(children))


def object_class_names(info: TypeInfo) -> set[str]:
    return {
        node.class_name
        for node in iter_type_tree(info)
        if node.kind == "object" and node.class_name
    }


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
    s = s.replace(" *", "*").replace("* ", "*").replace(" &", "&").replace("& ", "&")
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


_CALLBACK_PREFIXES = (
    "geode::Function<",
    "Function<",
    "std::function<",
    "geode::utils::MiniFunction<",
    "utils::MiniFunction<",
    "MiniFunction<",
    "std::move_only_function<",
)


def callback_inner(n: str) -> str | None:
    start = n.find("<")
    if start == -1 or (n[:start] + "<") not in _CALLBACK_PREFIXES:
        return None
    depth = 0
    for i in range(start, len(n)):
        if n[i] == "<":
            depth += 1
        elif n[i] == ">":
            depth -= 1
            if depth == 0:
                return n[start + 1 : i]
    return None


def template_inner(n: str, prefix: str) -> str | None:
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


def is_sel_type(t: str) -> bool:
    return normalize_type(t) in SEL_TYPES


def sel_variant(t: str) -> str:
    return SEL_TYPES[normalize_type(t)][0]


def sel_lua_type(t: str) -> str:
    return SEL_TYPES[normalize_type(t)][1]
