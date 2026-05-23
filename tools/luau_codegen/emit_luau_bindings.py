from __future__ import annotations

import re
from collections import defaultdict
from typing import Dict, Iterable, List, Tuple

from broma_parser import Arg, Class, Method, Root
from model import cxx_name, lua_namespace, object_classes, short_name, status_for
from type_map import TypeInfo, classify_arg, classify_return


INACCESSIBLE_METHODS = {
    ("CCActionManager", "update"),
    ("CCAnimationCache", "parseVersion1"),
    ("CCAnimationCache", "parseVersion2"),
    ("CCAtlasNode", "calculateMaxItems"),
    ("CCAtlasNode", "setIgnoreContentScaleFactor"),
    ("CCAtlasNode", "updateBlendFunc"),
    ("CCAtlasNode", "updateOpacityModifyRGB"),
    ("CCBMFontConfiguration", "parseCommonArguments"),
    ("CCBMFontConfiguration", "parseImageFileName"),
    ("CCBMFontConfiguration", "parseInfoArguments"),
    ("CCBMFontConfiguration", "parseKerningEntry"),
    ("CCBMFontConfiguration", "purgeFontDefDictionary"),
    ("CCBMFontConfiguration", "purgeKerningDictionary"),
    ("CCControlPotentiometer", "angleInDegreesBetweenLineFromPoint_toPoint_toLineFromPoint_toPoint"),
    ("CCControl", "create"),
    ("CCControlButton", "doesAdjustBackgroundImage"),
    ("CCControlButton", "getColor"),
    ("CCControlButton", "getOpacity"),
    ("CCControlButton", "setAdjustBackgroundImage"),
    ("CCControlButton", "setColor"),
    ("CCControlButton", "setOpacity"),
    ("CCControlColourPicker", "updateControlPicker"),
    ("CCControlColourPicker", "updateHueAndControlPicker"),
    ("CCControlHuePicker", "checkSliderPosition"),
    ("CCControlHuePicker", "setHue"),
    ("CCControlHuePicker", "setHuePercentage"),
    ("CCControlHuePicker", "updateSliderPosition"),
    ("CCControlPotentiometer", "distanceBetweenPointAndPoint"),
    ("CCControlPotentiometer", "potentiometerBegan"),
    ("CCControlPotentiometer", "potentiometerEnded"),
    ("CCControlPotentiometer", "potentiometerMoved"),
    ("CCControlSaturationBrightnessPicker", "checkSliderPosition"),
    ("CCControlSaturationBrightnessPicker", "updateSliderPosition"),
    ("CCControlSlider", "isTouchInside"),
    ("CCControlSlider", "locationFromTouch"),
    ("CCControlSlider", "setEnabled"),
    ("CCControlSlider", "setMaximumValue"),
    ("CCControlSlider", "setMinimumValue"),
    ("CCControlSlider", "setValue"),
    ("CCControlSlider", "sliderBegan"),
    ("CCControlSlider", "sliderEnded"),
    ("CCControlSlider", "sliderMoved"),
    ("CCControlSlider", "valueForLocation"),
    ("CCControlStepper", "startAutorepeat"),
    ("CCControlStepper", "stopAutorepeat"),
    ("CCControlStepper", "updateLayoutUsingTouchLocation"),
    ("CCDictionary", "setObjectUnSafe"),
    ("CCDirector", "calculateDeltaTime"),
    ("CCDirector", "calculateMPF"),
    ("CCDirector", "createStatsLabel"),
    ("CCDirector", "purgeDirector"),
    ("CCDirector", "setNextScene"),
    ("CCDirector", "showStats"),
    ("CCDrawNode", "ensureCapacity"),
    ("CCDrawNode", "render"),
    ("CCGLProgram", "description"),
    ("CCLabelTTF", "canAttachWithIME"),
    ("CCLabelTTF", "canDetachWithIME"),
    ("CCLabelTTF", "deleteBackward"),
    ("CCLabelTTF", "deleteForward"),
    ("CCLabelTTF", "draw"),
    ("CCLabelTTF", "getContentText"),
    ("CCLabelTTF", "updateTexture"),
    ("CCLayerColor", "updateColor"),
    ("CCLabelBMFont", "getLetterPosXLeft"),
    ("CCLabelBMFont", "getLetterPosXRight"),
    ("CCLabelBMFont", "kerningAmountForFirst"),
    ("CCMenu", "itemForTouch"),
    ("CCMenuItemFont", "recreateLabel"),
    ("CCMenuItemSprite", "updateImagesVisibility"),
    ("CCNode", "childrenAlloc"),
    ("CCNode", "convertToWindowSpace"),
    ("CCNode", "detachChild"),
    ("CCNode", "insertChild"),
    ("CCNode", "resetGlobalOrderOfArrival"),
    ("CCNotificationCenter", "observerExisted"),
    ("CCParallaxNode", "absolutePosition"),
    ("CCParticleBatchNode", "addChildHelper"),
    ("CCParticleBatchNode", "increaseAtlasCapacityTo"),
    ("CCParticleBatchNode", "searchNewPositionInChildrenForZ"),
    ("CCParticleBatchNode", "updateAllAtlasIndexes"),
    ("CCParticleBatchNode", "updateBlendFunc"),
    ("CCParticleSystem", "updateBlendFunc"),
    ("CCParticleSystemQuad", "allocMemory"),
    ("CCParticleSystemQuad", "setupVBOandVAO"),
    ("CCProgressTimer", "boundaryTexCoord"),
    ("CCProgressTimer", "updateBar"),
    ("CCProgressTimer", "updateColor"),
    ("CCProgressTimer", "updateProgress"),
    ("CCProgressTimer", "updateRadial"),
    ("CCRenderTexture", "beginWithClear"),
    ("CCScale9Sprite", "updateCapInset"),
    ("CCScale9Sprite", "updatePositions"),
    ("CCScriptHandlerEntry", "create"),
    ("CCSchedulerScriptHandlerEntry", "create"),
    ("CCSchedulerScriptHandlerEntry", "init"),
    ("CCScrollView", "afterDraw"),
    ("CCScrollView", "beforeDraw"),
    ("CCScrollView", "deaccelerateScrolling"),
    ("CCScrollView", "getViewRect"),
    ("CCScrollView", "performedAnimatedScroll"),
    ("CCScrollView", "relocateContainer"),
    ("CCScrollView", "stoppedAnimatedScroll"),
    ("CCShaderCache", "init"),
    ("CCShaderCache", "loadDefaultShader"),
    ("CCSprite", "setDirtyRecursively"),
    ("CCSprite", "setReorderChildDirtyRecursively"),
    ("CCSprite", "setTextureCoords"),
    ("CCSprite", "updateBlendFunc"),
    ("CCSprite", "updateColor"),
    ("CCSpriteBatchNode", "addSpriteWithoutQuad"),
    ("CCSpriteBatchNode", "insertQuadFromSprite"),
    ("CCSpriteBatchNode", "swap"),
    ("CCSpriteBatchNode", "updateBlendFunc"),
    ("CCSpriteBatchNode", "updateQuadFromSprite"),
    ("CCSpriteFrameCache", "addSpriteFramesWithDictionary"),
    ("CCSpriteFrameCache", "removeSpriteFramesFromDictionary"),
    ("CCSplitRows", "initWithDuration"),
    ("CCSplitRows", "startWithTarget"),
    ("CCSplitRows", "update"),
    ("CCTMXLayer", "appendTileForGID"),
    ("CCTMXLayer", "atlasIndexForExistantZ"),
    ("CCTMXLayer", "atlasIndexForNewZ"),
    ("CCTMXLayer", "calculateLayerOffset"),
    ("CCTMXLayer", "insertTileForGID"),
    ("CCTMXLayer", "parseInternalProperties"),
    ("CCTMXLayer", "positionForHexAt"),
    ("CCTMXLayer", "positionForIsoAt"),
    ("CCTMXLayer", "positionForOrthoAt"),
    ("CCTMXLayer", "reusedTileWithRect"),
    ("CCTMXLayer", "setupTileSprite"),
    ("CCTMXLayer", "updateTileForGID"),
    ("CCTMXLayer", "vertexZForPos"),
    ("CCTMXMapInfo", "internalInit"),
    ("CCTMXTiledMap", "buildWithMapInfo"),
    ("CCTMXTiledMap", "parseLayer"),
    ("CCTMXTiledMap", "tilesetForLayer"),
    ("CCTableView", "__indexFromOffset"),
    ("CCTableView", "__offsetFromIndex"),
    ("CCTableView", "_addCellIfNecessary"),
    ("CCTableView", "_indexFromOffset"),
    ("CCTableView", "_moveCellOutOfSight"),
    ("CCTableView", "_offsetFromIndex"),
    ("CCTableView", "_setIndexForCell"),
    ("CCTableView", "_updateCellPositions"),
    ("CCTextFieldTTF", "canAttachWithIME"),
    ("CCTextFieldTTF", "canDetachWithIME"),
    ("CCTextFieldTTF", "deleteBackward"),
    ("CCTextFieldTTF", "deleteForward"),
    ("CCTextFieldTTF", "draw"),
    ("CCTextFieldTTF", "getContentText"),
    ("CCTexture2D", "initPremultipliedATextureWithImage"),
    ("CCTextureAtlas", "mapBuffers"),
    ("CCTextureAtlas", "setupIndices"),
    ("CCTextureAtlas", "setupVBOandVAO"),
    ("CCTextureCache", "addImageAsyncCallBack"),
    ("CCTextureETC", "loadTexture"),
    ("CCTexturePVR", "createGLTexture"),
    ("CCTileMapAtlas", "calculateItemsToRender"),
    ("CCTileMapAtlas", "loadTGAfile"),
    ("CCTileMapAtlas", "updateAtlasValueAt"),
    ("CCTileMapAtlas", "updateAtlasValues"),
    ("CCTouchDispatcher", "decrementForcePrio"),
    ("CCTouchDispatcher", "forceAddHandler"),
    ("CCTouchDispatcher", "forceRemoveAllDelegates"),
    ("CCTouchDispatcher", "forceRemoveDelegate"),
    ("CCTouchDispatcher", "incrementForcePrio"),
    ("CCTouchDispatcher", "rearrangeHandlers"),
    ("CCTouchScriptHandlerEntry", "create"),
    ("CCTouchScriptHandlerEntry", "init"),
    ("CCTransitionCrossFade", "draw"),
    ("CCTransitionCrossFade", "onEnter"),
    ("CCTransitionCrossFade", "onExit"),
    ("CCTransitionFadeTR", "sceneOrder"),
    ("CCTransitionPageTurn", "sceneOrder"),
    ("CCTransitionProgress", "progressTimerNodeWithRenderTexture"),
    ("CCTransitionProgress", "sceneOrder"),
    ("CCTransitionProgress", "setupTransition"),
    ("CCTransitionProgressHorizontal", "progressTimerNodeWithRenderTexture"),
    ("CCTransitionProgressInOut", "progressTimerNodeWithRenderTexture"),
    ("CCTransitionProgressInOut", "sceneOrder"),
    ("CCTransitionProgressInOut", "setupTransition"),
    ("CCTransitionProgressOutIn", "progressTimerNodeWithRenderTexture"),
    ("CCTransitionProgressRadialCCW", "progressTimerNodeWithRenderTexture"),
    ("CCTransitionProgressRadialCW", "progressTimerNodeWithRenderTexture"),
    ("CCTransitionProgressVertical", "progressTimerNodeWithRenderTexture"),
    ("CCTransitionScene", "sceneOrder"),
    ("CCTransitionScene", "setNewScene"),
    ("CCTransitionSlideInB", "sceneOrder"),
    ("CCTransitionSlideInL", "sceneOrder"),
    ("CCTransitionSlideInR", "sceneOrder"),
    ("CCTransitionSlideInT", "sceneOrder"),
    ("CCTransitionTurnOffTiles", "easeActionWithAction"),
    ("CCTransitionTurnOffTiles", "onEnter"),
    ("CCTransitionTurnOffTiles", "sceneOrder"),
    ("EditorUI", "onToggleTraceIn"),
    ("EditorUI", "onToggleTraceOut"),
    ("GJBaseGameLayer", "checkSpawnAbuse"),
    ("GJBaseGameLayer", "isButtonAllowed"),
    ("GJOptionsLayer", "getToggleButton"),
    ("LevelSearchLayer", "onPasteClipboard"),
}

