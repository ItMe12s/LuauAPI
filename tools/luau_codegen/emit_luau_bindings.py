from __future__ import annotations

import re
from collections import defaultdict
from typing import Dict, Iterable, List, Tuple

from broma_parser import Arg, Class, Method, Root
from model import cxx_name, lua_namespace, object_classes, short_name, status_for
from type_map import TypeInfo, classify_arg, classify_return, normalize_type


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
    (
        "CCControlPotentiometer",
        "angleInDegreesBetweenLineFromPoint_toPoint_toLineFromPoint_toPoint",
    ),
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


def _supported(
    cls: Class, m: Method, objects: Dict[str, Class], target_platform: str
) -> tuple[bool, str]:
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
    if not _platform_value(m, target_platform) and not cls.source.endswith(
        "Cocos2d.bro"
    ):
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


def _returns_owned(m: Method) -> bool:
    if not m.is_static:
        return False
    name = m.name.lower()
    if name.startswith("create"):
        return True
    if name in ("copy", "clone"):
        return True
    return False


def _check_arg(arg: Arg, info: TypeInfo, idx: int, var: str, label: str) -> list[str]:
    cxx = info.cxx_type
    if info.kind == "bool":
        return [f'        auto {var} = luax::check<bool>(L, {idx}, "{label}");\n']
    if info.kind == "number":
        if cxx in ("float", "double"):
            return [f'        auto {var} = luax::check<{cxx}>(L, {idx}, "{label}");\n']
        return [
            f'        auto {var} = static_cast<{cxx}>(luax::check<int>(L, {idx}, "{label}"));\n'
        ]
    if info.kind == "string":
        if cxx == "char const*" or cxx == "const char*":
            return [
                f'        auto {var}_storage = luax::check<std::string>(L, {idx}, "{label}");\n',
                f"        auto {var} = {var}_storage.c_str();\n",
            ]
        if cxx == "gd::string":
            return [
                f'        auto {var}_storage = luax::check<std::string>(L, {idx}, "{label}");\n',
                f"        auto {var} = gd::string({var}_storage.c_str());\n",
            ]
        return [
            f'        auto {var} = luax::check<std::string>(L, {idx}, "{label}");\n'
        ]
    if info.kind == "value":
        if info.lua_type == "CCPoint":
            return [f'        auto {var} = luax::toPoint(L, {idx}, "{label}");\n']
        if info.lua_type == "CCSize":
            return [f'        auto {var} = luax::toSize(L, {idx}, "{label}");\n']
        if info.lua_type == "CCRect":
            return [f'        auto {var} = luax::toRect(L, {idx}, "{label}");\n']
        if info.lua_type == "RGBColor":
            return [f'        auto {var} = luax::toColor3B(L, {idx}, "{label}");\n']
    if info.kind == "object":
        return [
            f'        auto {var} = luax::Usertype<{info.cxx_type[:-1]}>::check(L, {idx}, "{label}");\n'
        ]
    raise AssertionError(info)


def _push_return(info: TypeInfo, expr: str, owned: bool) -> list[str]:
    if info.kind == "void":
        return ["        return 0;\n"]
    if info.kind == "bool":
        return [f"        luax::push(L, {expr});\n", "        return 1;\n"]
    if info.kind == "number":
        return [
            f"        lua_pushnumber(L, static_cast<double>({expr}));\n",
            "        return 1;\n",
        ]
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
        return [
            f"        luax::Usertype<{info.cxx_type[:-1]}>::{push}(L, {expr});\n",
            "        return 1;\n",
        ]
    raise AssertionError(info)


