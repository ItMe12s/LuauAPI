from __future__ import annotations

import re
import sys
from collections import Counter, defaultdict
from pathlib import Path

from luau_codegen.model.value_struct_gate import DENIED_VALUE_STRUCTS


def main() -> int:
    path = Path(__file__).resolve().parents[2] / "types" / "geode.d.luau"
    if not path.is_file():
        print(f"missing {path}", file=sys.stderr)
        return 1

    lines = path.read_text(encoding="utf-8").splitlines()
    skipped = [line.strip() for line in lines if "-- skipped" in line]
    print(f"total_skipped: {len(skipped)}")

    reasons = Counter()
    for line in skipped:
        match = re.search(r"-- skipped [^:]+: (.+)$", line)
        reasons[match.group(1) if match else "<unparsed>"] += 1

    print("\nTop reasons:")
    for reason, count in reasons.most_common(40):
        print(f"  {count:4d}  {reason}")

    buckets: dict[str, list[str]] = defaultdict(list)
    for line in skipped:
        match = re.search(r"-- skipped ([^:]+): (.+)$", line)
        if not match:
            buckets["unparsed"].append(line)
            continue
        field_name, reason = match.group(1), match.group(2)
        entry = f"{field_name}: {reason}"
        if reason.startswith(
            ("unsupported-arg:gd::set<", "unsupported-return:gd::set<")
        ) or (
            "gd::unordered_set<" in reason
            and reason.startswith(("unsupported-arg:", "unsupported-return:"))
        ):
            if "*" in reason:
                buckets["object_set"].append(entry)
            else:
                buckets["set_primitive"].append(entry)
        elif (
            reason.startswith(
                ("unsupported-arg:gd::vector<", "unsupported-return:gd::vector<")
            )
            and "*" in reason
        ):
            if "void*" in reason:
                buckets["object_vector_void"].append(entry)
            elif "gd::vector<gd::vector<" in reason:
                buckets["object_vector_nested"].append(entry)
            else:
                buckets["object_vector"].append(entry)
        elif "std::pair" in reason:
            buckets["pair"].append(entry)
        elif reason.startswith("unsupported-arg:") and any(
            f"<{name}>" in reason or reason.endswith(f":{name}")
            for name in DENIED_VALUE_STRUCTS
        ):
            buckets["value_struct_denied"].append(entry)
        elif reason.startswith("unsupported-arg:") and not any(
            token in reason
            for token in (
                "gd::vector",
                "gd::map",
                "gd::set",
                "gd::unordered_map",
                "gd::unordered_set",
            )
        ):
            buckets["residual_enum_or_type"].append(entry)
        elif reason in {
            "array",
            "reference",
            "function-pointer",
            "string-pointer",
            "inaccessible-field",
        } or reason.startswith("inaccessible-type:"):
            buckets["policy"].append(entry)
        else:
            buckets["other"].append(entry)

    print("\nCategory counts:")
    for key in sorted(buckets):
        print(f"  {key}: {len(buckets[key])}")

    for key in (
        "object_set",
        "object_vector",
        "object_vector_nested",
        "object_vector_void",
        "pair",
        "value_struct_denied",
        "residual_enum_or_type",
    ):
        samples = buckets.get(key, [])
        if not samples:
            continue
        print(f"\n{key} samples:")
        for sample in samples[:20]:
            print(f"  {sample}")
        if len(samples) > 20:
            print(f"  ... {len(samples) - 20} more")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