INACCESSIBLE_CLASSES = {
    "CCGrabber",
    "CCProfilingTimer",
    "CCSchedulerScriptHandlerEntry",
    "CCScriptHandlerEntry",
    "CCTexturePVR",
    "CCTouchScriptHandlerEntry",
}


def _id(value: str) -> str:
    return re.sub(r"[^A-Za-z0-9_]", "_", value)


def _method_key(m: Method) -> str:
    return f"{m.name}({','.join(a.type for a in m.args)})"


def _platform_value(m: Method, target_platform: str) -> str:
    value = m.platforms.get(target_platform, "")
    if value:
        return value
    if target_platform == "mac":
        return m.platforms.get("imac", "") or m.platforms.get("m1", "")
    if target_platform == "imac":
        return m.platforms.get("mac", "")
    if target_platform == "m1":
        return m.platforms.get("mac", "")
    if target_platform == "android":
        return m.platforms.get("android64", "") or m.platforms.get("android32", "")
    return ""


def _supported(cls: Class, m: Method, objects: Dict[str, Class], target_platform: str) -> tuple[bool, str]:
    if cls.name in INACCESSIBLE_CLASSES:
        return False, "inaccessible-class"
    if (cls.name, m.name) in INACCESSIBLE_METHODS:
        return False, "inaccessible"
    if m.is_ctor:
        return False, "constructor"
    if m.is_dtor:
        return False, "destructor"
    if m.is_callback:
        return False, "callback"
    if m.name.startswith("operator"):
        return False, "operator"
    if status_for(m.platforms) == "Missing":
        return False, "missing-address"
    if not _platform_value(m, target_platform) and not cls.source.endswith("Cocos2d.bro"):
        return False, f"not-linkable:{target_platform}"
    if classify_return(m.ret, objects) is None:
        return False, f"unsupported-return:{m.ret}"
    for arg in m.args:
        if classify_arg(arg.type, objects) is None:
            return False, f"unsupported-arg:{arg.type}"
    return True, ""


