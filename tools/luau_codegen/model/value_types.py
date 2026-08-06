from __future__ import annotations

from dataclasses import dataclass
from types import MappingProxyType
from collections.abc import Iterable, Mapping
from typing import Literal

FieldKind = Literal[
    "number",
    "integer",
    "uchar",
    "float",
    "glfloat",
    "GLenum",
    "bool",
    "string",
    "enum",
    "object_nullable",
    "opaque_nullable",
    "nested",
    "container",
]
PushFieldKind = Literal[
    "number",
    "integer",
    "bool",
    "string",
    "enum",
    "object_nullable",
    "opaque_nullable",
    "nested",
    "container",
]
CocosEmitKind = Literal["standard", "ccrect"]


@dataclass(frozen=True)
class FieldDescriptor:
    name: str
    kind: FieldKind
    member: str
    cxx_type: str = ""
    nested_type: str = ""
    check_override: tuple[str, ...] = ()


@dataclass(frozen=True)
class PushFieldDescriptor:
    name: str
    kind: PushFieldKind
    member: str
    nested_type: str | None = None
    cxx_type: str = ""
    push_override: tuple[str, ...] = ()


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


def _member_assign_target(field: FieldDescriptor) -> str:
    tail = field.member.split(".", 1)[1] if "." in field.member else field.member
    return f"value.{tail}"


def _check_expr(field: FieldDescriptor) -> str:
    if field.kind == "number":
        return f'fieldNumber(L, idx, "{field.name}", method)'
    if field.kind == "integer":
        return f'static_cast<int>(fieldNumber(L, idx, "{field.name}", method))'
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
    if field.kind == "enum":
        cxx = field.cxx_type or "int"
        return f'static_cast<{cxx}>(static_cast<int>(fieldNumber(L, idx, "{field.name}", method)))'
    raise ValueError(f"unsupported single-line check kind: {field.kind}")


def _check_lines(field: FieldDescriptor) -> list[str]:
    target = _member_assign_target(field)
    if field.kind == "string":
        return [
            "        {\n",
            f'            auto _s = fieldString(L, idx, "{field.name}", method);\n',
            f"            {target} = {field.cxx_type or 'gd::string'}(_s.c_str());\n",
            "        }\n",
        ]
    if field.kind == "object_nullable":
        obj = field.cxx_type
        return [
            "        {\n",
            f'            lua_getfield(L, idx, "{field.name}");\n',
            "            if (lua_isnil(L, -1)) {\n",
            f"                {target} = nullptr;\n",
            "            } else {\n",
            f"                {target} = luax::Usertype<{obj}>::check(L, -1, method);\n",
            "            }\n",
            "            lua_pop(L, 1);\n",
            "        }\n",
        ]
    if field.kind == "opaque_nullable":
        opaque = field.cxx_type
        return [
            "        {\n",
            f'            lua_getfield(L, idx, "{field.name}");\n',
            "            if (lua_isnil(L, -1)) {\n",
            f"                {target} = nullptr;\n",
            "            } else {\n",
            f"                {target} = luax::checkOpaqueHandle<{opaque}>(L, -1, method);\n",
            "            }\n",
            "            lua_pop(L, 1);\n",
            "        }\n",
        ]
    if field.kind == "nested":
        nested = field.nested_type or field.cxx_type
        return [
            "        {\n",
            f'            lua_getfield(L, idx, "{field.name}");\n',
            f"            {target} = luax::check<{nested}>(L, -1, method);\n",
            "            lua_pop(L, 1);\n",
            "        }\n",
        ]
    if field.kind == "container":
        return list(field.check_override)
    return [f"        {target} = {_check_expr(field)};\n"]


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


_BUILTIN_VALUE_TYPE_SPECS: tuple[ValueTypeSpec, ...] = _VALUE_TYPE_SPECS

CCRECT_CXX_TYPE = "cocos2d::CCRect"


def _build_value_types(specs: tuple[ValueTypeSpec, ...]) -> dict[str, str]:
    out: dict[str, str] = {}
    for spec in specs:
        out[spec.cxx_type] = spec.lua_name
        for alias in spec.cxx_aliases:
            out[alias] = spec.lua_name
    return out


def _build_value_check_cxx_types(specs: tuple[ValueTypeSpec, ...]) -> dict[str, str]:
    return {spec.lua_name: spec.cxx_type for spec in specs}


@dataclass(frozen=True)
class ValueTypeCatalog:
    specs: tuple[ValueTypeSpec, ...]
    stub_order: tuple[str, ...]
    stub_body: Mapping[str, str]
    stub_deps: Mapping[str, tuple[str, ...]]
    cocos_structs: tuple[CocosValueStructDescriptor, ...]
    types: Mapping[str, str]
    check_cxx_types: Mapping[str, str]
    deferred_cxx_types: frozenset[str]

    @classmethod
    def from_specs(
        cls,
        generated_specs: Iterable[ValueTypeSpec] = (),
        *,
        include_builtins: bool = True,
    ) -> ValueTypeCatalog:
        specs = (_BUILTIN_VALUE_TYPE_SPECS if include_builtins else ()) + tuple(generated_specs)
        deferred = {
            spec.cocos.cxx_type
            for spec in specs
            if spec.cocos is not None
            and any(
                field.kind in {"container", "object_nullable", "opaque_nullable"}
                for field in (*spec.cocos.check_fields, *spec.cocos.push_fields)
            )
        }
        changed = True
        while changed:
            changed = False
            for spec in specs:
                desc = spec.cocos
                if desc is None or desc.cxx_type in deferred:
                    continue
                nested = {
                    field.nested_type
                    for field in (*desc.check_fields, *desc.push_fields)
                    if field.kind == "nested" and field.nested_type
                }
                if nested & deferred:
                    deferred.add(desc.cxx_type)
                    changed = True
        return cls(
            specs=specs,
            stub_order=tuple(spec.lua_name for spec in specs),
            stub_body=MappingProxyType({spec.lua_name: spec.luau_stub for spec in specs}),
            stub_deps=MappingProxyType(
                {spec.lua_name: spec.stub_deps for spec in specs if spec.stub_deps}
            ),
            cocos_structs=tuple(spec.cocos for spec in specs if spec.cocos is not None),
            types=MappingProxyType(_build_value_types(specs)),
            check_cxx_types=MappingProxyType(_build_value_check_cxx_types(specs)),
            deferred_cxx_types=frozenset(deferred),
        )

    @classmethod
    def builtins(cls) -> ValueTypeCatalog:
        return cls.from_specs()

    @classmethod
    def empty(cls) -> ValueTypeCatalog:
        return cls.from_specs(include_builtins=False)
