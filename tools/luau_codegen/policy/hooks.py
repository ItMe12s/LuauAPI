from __future__ import annotations

import re
from typing import TYPE_CHECKING, Dict

if TYPE_CHECKING:
    from luau_codegen.model.codegen_context import CodegenContext

from luau_codegen.convert.symbols import android_symbol
from luau_codegen.convert.type_map import (
    classify_arg,
    classify_return,
    is_out_reference,
    normalize_type,
)
from luau_codegen.policy.containers import _CONTAINER_KINDS
from luau_codegen.parse.broma import Class, Method
from luau_codegen.policy.filtering import direct_callable, platform_value
from luau_codegen.policy.link_attrs import class_link_platforms, platform_aliases


def concrete_hook_platform(target_platform: str) -> bool:
    if target_platform == "android":
        return False
    return True


def _android_linked(cls: Class, target_platform: str) -> bool:
    if target_platform not in ("android32", "android64"):
        return False
    links = class_link_platforms(cls)
    return bool(links & platform_aliases(target_platform))


def _apple_linked(cls: Class, target_platform: str) -> bool:
    if target_platform not in ("ios", "mac", "imac", "m1"):
        return False
    links = class_link_platforms(cls)
    return bool(links & platform_aliases(target_platform))


def hook_offset(m: Method, target_platform: str) -> str:
    if not concrete_hook_platform(target_platform):
        return ""
    value = platform_value(m, target_platform)
    token = value.split()[0] if value else ""
    if re.fullmatch(r"0x[0-9A-Fa-f]+", token):
        return token
    return ""


def _mac_universal_hook_address(imac_offset: str, m1_offset: str) -> str:
    return (
        "([]() -> void* {\n"
        "#if defined(__x86_64__)\n"
        f"        return reinterpret_cast<void*>(geode::base::get() + {imac_offset});\n"
        "#elif defined(__aarch64__) || defined(__arm64__)\n"
        f"        return reinterpret_cast<void*>(geode::base::get() + {m1_offset});\n"
        "#else\n"
        "        return nullptr;\n"
        "#endif\n"
        "    })()"
    )


def hook_address_expr(cls: Class, m: Method, target_platform: str) -> str:
    if target_platform == "mac":
        if _apple_linked(cls, target_platform):
            symbol = android_symbol(cls, m)
            return f'dlsym(RTLD_DEFAULT, "{symbol}")'
        imac_offset = hook_offset(m, "imac")
        m1_offset = hook_offset(m, "m1")
        if imac_offset and m1_offset:
            return _mac_universal_hook_address(imac_offset, m1_offset)
        return ""
    offset = hook_offset(m, target_platform)
    if offset:
        return f"reinterpret_cast<void*>(geode::base::get() + {offset})"
    if not concrete_hook_platform(target_platform):
        return ""
    if _android_linked(cls, target_platform):
        symbol = android_symbol(cls, m)
        return f'dlsym(luaapi_android_libcocos(), "{symbol}")'
    if _apple_linked(cls, target_platform):
        symbol = android_symbol(cls, m)
        return f'dlsym(RTLD_DEFAULT, "{symbol}")'
    return ""


def hookable(
    cls: Class,
    m: Method,
    objects: Dict[str, Class],
    target_platform: str,
    ctx: CodegenContext | None = None,
) -> bool:
    if m.is_static or m.is_ctor or m.is_dtor:
        return False
    if not direct_callable(cls, m, target_platform):
        return False
    if not hook_address_expr(cls, m, target_platform):
        return False
    ret = classify_return(m.ret, objects, ctx=ctx)
    if ret is None:
        return False
    if "&" in normalize_type(m.ret):
        return False
    if ret.kind == "string" and ret.cxx_type.endswith("*"):
        return False
    if ret.kind in _CONTAINER_KINDS:
        return False
    if ret.kind == "opaque_handle":
        return False
    if any(is_out_reference(arg.type) for arg in m.args):
        return False
    for arg in m.args:
        info = classify_arg(arg.type, objects, ctx=ctx)
        if info is None:
            return False
        if (
            info.kind in ("sel", "callback", "opaque_handle", "delegate")
            or info.kind in _CONTAINER_KINDS
        ):
            return False
    return True