def _call_label(cls: Class, m: Method) -> str:
    sep = "." if m.is_static else ":"
    return f"{cls.name}{sep}{m.name}"


def _check_arg(arg: Arg, info: TypeInfo, idx: int, var: str, label: str) -> list[str]:
    cxx = info.cxx_type
    if info.kind == "bool":
        return [f"        auto {var} = luax::check<bool>(L, {idx}, \"{label}\");\n"]
    if info.kind == "number":
        if cxx in ("float", "double"):
            return [f"        auto {var} = luax::check<{cxx}>(L, {idx}, \"{label}\");\n"]
        return [f"        auto {var} = static_cast<{cxx}>(luax::check<int>(L, {idx}, \"{label}\"));\n"]
    if info.kind == "string":
        if cxx == "char const*" or cxx == "const char*":
            return [
                f"        auto {var}_storage = luax::check<std::string>(L, {idx}, \"{label}\");\n",
                f"        auto {var} = {var}_storage.c_str();\n",
            ]
        if cxx == "gd::string":
            return [
                f"        auto {var}_storage = luax::check<std::string>(L, {idx}, \"{label}\");\n",
                f"        auto {var} = gd::string({var}_storage.c_str());\n",
            ]
        return [f"        auto {var} = luax::check<std::string>(L, {idx}, \"{label}\");\n"]
    if info.kind == "value":
        if info.lua_type == "CCPoint":
            return [f"        auto {var} = luax::toPoint(L, {idx}, \"{label}\");\n"]
        if info.lua_type == "CCSize":
            return [f"        auto {var} = luax::toSize(L, {idx}, \"{label}\");\n"]
        if info.lua_type == "CCRect":
            return [f"        auto {var} = luax::toRect(L, {idx}, \"{label}\");\n"]
        if info.lua_type == "RGBColor":
            return [f"        auto {var} = luax::toColor3B(L, {idx}, \"{label}\");\n"]
    if info.kind == "object":
        return [f"        auto {var} = luax::Usertype<{info.cxx_type[:-1]}>::check(L, {idx}, \"{label}\");\n"]
    raise AssertionError(info)


