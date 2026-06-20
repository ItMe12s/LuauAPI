from __future__ import annotations

from dataclasses import dataclass
from typing import Literal

from luau_codegen.model.value_struct_gate import GATED_VALUE_STRUCTS

FieldKind = Literal["number", "uchar", "float", "glfloat", "GLenum", "bool"]
PushFieldKind = Literal["number", "integer", "bool", "nested"]
CocosEmitKind = Literal["standard", "ccrect"]


@dataclass(frozen=True)
class FieldDescriptor:
    name: str
    kind: FieldKind
    member: str


@dataclass(frozen=True)
class PushFieldDescriptor:
    name: str
    kind: PushFieldKind
    member: str
    nested_type: str | None = None


@dataclass(frozen=True)
class CocosValueStructDescriptor:
    cxx_type: str
    check_fields: tuple[FieldDescriptor, ...]
    push_fields: tuple[PushFieldDescriptor, ...]
    push_capacity: int


@dataclass(frozen=True)
class ValueTypeSpec:
    lua_name: str
    cxx_type: str
    cxx_aliases: tuple[str, ...] = ()
    luau_stub: str = ""
    stub_deps: tuple[str, ...] = ()
    cocos: CocosValueStructDescriptor | None = None
    cocos_emit: CocosEmitKind | None = None


def _check_expr(field: FieldDescriptor) -> str:
    if field.kind == "number":
        return f'fieldNumber(L, idx, "{field.name}", method)'
    if field.kind == "uchar":
        return f'static_cast<unsigned char>(fieldNumber(L, idx, "{field.name}", method))'
    if field.kind == "float":
        return f'static_cast<float>(fieldNumber(L, idx, "{field.name}", method))'
    if field.kind == "glfloat":
        return f'static_cast<GLfloat>(fieldNumber(L, idx, "{field.name}", method))'
    if field.kind == "GLenum":
        return f'static_cast<GLenum>(fieldNumber(L, idx, "{field.name}", method))'
    if field.kind == "bool":
        return f'fieldBool(L, idx, "{field.name}", method)'
    raise ValueError(f"unsupported field kind: {field.kind}")


