from __future__ import annotations

from collections import defaultdict
from dataclasses import dataclass, field, replace

from luau_codegen.parse.broma import Class, Field, Function, Method, Root
from luau_codegen.policy.fields import (
    bindable_field,
    field_applies_on_platform,
    field_key,
    field_skipped_object_ref,
)
from luau_codegen.policy.filtering import (
    group_supported,
    linkless_class_names,
    prune_skipped_class_refs,
)
from luau_codegen.policy.free_functions import (
    FreeFunctionSkip,
    free_function_key,
    free_function_skipped_object_ref,
    group_supported_free_functions,
)
from luau_codegen.policy.hooks import hookable
from luau_codegen.policy.intersection import (
    IntersectionResult,
    IntersectionStats,
    collect_intersection,
    method_key,
)
from luau_codegen.model.platforms import intersection_platforms
from luau_codegen.model.domain import (
    ClassHierarchy,
    build_class_lookup,
    resolve_base,
)
from luau_codegen.model.codegen_context import CodegenContext
from luau_codegen.model.type_analysis import TypeAnalysis
from luau_codegen.util import binding_filename

from luau_codegen.emit.bindings.cocos_enums import ENUM_KEY_CODES_MANIFEST
from luau_codegen.emit.bindings.geode_enums import GEODE_ENUMS_BINDING, GEODE_ENUMS_MANIFEST


def _ctx_from_root(root: Root) -> CodegenContext:
    return root.codegen_ctx or CodegenContext.static()


@dataclass
class EmitPlan:
    classes: list[Class]
    objects: dict[str, Class]
    skipped: list[tuple[str, str, str]]
    skipped_by_class: dict[str, list[tuple[Method, str]]]
    skipped_classes: set[str]
    supported_by_class: dict[str, dict[str, list[Method]]]
    hook_targets_by_class: dict[str, list[tuple[Class, Method]]]
    field_targets_by_class: dict[str, list[tuple[Class, Field]]]
    depths: dict[str, int]
    hierarchy: ClassHierarchy | None = None
    supported_free_functions: list[Function] = field(default_factory=list)
    skipped_free_functions: list[FreeFunctionSkip] = field(default_factory=list)
    emitted_classes: list[Class] = field(default_factory=list)
    intersection_stats: IntersectionStats = field(default_factory=IntersectionStats)
    ctx: CodegenContext = field(default_factory=CodegenContext.static)
    type_analysis: TypeAnalysis | None = field(default=None, repr=False)

    def __post_init__(self) -> None:
        if self.type_analysis is None:
            self.type_analysis = TypeAnalysis(self.objects, self.ctx)


def _type_needed_class_names(plan: EmitPlan) -> set[str]:
    from luau_codegen.convert.type_primitives import object_class_names

    assert plan.type_analysis is not None
    referenced: set[str] = set()

    def add(info) -> None:
        if info:
            referenced.update(
                name
                for name in object_class_names(info)
                if name in plan.objects and not plan.objects[name].methods
            )

    for cls_name, grouped in plan.supported_by_class.items():
        for methods in grouped.values():
            for m in methods:
                add(plan.type_analysis.classify_return(m.ret, owner_class=cls_name))
                for arg in m.args:
                    add(plan.type_analysis.classify_arg(arg.type, owner_class=cls_name))

    for targets in plan.field_targets_by_class.values():
        for field_cls, bound_field in targets:
            _, _, arg, ret = bindable_field(
                bound_field,
                plan.objects,
                field_cls,
                ctx=plan.ctx,
                analysis=plan.type_analysis,
            )
            add(arg)
            add(ret)

    for fn in plan.supported_free_functions:
        add(plan.type_analysis.classify_return(fn.ret))
        for arg in fn.args:
            add(plan.type_analysis.classify_arg(arg.type))

    lookup = build_class_lookup(plan.classes)
    keep_bases: set[str] = set()
    for cls in plan.classes:
        if cls.name in plan.skipped_classes:
            continue
        for base in cls.bases:
            base_cls = resolve_base(lookup, base)
            if base_cls:
                keep_bases.add(base_cls.name)
    return referenced | keep_bases


