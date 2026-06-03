from __future__ import annotations

CC_C_ARRAY_FIELD_ELEMENTS: dict[tuple[str, str], str] = {
    ("CCKeyboardDispatcher", "m_pHandlersToAdd"): "CCKeyboardHandler",
    ("CCKeyboardDispatcher", "m_pHandlersToRemove"): "CCKeyboardHandler",
    ("CCKeypadDispatcher", "m_pHandlersToAdd"): "CCKeypadHandler",
    ("CCKeypadDispatcher", "m_pHandlersToRemove"): "CCKeypadHandler",
    ("CCMouseDispatcher", "m_pHandlersToAdd"): "CCMouseHandler",
    ("CCMouseDispatcher", "m_pHandlersToRemove"): "CCMouseHandler",
    ("CCTouchDispatcher", "m_pHandlersToRemove"): "CCTouchHandler",
}

CC_C_ARRAY_POINTER_TYPES = frozenset(
    {
        "cocos2d::ccCArray*",
        "ccCArray*",
    }
)


def proven_cc_c_array_element(owner_class: str, field_name: str) -> str | None:
    return CC_C_ARRAY_FIELD_ELEMENTS.get((owner_class, field_name))
