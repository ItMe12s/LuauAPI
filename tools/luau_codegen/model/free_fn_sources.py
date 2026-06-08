from __future__ import annotations

from typing import FrozenSet, Optional, Tuple

FreeFnSource = Tuple[str, FrozenSet[str], Optional[FrozenSet[str]]]

FREE_FUNCTION_SOURCES: Tuple[FreeFnSource, ...] = (
    (
        "utils/general.hpp",
        frozenset(
            {
                "geode::utils",
                "geode::utils::clipboard",
                "geode::utils::game",
                "geode::utils::thread",
                "geode::utils::platform",
            }
        ),
        None,
    ),
    ("ui/Popup.hpp", frozenset({"geode"}), frozenset({"createQuickPopup"})),
    ("ui/GeodeUI.hpp", frozenset({"geode"}), None),
    ("utils/string.hpp", frozenset({"geode::utils::string"}), None),
    ("utils/random.hpp", frozenset({"geode::utils::random"}), None),
    ("utils/cocos.hpp", frozenset({"geode::cocos"}), None),
)


def free_function_includes() -> Tuple[str, ...]:
    return tuple(f"Geode/{rel}" for rel, _ns, _names in FREE_FUNCTION_SOURCES)