def _emitted_classes(plan: EmitPlan) -> list[Class]:
    out: list[Class] = []
    for cls in plan.classes:
        if cls.name in plan.skipped_classes:
            continue
        grouped = plan.supported_by_class[cls.name]
        if (
            grouped
            or plan.hook_targets_by_class[cls.name]
            or plan.field_targets_by_class.get(cls.name, [])
        ):
            out.append(cls)
    return out


def _prune_field_object_refs(
    field_targets_by_class: dict[str, list[tuple[Class, Field]]],
    objects: dict[str, Class],
    skipped_classes: set[str],
    target_platform: str,
    ctx: CodegenContext | None = None,
    analysis: TypeAnalysis | None = None,
) -> list[tuple[str, str, str]]:
    pruned: list[tuple[str, str, str]] = []
    for cls_name, targets in field_targets_by_class.items():
        kept: list[tuple[Class, Field]] = []
        for field_cls, bound_field in targets:
            skipped_ref = field_skipped_object_ref(
                bound_field,
                objects,
                skipped_classes,
                field_cls,
                ctx=ctx,
                analysis=analysis,
            )
            if skipped_ref:
                reason = f"not-callable-type:{target_platform}:{skipped_ref}"
                pruned.append((cls_name, bound_field.name, reason))
            else:
                kept.append((field_cls, bound_field))
        field_targets_by_class[cls_name] = kept
    return pruned


def _filter_free_function_object_refs(
    functions: list[Function],
    objects: dict[str, Class],
    skipped_classes: set[str],
    target_platform: str,
    ctx: CodegenContext | None = None,
    analysis: TypeAnalysis | None = None,
) -> tuple[list[Function], list[FreeFunctionSkip]]:
    kept: list[Function] = []
    skipped: list[FreeFunctionSkip] = []
    for fn in functions:
        skipped_ref = free_function_skipped_object_ref(
            fn, objects, skipped_classes, ctx=ctx, analysis=analysis
        )
        if skipped_ref:
            reason = f"not-callable-type:{target_platform}:{skipped_ref}"
            skipped.append((free_function_key(fn), fn.lua_path, fn.name, reason))
            continue
        kept.append(fn)
    return kept, skipped


