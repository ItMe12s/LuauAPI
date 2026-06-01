from __future__ import annotations

from luau_codegen.parse.broma import Root
from luau_codegen.emit.plan import EmitPlan, collect_plan
from luau_codegen.emit.cxx_templates import emit_internal_hpp
from luau_codegen.util.paths import binding_filename
from luau_codegen.emit.bindings.class_file import _emit_class_file
from luau_codegen.emit.bindings.common import _emit_common_file
from luau_codegen.emit.bindings.free_functions import (
    FREE_FUNCTIONS_FILE,
    emit_free_functions_file,
)

__all__ = [
    "emit",
    "emit_free_functions_file",
    "FREE_FUNCTIONS_FILE",
]


def emit(
    root: Root, target_platform: str = "win", plan: EmitPlan | None = None
) -> tuple[dict[str, str], list[tuple[str, str, str]]]:
    if plan is None:
        plan = collect_plan(root, target_platform)

    files: dict[str, str] = {
        "bindings_internal.hpp": emit_internal_hpp(),
        "bindings_common.cpp": _emit_common_file(
            plan.emitted_classes, plan, target_platform
        ),
        FREE_FUNCTIONS_FILE: emit_free_functions_file(
            plan.supported_free_functions, plan.objects
        ),
    }

    for cls in plan.emitted_classes:
        files[binding_filename(cls.name)] = _emit_class_file(
            cls,
            plan.supported_by_class[cls.name],
            plan.hook_targets_by_class[cls.name],
            plan.field_targets_by_class.get(cls.name, []),
            plan.objects,
            plan.skipped_classes,
            plan.depths[cls.name],
            target_platform,
        )

    return files, plan.skipped
