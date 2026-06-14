from __future__ import annotations

from dataclasses import dataclass, field
from typing import TYPE_CHECKING, Dict, Optional, Set, Tuple

from luau_codegen.model.domain import short_name
from luau_codegen.model.value_struct_gate import GATED_VALUE_STRUCTS
from luau_codegen.parse.broma import Class

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
    **GATED_VALUE_STRUCTS,
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
    "CCObjectType",
    "CCVerticalTextAlignment",
    "ccGLServerState",
    "ccScriptType",
    "ccTouchesMode",
    "tCCMenuState",
    "tCCPositionType",
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
    from luau_codegen.model.geode_enums import EnumInfo

    enums = {name: EnumInfo(name=name, cxx_name=cxx) for name, cxx in names_to_cxx.items()}
    return Ctx.with_geode_enums(enums, skip=skip)


def enum_cxx_type(n: str, base: str, ctx: CodegenContext | None = None) -> str:
    return _resolve_ctx(ctx).enum_cxx_type(n, base)


def enum_lua_names(namespace: str, ctx: CodegenContext | None = None) -> Set[str]:
    return _resolve_ctx(ctx).enum_lua_names(namespace)


from luau_codegen.convert.type_containers import STD_ARRAY_MAX_SIZE  # noqa: E402
from luau_codegen.convert.type_normalization import (  # noqa: E402
    callback_inner,
    is_const_reference,
    is_out_reference,
    is_reference_type,
    normalize_type,
    strip_ref,
    template_inner,
    without_pointer,
)


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


def is_sel_type(t: str) -> bool:
    return normalize_type(t) in SEL_TYPES


def sel_variant(t: str) -> str:
    return SEL_TYPES[normalize_type(t)][0]


def sel_lua_type(t: str) -> str:
    return SEL_TYPES[normalize_type(t)][1]


from luau_codegen.convert.type_classification import (  # noqa: E402
    classify_arg,
    classify_return,
    method_input_arg_count,
    require_classify_arg,
    require_classify_return,
)


__all__ = [
    "CALLBACK_ALIASES",
    "CLASS_CALLBACK_ALIASES",
    "COCOS_ENUM_TYPES",
    "FMOD_ENUM_TYPES",
    "GD_ENUM_TYPES",
    "NUMERIC_TYPES",
    "OPAQUE_HANDLE_TYPES",
    "SEL_TYPES",
    "STATIC_ENUM_CXX_NAMES",
    "STD_ARRAY_MAX_SIZE",
    "STRING_TYPES",
    "TypeInfo",
    "UNSIGNED_NUMERIC_TYPES",
    "VALUE_CHECK_CXX_TYPES",
    "VALUE_TYPES",
    "WIDE_INTEGER_TYPES",
    "callback_inner",
    "classify_arg",
    "classify_return",
    "cxx_class_name",
    "enum_cxx_type",
    "enum_lua_names",
    "is_const_reference",
    "is_out_reference",
    "is_reference_type",
    "is_sel_type",
    "method_input_arg_count",
    "normalize_type",
    "register_geode_enums",
    "require_classify_arg",
    "require_classify_return",
    "resolve_object_class",
    "sel_lua_type",
    "sel_variant",
    "strip_ref",
    "template_inner",
    "without_pointer",
]