def collect_platform_plan(
    root: Root,
    target_platform: str = "win",
    *,
    hierarchy: ClassHierarchy | None = None,
    ctx: CodegenContext | None = None,
    analysis: TypeAnalysis | None = None,
) -> EmitPlan:
    hierarchy = hierarchy or ClassHierarchy(root.classes)
    classes = hierarchy.object_classes()
    objects = build_class_lookup(classes)
    ctx = ctx or _ctx_from_root(root)
    type_analysis = analysis or TypeAnalysis(objects, ctx)
    skipped: list[tuple[str, str, str]] = []
    supported_by_class: dict[str, dict[str, list[Method]]] = {}
    skipped_by_class: dict[str, list[tuple[Method, str]]] = {}

    for cls in classes:
        grouped, cls_skipped = group_supported(
            cls, objects, target_platform, ctx=ctx, analysis=type_analysis
        )
        supported_by_class[cls.name] = grouped
        skipped_by_class[cls.name] = cls_skipped
        for method, reason in cls_skipped:
            skipped.append((cls.name, method.name, reason))

    supported_free_functions, skipped_free_functions = group_supported_free_functions(
        root.functions, objects, target_platform, ctx=ctx, analysis=type_analysis
    )

    skipped_classes = linkless_class_names(
        classes,
        objects,
        supported_by_class,
        skipped_by_class,
        target_platform,
        ctx=ctx,
        free_functions=supported_free_functions,
        analysis=type_analysis,
    )
    skipped.extend(
        prune_skipped_class_refs(
            supported_by_class,
            skipped_by_class,
            objects,
            skipped_classes,
            target_platform,
            ctx=ctx,
            analysis=type_analysis,
        )
    )
    depths = {cls.name: hierarchy.depth(cls, skipped_classes) for cls in classes}

    hook_targets_by_class: dict[str, list[tuple[Class, Method]]] = defaultdict(list)
    field_targets_by_class: dict[str, list[tuple[Class, Field]]] = defaultdict(list)
    for cls in classes:
        if cls.name in skipped_classes:
            continue
        grouped = supported_by_class[cls.name]
        for methods in grouped.values():
            for method in methods:
                if hookable(
                    cls,
                    method,
                    objects,
                    target_platform,
                    ctx=ctx,
                    analysis=type_analysis,
                ):
                    hook_targets_by_class[cls.name].append((cls, method))
        for cls_field in cls.fields:
            if not field_applies_on_platform(cls_field, target_platform):
                continue
            ok, _, _, _ = bindable_field(
                cls_field,
                objects,
                cls,
                ctx=ctx,
                analysis=type_analysis,
            )
            if ok:
                field_targets_by_class[cls.name].append((cls, cls_field))

    skipped.extend(
        _prune_field_object_refs(
            field_targets_by_class,
            objects,
            skipped_classes,
            target_platform,
            ctx=ctx,
            analysis=type_analysis,
        )
    )

    supported_free_functions, free_ref_skips = _filter_free_function_object_refs(
        supported_free_functions,
        objects,
        skipped_classes,
        target_platform,
        ctx=ctx,
        analysis=type_analysis,
    )
    skipped_free_functions.extend(free_ref_skips)

    plan = EmitPlan(
        classes=classes,
        objects=objects,
        ctx=ctx,
        skipped=skipped,
        skipped_by_class=skipped_by_class,
        skipped_classes=skipped_classes,
        supported_by_class=supported_by_class,
        hook_targets_by_class=hook_targets_by_class,
        field_targets_by_class=field_targets_by_class,
        depths=depths,
        hierarchy=hierarchy,
        type_analysis=type_analysis,
        supported_free_functions=supported_free_functions,
        skipped_free_functions=skipped_free_functions,
    )
    plan.emitted_classes = _emitted_classes(plan)
    return plan


def _intersection_reason(missing: tuple[str, ...]) -> str:
    return "intersection-missing-platform:" + ",".join(missing)


def _append_skip(plan: EmitPlan, cls: Class, method: Method, reason: str) -> None:
    plan.skipped.append((cls.name, method.name, reason))
    plan.skipped_by_class.setdefault(cls.name, []).append((method, reason))


def _append_free_function_skip(
    plan: EmitPlan,
    stats: IntersectionStats,
    fn: Function,
    reason: str,
) -> None:
    plan.skipped_free_functions.append((free_function_key(fn), fn.lua_path, fn.name, reason))
    stats.removed_free_functions.append((fn.lua_path, fn.name, reason))


def _apply_free_function_intersection(
    plan: EmitPlan,
    target_platform: str,
    result: IntersectionResult,
    stats: IntersectionStats,
) -> None:
    kept: list[Function] = []
    for fn in plan.supported_free_functions:
        key = free_function_key(fn)
        if key in result.common_supported_free_function_keys:
            kept.append(fn)
            continue
        missing = result.missing_free_function_platforms_by_key.get(key, ())
        if not missing:
            missing = tuple(
                platform for platform in result.platforms if platform != target_platform
            )
        reason = _intersection_reason(missing)
        _append_free_function_skip(plan, stats, fn, reason)
    plan.supported_free_functions = kept


def _prune_free_function_refs(
    plan: EmitPlan,
    target_platform: str,
    stats: IntersectionStats,
) -> None:
    kept, skipped = _filter_free_function_object_refs(
        plan.supported_free_functions,
        plan.objects,
        plan.skipped_classes,
        target_platform,
        ctx=plan.ctx,
        analysis=plan.type_analysis,
    )
    plan.supported_free_functions = kept
    for key, lua_path, name, reason in skipped:
        plan.skipped_free_functions.append((key, lua_path, name, reason))
        stats.removed_free_functions.append((lua_path, name, reason))


