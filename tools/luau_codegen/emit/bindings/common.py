from __future__ import annotations

from typing import List

from luau_codegen.parse.broma import Class
from luau_codegen.emit.cxx_templates import emit_hook_support, file_preamble
from luau_codegen.policy.hooks import hook_address_expr
from luau_codegen.emit.plan import EmitPlan
from luau_codegen.emit.bindings.class_file import _gen_ns


def _emit_common_file(
    emitted_classes: List[Class], plan: EmitPlan, target_platform: str
) -> str:
    out = [file_preamble(), '#include "bindings_internal.hpp"\n\n']
    for cls in emitted_classes:
        gen_ns = _gen_ns(cls)
        out.append(f"namespace luauapi_gen::{gen_ns} {{\n")
        out.append("LuaHookTarget const* hookTargets();\n")
        out.append("std::size_t hookTargetCount();\n")
        out.append("}\n\n")
    out.append("namespace luauapi_gen {\n\n")
    out.append(
        f'static_assert({len(emitted_classes)} < LUA_UTAG_LIMIT, "LuauAPI generated userdata tags exceed LUA_UTAG_LIMIT");\n\n'
    )
    out.append("LuaHookTarget const* findHookTarget(std::string_view id);\n\n")
    out.append(emit_hook_support())
    ccobject = plan.objects.get("CCObject")
    release = None
    if ccobject:
        release = next(
            (m for m in ccobject.methods if m.name == "release" and not m.args), None
        )
    release_address = (
        hook_address_expr(ccobject, release, target_platform)
        if ccobject and release
        else ""
    )
    if release_address:
        out.append("void luaapi_fields_release_hook(cocos2d::CCObject* self) {\n")
        out.append("    luax::Fields::evictIfFinalRelease(self);\n")
        out.append("    luax::evictMenuHandlersIfFinalRelease(self);\n")
        out.append("    self->release();\n")
        out.append("}\n\n")
        out.append("geode::Result<void> installFieldsReleaseHook() {\n")
        out.append("    static std::optional<geode::Result<void>> installResult;\n")
        out.append("    if (installResult) return *installResult;\n")
        out.append(
            f'    auto result = geode::Mod::get()->hook({release_address}, &luaapi_fields_release_hook, "LuauAPI CCObject::release fields cleanup", tulip::hook::TulipConvention::Default);\n'
        )
        out.append("    if (result.isErr()) {\n")
        out.append("        installResult = geode::Err(result.unwrapErr());\n")
        out.append("        return *installResult;\n")
        out.append("    }\n")
        out.append("    auto* hook = result.unwrap();\n")
        out.append("    if (hook && !hook->isEnabled()) {\n")
        out.append("        auto enableResult = hook->enable();\n")
        out.append("        if (enableResult.isErr()) {\n")
        out.append(
            "            installResult = geode::Err(enableResult.unwrapErr());\n"
        )
        out.append("            return *installResult;\n")
        out.append("        }\n")
        out.append("    }\n")
        out.append("    installResult = geode::Ok();\n")
        out.append("    return *installResult;\n")
        out.append("}\n\n")
    else:
        out.append(
            "geode::Result<void> installFieldsReleaseHook() { return geode::Ok(); }\n\n"
        )

    if emitted_classes:
        out.append("struct HookRange {\n")
        out.append("    LuaHookTarget const* (*targets)();\n")
        out.append("    std::size_t (*count)();\n")
        out.append("};\n\n")
        out.append("HookRange const kAllHookRanges[] = {\n")
        for cls in emitted_classes:
            gen_ns = _gen_ns(cls)
            out.append(f"    {{ {gen_ns}::hookTargets, {gen_ns}::hookTargetCount }},\n")
        out.append("};\n\n")
    else:
        out.append("struct HookRange {\n")
        out.append("    LuaHookTarget const* (*targets)();\n")
        out.append("    std::size_t (*count)();\n")
        out.append("};\n\n")
        out.append("HookRange const kAllHookRanges[] = {};\n\n")

    out.append("LuaHookTarget const* findHookTarget(std::string_view id) {\n")
    out.append("    for (auto const& range : kAllHookRanges) {\n")
    out.append("        auto const* targets = range.targets();\n")
    out.append("        auto targetCount = range.count();\n")
    out.append("        if (!targets || targetCount == 0) continue;\n")
    out.append("        for (std::size_t i = 0; i < targetCount; ++i) {\n")
    out.append("            if (targets[i].id == id) return &targets[i];\n")
    out.append("        }\n")
    out.append("    }\n")
    out.append("    return nullptr;\n")
    out.append("}\n\n")

    out.append("geode::Result<void> bindCommon(lua_State* L) {\n")
    out.append('    luax::getOrCreateTable(L, "geode");\n')
    out.append('    lua_pushcfunction(L, &luaapi_geode_hook, "geode.hook");\n')
    out.append('    lua_setfield(L, -2, "hook");\n')
    out.append('    lua_pushcfunction(L, &luaapi_geode_skip, "geode.skip");\n')
    out.append('    lua_setfield(L, -2, "skip");\n')
    out.append('    lua_pushcfunction(L, &luaapi_geode_fields, "geode.fields");\n')
    out.append('    lua_setfield(L, -2, "fields");\n')
    out.append("    lua_pop(L, 1);\n")
    out.append(
        "    if (auto hookResult = installFieldsReleaseHook(); hookResult.isErr()) {\n"
    )
    out.append("        return geode::Err(hookResult.unwrapErr());\n")
    out.append("    }\n")
    out.append("    if (auto* host = luax::BindingHost::fromState(L)) {\n")
    out.append("        host->registerShutdownHook(&clearHookRegistry);\n")
    out.append("        host->registerShutdownHook(&luax::Fields::clear);\n")
    out.append("    }\n")
    out.append("    return geode::Ok();\n")
    out.append("}\n\n")
    out.append("} // namespace luauapi_gen\n\n")
    out.append("namespace {\n")
    out.append(
        "    LUAX_BINDING_PRIORITY(GeneratedBromaCommon, luauapi_gen::bindCommon, -1)\n"
    )
    out.append("}\n")
    return "".join(out)
