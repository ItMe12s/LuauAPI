from __future__ import annotations

import json
import os

from luau_codegen.parse.broma import Root
from luau_codegen.emit.plan import EmitPlan, ambiguous_overloads
from luau_codegen.policy.fields import field_key
from luau_codegen.policy.free_functions import free_function_key
from luau_codegen.model.domain import object_classes, status_for
from luau_codegen.cli.io import _write_if_changed


def emit_schema(root: Root, path: str, plan: EmitPlan) -> None:
    bound_fields = {
        field_key(cls, field)
        for targets in plan.field_targets_by_class.values()
        for cls, field in targets
    }
    classes = []
    for cls in root.classes:
        classes.append(
            {
                "name": cls.name,
                "qualifiedName": cls.qualified_name,
                "bases": cls.bases,
                "attributes": cls.attributes,
                "source": os.path.basename(cls.source),
                "line": cls.line,
                "methods": [
                    {
                        "name": m.name,
                        "ret": m.ret,
                        "args": [{"type": a.type, "name": a.name} for a in m.args],
                        "static": m.is_static,
                        "virtual": m.is_virtual,
                        "const": m.is_const,
                        "ctor": m.is_ctor,
                        "dtor": m.is_dtor,
                        "platforms": m.platforms,
                        "status": status_for(m.platforms),
                        "line": m.line,
                    }
                    for m in cls.methods
                ],
                "fields": [
                    {
                        "name": f.name,
                        "type": f.type,
                        "count": f.count,
                        "line": f.line,
                        "bound": field_key(cls, f) in bound_fields,
                    }
                    for f in cls.fields
                ],
            }
        )
    ambiguous = [
        {"class": cls, "method": method, "reason": reason}
        for cls, method, reason in ambiguous_overloads(plan)
    ]
    supported_free_functions = [
        {
            "key": free_function_key(fn),
            "namespace": fn.namespace,
            "luaPath": fn.lua_path,
            "name": fn.name,
            "ret": fn.ret,
            "args": [{"type": a.type, "name": a.name} for a in fn.args],
        }
        for fn in plan.supported_free_functions
    ]
    _write_if_changed(
        path,
        json.dumps(
            {
                "classes": classes,
                "ambiguousOverloads": ambiguous,
                "supportedFreeFunctions": supported_free_functions,
            },
            indent=2,
        )
        + "\n",
    )