def _apply_intersection(
    plan: EmitPlan,
    target_platform: str,
    result: IntersectionResult,
) -> EmitPlan:
    stats = IntersectionStats(
        enabled=True,
        platforms=result.platforms,
        common_supported_methods=len(result.common_supported_method_keys),
        common_hook_targets=len(result.common_hook_method_keys),
        common_fields=len(result.common_field_keys),
        common_free_functions=len(result.common_supported_free_function_keys),
    )
    plan.intersection_stats = stats
    _apply_free_function_intersection(plan, target_platform, result, stats)

    for cls in plan.classes:
        grouped = plan.supported_by_class[cls.name]
        for name, methods in list(grouped.items()):
            kept: list[Method] = []
            for method in methods:
                key = method_key(cls, method)
                if key in result.common_supported_method_keys:
                    kept.append(method)
                    continue
                missing = result.missing_supported_platforms_by_key.get(key, ())
                if not missing:
                    missing = tuple(
                        platform for platform in result.platforms if platform != target_platform
                    )
                reason = _intersection_reason(missing)
                _append_skip(plan, cls, method, reason)
                stats.removed_methods.append((cls.name, method.name, reason))
            if kept:
                grouped[name] = kept
            else:
                del grouped[name]

        kept_hooks: list[tuple[Class, Method]] = []
        for hook_cls, method in plan.hook_targets_by_class.get(cls.name, []):
            key = method_key(hook_cls, method)
            if key in result.common_hook_method_keys:
                kept_hooks.append((hook_cls, method))
                continue
            missing = result.missing_hook_platforms_by_key.get(key, ())
            reason = _intersection_reason(missing)
            stats.removed_hooks.append((hook_cls.name, method.name, reason))
        plan.hook_targets_by_class[cls.name] = kept_hooks

        kept_fields: list[tuple[Class, Field]] = []
        for field_cls, bound_field in plan.field_targets_by_class.get(cls.name, []):
            key = field_key(field_cls, bound_field)
            if key in result.common_field_keys:
                kept_fields.append((field_cls, bound_field))
                continue
            missing = result.missing_field_platforms_by_key.get(key, ())
            reason = _intersection_reason(missing)
            stats.removed_fields.append((field_cls.name, bound_field.name, reason))
        plan.field_targets_by_class[cls.name] = kept_fields

    changed = True
    while changed:
        changed = False
        type_needed = _type_needed_class_names(plan)
        outputless = {
            cls.name
            for cls in plan.classes
            if cls.name not in type_needed
            and not plan.supported_by_class[cls.name]
            and not plan.hook_targets_by_class[cls.name]
            and not plan.field_targets_by_class.get(cls.name, [])
        }
        new_skipped = outputless - plan.skipped_classes
        if new_skipped:
            plan.skipped_classes.update(new_skipped)
            changed = True

        pruned = prune_skipped_class_refs(
            plan.supported_by_class,
            plan.skipped_by_class,
            plan.objects,
            plan.skipped_classes,
            target_platform,
            ctx=plan.ctx,
            analysis=plan.type_analysis,
        )
        if pruned:
            plan.skipped.extend(pruned)
            changed = True

        field_pruned = _prune_field_object_refs(
            plan.field_targets_by_class,
            plan.objects,
            plan.skipped_classes,
            target_platform,
            ctx=plan.ctx,
            analysis=plan.type_analysis,
        )
        if field_pruned:
            plan.skipped.extend(field_pruned)
            changed = True

        for cls_name in list(plan.hook_targets_by_class):
            if cls_name in plan.skipped_classes and plan.hook_targets_by_class[cls_name]:
                plan.hook_targets_by_class[cls_name] = []
                changed = True
            if cls_name in plan.skipped_classes and plan.field_targets_by_class[cls_name]:
                plan.field_targets_by_class[cls_name] = []
                changed = True

    hierarchy = plan.hierarchy or ClassHierarchy(plan.classes)
    plan.hierarchy = hierarchy
    plan.depths = {cls.name: hierarchy.depth(cls, plan.skipped_classes) for cls in plan.classes}
    _prune_free_function_refs(plan, target_platform, stats)
    plan.emitted_classes = _emitted_classes(plan)
    return plan


