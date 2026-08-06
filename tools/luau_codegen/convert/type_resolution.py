from __future__ import annotations

from luau_codegen.convert.type_primitives import without_pointer
from luau_codegen.model.codegen_context import CodegenContext
from luau_codegen.model.domain import short_name
from luau_codegen.parse.broma import Class


def _resolve_ctx(ctx: CodegenContext | None) -> CodegenContext:
    return ctx if ctx is not None else CodegenContext.static()


def enum_cxx_type(n: str, base: str, ctx: CodegenContext | None = None) -> str:
    return _resolve_ctx(ctx).enum_cxx_type(n, base)


def enum_lua_names(namespace: str, ctx: CodegenContext | None = None) -> frozenset[str]:
    return _resolve_ctx(ctx).enum_lua_names(namespace)


def cxx_class_name(cls: Class) -> str:
    return f"{cls.namespace}::{cls.name}" if cls.namespace else cls.name


def resolve_object_class(t: str, classes: dict[str, Class]) -> Class | None:
    base = without_pointer(t).lstrip(":")
    if base in classes:
        return classes[base]
    return classes.get(short_name(base))
