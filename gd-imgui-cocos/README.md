# gd-imgui-cocos

## Summary

LuauAPI-internal fork of upstream gd-imgui-cocos. It is not a standalone library.
LuauAPI fetches Dear ImGui in `cmake/ImGui.cmake` and wires this tree through `cmake/ImGuiCocos.cmake`.

## Related

- [Building from source](../docs/contributor/building.md)
- [ImGui draw scheduler](../docs/contributor/internals/imgui-draw-scheduler.md)

## Source

- `gd-imgui-cocos/CMakeLists.txt`
- `cmake/ImGui.cmake`
- `cmake/ImGuiCocos.cmake`
