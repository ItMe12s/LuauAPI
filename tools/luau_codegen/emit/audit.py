from __future__ import annotations

from collections import Counter
from typing import Any

from luau_codegen.parse.broma import Root
from luau_codegen.emit.plan import EmitPlan

_SAMPLE_LIMIT = 25

_BUCKET_DEFS: tuple[tuple[str, str], ...] = (
    ("callback_method", "Broma callback methods"),
    ("sel_arg", "cocos2d selector args"),
    ("std_function_arg", "std::function / geode::Function args"),
    ("callback_alias", "Callback alias args"),
    ("delegate_arg", "delegate pointer args"),
    ("container_arg", "gd container args"),
    ("value_type_arg", "FMOD / Kazmath value types"),
    ("http_async_excluded", "HTTP / async (scan excluded)"),
    ("other", "unclassified skips"),
)

_VALUE_TYPE_PREFIXES = ("FMOD::", "FMOD_", "km", "Kazmath", "kazmath")


def _type_from_reason(reason: str) -> str:
    for prefix in (
        "unsupported-arg:",
        "unsupported-return:",
        "free-function-unsupported-arg:",
    ):
        if reason.startswith(prefix):
            return reason[len(prefix) :]
    return ""


def _classify_method_skip(reason: str) -> str:
    if reason == "callback":
        return "callback_method"
    type_part = _type_from_reason(reason)
    check = type_part or reason
    if reason.startswith("unsupported-arg:cocos2d::SEL_"):
        return "sel_arg"
    if any(
        token in check
        for token in ("std::function", "geode::Function", "Function<", "MiniFunction")
    ):
        return "std_function_arg"
    if type_part == "Callback" or check.endswith(":Callback"):
        return "callback_alias"
    if "Delegate" in check and (
        reason.startswith("unsupported-arg:") or reason.startswith("unsupported-return:")
    ):
        return "delegate_arg"
    if any(
        token in check
        for token in (
            "gd::vector",
            "gd::map",
            "gd::set",
            "gd::unordered_map",
            "gd::unordered_set",
        )
    ):
        return "container_arg"
    if any(token in check for token in _VALUE_TYPE_PREFIXES):
        return "value_type_arg"
    return "other"


def _classify_free_function_skip(reason: str) -> str:
    check = _type_from_reason(reason) or reason
    if any(token in check for token in ("Task", "async", "web::", "web/")):
        return "http_async_excluded"
    if any(
        token in check
        for token in ("std::function", "geode::Function", "Function<", "MiniFunction")
    ):
        return "std_function_arg"
    if "Delegate" in check:
        return "delegate_arg"
    if any(
        token in check
        for token in (
            "gd::vector",
            "gd::map",
            "gd::set",
            "gd::unordered_map",
            "gd::unordered_set",
        )
    ):
        return "container_arg"
    if any(token in check for token in _VALUE_TYPE_PREFIXES):
        return "value_type_arg"
    return "other"


def _empty_bucket(bucket_id: str, label: str) -> dict[str, Any]:
    return {
        "id": bucket_id,
        "label": label,
        "count": 0,
        "reasonHistogram": {},
        "samples": [],
    }


def collect_audit(plan: EmitPlan, root: Root) -> dict[str, Any]:
    del root
    buckets: dict[str, dict[str, Any]] = {
        bucket_id: _empty_bucket(bucket_id, label) for bucket_id, label in _BUCKET_DEFS
    }
    reason_counters: dict[str, Counter[str]] = {
        bucket_id: Counter() for bucket_id, _ in _BUCKET_DEFS
    }
    sample_lists: dict[str, list[str]] = {bucket_id: [] for bucket_id, _ in _BUCKET_DEFS}

    for cls_name, method_name, reason in plan.skipped:
        bucket_id = _classify_method_skip(reason)
        reason_counters[bucket_id][reason] += 1
        entry = f"{cls_name}.{method_name}: {reason}"
        if len(sample_lists[bucket_id]) < _SAMPLE_LIMIT:
            sample_lists[bucket_id].append(entry)

    for _, lua_path, name, reason in plan.skipped_free_functions:
        bucket_id = _classify_free_function_skip(reason)
        reason_counters[bucket_id][reason] += 1
        entry = f"{lua_path}.{name}: {reason}"
        if len(sample_lists[bucket_id]) < _SAMPLE_LIMIT:
            sample_lists[bucket_id].append(entry)

    for bucket_id, _ in _BUCKET_DEFS:
        buckets[bucket_id]["count"] = sum(reason_counters[bucket_id].values())
        buckets[bucket_id]["reasonHistogram"] = dict(reason_counters[bucket_id].most_common())
        buckets[bucket_id]["samples"] = sorted(sample_lists[bucket_id])

    return {
        "buckets": buckets,
        "totalSkippedMethods": len(plan.skipped),
        "totalSkippedFreeFunctions": len(plan.skipped_free_functions),
    }


def emit_markdown(data: dict[str, Any]) -> str:
    lines = [
        "# LuauAPI skip audit\n\n",
        "Callback, delegate, container, and related skips grouped by category.\n",
        "See `docs/reference/lua/callbacks.md` and `docs/reference/lua/delegates.md` "
        "for supported callback and delegate patterns.\n\n",
        "## Summary\n\n",
        "| Bucket | Skips | Category |\n",
        "|--------|-------|----------|\n",
    ]
    for bucket_id, label in _BUCKET_DEFS:
        bucket = data["buckets"][bucket_id]
        if bucket["count"] == 0:
            continue
        lines.append(f"| {bucket_id} | {bucket['count']} | {label} |\n")

    lines.append(f"\n- Methods skipped (intersected plan): **{data['totalSkippedMethods']}**\n")
    lines.append(f"- Free functions skipped: **{data['totalSkippedFreeFunctions']}**\n\n")

    lines.append("## Samples by bucket\n\n")
    for bucket_id, _ in _BUCKET_DEFS:
        bucket = data["buckets"][bucket_id]
        if bucket["count"] == 0:
            continue
        lines.append(f"### {bucket_id}\n\n")
        if bucket["reasonHistogram"]:
            lines.append("Top reasons:\n\n")
            for reason, count in list(bucket["reasonHistogram"].items())[:10]:
                lines.append(f"- {reason}: {count}\n")
            lines.append("\n")
        if bucket["samples"]:
            lines.append("Sample skips:\n\n")
            for sample in bucket["samples"]:
                lines.append(f"- {sample}\n")
            remaining = bucket["count"] - len(bucket["samples"])
            if remaining > 0:
                lines.append(f"- ... {remaining} more\n")
            lines.append("\n")

    return "".join(lines)