def _push_return(info: TypeInfo, expr: str, owned: bool) -> list[str]:
    if info.kind == "void":
        return ["        return 0;\n"]
    if info.kind == "bool":
        return [f"        luax::push(L, {expr});\n", "        return 1;\n"]
    if info.kind == "number":
        return [f"        lua_pushnumber(L, static_cast<double>({expr}));\n", "        return 1;\n"]
    if info.kind == "string":
        if info.cxx_type.endswith("*"):
            return [f"        luax::push(L, {expr});\n", "        return 1;\n"]
        return [f"        luax::push(L, std::string({expr}));\n", "        return 1;\n"]
    if info.kind == "value":
        if info.lua_type == "CCPoint":
            return [f"        luax::pushPoint(L, {expr});\n", "        return 1;\n"]
        if info.lua_type == "CCSize":
            return [f"        luax::pushSize(L, {expr});\n", "        return 1;\n"]
        if info.lua_type == "CCRect":
            return [f"        luax::pushRect(L, {expr});\n", "        return 1;\n"]
        if info.lua_type == "RGBColor":
            return [f"        luax::pushColor3B(L, {expr});\n", "        return 1;\n"]
    if info.kind == "object":
        push = "pushOwned" if owned else "pushBorrowed"
        return [f"        luax::Usertype<{info.cxx_type[:-1]}>::{push}(L, {expr});\n", "        return 1;\n"]
    raise AssertionError(info)


