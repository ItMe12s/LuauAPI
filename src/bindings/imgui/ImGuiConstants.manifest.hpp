#pragma once

#define LUAX_IMGUI_ENUM_ENTRY(Prefix, name) {#name, Prefix##_##name},

#define LUAX_IMGUI_WINDOW_FLAGS(X)                 \
    X(ImGuiWindowFlags, None)                      \
    X(ImGuiWindowFlags, NoTitleBar)                \
    X(ImGuiWindowFlags, NoResize)                  \
    X(ImGuiWindowFlags, NoMove)                    \
    X(ImGuiWindowFlags, NoScrollbar)               \
    X(ImGuiWindowFlags, NoScrollWithMouse)         \
    X(ImGuiWindowFlags, NoCollapse)                \
    X(ImGuiWindowFlags, AlwaysAutoResize)          \
    X(ImGuiWindowFlags, NoBackground)              \
    X(ImGuiWindowFlags, NoSavedSettings)           \
    X(ImGuiWindowFlags, NoMouseInputs)             \
    X(ImGuiWindowFlags, MenuBar)                   \
    X(ImGuiWindowFlags, HorizontalScrollbar)       \
    X(ImGuiWindowFlags, NoFocusOnAppearing)        \
    X(ImGuiWindowFlags, NoBringToFrontOnFocus)     \
    X(ImGuiWindowFlags, AlwaysVerticalScrollbar)   \
    X(ImGuiWindowFlags, AlwaysHorizontalScrollbar) \
    X(ImGuiWindowFlags, NoNavInputs)               \
    X(ImGuiWindowFlags, NoNavFocus)                \
    X(ImGuiWindowFlags, UnsavedDocument)           \
    X(ImGuiWindowFlags, NoNav)                     \
    X(ImGuiWindowFlags, NoDecoration)              \
    X(ImGuiWindowFlags, NoInputs)

#define LUAX_IMGUI_CHILD_FLAGS(X)              \
    X(ImGuiChildFlags, None)                   \
    X(ImGuiChildFlags, Borders)                \
    X(ImGuiChildFlags, AlwaysUseWindowPadding) \
    X(ImGuiChildFlags, ResizeX)                \
    X(ImGuiChildFlags, ResizeY)                \
    X(ImGuiChildFlags, AutoResizeX)            \
    X(ImGuiChildFlags, AutoResizeY)            \
    X(ImGuiChildFlags, AlwaysAutoResize)       \
    X(ImGuiChildFlags, FrameStyle)             \
    X(ImGuiChildFlags, NavFlattened)

#define LUAX_IMGUI_INPUT_TEXT_FLAGS(X)          \
    X(ImGuiInputTextFlags, None)                \
    X(ImGuiInputTextFlags, CharsDecimal)        \
    X(ImGuiInputTextFlags, CharsHexadecimal)    \
    X(ImGuiInputTextFlags, CharsScientific)     \
    X(ImGuiInputTextFlags, CharsUppercase)      \
    X(ImGuiInputTextFlags, CharsNoBlank)        \
    X(ImGuiInputTextFlags, AllowTabInput)       \
    X(ImGuiInputTextFlags, EnterReturnsTrue)    \
    X(ImGuiInputTextFlags, EscapeClearsAll)     \
    X(ImGuiInputTextFlags, CtrlEnterForNewLine) \
    X(ImGuiInputTextFlags, ReadOnly)            \
    X(ImGuiInputTextFlags, Password)            \
    X(ImGuiInputTextFlags, AlwaysOverwrite)     \
    X(ImGuiInputTextFlags, AutoSelectAll)       \
    X(ImGuiInputTextFlags, NoHorizontalScroll)  \
    X(ImGuiInputTextFlags, NoUndoRedo)          \
    X(ImGuiInputTextFlags, ElideLeft)           \
    X(ImGuiInputTextFlags, ParseEmptyRefVal)    \
    X(ImGuiInputTextFlags, DisplayEmptyRefVal)  \
    X(ImGuiInputTextFlags, WordWrap)