_VALUE_TYPE_SPECS: tuple[ValueTypeSpec, ...] = (
    ValueTypeSpec(
        lua_name="CCPoint",
        cxx_type="cocos2d::CCPoint",
        cxx_aliases=("CCPoint",),
        luau_stub="export type CCPoint = { x: number, y: number }\n",
        cocos=CocosValueStructDescriptor(
            cxx_type="cocos2d::CCPoint",
            check_fields=(
                FieldDescriptor("x", "number", "point.x"),
                FieldDescriptor("y", "number", "point.y"),
            ),
            push_fields=(
                PushFieldDescriptor("x", "number", "point.x"),
                PushFieldDescriptor("y", "number", "point.y"),
            ),
            push_capacity=2,
        ),
    ),
    ValueTypeSpec(
        lua_name="CCSize",
        cxx_type="cocos2d::CCSize",
        cxx_aliases=("CCSize",),
        luau_stub="export type CCSize = { width: number, height: number }\n",
        cocos=CocosValueStructDescriptor(
            cxx_type="cocos2d::CCSize",
            check_fields=(
                FieldDescriptor("width", "number", "size.width"),
                FieldDescriptor("height", "number", "size.height"),
            ),
            push_fields=(
                PushFieldDescriptor("width", "number", "size.width"),
                PushFieldDescriptor("height", "number", "size.height"),
            ),
            push_capacity=2,
        ),
    ),
    ValueTypeSpec(
        lua_name="RGBColor",
        cxx_type="cocos2d::ccColor3B",
        cxx_aliases=("ccColor3B",),
        luau_stub="export type RGBColor = { r: number, g: number, b: number }\n",
        cocos=CocosValueStructDescriptor(
            cxx_type="cocos2d::ccColor3B",
            check_fields=(
                FieldDescriptor("r", "uchar", "color.r"),
                FieldDescriptor("g", "uchar", "color.g"),
                FieldDescriptor("b", "uchar", "color.b"),
            ),
            push_fields=(
                PushFieldDescriptor("r", "integer", "color.r"),
                PushFieldDescriptor("g", "integer", "color.g"),
                PushFieldDescriptor("b", "integer", "color.b"),
            ),
            push_capacity=3,
        ),
    ),
    ValueTypeSpec(
        lua_name="RGBAColor",
        cxx_type="cocos2d::ccColor4B",
        cxx_aliases=("ccColor4B",),
        luau_stub="export type RGBAColor = { r: number, g: number, b: number, a: number }\n",
        cocos=CocosValueStructDescriptor(
            cxx_type="cocos2d::ccColor4B",
            check_fields=(
                FieldDescriptor("r", "uchar", "color.r"),
                FieldDescriptor("g", "uchar", "color.g"),
                FieldDescriptor("b", "uchar", "color.b"),
                FieldDescriptor("a", "uchar", "color.a"),
            ),
            push_fields=(
                PushFieldDescriptor("r", "integer", "color.r"),
                PushFieldDescriptor("g", "integer", "color.g"),
                PushFieldDescriptor("b", "integer", "color.b"),
                PushFieldDescriptor("a", "integer", "color.a"),
            ),
            push_capacity=4,
        ),
    ),
    ValueTypeSpec(
        lua_name="RGBAFloatColor",
        cxx_type="cocos2d::ccColor4F",
        cxx_aliases=("ccColor4F",),
        luau_stub=("export type RGBAFloatColor = { r: number, g: number, b: number, a: number }\n"),
        cocos=CocosValueStructDescriptor(
            cxx_type="cocos2d::ccColor4F",
            check_fields=(
                FieldDescriptor("r", "glfloat", "color.r"),
                FieldDescriptor("g", "glfloat", "color.g"),
                FieldDescriptor("b", "glfloat", "color.b"),
                FieldDescriptor("a", "glfloat", "color.a"),
            ),
            push_fields=(
                PushFieldDescriptor("r", "number", "color.r"),
                PushFieldDescriptor("g", "number", "color.g"),
                PushFieldDescriptor("b", "number", "color.b"),
                PushFieldDescriptor("a", "number", "color.a"),
            ),
            push_capacity=4,
        ),
    ),
    ValueTypeSpec(
        lua_name="BlendFunc",
        cxx_type="cocos2d::ccBlendFunc",
        cxx_aliases=("ccBlendFunc",),
        luau_stub="export type BlendFunc = { src: number, dst: number }\n",
        cocos=CocosValueStructDescriptor(
            cxx_type="cocos2d::ccBlendFunc",
            check_fields=(
                FieldDescriptor("src", "GLenum", "blend.src"),
                FieldDescriptor("dst", "GLenum", "blend.dst"),
            ),
            push_fields=(
                PushFieldDescriptor("src", "number", "blend.src"),
                PushFieldDescriptor("dst", "number", "blend.dst"),
            ),
            push_capacity=2,
        ),
    ),
    ValueTypeSpec(
        lua_name="HSVValue",
        cxx_type="cocos2d::ccHSVValue",
        cxx_aliases=("ccHSVValue",),
        luau_stub=(
            "export type HSVValue = {\n"
            "    h: number,\n"
            "    s: number,\n"
            "    v: number,\n"
            "    absoluteSaturation: boolean,\n"
            "    absoluteBrightness: boolean,\n"
            "}\n"
        ),
        cocos=CocosValueStructDescriptor(
            cxx_type="cocos2d::ccHSVValue",
            check_fields=(
                FieldDescriptor("h", "float", "hsv.h"),
                FieldDescriptor("s", "float", "hsv.s"),
                FieldDescriptor("v", "float", "hsv.v"),
                FieldDescriptor("absoluteSaturation", "bool", "hsv.absoluteSaturation"),
                FieldDescriptor("absoluteBrightness", "bool", "hsv.absoluteBrightness"),
            ),
            push_fields=(
                PushFieldDescriptor("h", "number", "hsv.h"),
                PushFieldDescriptor("s", "number", "hsv.s"),
                PushFieldDescriptor("v", "number", "hsv.v"),
                PushFieldDescriptor("absoluteSaturation", "bool", "hsv.absoluteSaturation"),
                PushFieldDescriptor("absoluteBrightness", "bool", "hsv.absoluteBrightness"),
            ),
            push_capacity=5,
        ),
    ),
    ValueTypeSpec(
        lua_name="CCAffineTransform",
        cxx_type="cocos2d::CCAffineTransform",
        cxx_aliases=("CCAffineTransform",),
        luau_stub=(
            "export type CCAffineTransform = { a: number, b: number, c: number, d: number, tx: number, ty: number }\n"
        ),
        cocos=CocosValueStructDescriptor(
            cxx_type="cocos2d::CCAffineTransform",
            check_fields=(
                FieldDescriptor("a", "float", "t.a"),
                FieldDescriptor("b", "float", "t.b"),
                FieldDescriptor("c", "float", "t.c"),
                FieldDescriptor("d", "float", "t.d"),
                FieldDescriptor("tx", "float", "t.tx"),
                FieldDescriptor("ty", "float", "t.ty"),
            ),
            push_fields=(
                PushFieldDescriptor("a", "number", "t.a"),
                PushFieldDescriptor("b", "number", "t.b"),
                PushFieldDescriptor("c", "number", "t.c"),
                PushFieldDescriptor("d", "number", "t.d"),
                PushFieldDescriptor("tx", "number", "t.tx"),
                PushFieldDescriptor("ty", "number", "t.ty"),
            ),
            push_capacity=6,
        ),
    ),
    ValueTypeSpec(
        lua_name="CCRect",
        cxx_type="cocos2d::CCRect",
        cxx_aliases=("CCRect",),
        luau_stub="export type CCRect = { origin: CCPoint, size: CCSize }\n",
        stub_deps=("CCPoint", "CCSize"),
        cocos_emit="ccrect",
    ),
    ValueTypeSpec(
        lua_name="UIButtonConfig",
        cxx_type="UIButtonConfig",
        luau_stub=(
            "export type UIButtonConfig = {\n"
            "    width: number,\n"
            "    height: number,\n"
            "    deadzone: number,\n"
            "    scale: number,\n"
            "    opacity: number,\n"
            "    radius: number,\n"
            "    modeB: boolean,\n"
            "    snap: boolean,\n"
            "    position: CCPoint,\n"
            "    oneButton: boolean,\n"
            "    player2: boolean,\n"
            "    split: boolean,\n"
            "}\n"
        ),
        stub_deps=("CCPoint",),
    ),
    ValueTypeSpec(
        lua_name="SmartPrefabResult",
        cxx_type="SmartPrefabResult",
        luau_stub=(
            "export type SmartPrefabResult = {\n"
            "    smartPrefab: GJSmartPrefab?,\n"
            "    binaryKey: string,\n"
            "    prefabKey: string,\n"
            "    prefabCount: number,\n"
            "    unrequired: boolean,\n"
            "    rotation: number,\n"
            "    flipX: boolean,\n"
            "    flipY: boolean,\n"
            "    ignoreCorners: boolean,\n"
            "}\n"
        ),
        stub_deps=("GJSmartPrefab",),
    ),
)