def _push_value(info: TypeInfo, expr: str, owned: bool = False) -> list[str]:
    if info.kind == "void":
        return ["        lua_pushnil(L);\n"]
    if info.kind == "bool":
        return [f"        luax::push(L, {expr});\n"]
    if info.kind == "number":
        return [f"        lua_pushnumber(L, static_cast<double>({expr}));\n"]
    if info.kind == "string":
        if info.cxx_type.endswith("*"):
            return [f"        luax::push(L, {expr});\n"]
        return [f"        luax::push(L, std::string({expr}));\n"]
    if info.kind == "value":
        if info.lua_type == "CCPoint":
            return [f"        luax::pushPoint(L, {expr});\n"]
        if info.lua_type == "CCSize":
            return [f"        luax::pushSize(L, {expr});\n"]
        if info.lua_type == "CCRect":
            return [f"        luax::pushRect(L, {expr});\n"]
        if info.lua_type == "RGBColor":
            return [f"        luax::pushColor3B(L, {expr});\n"]
    if info.kind == "object":
        push = "pushOwned" if owned else "pushBorrowed"
        return [f"        luax::Usertype<{info.cxx_type[:-1]}>::{push}(L, {expr});\n"]
    raise AssertionError(info)


def _hook_id(cls: Class, m: Method) -> str:
    return f"{lua_namespace(cls)}.{cls.name}:{m.name}/{len(m.args)}"


def _hook_suffix(cls: Class, m: Method) -> str:
    return f"{_id(cls.name)}_{_id(m.name)}_{len(m.args)}"


def _hook_offset(m: Method, target_platform: str) -> str:
    value = _platform_value(m, target_platform)
    token = value.split()[0] if value else ""
    if token.startswith("0x"):
        return token
    return ""


def _hookable(
    cls: Class, m: Method, objects: Dict[str, Class], target_platform: str
) -> bool:
    if m.is_static or m.is_ctor or m.is_dtor:
        return False
    if not _hook_offset(m, target_platform):
        return False
    ret = classify_return(m.ret, objects)
    if ret is None:
        return False
    if "&" in normalize_type(m.ret):
        return False
    if ret.kind == "string" and ret.cxx_type.endswith("*"):
        return False
    if any("&" in normalize_type(arg.type) for arg in m.args):
        return False
    return all(classify_arg(arg.type, objects) is not None for arg in m.args)


