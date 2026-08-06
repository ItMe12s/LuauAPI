from __future__ import annotations


FreeFnSource = tuple[str, frozenset[str], frozenset[str] | None]

FREE_FUNCTION_SOURCES: tuple[FreeFnSource, ...] = (
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
    ("ui/General.hpp", frozenset({"geode"}), frozenset({"pushSceneWithLayer"})),
    ("ui/Popup.hpp", frozenset({"geode"}), frozenset({"createQuickPopup"})),
    ("ui/GeodeUI.hpp", frozenset({"geode"}), None),
    ("utils/string.hpp", frozenset({"geode::utils::string"}), None),
    ("utils/random.hpp", frozenset({"geode::utils::random"}), None),
    ("utils/cocos.hpp", frozenset({"geode::cocos"}), None),
)


def free_function_includes() -> tuple[str, ...]:
    return tuple(f"Geode/{rel}" for rel, _ns, _names in FREE_FUNCTION_SOURCES)
