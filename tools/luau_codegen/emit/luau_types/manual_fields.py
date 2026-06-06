from __future__ import annotations

from typing import Dict, List

MANUAL_FREE_FN_FIELDS: Dict[str, List[str]] = {
    "geode.utils": [
        "web: WebNamespace",
    ],
    "geode.cocos": [
        "invert3B: (arg1: RGBColor) -> RGBColor",
        "invert4B: (arg1: RGBAColor) -> RGBAColor",
        "lighten3B: (arg1: RGBColor, arg2: number) -> RGBColor",
        "darken3B: (arg1: RGBColor, arg2: number) -> RGBColor",
        "to3B: (arg1: RGBAColor) -> RGBColor",
        "to4B: (arg1: RGBColor, arg2: number?) -> RGBAColor",
        "to4F: (arg1: RGBAColor) -> RGBAFloatColor",
        "ccDrawColor4B: (arg1: RGBAColor) -> ()",
        "cc3bFromHexString: (arg1: string, arg2: boolean?) -> (RGBColor?, string?)",
        "cc4bFromHexString: (arg1: string, arg2: boolean?, arg3: boolean?) -> (RGBAColor?, string?)",
        "getObjectName: (arg1: CCObject) -> string",
        "handleTouchPriority: (arg1: CCNode, arg2: boolean?) -> ()",
        "handleTouchPriorityWith: (arg1: CCNode, arg2: number, arg3: boolean?) -> ()",
    ],
    "geode.utils.base64": [
        "encode: (data: string, variant: number?) -> string",
        "decode: (data: string, variant: number?) -> (string?, string?)",
        "decodeString: (data: string, variant: number?) -> (string?, string?)",
        "Variant: { Normal: number, NormalNoPad: number, Url: number, UrlWithPad: number }",
    ],
    "geode.utils.permission": [
        "getPermissionStatus: (permission: number) -> boolean",
        "requestPermission: (permission: number, callback: (granted: boolean) -> ()) -> ()",
        "Permission: { ReadAllFiles: number, RecordAudio: number }",
    ],
    "geode.ColorProvider": [
        "define: (id: string, color: RGBAColor) -> RGBAColor",
        "override: (id: string, color: RGBAColor) -> RGBAColor",
        "reset: (id: string) -> RGBAColor",
        "color: (id: string) -> RGBAColor",
        "color3b: (id: string) -> RGBColor",
    ],
    "geode.VersionInfo": [
        "parse: (version: string) -> ({ major: number, minor: number, patch: number }?, string?)",
        "compare: (a: string, b: string) -> (number?, string?)",
        "matches: (constraint: string, version: string) -> (boolean?, string?)",
    ],
    "geode.Keybind": [
        "fromString: (str: string) -> ({ key: number, modifiers: number }?, string?)",
        "toString: (keybind: { key: number, modifiers: number }) -> string",
        "createNode: (keybind: { key: number, modifiers: number }) -> CCNode?",
    ],
}