#define LUAX_IMGUI_ITEM_FLAGS(X)         \
    X(ImGuiItemFlags, None)              \
    X(ImGuiItemFlags, NoTabStop)         \
    X(ImGuiItemFlags, NoNav)             \
    X(ImGuiItemFlags, NoNavDefaultFocus) \
    X(ImGuiItemFlags, ButtonRepeat)      \
    X(ImGuiItemFlags, AutoClosePopups)   \
    X(ImGuiItemFlags, AllowDuplicateId)  \
    X(ImGuiItemFlags, Disabled)

#define LUAX_IMGUI_TREE_NODE_FLAGS(X)           \
    X(ImGuiTreeNodeFlags, None)                 \
    X(ImGuiTreeNodeFlags, Selected)             \
    X(ImGuiTreeNodeFlags, Framed)               \
    X(ImGuiTreeNodeFlags, AllowOverlap)         \
    X(ImGuiTreeNodeFlags, NoTreePushOnOpen)     \
    X(ImGuiTreeNodeFlags, NoAutoOpenOnLog)      \
    X(ImGuiTreeNodeFlags, DefaultOpen)          \
    X(ImGuiTreeNodeFlags, OpenOnDoubleClick)    \
    X(ImGuiTreeNodeFlags, OpenOnArrow)          \
    X(ImGuiTreeNodeFlags, Leaf)                 \
    X(ImGuiTreeNodeFlags, Bullet)               \
    X(ImGuiTreeNodeFlags, FramePadding)         \
    X(ImGuiTreeNodeFlags, SpanAvailWidth)       \
    X(ImGuiTreeNodeFlags, SpanFullWidth)        \
    X(ImGuiTreeNodeFlags, SpanLabelWidth)       \
    X(ImGuiTreeNodeFlags, SpanAllColumns)       \
    X(ImGuiTreeNodeFlags, NavLeftJumpsToParent) \
    X(ImGuiTreeNodeFlags, DrawLinesNone)        \
    X(ImGuiTreeNodeFlags, DrawLinesFull)        \
    X(ImGuiTreeNodeFlags, DrawLinesToNodes)     \
    X(ImGuiTreeNodeFlags, CollapsingHeader)

#define LUAX_IMGUI_POPUP_FLAGS(X)               \
    X(ImGuiPopupFlags, None)                    \
    X(ImGuiPopupFlags, MouseButtonLeft)         \
    X(ImGuiPopupFlags, MouseButtonRight)        \
    X(ImGuiPopupFlags, MouseButtonMiddle)       \
    X(ImGuiPopupFlags, NoReopen)                \
    X(ImGuiPopupFlags, NoOpenOverExistingPopup) \
    X(ImGuiPopupFlags, NoOpenOverItems)         \
    X(ImGuiPopupFlags, AnyPopupId)              \
    X(ImGuiPopupFlags, AnyPopupLevel)           \
    X(ImGuiPopupFlags, AnyPopup)

#define LUAX_IMGUI_SELECTABLE_FLAGS(X)         \
    X(ImGuiSelectableFlags, None)              \
    X(ImGuiSelectableFlags, NoAutoClosePopups) \
    X(ImGuiSelectableFlags, SpanAllColumns)    \
    X(ImGuiSelectableFlags, AllowDoubleClick)  \
    X(ImGuiSelectableFlags, Disabled)          \
    X(ImGuiSelectableFlags, AllowOverlap)      \
    X(ImGuiSelectableFlags, Highlight)         \
    X(ImGuiSelectableFlags, SelectOnNav)