def _emit_return_override(
    info: TypeInfo, var: str, idx: str, target_id: str
) -> list[str]:
    if info.kind == "void":
        return ["        return true;\n"]
    if info.kind == "bool":
        return [
            f"        if (!lua_isboolean(L, {idx})) return false;\n",
            f"        {var} = lua_toboolean(L, {idx}) != 0;\n",
            "        return true;\n",
        ]
    if info.kind == "number":
        return [
            f"        if (!lua_isnumber(L, {idx})) return false;\n",
            f"        {var} = static_cast<{info.cxx_type}>(lua_tonumber(L, {idx}));\n",
            "        return true;\n",
        ]
    if info.kind == "string":
        if info.cxx_type == "gd::string":
            return [
                f"        if (!lua_isstring(L, {idx})) return false;\n",
                "        size_t len = 0;\n",
                f"        char const* text = lua_tolstring(L, {idx}, &len);\n",
                '        auto storage = std::string(text ? text : "", len);\n',
                "        result = gd::string(storage.c_str());\n",
                "        return true;\n",
            ]
        return [
            f"        if (!lua_isstring(L, {idx})) return false;\n",
            "        size_t len = 0;\n",
            f"        char const* text = lua_tolstring(L, {idx}, &len);\n",
            f'        {var} = std::string(text ? text : "", len);\n',
            "        return true;\n",
        ]
    if info.kind == "value":
        if info.lua_type == "CCPoint":
            return [
                f"        if (!lua_istable(L, {idx})) return false;\n",
                f'        lua_getfield(L, {idx}, "x");\n',
                "        if (!lua_isnumber(L, -1)) { lua_pop(L, 1); return false; }\n",
                "        auto x = static_cast<float>(lua_tonumber(L, -1));\n",
                "        lua_pop(L, 1);\n",
                f'        lua_getfield(L, {idx}, "y");\n',
                "        if (!lua_isnumber(L, -1)) { lua_pop(L, 1); return false; }\n",
                "        auto y = static_cast<float>(lua_tonumber(L, -1));\n",
                "        lua_pop(L, 1);\n",
                f"        {var} = cocos2d::CCPoint(x, y);\n",
                "        return true;\n",
            ]
        if info.lua_type == "CCSize":
            return [
                f"        if (!lua_istable(L, {idx})) return false;\n",
                f'        lua_getfield(L, {idx}, "width");\n',
                "        if (!lua_isnumber(L, -1)) { lua_pop(L, 1); return false; }\n",
                "        auto width = static_cast<float>(lua_tonumber(L, -1));\n",
                "        lua_pop(L, 1);\n",
                f'        lua_getfield(L, {idx}, "height");\n',
                "        if (!lua_isnumber(L, -1)) { lua_pop(L, 1); return false; }\n",
                "        auto height = static_cast<float>(lua_tonumber(L, -1));\n",
                "        lua_pop(L, 1);\n",
                f"        {var} = cocos2d::CCSize(width, height);\n",
                "        return true;\n",
            ]
        if info.lua_type == "CCRect":
            return [
                f"        if (!lua_istable(L, {idx})) return false;\n",
                f'        lua_getfield(L, {idx}, "origin");\n',
                "        if (!lua_istable(L, -1)) { lua_pop(L, 1); return false; }\n",
                '        auto origin = luax::toPoint(L, -1, "hook return");\n',
                "        lua_pop(L, 1);\n",
                f'        lua_getfield(L, {idx}, "size");\n',
                "        if (!lua_istable(L, -1)) { lua_pop(L, 1); return false; }\n",
                '        auto size = luax::toSize(L, -1, "hook return");\n',
                "        lua_pop(L, 1);\n",
                f"        {var} = cocos2d::CCRect(origin.x, origin.y, size.width, size.height);\n",
                "        return true;\n",
            ]
        if info.lua_type == "RGBColor":
            return [
                f"        if (!lua_istable(L, {idx})) return false;\n",
                f'        lua_getfield(L, {idx}, "r");\n',
                "        if (!lua_isnumber(L, -1)) { lua_pop(L, 1); return false; }\n",
                "        auto r = static_cast<GLubyte>(lua_tointeger(L, -1));\n",
                "        lua_pop(L, 1);\n",
                f'        lua_getfield(L, {idx}, "g");\n',
                "        if (!lua_isnumber(L, -1)) { lua_pop(L, 1); return false; }\n",
                "        auto g = static_cast<GLubyte>(lua_tointeger(L, -1));\n",
                "        lua_pop(L, 1);\n",
                f'        lua_getfield(L, {idx}, "b");\n',
                "        if (!lua_isnumber(L, -1)) { lua_pop(L, 1); return false; }\n",
                "        auto b = static_cast<GLubyte>(lua_tointeger(L, -1));\n",
                "        lua_pop(L, 1);\n",
                f"        {var} = cocos2d::ccColor3B{{r, g, b}};\n",
                "        return true;\n",
            ]
    if info.kind == "object":
        return [
            f"        if (lua_isnil(L, {idx})) {{ {var} = nullptr; return true; }}\n",
            f"        auto obj = luax::Usertype<{info.cxx_type[:-1]}>::tryCheck(L, {idx});\n",
            "        if (!obj) return false;\n",
            f"        {var} = obj;\n",
            "        return true;\n",
        ]
    raise AssertionError(info)