def _emit_invoke(cls: Class, m: Method, objects: Dict[str, Class], suffix: str) -> str:
    fn = f"luaapi_{_id(cls.name)}_{_id(m.name)}{suffix}"
    label = _call_label(cls, m)
    ret = classify_return(m.ret, objects)
    assert ret is not None
    out = [f"    int {fn}(lua_State* L) {{\n"]
    expected = len(m.args) if m.is_static else len(m.args) + 1
    out.append(f"        if (lua_gettop(L) != {expected}) luaL_error(L, \"{label} expected {expected} args\");\n")
    call_args: List[str] = []
    if not m.is_static:
        out.append(f"        auto self = luax::Usertype<{cxx_name(cls)}>::check(L, 1, \"{label}\");\n")
    for i, arg in enumerate(m.args):
        info = classify_arg(arg.type, objects)
        assert info is not None
        var = f"arg{i}"
        out.extend(_check_arg(arg, info, i + (1 if m.is_static else 2), var, label))
        call_args.append(var)
    args = ", ".join(call_args)
    target = f"{cxx_name(cls)}::{m.name}({args})" if m.is_static else f"self->{m.name}({args})"
    if ret.kind == "void":
        out.append(f"        {target};\n")
        out.extend(_push_return(ret, "", False))
    else:
        out.append(f"        auto result = {target};\n")
        out.extend(_push_return(ret, "result", m.is_static and m.name.startswith("create")))
    out.append("    }\n\n")
    return "".join(out)


