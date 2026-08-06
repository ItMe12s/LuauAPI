from __future__ import annotations

from collections.abc import Mapping


PLATFORMS = ("win", "imac", "m1", "ios", "android", "android32", "android64", "mac")
VALID_PLATFORMS = frozenset(PLATFORMS)
STRICT_DIRECT_PLATFORMS = frozenset({"ios"})
INTERSECTION_PLATFORMS = ("win", "m1", "ios", "android32", "android64")
PLATFORM_BLOCK_TOKENS = VALID_PLATFORMS
PLATFORM_SCOPE_CANDIDATES = frozenset(INTERSECTION_PLATFORMS) | {"imac"}
PLATFORM_ALIASES: dict[str, frozenset[str]] = {
    "mac": frozenset({"mac", "imac", "m1"}),
    "imac": frozenset({"mac", "imac"}),
    "m1": frozenset({"mac", "m1"}),
    "android": frozenset({"android", "android32", "android64"}),
    "android32": frozenset({"android", "android32"}),
    "android64": frozenset({"android", "android64"}),
}
SCANNED_LINK_PLATFORMS = (
    "win",
    "android",
    "android32",
    "android64",
    "imac",
    "m1",
    "ios",
)
SCANNED_LINK_ATTR = f"link({', '.join(SCANNED_LINK_PLATFORMS)})"


def platform_aliases(target_platform: str) -> frozenset[str]:
    return PLATFORM_ALIASES.get(target_platform, frozenset({target_platform}))


def platform_value(platforms: Mapping[str, str], target_platform: str) -> str:
    value = platforms.get(target_platform, "")
    if value:
        return value
    if target_platform == "mac":
        return platforms.get("imac", "") or platforms.get("m1", "")
    if target_platform in ("imac", "m1"):
        return platforms.get("mac", "")
    if target_platform == "android":
        return platforms.get("android64", "") or platforms.get("android32", "")
    return ""


def intersection_platforms(target_platform: str = "win") -> tuple[str, ...]:
    if target_platform == "mac":
        return ("win", "imac", "m1", "ios", "android32", "android64")
    mac = "imac" if target_platform == "imac" else "m1"
    return ("win", mac, "ios", "android32", "android64")