def _emit_invoke(cls: Class, m: Method, objects: Dict[str, Class], suffix: str) -> str:
    fn = f"luaapi_{_id(cls.name)}_{_id(m.name)}{suffix}"
    label = _call_label(cls, m)
    ret = classify_return(m.ret, objects)
    assert ret is not None
    out = [f"    int {fn}(lua_State* L) {{\n"]
    expected = len(m.args) if m.is_static else len(m.args) + 1
    out.append(
        f'        if (lua_gettop(L) != {expected}) luaL_error(L, "{label} expected {expected} args");\n'
    )
    call_args: List[str] = []
    if not m.is_static:
        out.append(
            f'        auto self = luax::Usertype<{cxx_name(cls)}>::check(L, 1, "{label}");\n'
        )
    for i, arg in enumerate(m.args):
        info = classify_arg(arg.type, objects)
        assert info is not None
        var = f"arg{i}"
        out.extend(_check_arg(arg, info, i + (1 if m.is_static else 2), var, label))
        call_args.append(var)
    args = ", ".join(call_args)
    target = (
        f"{cxx_name(cls)}::{m.name}({args})"
        if m.is_static
        else f"self->{m.name}({args})"
    )
    if ret.kind == "void":
        out.append(f"        {target};\n")
        out.extend(_push_return(ret, "", False))
    else:
        out.append(f"        auto result = {target};\n")
        out.extend(_push_return(ret, "result", _returns_owned(m)))
    out.append("    }\n\n")
    return "".join(out)