def _copy_mutable_plan_collections(plan: EmitPlan) -> EmitPlan:
    return replace(
        plan,
        skipped=list(plan.skipped),
        skipped_by_class={name: list(entries) for name, entries in plan.skipped_by_class.items()},
        skipped_classes=set(plan.skipped_classes),
        supported_by_class={
            cls_name: {name: list(methods) for name, methods in grouped.items()}
            for cls_name, grouped in plan.supported_by_class.items()
        },
        hook_targets_by_class={
            name: list(targets) for name, targets in plan.hook_targets_by_class.items()
        },
        field_targets_by_class={
            name: list(targets) for name, targets in plan.field_targets_by_class.items()
        },
        supported_free_functions=list(plan.supported_free_functions),
        skipped_free_functions=list(plan.skipped_free_functions),
        emitted_classes=list(plan.emitted_classes),
        intersection_stats=IntersectionStats(),
    )


def collect_plan(
    root: Root,
    target_platform: str = "win",
    plans_by_platform: dict[str, EmitPlan] | None = None,
) -> EmitPlan:
    platforms = intersection_platforms(target_platform)
    raw_plans = plans_by_platform or {}
    if raw_plans:
        exemplar = next(iter(raw_plans.values()))
        hierarchy = exemplar.hierarchy or ClassHierarchy(root.classes)
        ctx = exemplar.ctx
        analysis = exemplar.type_analysis or TypeAnalysis(exemplar.objects, ctx)
    else:
        hierarchy = ClassHierarchy(root.classes)
        classes = hierarchy.object_classes()
        ctx = _ctx_from_root(root)
        analysis = TypeAnalysis(build_class_lookup(classes), ctx)
    missing = [platform for platform in platforms if platform not in raw_plans]
    if missing:
        raw_plans = dict(raw_plans)
        for platform in missing:
            raw_plans[platform] = collect_platform_plan(
                root, platform, hierarchy=hierarchy, ctx=ctx, analysis=analysis
            )

    if target_platform in raw_plans:
        plan = (
            _copy_mutable_plan_collections(raw_plans[target_platform])
            if plans_by_platform
            else raw_plans[target_platform]
        )
    else:
        plan = collect_platform_plan(
            root,
            target_platform,
            hierarchy=hierarchy,
            ctx=ctx,
            analysis=analysis,
        )

    result = collect_intersection(raw_plans, platforms=platforms)
    return _apply_intersection(plan, target_platform, result)


def plan_outputs(
    root: Root,
    target_platform: str = "win",
    plan: EmitPlan | None = None,
    plans_by_platform: dict[str, EmitPlan] | None = None,
) -> list[str]:
    if plan is None:
        plan = collect_plan(root, target_platform, plans_by_platform=plans_by_platform)
    outputs = [
        "bindings_internal.hpp",
        "bindings_common.cpp",
        ENUM_KEY_CODES_MANIFEST,
        GEODE_ENUMS_MANIFEST,
        GEODE_ENUMS_BINDING,
        "bindings_free_functions.cpp",
    ]
    outputs.extend(binding_filename(cls.name) for cls in plan.emitted_classes)
    return outputs


def hook_target_count(
    root: Root,
    target_platform: str = "win",
    plan: EmitPlan | None = None,
    plans_by_platform: dict[str, EmitPlan] | None = None,
) -> int:
    if plan is None:
        plan = collect_plan(root, target_platform, plans_by_platform=plans_by_platform)
    return sum(len(targets) for targets in plan.hook_targets_by_class.values())


def field_target_count(
    root: Root,
    target_platform: str = "win",
    plan: EmitPlan | None = None,
    plans_by_platform: dict[str, EmitPlan] | None = None,
) -> int:
    if plan is None:
        plan = collect_plan(root, target_platform, plans_by_platform=plans_by_platform)
    return sum(len(targets) for targets in plan.field_targets_by_class.values())


def ambiguous_overloads(plan: EmitPlan) -> list[tuple[str, str, str]]:
    return [
        (cls, method, reason)
        for cls, method, reason in plan.skipped
        if reason.startswith("ambiguous-overload-arity:")
    ]
