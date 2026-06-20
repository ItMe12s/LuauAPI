from __future__ import annotations

from typing import TYPE_CHECKING, Dict, List, Optional

if TYPE_CHECKING:
    from luau_codegen.parse.broma import Root
    from luau_codegen.emit.plan import EmitPlan

__all__ = ["emit"]


def __getattr__(name: str):
    if name in ("emit_free_functions_file", "FREE_FUNCTIONS_FILE"):
        from luau_codegen.emit.bindings import free_functions

        return getattr(free_functions, name)
    raise AttributeError(f"module {__name__!r} has no attribute {name!r}")


def emit(
    root: Root,
    target_platform: str = "win",
    plan: EmitPlan | None = None,
    manual_fields: Optional[Dict[str, List[str]]] = None,
) -> tuple[dict[str, str], list[tuple[str, str, str]]]:
    from luau_codegen.emit.plan import collect_plan
    from luau_codegen.emit.cxx_templates import emit_internal_hpp
    from luau_codegen.util import binding_filename
    from luau_codegen.emit.bindings.class_file import _emit_class_file
    from luau_codegen.emit.bindings.common import _emit_common_file
    from luau_codegen.emit.bindings.cocos_enums import (
        ENUM_KEY_CODES_MANIFEST,
        emit_enum_key_codes_manifest,
    )
    from luau_codegen.emit.bindings.free_functions import (
        FREE_FUNCTIONS_FILE,
        emit_free_functions_file,
    )
    from luau_codegen.emit.bindings.geode_enums import (
        GEODE_ENUMS_BINDING,
        GEODE_ENUMS_MANIFEST,
        emit_geode_enums_binding,
        emit_geode_enums_manifest,
    )

    if plan is None:
        plan = collect_plan(root, target_platform)

    files: dict[str, str] = {
        "bindings_internal.hpp": emit_internal_hpp(),
        "bindings_common.cpp": _emit_common_file(plan.emitted_classes, plan, target_platform),
        ENUM_KEY_CODES_MANIFEST: emit_enum_key_codes_manifest(plan.ctx),
        GEODE_ENUMS_MANIFEST: emit_geode_enums_manifest(plan.ctx),
        GEODE_ENUMS_BINDING: emit_geode_enums_binding(plan.ctx),
        FREE_FUNCTIONS_FILE: emit_free_functions_file(
            plan.supported_free_functions,
            plan.objects,
            ctx=plan.ctx,
            manual_fields=manual_fields,
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
            ctx=plan.ctx,
        )

    return files, plan.skipped