def _emit_hook_support() -> str:
    return """    struct LuaHookCallback {
        std::size_t id = 0;
        luax::LuaRef ref;
        bool removed = false;
    };

    struct LuaHookTarget;

    struct LuaHookState {
        LuaHookTarget const* target = nullptr;
        geode::Hook* hook = nullptr;
        std::vector<std::shared_ptr<LuaHookCallback>> callbacks;
    };

    using CreateHookFn = geode::Result<geode::Hook*>(*)(std::string const&);

    struct LuaHookTarget {
        char const* id;
        char const* displayName;
        CreateHookFn createHook;
    };

    std::unordered_map<std::string, LuaHookState>& hookStates() {
        static auto* value = new std::unordered_map<std::string, LuaHookState>();
        return *value;
    }

    std::size_t& nextHookCallbackId() {
        static auto value = std::size_t(1);
        return value;
    }

    void clearHookRegistry() {
        hookStates().clear();
    }

    LuaHookTarget const* findHookTarget(std::string_view id);

    LuaHookState* findHookState(std::size_t callbackId) {
        for (auto& [_, state] : hookStates()) {
            for (auto const& callback : state.callbacks) {
                if (callback && callback->id == callbackId) {
                    return &state;
                }
            }
        }
        return nullptr;
    }

    std::shared_ptr<LuaHookCallback> findHookCallback(std::size_t callbackId) {
        for (auto& [_, state] : hookStates()) {
            for (auto const& callback : state.callbacks) {
                if (callback && callback->id == callbackId) {
                    return callback;
                }
            }
        }
        return nullptr;
    }

    bool stateHasLiveCallbacks(LuaHookState const& state) {
        for (auto const& callback : state.callbacks) {
            if (callback && !callback->removed) return true;
        }
        return false;
    }

    std::size_t hookHandleId(lua_State* L) {
        return static_cast<std::size_t>(lua_tointeger(L, lua_upvalueindex(1)));
    }

    int luaapi_hook_handle_enable(lua_State* L) {
        auto* state = findHookState(hookHandleId(L));
        if (!state || !state->hook) {
            lua_pushboolean(L, false);
            return 1;
        }
        auto result = state->hook->enable();
        lua_pushboolean(L, result.isOk());
        if (result.isErr()) {
            luax::push(L, result.unwrapErr());
            return 2;
        }
        return 1;
    }

    int luaapi_hook_handle_disable(lua_State* L) {
        auto* state = findHookState(hookHandleId(L));
        if (!state || !state->hook) {
            lua_pushboolean(L, false);
            return 1;
        }
        auto result = state->hook->disable();
        lua_pushboolean(L, result.isOk());
        if (result.isErr()) {
            luax::push(L, result.unwrapErr());
            return 2;
        }
        return 1;
    }

    int luaapi_hook_handle_remove(lua_State* L) {
        auto id = hookHandleId(L);
        auto callback = findHookCallback(id);
        auto* state = findHookState(id);
        if (!callback || callback->removed) {
            lua_pushboolean(L, false);
            return 1;
        }
        callback->removed = true;
        callback->ref.reset();
        if (state && state->hook && !stateHasLiveCallbacks(*state)) {
            auto result = state->hook->disable();
            if (result.isErr()) {
                geode::log::warn("luau hook disable failed after remove: {}", result.unwrapErr());
            }
        }
        lua_pushboolean(L, true);
        return 1;
    }

    int luaapi_hook_handle_is_enabled(lua_State* L) {
        auto* state = findHookState(hookHandleId(L));
        lua_pushboolean(L, state && state->hook && state->hook->isEnabled());
        return 1;
    }

    void pushHookHandle(lua_State* L, std::size_t callbackId) {
        lua_createtable(L, 0, 4);
        lua_pushinteger(L, static_cast<lua_Integer>(callbackId));
        lua_pushcclosure(L, &luaapi_hook_handle_enable, nullptr, 1);
        lua_setfield(L, -2, "enable");
        lua_pushinteger(L, static_cast<lua_Integer>(callbackId));
        lua_pushcclosure(L, &luaapi_hook_handle_disable, nullptr, 1);
        lua_setfield(L, -2, "disable");
        lua_pushinteger(L, static_cast<lua_Integer>(callbackId));
        lua_pushcclosure(L, &luaapi_hook_handle_remove, nullptr, 1);
        lua_setfield(L, -2, "remove");
        lua_pushinteger(L, static_cast<lua_Integer>(callbackId));
        lua_pushcclosure(L, &luaapi_hook_handle_is_enabled, nullptr, 1);
        lua_setfield(L, -2, "isEnabled");
    }

    int luaapi_geode_hook(lua_State* L) {
        if (lua_gettop(L) != 2) luaL_error(L, "geode.hook expected 2 args");
        char const* id = lua_tostring(L, 1);
        if (!id || !*id) luaL_error(L, "geode.hook expected target id at arg 1");
        if (!lua_isfunction(L, 2)) luaL_error(L, "geode.hook expected function at arg 2");

        auto* target = findHookTarget(id);
        if (!target) luaL_error(L, "geode.hook target not found: %s", id);

        auto& state = hookStates()[id];
        state.target = target;
        if (!state.hook) {
            auto result = target->createHook(target->displayName);
            if (result.isErr()) {
                luaL_error(L, "geode.hook failed for %s: %s", id, result.unwrapErr().c_str());
            }
            state.hook = result.unwrap();
            if (!state.hook->isEnabled()) {
                auto enableResult = state.hook->enable();
                if (enableResult.isErr()) {
                    luaL_error(L, "geode.hook enable failed for %s: %s", id, enableResult.unwrapErr().c_str());
                }
            }
        } else if (!state.hook->isEnabled()) {
            auto result = state.hook->enable();
            if (result.isErr()) {
                luaL_error(L, "geode.hook enable failed for %s: %s", id, result.unwrapErr().c_str());
            }
        }

        auto callback = std::make_shared<LuaHookCallback>();
        callback->id = nextHookCallbackId()++;
        callback->ref.reset(L, 2);
        state.callbacks.push_back(callback);
        pushHookHandle(L, callback->id);
        return 1;
    }

    template <class PushContext, class ApplyReturn>
    void runLuaPostHooks(char const* targetId, PushContext pushContext, ApplyReturn applyReturn) {
        auto it = hookStates().find(targetId);
        if (it == hookStates().end()) return;
        auto callbacks = it->second.callbacks;
        auto& runtime = luax::Runtime::instance();
        auto* L = runtime.state();
        for (auto const& callback : callbacks) {
            if (!callback || callback->removed || !callback->ref.valid()) continue;
            int top = lua_gettop(L);
            if (!callback->ref.push()) {
                lua_settop(L, top);
                continue;
            }
            pushContext(L);
            luax::Runtime::ResourcesRootScope rootScope(runtime, callback->ref.resourcesRoot());
            if (runtime.protectedCall(1, 1, targetId, 50)) {
                if (!lua_isnil(L, -1) && !applyReturn(L, -1)) {
                    geode::log::warn("luau hook [{}] returned invalid override", targetId);
                }
            }
            lua_settop(L, top);
        }
    }

"""