#define LUAX_IMGUI_COLOR_EDIT_FLAGS(X)       \
    X(ImGuiColorEditFlags, None)             \
    X(ImGuiColorEditFlags, NoAlpha)          \
    X(ImGuiColorEditFlags, NoPicker)         \
    X(ImGuiColorEditFlags, NoOptions)        \
    X(ImGuiColorEditFlags, NoSmallPreview)   \
    X(ImGuiColorEditFlags, NoInputs)         \
    X(ImGuiColorEditFlags, NoTooltip)        \
    X(ImGuiColorEditFlags, NoLabel)          \
    X(ImGuiColorEditFlags, NoSidePreview)    \
    X(ImGuiColorEditFlags, NoDragDrop)       \
    X(ImGuiColorEditFlags, NoBorder)         \
    X(ImGuiColorEditFlags, NoColorMarkers)   \
    X(ImGuiColorEditFlags, AlphaOpaque)      \
    X(ImGuiColorEditFlags, AlphaNoBg)        \
    X(ImGuiColorEditFlags, AlphaPreviewHalf) \
    X(ImGuiColorEditFlags, AlphaBar)         \
    X(ImGuiColorEditFlags, DisplayRGB)       \
    X(ImGuiColorEditFlags, DisplayHSV)       \
    X(ImGuiColorEditFlags, DisplayHex)       \
    X(ImGuiColorEditFlags, Uint8)            \
    X(ImGuiColorEditFlags, Float)            \
    X(ImGuiColorEditFlags, PickerHueBar)     \
    X(ImGuiColorEditFlags, PickerHueWheel)   \
    X(ImGuiColorEditFlags, InputRGB)         \
    X(ImGuiColorEditFlags, InputHSV)

#define LUAX_IMGUI_SLIDER_FLAGS(X)       \
    X(ImGuiSliderFlags, None)            \
    X(ImGuiSliderFlags, Logarithmic)     \
    X(ImGuiSliderFlags, NoRoundToFormat) \
    X(ImGuiSliderFlags, NoInput)         \
    X(ImGuiSliderFlags, WrapAround)      \
    X(ImGuiSliderFlags, ClampOnInput)    \
    X(ImGuiSliderFlags, ClampZeroRange)  \
    X(ImGuiSliderFlags, NoSpeedTweaks)   \
    X(ImGuiSliderFlags, AlwaysClamp)

#define LUAX_IMGUI_COMBO_FLAGS(X)      \
    X(ImGuiComboFlags, None)           \
    X(ImGuiComboFlags, PopupAlignLeft) \
    X(ImGuiComboFlags, HeightSmall)    \
    X(ImGuiComboFlags, HeightRegular)  \
    X(ImGuiComboFlags, HeightLarge)    \
    X(ImGuiComboFlags, HeightLargest)  \
    X(ImGuiComboFlags, NoArrowButton)  \
    X(ImGuiComboFlags, NoPreview)      \
    X(ImGuiComboFlags, WidthFitPreview)

#define LUAX_IMGUI_TAB_BAR_FLAGS(X)                   \
    X(ImGuiTabBarFlags, None)                         \
    X(ImGuiTabBarFlags, Reorderable)                  \
    X(ImGuiTabBarFlags, AutoSelectNewTabs)            \
    X(ImGuiTabBarFlags, TabListPopupButton)           \
    X(ImGuiTabBarFlags, NoCloseWithMiddleMouseButton) \
    X(ImGuiTabBarFlags, NoTabListScrollingButtons)    \
    X(ImGuiTabBarFlags, NoTooltip)                    \
    X(ImGuiTabBarFlags, DrawSelectedOverline)         \
    X(ImGuiTabBarFlags, FittingPolicyMixed)           \
    X(ImGuiTabBarFlags, FittingPolicyShrink)          \
    X(ImGuiTabBarFlags, FittingPolicyScroll)

#define LUAX_IMGUI_TAB_ITEM_FLAGS(X)                   \
    X(ImGuiTabItemFlags, None)                         \
    X(ImGuiTabItemFlags, UnsavedDocument)              \
    X(ImGuiTabItemFlags, SetSelected)                  \
    X(ImGuiTabItemFlags, NoCloseWithMiddleMouseButton) \
    X(ImGuiTabItemFlags, NoPushId)                     \
    X(ImGuiTabItemFlags, NoTooltip)                    \
    X(ImGuiTabItemFlags, NoReorder)                    \
    X(ImGuiTabItemFlags, Leading)                      \
    X(ImGuiTabItemFlags, Trailing)                     \
    X(ImGuiTabItemFlags, NoAssumedClosure)