_GATED_SPECS = tuple(spec for spec in _VALUE_TYPE_SPECS if spec.lua_name in GATED_VALUE_STRUCTS)
_BUILTIN_SPECS = tuple(
    spec for spec in _VALUE_TYPE_SPECS if spec.lua_name not in GATED_VALUE_STRUCTS
)

VALUE_TYPE_SPECS: tuple[ValueTypeSpec, ...] = _BUILTIN_SPECS + _GATED_SPECS

VALUE_STUB_ORDER: tuple[str, ...] = tuple(spec.lua_name for spec in VALUE_TYPE_SPECS)

VALUE_STUB_BODY: dict[str, str] = {spec.lua_name: spec.luau_stub for spec in VALUE_TYPE_SPECS}

VALUE_STUB_DEPS: dict[str, tuple[str, ...]] = {
    spec.lua_name: spec.stub_deps for spec in VALUE_TYPE_SPECS if spec.stub_deps
}

COCOS_VALUE_STRUCTS: tuple[CocosValueStructDescriptor, ...] = tuple(
    spec.cocos for spec in VALUE_TYPE_SPECS if spec.cocos is not None
)

CCRECT_CXX_TYPE = "cocos2d::CCRect"


def _build_value_types() -> dict[str, str]:
    out: dict[str, str] = {}
    for spec in VALUE_TYPE_SPECS:
        out[spec.cxx_type] = spec.lua_name
        for alias in spec.cxx_aliases:
            out[alias] = spec.lua_name
    return out


def _build_value_check_cxx_types() -> dict[str, str]:
    out: dict[str, str] = {spec.lua_name: spec.cxx_type for spec in VALUE_TYPE_SPECS}
    for lua_name, cxx_name in GATED_VALUE_STRUCTS.items():
        if lua_name not in out:
            out[lua_name] = cxx_name
    return out


VALUE_TYPES = _build_value_types()
VALUE_CHECK_CXX_TYPES = _build_value_check_cxx_types()