def _emit_hook_target(
    cls: Class, m: Method, objects: Dict[str, Class], target_platform: str
) -> str:
    ret = classify_return(m.ret, objects)
    assert ret is not None
    args = []
    call_args = []
    for i, arg in enumerate(m.args):
        info = classify_arg(arg.type, objects)
        assert info is not None
        name = f"arg{i}"
        args.append((arg, info, name))
        call_args.append(name)

    suffix = _hook_suffix(cls, m)
    target_id = _hook_id(cls, m)
    display_name = f"{cls.name}::{m.name}"
    cxx_cls = cxx_name(cls)
    ret_type = "void" if ret.kind == "void" else ret.cxx_type
    params = [f"{cxx_cls}* self"]
    params.extend(f"{info.cxx_type} {name}" for _, info, name in args)
    params_text = ", ".join(params)
    call = f"self->{m.name}({', '.join(call_args)})"
    out = [f"    {ret_type} luaapi_hook_{suffix}({params_text}) {{\n"]
    if ret.kind == "void":
        out.append(f"        {call};\n")
    else:
        out.append(f"        auto result = {call};\n")
    out.append(f'        runLuaPostHooks("{target_id}", [&](lua_State* L) {{\n')
    out.append("            lua_createtable(L, 0, 4);\n")
    out.extend(
        f"    {line}"
        for line in _push_value(
            TypeInfo("object", f"{cxx_cls}*", cls.name, cls.name), "self"
        )
    )
    out.append('            lua_setfield(L, -2, "self");\n')
    out.append(f"            lua_createtable(L, {len(args)}, 0);\n")
    for i, (_, info, name) in enumerate(args, start=1):
        out.extend(f"    {line}" for line in _push_value(info, name))
        out.append(f"            lua_rawseti(L, -2, {i});\n")
    out.append('            lua_setfield(L, -2, "args");\n')
    result_expr = "nullptr" if ret.kind == "void" else "result"
    out.extend(f"    {line}" for line in _push_value(ret, result_expr))
    out.append('            lua_setfield(L, -2, "result");\n')
    out.append(f'            lua_pushliteral(L, "{target_id}");\n')
    out.append('            lua_setfield(L, -2, "target");\n')
    out.append("        }, [&](lua_State* L, int idx) -> bool {\n")
    if ret.kind == "void":
        out.append("            return true;\n")
    else:
        out.extend(
            f"    {line}"
            for line in _emit_return_override(ret, "result", "idx", target_id)
        )
    out.append("        });\n")
    if ret.kind != "void":
        out.append("        return result;\n")
    out.append("    }\n\n")
    offset = _hook_offset(m, target_platform)
    out.append(
        f"    geode::Result<geode::Hook*> luaapi_create_hook_{suffix}(std::string const& displayName) {{\n"
    )
    out.append(
        f"        return geode::Mod::get()->hook(reinterpret_cast<void*>(geode::base::get() + {offset}), "
        f"&luaapi_hook_{suffix}, displayName, tulip::hook::TulipConvention::Default);\n"
    )
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


