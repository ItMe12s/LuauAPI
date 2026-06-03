from __future__ import annotations

import os
import re
import shutil
import sys
import tempfile
import types
import unittest
import warnings
from unittest import mock

ROOT = os.path.dirname(
    os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
)
TOOLS_DIR = os.path.join(ROOT, "tools")
SCRIPTS_DIR = os.path.join(TOOLS_DIR, "scripts")
if TOOLS_DIR not in sys.path:
    sys.path.insert(0, TOOLS_DIR)
if SCRIPTS_DIR not in sys.path:
    sys.path.insert(0, SCRIPTS_DIR)

from luau_codegen.emit.delegates import (  # type: ignore[import-unresolved]
    DelegateMethod,
    DelegateSpec,
    collect as collect_delegate_specs,
    cpp_emit_supported,
    emit_gen_hpp,
    emit_override,
    emit_specs_py,
    fallback_bindings_dir,
)


def _install_fixture_delegate_specs() -> None:
    fixture = fallback_bindings_dir()
    if not fixture.is_dir():
        return
    name = "luau_codegen.model.delegate_specs"
    module = types.ModuleType(name)
    module.__file__ = str(fixture / "_fixture_delegate_specs.py")
    sys.modules[name] = module
    exec(emit_specs_py(collect_delegate_specs(fixture)), module.__dict__)


_install_fixture_delegate_specs()
DELEGATE_SPECS = sys.modules["luau_codegen.model.delegate_specs"].DELEGATE_SPECS

from luau_codegen.parse.broma import (  # type: ignore[import-unresolved]
    Arg,
    Class,
    Field,
    Function,
    Method,
    Root,
    parse_file,
)
from luau_codegen.emit.bindings import emit as emit_luau_bindings  # type: ignore[import-unresolved]
from luau_codegen.emit.bindings import emit_free_functions_file  # type: ignore[import-unresolved]
from luau_codegen.emit.bindings.class_file import (  # type: ignore[import-unresolved]
    _emit_class_file,
    _emit_dispatcher,
)
from luau_codegen.emit.bindings.common import _emit_common_file  # type: ignore[import-unresolved]
from luau_codegen.policy.free_functions import (  # type: ignore[import-unresolved]
    free_function_allowed as free_fn_allowed,
    free_function_key,
    free_function_supported,
    free_function_unsupported_reason,
    group_supported_free_functions,
)
from luau_codegen.emit.luau_types import TYPES_FILE  # type: ignore[import-unresolved]
from luau_codegen.emit.luau_types import emit as emit_luau_types  # type: ignore[import-unresolved]
from luau_codegen.convert import type_map as type_map_module  # type: ignore[import-unresolved]
from luau_codegen.model.codegen_context import CodegenContext  # type: ignore[import-unresolved]
from luau_codegen.convert.type_map import (  # type: ignore[import-unresolved]
    COCOS_ENUM_TYPES,
    GD_ENUM_TYPES,
    STD_ARRAY_MAX_SIZE,
    TypeInfo,
    classify_arg,
    classify_return,
    is_const_reference,
    is_out_reference,
    register_geode_enums,
)
from luau_codegen.convert.type_map import (  # type: ignore[import-unresolved]
    method_input_arg_count as _input_arg_count,
)
from luau_codegen.policy.filtering import group_supported, supported  # type: ignore[import-unresolved]
from luau_codegen.policy.link_attrs import (  # type: ignore[import-unresolved]
    class_link_platforms,
    is_link_platform,
)
from luau_codegen.convert.symbols import android_symbol  # type: ignore[import-unresolved]
from luau_codegen.policy.hooks import (  # type: ignore[import-unresolved]
    hook_address_expr,
    hook_offset,
    hookable,
)
from luau_codegen.emit.cxx_templates import (  # type: ignore[import-unresolved]
    emit_hook_support,
    emit_internal_hpp,
)
from luau_codegen.policy.intersection import intersection_platforms  # type: ignore[import-unresolved]
from luau_codegen.parse.collect import collect_bindings_root  # type: ignore[import-unresolved]
from luau_codegen.cli.main import main as codegen_main  # type: ignore[import-unresolved]
from luau_codegen.convert.marshalling import (  # type: ignore[import-unresolved]
    _push_impl,
    check_arg,
    emit_stack_check,
    push_return,
    push_value,
    sel_call_args,
    sel_menu_call_args,
    sel_selector_call_arg,
)
from luau_codegen.model.domain import (  # type: ignore[import-unresolved]
    build_class_lookup,
    codegen_object_map,
    object_classes,
    resolve_base,
)
from luau_codegen.emit.parity import collect_parity, emit_markdown  # type: ignore[import-unresolved]
from luau_codegen.emit.audit import collect_audit, emit_markdown as emit_audit_markdown  # type: ignore[import-unresolved]
from luau_codegen.emit.plan import (  # type: ignore[import-unresolved]
    collect_plan,
    collect_platform_plan,
    plan_outputs,
)


def all_platforms(value: str = "0x1") -> dict[str, str]:
    return {
        "win": value,
        "m1": value,
        "ios": value,
        "android32": value,
        "android64": value,
    }


def types_text(files: dict[str, str]) -> str:
    return files[TYPES_FILE]


__all__ = [
    # stdlib re-exports
    "os",
    "re",
    "shutil",
    "tempfile",
    "unittest",
    "warnings",
    "mock",
    "ROOT",
    # parse.broma
    "Arg",
    "Class",
    "Field",
    "Function",
    "Method",
    "Root",
    "parse_file",
    # emit.bindings
    "emit_luau_bindings",
    "emit_free_functions_file",
    "_emit_class_file",
    "_emit_dispatcher",
    "_emit_common_file",
    # policy.free_functions
    "free_fn_allowed",
    "free_function_key",
    "free_function_supported",
    "free_function_unsupported_reason",
    "group_supported_free_functions",
    # emit.luau_types
    "TYPES_FILE",
    "emit_luau_types",
    # convert.type_map
    "type_map_module",
    "CodegenContext",
    "COCOS_ENUM_TYPES",
    "GD_ENUM_TYPES",
    "STD_ARRAY_MAX_SIZE",
    "TypeInfo",
    "classify_arg",
    "classify_return",
    "is_const_reference",
    "is_out_reference",
    "_input_arg_count",
    "register_geode_enums",
    # policy.filtering / link_attrs
    "group_supported",
    "supported",
    "class_link_platforms",
    "is_link_platform",
    # convert.symbols / policy.hooks
    "android_symbol",
    "hook_address_expr",
    "hook_offset",
    "hookable",
    # emit.cxx_templates
    "emit_hook_support",
    "emit_internal_hpp",
    # policy.intersection
    "intersection_platforms",
    # cli
    "collect_bindings_root",
    "codegen_main",
    # convert.marshalling
    "_push_impl",
    "check_arg",
    "emit_stack_check",
    "push_return",
    "push_value",
    "sel_call_args",
    "sel_menu_call_args",
    "sel_selector_call_arg",
    # model.domain
    "build_class_lookup",
    "codegen_object_map",
    "object_classes",
    "resolve_base",
    # emit.parity
    "collect_parity",
    "emit_markdown",
    # emit.audit
    "collect_audit",
    "emit_audit_markdown",
    # emit.plan
    "collect_plan",
    "collect_platform_plan",
    "plan_outputs",
    # delegate generator
    "DELEGATE_SPECS",
    "DelegateMethod",
    "DelegateSpec",
    "collect_delegate_specs",
    "cpp_emit_supported",
    "emit_gen_hpp",
    "emit_override",
    # helpers
    "all_platforms",
    "types_text",
]