def emit_report(
    root: Root,
    path: str,
    cxx_paths: list[str],
    types_path: str,
    skipped: list[tuple[str, str, str]],
    target_platform: str,
    hook_target_count: int,
    field_target_count: int,
    plan: EmitPlan | None = None,
    plans_by_platform: dict[str, EmitPlan] | None = None,
) -> None:
    obj = object_classes(root)
    total_methods = sum(len(c.methods) for c in root.classes)
    emitted_methods = total_methods - len(skipped)
    reasons: dict[str, int] = {}
    for _, _, reason in skipped:
        reasons[reason] = reasons.get(reason, 0) + 1
    free_reasons: dict[str, int] = {}
    free_skips = plan.skipped_free_functions if plan else []
    for _, _, _, reason in free_skips:
        free_reasons[reason] = free_reasons.get(reason, 0) + 1
    lines = [
        "# LuauAPI codegen report\n\n",
        f"- Target platform: **{target_platform}**\n",
        f"- Classes parsed: **{len(root.classes)}**\n",
        f"- CCObject classes emitted: **{len(obj)}**\n",
        f"- Methods parsed: **{total_methods}**\n",
        f"- Methods emitted: **{emitted_methods}**\n",
        f"- Methods skipped: **{len(skipped)}**\n",
        f"- Free functions parsed: **{len(root.functions)}**\n",
        (
            "- Free functions emitted: "
            f"**{len(plan.supported_free_functions) if plan else 0}**\n"
        ),
        f"- Free functions skipped: **{len(free_skips)}**\n",
        f"- Hook targets emitted: **{hook_target_count}**\n",
        f"- Fields bound: **{field_target_count}**\n",
        f"- Generated binding files: **{len(cxx_paths)}**\n",
    ]
    if plan and plan.intersection_stats.enabled:
        stats = plan.intersection_stats
        lines.extend(
            [
                "- Intersection mode: **enabled**\n",
                f"- Intersection platforms: **{', '.join(stats.platforms)}**\n",
                f"- Common binding methods: **{stats.common_supported_methods}**\n",
                f"- Common hook targets: **{stats.common_hook_targets}**\n",
                f"- Common fields: **{stats.common_fields}**\n",
                f"- Common free functions: **{stats.common_free_functions}**\n",
                f"- Methods removed by intersection: **{len(stats.removed_methods)}**\n",
                f"- Hooks removed by intersection: **{len(stats.removed_hooks)}**\n",
                (
                    "- Free functions removed by intersection: "
                    f"**{len(stats.removed_free_functions)}**\n"
                ),
            ]
        )
    for cxx_path in cxx_paths:
        lines.append(f"  - `{cxx_path}`\n")
    lines.append(f"- Generated Luau types: `{types_path}`\n\n")
    if plans_by_platform:
        lines.append("## Linkless classes\n\n")
        lines.append(
            "Per platform, classes with no callable members "
            "(linkless / skipped, treated as opaque).\n\n"
        )
        for platform in sorted(plans_by_platform):
            names = sorted(plans_by_platform[platform].skipped_classes)
            lines.append(f"### {platform}: {len(names)}\n\n")
            for name in names[:2000]:
                lines.append(f"- {name}\n")
            if len(names) > 2000:
                lines.append(f"- ... {len(names) - 2000} more\n")
            lines.append("\n")
    lines.append("## Skip reasons\n\n")
    for reason, count in sorted(reasons.items(), key=lambda i: (-i[1], i[0])):
        lines.append(f"- {reason}: {count}\n")
    lines.append("\n## Free-function skip reasons\n\n")
    if free_reasons:
        for reason, count in sorted(free_reasons.items(), key=lambda i: (-i[1], i[0])):
            lines.append(f"- {reason}: {count}\n")
    else:
        lines.append("- none\n")
    ambiguous = ambiguous_overloads(plan) if plan else []
    lines.append("\n## Ambiguous overloads\n\n")
    if ambiguous:
        for cls, method, reason in ambiguous[:2000]:
            lines.append(f"- {cls}.{method}: {reason}\n")
        if len(ambiguous) > 2000:
            lines.append(f"- ... {len(ambiguous) - 2000} more\n")
    else:
        lines.append("- none\n")
    lines.append("\n## Operational notes\n\n")
    lines.append(
        "- generated Luau types are written to repo `types/` for LSP use and remain gitignored\n"
    )
    lines.append(
        "- geode_bindings is pinned via LUAUAPI_BINDINGS_GIT_TAG in CMake, bump mod.json GD version and the pin together\n"
    )
    lines.append(
        "- generated binding .cpp files are listed at CMake configure via --list-outputs, reconfigure when new bindings_<Class>.cpp are emitted\n"
    )
    lines.append(
        "- userdata tag budget assert remains generated-class based, runtime distinct-type tightening is deferred\n"
    )
    lines.append(
        "- removed hook callback slots compact on later registry operations, eager compaction is deferred\n"
    )
    lines.append("\n## Skipped methods\n\n")
    for cls, method, reason in skipped[:2000]:
        lines.append(f"- {cls}.{method}: {reason}\n")
    if len(skipped) > 2000:
        lines.append(f"- ... {len(skipped) - 2000} more\n")
    lines.append("\n## Skipped free functions\n\n")
    for _, lua_path, name, reason in free_skips[:2000]:
        lines.append(f"- {lua_path}.{name}: {reason}\n")
    if len(free_skips) > 2000:
        lines.append(f"- ... {len(free_skips) - 2000} more\n")
    _write_if_changed(path, "".join(lines))