#define LUAX_IMGUI_TABLE_FLAGS(X)            \
    X(ImGuiTableFlags, None)                 \
    X(ImGuiTableFlags, Resizable)            \
    X(ImGuiTableFlags, Reorderable)          \
    X(ImGuiTableFlags, Hideable)             \
    X(ImGuiTableFlags, Sortable)             \
    X(ImGuiTableFlags, NoSavedSettings)      \
    X(ImGuiTableFlags, ContextMenuInBody)    \
    X(ImGuiTableFlags, RowBg)                \
    X(ImGuiTableFlags, BordersInnerH)        \
    X(ImGuiTableFlags, BordersOuterH)        \
    X(ImGuiTableFlags, BordersInnerV)        \
    X(ImGuiTableFlags, BordersOuterV)        \
    X(ImGuiTableFlags, BordersH)             \
    X(ImGuiTableFlags, BordersV)             \
    X(ImGuiTableFlags, BordersInner)         \
    X(ImGuiTableFlags, BordersOuter)         \
    X(ImGuiTableFlags, Borders)              \
    X(ImGuiTableFlags, SizingFixedFit)       \
    X(ImGuiTableFlags, SizingFixedSame)      \
    X(ImGuiTableFlags, SizingStretchProp)    \
    X(ImGuiTableFlags, SizingStretchSame)    \
    X(ImGuiTableFlags, ScrollX)              \
    X(ImGuiTableFlags, ScrollY)              \
    X(ImGuiTableFlags, PadOuterX)            \
    X(ImGuiTableFlags, NoPadOuterX)          \
    X(ImGuiTableFlags, NoPadInnerX)          \
    X(ImGuiTableFlags, NoBordersInBody)      \
    X(ImGuiTableFlags, NoHostExtendX)        \
    X(ImGuiTableFlags, NoHostExtendY)        \
    X(ImGuiTableFlags, NoKeepColumnsVisible) \
    X(ImGuiTableFlags, NoClip)               \
    X(ImGuiTableFlags, SortMulti)            \
    X(ImGuiTableFlags, SortTristate)         \
    X(ImGuiTableFlags, HighlightHoveredColumn)

#define LUAX_IMGUI_TABLE_COLUMN_FLAGS(X)          \
    X(ImGuiTableColumnFlags, None)                \
    X(ImGuiTableColumnFlags, Disabled)            \
    X(ImGuiTableColumnFlags, DefaultHide)         \
    X(ImGuiTableColumnFlags, DefaultSort)         \
    X(ImGuiTableColumnFlags, WidthStretch)        \
    X(ImGuiTableColumnFlags, WidthFixed)          \
    X(ImGuiTableColumnFlags, NoResize)            \
    X(ImGuiTableColumnFlags, NoReorder)           \
    X(ImGuiTableColumnFlags, NoHide)              \
    X(ImGuiTableColumnFlags, NoClip)              \
    X(ImGuiTableColumnFlags, NoSort)              \
    X(ImGuiTableColumnFlags, NoHeaderLabel)       \
    X(ImGuiTableColumnFlags, IndentEnable)        \
    X(ImGuiTableColumnFlags, IndentDisable)       \
    X(ImGuiTableColumnFlags, AngledHeader)        \
    X(ImGuiTableColumnFlags, PreferSortAscending) \
    X(ImGuiTableColumnFlags, PreferSortDescending)

#define LUAX_IMGUI_HOVERED_FLAGS(X)                    \
    X(ImGuiHoveredFlags, None)                         \
    X(ImGuiHoveredFlags, ChildWindows)                 \
    X(ImGuiHoveredFlags, RootWindow)                   \
    X(ImGuiHoveredFlags, AnyWindow)                    \
    X(ImGuiHoveredFlags, NoPopupHierarchy)             \
    X(ImGuiHoveredFlags, AllowWhenBlockedByPopup)      \
    X(ImGuiHoveredFlags, AllowWhenBlockedByActiveItem) \
    X(ImGuiHoveredFlags, AllowWhenOverlappedByItem)    \
    X(ImGuiHoveredFlags, AllowWhenOverlappedByWindow)  \
    X(ImGuiHoveredFlags, AllowWhenOverlapped)          \
    X(ImGuiHoveredFlags, AllowWhenDisabled)            \
    X(ImGuiHoveredFlags, NoNavOverride)                \
    X(ImGuiHoveredFlags, RectOnly)                     \
    X(ImGuiHoveredFlags, RootAndChildWindows)          \
    X(ImGuiHoveredFlags, ForTooltip)                   \
    X(ImGuiHoveredFlags, Stationary)                   \
    X(ImGuiHoveredFlags, DelayNone)                    \
    X(ImGuiHoveredFlags, DelayShort)                   \
    X(ImGuiHoveredFlags, DelayNormal)                  \
    X(ImGuiHoveredFlags, NoSharedDelay)