def _group_supported(
    cls: Class,
    objects: Dict[str, Class],
    target_platform: str = "win",
) -> tuple[dict[str, list[Method]], list[tuple[Method, str]]]:
    skipped: list[tuple[Method, str]] = []
    by_name: dict[str, list[Method]] = defaultdict(list)
    seen: set[str] = set()
    for m in cls.methods:
        key = _method_key(m)
        if key in seen:
            skipped.append((m, "duplicate-signature"))
            continue
        seen.add(key)
        ok, reason = _supported(cls, m, objects, target_platform)
        if not ok:
            skipped.append((m, reason))
            continue
        by_name[m.name].append(m)

    for name, methods in list(by_name.items()):
        by_arity: dict[int, list[Method]] = defaultdict(list)
        for m in methods:
            by_arity[len(m.args)].append(m)
        kept: list[Method] = []
        for arity, overloads in by_arity.items():
            if len(overloads) == 1:
                kept.extend(overloads)
            else:
                for m in overloads:
                    skipped.append((m, f"ambiguous-overload-arity:{arity}"))
        if kept:
            by_name[name] = kept
        else:
            del by_name[name]
    return by_name, skipped


def _linkless_class_names(
    classes: List[Class],
    objects: Dict[str, Class],
    supported_by_class: dict[str, dict[str, list[Method]]],
    skipped_by_class: dict[str, list[tuple[Method, str]]],
    target_platform: str,
) -> set[str]:
    referenced: set[str] = set()
    for grouped in supported_by_class.values():
        for methods in grouped.values():
            for m in methods:
                ret = classify_return(m.ret, objects)
                if ret and ret.kind == "object":
                    referenced.add(ret.class_name)
                for arg in m.args:
                    info = classify_arg(arg.type, objects)
                    if info and info.kind == "object":
                        referenced.add(info.class_name)

    out: set[str] = set()
    no_link = f"not-linkable:{target_platform}"
    for cls in classes:
        if supported_by_class.get(cls.name):
            continue
        reasons = [reason for _, reason in skipped_by_class.get(cls.name, [])]
        if cls.name in referenced:
            continue
        if any(reason in ("inaccessible-class", no_link) for reason in reasons):
            out.add(cls.name)

    by_short = {cls.name: cls for cls in classes}
    keep_bases: set[str] = set()
    for cls in classes:
        if cls.name in out:
            continue
        for base in cls.bases:
            base_cls = by_short.get(short_name(base))
            if base_cls:
                keep_bases.add(base_cls.name)
    return out - keep_bases


def _emit_dispatcher(cls: Class, name: str, methods: List[Method], objects: Dict[str, Class]) -> str:
    if len(methods) == 1:
        return ""
    first = methods[0]
    fn = f"luaapi_{_id(cls.name)}_{_id(name)}"
    out = [f"    int {fn}(lua_State* L) {{\n"]
    adjust = 0 if first.is_static else 1
    out.append(f"        switch (lua_gettop(L) - {adjust}) {{\n")
    for idx, m in enumerate(methods):
        out.append(f"            case {len(m.args)}: return luaapi_{_id(cls.name)}_{_id(name)}_{idx}(L);\n")
    out.append("            default: break;\n")
    out.append("        }\n")
    out.append(f"        luaL_error(L, \"{_call_label(cls, first)} unsupported overload arity\");\n")
    out.append("    }\n\n")
    return "".join(out)


def _emit_compat_functions(objects: Dict[str, Class]) -> str:
    if "GameManager" not in objects:
        return ""
    return (
        "    int luaapi_GameManager_get(lua_State* L) {\n"
        "        if (lua_gettop(L) != 0) luaL_error(L, \"GameManager.get expected 0 args\");\n"
        "        auto* result = GameManager::get();\n"
        "        if (!result) luaL_error(L, \"GameManager.get singleton not available\");\n"
        "        luax::Usertype<GameManager>::pushBorrowed(L, result);\n"
        "        return 1;\n"
        "    }\n\n"
    )


