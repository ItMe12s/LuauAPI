from __future__ import annotations

from dataclasses import dataclass

from luau_codegen.convert.sel_args import iter_lua_method_args
from luau_codegen.convert.type_classification import classify_arg, classify_return
from luau_codegen.convert.type_primitives import TypeInfo
from luau_codegen.model.codegen_context import CodegenContext
from luau_codegen.parse.broma import Class, Function, Method


@dataclass(frozen=True)
class CallableSignature:
    return_info: TypeInfo
    arg_infos: tuple[TypeInfo, ...]
    lua_input_args: tuple[TypeInfo, ...]
    out_args: tuple[TypeInfo, ...]
    input_arity: int


class TypeAnalysis:
    def __init__(self, objects: dict[str, Class], ctx: CodegenContext) -> None:
        self.objects = objects
        self.ctx = ctx
        self._args: dict[tuple[str, str, str], TypeInfo | None] = {}
        self._returns: dict[tuple[str, str, str], TypeInfo | None] = {}
        self._signatures: dict[tuple[object, ...], CallableSignature | None] = {}

    def classify_arg(
        self, raw_type: str, *, owner_class: str = "", field_name: str = ""
    ) -> TypeInfo | None:
        key = (raw_type, owner_class, field_name)
        if key not in self._args:
            self._args[key] = classify_arg(
                raw_type,
                self.objects,
                owner_class=owner_class,
                field_name=field_name,
                ctx=self.ctx,
            )
        return self._args[key]

    def classify_return(
        self, raw_type: str, *, owner_class: str = "", field_name: str = ""
    ) -> TypeInfo | None:
        key = (raw_type, owner_class, field_name)
        if key not in self._returns:
            self._returns[key] = classify_return(
                raw_type,
                self.objects,
                owner_class=owner_class,
                field_name=field_name,
                ctx=self.ctx,
            )
        return self._returns[key]

    def require_arg(
        self, raw_type: str, *, owner_class: str = "", field_name: str = ""
    ) -> TypeInfo:
        info = self.classify_arg(raw_type, owner_class=owner_class, field_name=field_name)
        if info is None:
            raise ValueError(f"unsupported arg type: {raw_type}")
        return info

    def require_return(
        self, raw_type: str, *, owner_class: str = "", field_name: str = ""
    ) -> TypeInfo:
        info = self.classify_return(raw_type, owner_class=owner_class, field_name=field_name)
        if info is None:
            raise ValueError(f"unsupported return type: {raw_type}")
        return info

    def signature(
        self, callable_: Method | Function, *, owner_class: str = ""
    ) -> CallableSignature | None:
        is_instance = isinstance(callable_, Method) and not callable_.is_static
        key = (
            callable_.ret,
            tuple(arg.type for arg in callable_.args),
            owner_class,
            is_instance,
        )
        if key in self._signatures:
            return self._signatures[key]
        ret = self.classify_return(callable_.ret, owner_class=owner_class)
        args = tuple(self.classify_arg(arg.type, owner_class=owner_class) for arg in callable_.args)
        if ret is None or any(info is None for info in args):
            self._signatures[key] = None
            return None
        arg_infos = tuple(info for info in args if info is not None)
        lua_args = tuple(
            iter_lua_method_args(callable_, arg_infos, ret_kind=ret.kind, is_instance=is_instance)
        )
        result = CallableSignature(
            return_info=ret,
            arg_infos=arg_infos,
            lua_input_args=tuple(arg.info for arg in lua_args if not arg.out_only),
            out_args=tuple(arg.info for arg in lua_args if arg.out_only),
            input_arity=sum(not arg.out_only for arg in lua_args),
        )
        self._signatures[key] = result
        return result