#define LUAX_IMGUI_FOCUSED_FLAGS(X)        \
    X(ImGuiFocusedFlags, None)             \
    X(ImGuiFocusedFlags, ChildWindows)     \
    X(ImGuiFocusedFlags, RootWindow)       \
    X(ImGuiFocusedFlags, AnyWindow)        \
    X(ImGuiFocusedFlags, NoPopupHierarchy) \
    X(ImGuiFocusedFlags, RootAndChildWindows)

#define LUAX_IMGUI_DRAG_DROP_FLAGS(X)               \
    X(ImGuiDragDropFlags, None)                     \
    X(ImGuiDragDropFlags, SourceNoPreviewTooltip)   \
    X(ImGuiDragDropFlags, SourceNoDisableHover)     \
    X(ImGuiDragDropFlags, SourceNoHoldToOpenOthers) \
    X(ImGuiDragDropFlags, SourceAllowNullID)        \
    X(ImGuiDragDropFlags, SourceExtern)             \
    X(ImGuiDragDropFlags, PayloadAutoExpire)        \
    X(ImGuiDragDropFlags, PayloadNoCrossContext)    \
    X(ImGuiDragDropFlags, PayloadNoCrossProcess)    \
    X(ImGuiDragDropFlags, AcceptBeforeDelivery)     \
    X(ImGuiDragDropFlags, AcceptNoDrawDefaultRect)  \
    X(ImGuiDragDropFlags, AcceptNoPreviewTooltip)   \
    X(ImGuiDragDropFlags, AcceptDrawAsHovered)      \
    X(ImGuiDragDropFlags, AcceptPeekOnly)

#define LUAX_IMGUI_MULTI_SELECT_FLAGS(X)            \
    X(ImGuiMultiSelectFlags, None)                  \
    X(ImGuiMultiSelectFlags, SingleSelect)          \
    X(ImGuiMultiSelectFlags, NoSelectAll)           \
    X(ImGuiMultiSelectFlags, NoRangeSelect)         \
    X(ImGuiMultiSelectFlags, NoAutoSelect)          \
    X(ImGuiMultiSelectFlags, NoAutoClear)           \
    X(ImGuiMultiSelectFlags, NoAutoClearOnReselect) \
    X(ImGuiMultiSelectFlags, BoxSelect1d)           \
    X(ImGuiMultiSelectFlags, BoxSelect2d)           \
    X(ImGuiMultiSelectFlags, BoxSelectNoScroll)     \
    X(ImGuiMultiSelectFlags, ClearOnEscape)         \
    X(ImGuiMultiSelectFlags, ClearOnClickVoid)      \
    X(ImGuiMultiSelectFlags, ScopeWindow)           \
    X(ImGuiMultiSelectFlags, ScopeRect)             \
    X(ImGuiMultiSelectFlags, SelectOnAuto)          \
    X(ImGuiMultiSelectFlags, SelectOnClickAlways)   \
    X(ImGuiMultiSelectFlags, SelectOnClickRelease)  \
    X(ImGuiMultiSelectFlags, NavWrapX)              \
    X(ImGuiMultiSelectFlags, NoSelectOnRightClick)

#define LUAX_IMGUI_COL_ENTRY(name) {#name, ImGuiCol_##name},