def _emit_dispatcher(
    cls: Class, name: str, methods: List[Method], objects: Dict[str, Class]
) -> str:
    if len(methods) == 1:
        return ""
    first = methods[0]
    fn = f"luaapi_{_id(cls.name)}_{_id(name)}"
    out = [f"    int {fn}(lua_State* L) {{\n"]
    adjust = 0 if first.is_static else 1
    out.append(f"        switch (lua_gettop(L) - {adjust}) {{\n")
    for idx, m in enumerate(methods):
        out.append(
            f"            case {len(m.args)}: return luaapi_{_id(cls.name)}_{_id(name)}_{idx}(L);\n"
        )
    out.append("            default: break;\n")
    out.append("        }\n")
    out.append(
        f'        luaL_error(L, "{_call_label(cls, first)} unsupported overload arity");\n'
    )
    out.append("    }\n\n")
    return "".join(out)


def _emit_compat_functions(objects: Dict[str, Class]) -> str:
    if "GameManager" not in objects:
        return ""
    return (
        "    int luaapi_GameManager_get(lua_State* L) {\n"
        '        if (lua_gettop(L) != 0) luaL_error(L, "GameManager.get expected 0 args");\n'
        "        auto* result = GameManager::get();\n"
        '        if (!result) luaL_error(L, "GameManager.get singleton not available");\n'
        "        luax::Usertype<GameManager>::pushBorrowed(L, result);\n"
        "        return 1;\n"
        "    }\n\n"
    )