def emit(root: Root, target_platform: str = "win") -> tuple[str, list[tuple[str, str, str]]]:
    classes = object_classes(root)
    objects = {cls.name: cls for cls in classes}
    objects.update({cls.qualified_name: cls for cls in classes})
    skipped: list[tuple[str, str, str]] = []
    supported_by_class: dict[str, dict[str, list[Method]]] = {}
    skipped_by_class: dict[str, list[tuple[Method, str]]] = {}

    out = [
        "// Generated by LuauAPI codegen\n\n",
        "#include \"lua/Binding.hpp\"\n",
        "#include \"lua/bindings/internal/Stack.hpp\"\n",
        "#include \"lua/bindings/internal/TableUtil.hpp\"\n",
        "#include \"lua/bindings/internal/Types.hpp\"\n",
        "#include \"lua/bindings/internal/Usertype.hpp\"\n\n",
        "#include <Geode/Geode.hpp>\n",
        "#include <cocos2d.h>\n",
        "#include <lua.h>\n\n",
        "#include <string>\n\n",
        "namespace {\n\n",
    ]

    for cls in classes:
        grouped, cls_skipped = _group_supported(cls, objects, target_platform)
        supported_by_class[cls.name] = grouped
        skipped_by_class[cls.name] = cls_skipped
        for m, reason in cls_skipped:
            skipped.append((cls.name, m.name, reason))

    skipped_classes = _linkless_class_names(classes, objects, supported_by_class, skipped_by_class, target_platform)

    for cls in classes:
        if cls.name in skipped_classes:
            continue
        grouped = supported_by_class[cls.name]
        for methods in grouped.values():
            for idx, m in enumerate(methods):
                suffix = "" if len(methods) == 1 else f"_{idx}"
                out.append(_emit_invoke(cls, m, objects, suffix))
            out.append(_emit_dispatcher(cls, methods[0].name, methods, objects))

    out.append(_emit_compat_functions(objects))
    out.append("    void bindGeneratedBroma(lua_State* L) {\n")

    for cls in classes:
        if cls.name in skipped_classes:
            continue
        bases = []
        for base in cls.bases:
            base_cls = objects.get(short_name(base))
            if base_cls and base_cls.name not in skipped_classes:
                bases.append(f"luax::Usertype<{cxx_name(base_cls)}>::tag()")
        base_expr = "{ " + ", ".join(bases) + " }" if bases else "{}"
        out.append(f"        luax::Usertype<{cxx_name(cls)}>::registerType(L, \"{cls.name}\", {base_expr});\n")

    out.append("\n")
    for cls in classes:
        if cls.name in skipped_classes:
            continue
        grouped = supported_by_class[cls.name]
        for name, methods in grouped.items():
            if methods[0].is_static:
                continue
            fn = f"luaapi_{_id(cls.name)}_{_id(name)}"
            out.append(f"        luax::Usertype<{cxx_name(cls)}>::method(L, \"{name}\", &{fn});\n")

    out.append("\n")
    for cls in classes:
        if cls.name in skipped_classes:
            continue
        grouped = supported_by_class[cls.name]
        static_methods = [(name, methods) for name, methods in grouped.items() if methods[0].is_static]
        ns = lua_namespace(cls)
        out.append(f"        luax::getOrCreateTable(L, \"{ns}\");\n")
        out.append(f"        lua_createtable(L, 0, {len(static_methods)});\n")
        for name, methods in static_methods:
            fn = f"luaapi_{_id(cls.name)}_{_id(name)}"
            out.append(f"        lua_pushcfunction(L, &{fn}, \"{cls.name}.{name}\");\n")
            out.append(f"        lua_setfield(L, -2, \"{name}\");\n")
        if cls.name == "GameManager":
            out.append("        lua_pushcfunction(L, &luaapi_GameManager_get, \"GameManager.get\");\n")
            out.append("        lua_setfield(L, -2, \"get\");\n")
        out.append(f"        lua_setfield(L, -2, \"{cls.name}\");\n")
        out.append("        lua_pop(L, 1);\n")

    out.append("    }\n\n")
    out.append("    LUAX_BINDING_PRIORITY(GeneratedBroma, bindGeneratedBroma, 0)\n")
    out.append("}\n")
    return "".join(out), skipped