#define LUAX_IMGUI_COLORS(X)     \
    X(Text)                      \
    X(TextDisabled)              \
    X(WindowBg)                  \
    X(ChildBg)                   \
    X(PopupBg)                   \
    X(Border)                    \
    X(BorderShadow)              \
    X(FrameBg)                   \
    X(FrameBgHovered)            \
    X(FrameBgActive)             \
    X(TitleBg)                   \
    X(TitleBgActive)             \
    X(TitleBgCollapsed)          \
    X(MenuBarBg)                 \
    X(ScrollbarBg)               \
    X(ScrollbarGrab)             \
    X(ScrollbarGrabHovered)      \
    X(ScrollbarGrabActive)       \
    X(CheckMark)                 \
    X(CheckboxSelectedBg)        \
    X(SliderGrab)                \
    X(SliderGrabActive)          \
    X(Button)                    \
    X(ButtonHovered)             \
    X(ButtonActive)              \
    X(Header)                    \
    X(HeaderHovered)             \
    X(HeaderActive)              \
    X(Separator)                 \
    X(SeparatorHovered)          \
    X(SeparatorActive)           \
    X(ResizeGrip)                \
    X(ResizeGripHovered)         \
    X(ResizeGripActive)          \
    X(InputTextCursor)           \
    X(TabHovered)                \
    X(Tab)                       \
    X(TabSelected)               \
    X(TabSelectedOverline)       \
    X(TabDimmed)                 \
    X(TabDimmedSelected)         \
    X(TabDimmedSelectedOverline) \
    X(PlotLines)                 \
    X(PlotLinesHovered)          \
    X(PlotHistogram)             \
    X(PlotHistogramHovered)      \
    X(TableHeaderBg)             \
    X(TableBorderStrong)         \
    X(TableBorderLight)          \
    X(TableRowBg)                \
    X(TableRowBgAlt)             \
    X(TextLink)                  \
    X(TextSelectedBg)            \
    X(TreeLines)                 \
    X(DragDropTarget)            \
    X(DragDropTargetBg)          \
    X(UnsavedMarker)             \
    X(NavCursor)                 \
    X(NavWindowingHighlight)     \
    X(NavWindowingDimBg)         \
    X(ModalWindowDimBg)

#define LUAX_IMGUI_STYLE_VAR_ENTRY(name) {#name, ImGuiStyleVar_##name},

#define LUAX_IMGUI_STYLE_VARS(X)   \
    X(Alpha)                       \
    X(DisabledAlpha)               \
    X(WindowPadding)               \
    X(WindowRounding)              \
    X(WindowBorderSize)            \
    X(WindowMinSize)               \
    X(WindowTitleAlign)            \
    X(ChildRounding)               \
    X(ChildBorderSize)             \
    X(PopupRounding)               \
    X(PopupBorderSize)             \
    X(FramePadding)                \
    X(FrameRounding)               \
    X(FrameBorderSize)             \
    X(ItemSpacing)                 \
    X(ItemInnerSpacing)            \
    X(IndentSpacing)               \
    X(CellPadding)                 \
    X(ScrollbarSize)               \
    X(ScrollbarRounding)           \
    X(ScrollbarPadding)            \
    X(GrabMinSize)                 \
    X(GrabRounding)                \
    X(ImageRounding)               \
    X(ImageBorderSize)             \
    X(TabRounding)                 \
    X(TabBorderSize)               \
    X(TabMinWidthBase)             \
    X(TabMinWidthShrink)           \
    X(TabBarBorderSize)            \
    X(TabBarOverlineSize)          \
    X(TableAngledHeadersAngle)     \
    X(TableAngledHeadersTextAlign) \
    X(TreeLinesSize)               \
    X(TreeLinesRounding)           \
    X(DragDropTargetRounding)      \
    X(ButtonTextAlign)             \
    X(SelectableTextAlign)         \
    X(SeparatorSize)               \
    X(SeparatorTextBorderSize)     \
    X(SeparatorTextAlign)          \
    X(SeparatorTextPadding)

#define LUAX_IMGUI_COND(X)     \
    X(ImGuiCond, None)         \
    X(ImGuiCond, Always)       \
    X(ImGuiCond, Once)         \
    X(ImGuiCond, FirstUseEver) \
    X(ImGuiCond, Appearing)