def emit(
    root: Root, target_platform: str = "win"
) -> tuple[str, list[tuple[str, str, str]]]:
    classes = object_classes(root)
    objects = {cls.name: cls for cls in classes}
    objects.update({cls.qualified_name: cls for cls in classes})
    skipped: list[tuple[str, str, str]] = []
    supported_by_class: dict[str, dict[str, list[Method]]] = {}
    skipped_by_class: dict[str, list[tuple[Method, str]]] = {}

    out = [
        "// Generated by LuauAPI codegen\n\n",
        '#include "lua/Runtime.hpp"\n',
        '#include "lua/Binding.hpp"\n',
        '#include "lua/bindings/internal/LuaRef.hpp"\n',
        '#include "lua/bindings/internal/Stack.hpp"\n',
        '#include "lua/bindings/internal/TableUtil.hpp"\n',
        '#include "lua/bindings/internal/Types.hpp"\n',
        '#include "lua/bindings/internal/Usertype.hpp"\n\n',
        "#include <Geode/Geode.hpp>\n",
        "#include <Geode/loader/Hook.hpp>\n",
        "#include <Geode/loader/Mod.hpp>\n",
        "#include <Geode/loader/Tulip.hpp>\n",
        "#include <cocos2d.h>\n",
        "#include <lua.h>\n\n",
        "#include <cstddef>\n",
        "#include <memory>\n",
        "#include <string>\n",
        "#include <string_view>\n",
        "#include <unordered_map>\n",
        "#include <vector>\n\n",
        "namespace {\n\n",
        _emit_hook_support(),
    ]

    for cls in classes:
        grouped, cls_skipped = _group_supported(cls, objects, target_platform)
        supported_by_class[cls.name] = grouped
        skipped_by_class[cls.name] = cls_skipped
        for m, reason in cls_skipped:
            skipped.append((cls.name, m.name, reason))

    skipped_classes = _linkless_class_names(
        classes, objects, supported_by_class, skipped_by_class, target_platform
    )

    for cls in classes:
        if cls.name in skipped_classes:
            continue
        grouped = supported_by_class[cls.name]
        for methods in grouped.values():
            for idx, m in enumerate(methods):
                suffix = "" if len(methods) == 1 else f"_{idx}"
                out.append(_emit_invoke(cls, m, objects, suffix))
            out.append(_emit_dispatcher(cls, methods[0].name, methods, objects))

    hook_targets: list[tuple[Class, Method]] = []
    for cls in classes:
        if cls.name in skipped_classes:
            continue
        grouped = supported_by_class[cls.name]
        for methods in grouped.values():
            for m in methods:
                if _hookable(cls, m, objects, target_platform):
                    hook_targets.append((cls, m))
                    out.append(_emit_hook_target(cls, m, objects, target_platform))

    out.append("    LuaHookTarget const kLuaHookTargets[] = {\n")
    for cls, m in hook_targets:
        suffix = _hook_suffix(cls, m)
        out.append(
            f'        {{ "{_hook_id(cls, m)}", "{cls.name}::{m.name}", &luaapi_create_hook_{suffix} }},\n'
        )
    out.append("    };\n\n")
    out.append("    LuaHookTarget const* findHookTarget(std::string_view id) {\n")
    out.append("        for (auto const& target : kLuaHookTargets) {\n")
    out.append("            if (target.id == id) return &target;\n")
    out.append("        }\n")
    out.append("        return nullptr;\n")
    out.append("    }\n\n")

    out.append(_emit_compat_functions(objects))
    out.append("    geode::Result<void> bindGeneratedBroma(lua_State* L) {\n")

    for cls in classes:
        if cls.name in skipped_classes:
            continue
        bases = []
        for base in cls.bases:
            base_cls = objects.get(short_name(base))
            if base_cls and base_cls.name not in skipped_classes:
                bases.append(f"luax::Usertype<{cxx_name(base_cls)}>::tag()")
        base_expr = "{ " + ", ".join(bases) + " }" if bases else "{}"
        out.append(
            f'        luax::Usertype<{cxx_name(cls)}>::registerType(L, "{cls.name}", {base_expr});\n'
        )

    out.append("\n")
    for cls in classes:
        if cls.name in skipped_classes:
            continue
        grouped = supported_by_class[cls.name]
        for name, methods in grouped.items():
            if methods[0].is_static:
                continue
            fn = f"luaapi_{_id(cls.name)}_{_id(name)}"
            out.append(
                f'        luax::Usertype<{cxx_name(cls)}>::method(L, "{name}", &{fn});\n'
            )

    out.append("\n")
    for cls in classes:
        if cls.name in skipped_classes:
            continue
        grouped = supported_by_class[cls.name]
        static_methods = [
            (name, methods) for name, methods in grouped.items() if methods[0].is_static
        ]
        ns = lua_namespace(cls)
        out.append(f'        luax::getOrCreateTable(L, "{ns}");\n')
        out.append(f"        lua_createtable(L, 0, {len(static_methods)});\n")
        for name, methods in static_methods:
            fn = f"luaapi_{_id(cls.name)}_{_id(name)}"
            out.append(f'        lua_pushcfunction(L, &{fn}, "{cls.name}.{name}");\n')
            out.append(f'        lua_setfield(L, -2, "{name}");\n')
        if cls.name == "GameManager":
            out.append(
                '        lua_pushcfunction(L, &luaapi_GameManager_get, "GameManager.get");\n'
            )
            out.append('        lua_setfield(L, -2, "get");\n')
        out.append(f'        lua_setfield(L, -2, "{cls.name}");\n')
        out.append("        lua_pop(L, 1);\n")

    out.append("\n")
    out.append('        luax::getOrCreateTable(L, "geode");\n')
    out.append('        lua_pushcfunction(L, &luaapi_geode_hook, "geode.hook");\n')
    out.append('        lua_setfield(L, -2, "hook");\n')
    out.append("        lua_pop(L, 1);\n")
    out.append(
        "        if (auto* runtime = static_cast<luax::Runtime*>(lua_callbacks(L)->userdata)) {\n"
    )
    out.append("            runtime->registerShutdownHook(&clearHookRegistry);\n")
    out.append("        }\n")

    out.append("        return geode::Ok();\n")
    out.append("    }\n\n")
    out.append("    LUAX_BINDING_PRIORITY(GeneratedBroma, bindGeneratedBroma, 0)\n")
    out.append("}\n")
    return "".join(out), skipped
