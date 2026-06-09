from __future__ import annotations

from dataclasses import dataclass
from typing import Literal

FieldKind = Literal["number", "uchar", "float", "glfloat", "GLenum", "bool"]
PushFieldKind = Literal["number", "integer", "bool", "nested"]


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


COCOS_VALUE_STRUCTS: tuple[CocosValueStructDescriptor, ...] = (
    CocosValueStructDescriptor(
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
    CocosValueStructDescriptor(
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
    CocosValueStructDescriptor(
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
    CocosValueStructDescriptor(
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
    CocosValueStructDescriptor(
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
    CocosValueStructDescriptor(
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
    CocosValueStructDescriptor(
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
    CocosValueStructDescriptor(
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
)
