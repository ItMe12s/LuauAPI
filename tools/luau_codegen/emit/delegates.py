from __future__ import annotations

import re
import sys
import textwrap
import importlib
from dataclasses import dataclass, field
from pathlib import Path
from typing import TYPE_CHECKING

from luau_codegen.cli.io import _write_if_changed
from luau_codegen.convert.type_normalization import is_reference_type, strip_ref

if TYPE_CHECKING:
    from luau_codegen.model import delegate_specs as delegate_specs_module

DELEGATE_GEN_REL_PATHS: tuple[str, ...] = (
    "src/framework/callback/LuaDelegates.gen.hpp",
    "src/framework/callback/LuaDelegates.gen.cpp",
)


def repo_root() -> Path:
    return Path(__file__).resolve().parents[3]


def fallback_bindings_dir() -> Path:
    return repo_root() / "tests/luau_codegen/fixtures/delegate_bindings"


DELEGATE_SPECS_MODULE = "luau_codegen.model.delegate_specs"


def _lua_arg_tuple(values: list[str | None]) -> tuple[str, ...] | None:
    out: list[str] = []
    for value in values:
        if value is None:
            return None
        out.append(value)
    return tuple(out)


def _to_module_specs(
    specs: dict[str, DelegateSpec],
) -> dict[str, delegate_specs_module.DelegateSpec]:
    from luau_codegen.model import delegate_specs as mod_specs

    out: dict[str, mod_specs.DelegateSpec] = {}
    for cxx, spec in sorted(specs.items()):
        methods = []
        for m in spec.methods:
            if not cpp_emit_supported(spec, m):
                continue
            rl = lua_for(m.ret)
            args_lua = _lua_arg_tuple([lua_for(a[0]) for a in method_args(spec, m)])
            if rl is None or args_lua is None:
                continue
            methods.append(mod_specs.DelegateMethodSpec(m.name, rl, args_lua))
        if not methods:
            continue
        out[cxx] = mod_specs.DelegateSpec(
            cxx_type=cxx,
            lua_name=spec.lua_name,
            cpp_class=spec.cpp_class,
            create_fn=f"{spec.cpp_class}::create",
            methods=tuple(methods),
        )
    return out


def install_delegate_specs_module(
    specs: dict[str, DelegateSpec],
    *,
    specs_path: Path | str | None = None,
    module_name: str = DELEGATE_SPECS_MODULE,
    preserve_existing_on_empty: bool = False,
) -> None:
    mod = sys.modules.get(module_name)
    if mod is None:
        mod = importlib.import_module(module_name)
    if specs_path is not None:
        mod.__file__ = str(specs_path)
    if preserve_existing_on_empty and not specs and getattr(mod, "DELEGATE_SPECS", None):
        return
    module_specs = _to_module_specs(specs)
    setattr(mod, "DELEGATE_SPECS", module_specs)
    setattr(mod, "DELEGATE_CXX_TYPES", frozenset(module_specs.keys()))


COCOS_DELEGATES: dict[str, list[tuple[str, str, list[tuple[str, str]]]]] = {
    "cocos2d::CCTouchDelegate": [
        (
            "ccTouchBegan",
            "bool",
            [("cocos2d::CCTouch*", "touch"), ("cocos2d::CCEvent*", "event")],
        ),
        (
            "ccTouchMoved",
            "void",
            [("cocos2d::CCTouch*", "touch"), ("cocos2d::CCEvent*", "event")],
        ),
        (
            "ccTouchEnded",
            "void",
            [("cocos2d::CCTouch*", "touch"), ("cocos2d::CCEvent*", "event")],
        ),
        (
            "ccTouchCancelled",
            "void",
            [("cocos2d::CCTouch*", "touch"), ("cocos2d::CCEvent*", "event")],
        ),
        (
            "ccTouchesBegan",
            "void",
            [("cocos2d::CCSet*", "touches"), ("cocos2d::CCEvent*", "event")],
        ),
        (
            "ccTouchesMoved",
            "void",
            [("cocos2d::CCSet*", "touches"), ("cocos2d::CCEvent*", "event")],
        ),
        (
            "ccTouchesEnded",
            "void",
            [("cocos2d::CCSet*", "touches"), ("cocos2d::CCEvent*", "event")],
        ),
        (
            "ccTouchesCancelled",
            "void",
            [("cocos2d::CCSet*", "touches"), ("cocos2d::CCEvent*", "event")],
        ),
        ("setPreviousPriority", "void", [("int", "priority")]),
        ("getPreviousPriority", "int", []),
    ],
    "cocos2d::CCKeyboardDelegate": [
        ("keyDown", "void", [("cocos2d::enumKeyCodes", "key"), ("double", "dt")]),
        ("keyUp", "void", [("cocos2d::enumKeyCodes", "key"), ("double", "dt")]),
    ],
    "cocos2d::CCKeypadDelegate": [
        ("keyBackClicked", "void", []),
        ("keyMenuClicked", "void", []),
    ],
    "cocos2d::CCMouseDelegate": [
        ("rightKeyDown", "void", []),
        ("rightKeyUp", "void", []),
        ("scrollWheel", "void", [("float", "y"), ("float", "x")]),
    ],
    "cocos2d::CCAccelerometerDelegate": [
        ("didAccelerate", "void", [("cocos2d::CCAcceleration*", "acceleration")]),
    ],
    "cocos2d::CCDirectorDelegate": [
        ("updateProjection", "void", []),
    ],
    "cocos2d::CCIMEDelegate": [
        ("attachWithIME", "bool", []),
        ("detachWithIME", "bool", []),
    ],
    "cocos2d::CCTextFieldDelegate": [
        ("onTextFieldAttachWithIME", "bool", [("cocos2d::CCTextFieldTTF*", "sender")]),
        ("onTextFieldDetachWithIME", "bool", [("cocos2d::CCTextFieldTTF*", "sender")]),
        (
            "onTextFieldInsertText",
            "bool",
            [
                ("cocos2d::CCTextFieldTTF*", "sender"),
                ("char const*", "text"),
                ("int", "len"),
                ("cocos2d::enumKeyCodes", "keyCode"),
            ],
        ),
        (
            "onTextFieldDeleteBackward",
            "bool",
            [
                ("cocos2d::CCTextFieldTTF*", "sender"),
                ("char const*", "text"),
                ("int", "len"),
            ],
        ),
        ("onDraw", "bool", [("cocos2d::CCTextFieldTTF*", "sender")]),
        ("textChanged", "void", []),
    ],
    "cocos2d::extension::CCEditBoxDelegate": [
        (
            "editBoxEditingDidBegin",
            "void",
            [("cocos2d::extension::CCEditBox*", "editBox")],
        ),
        (
            "editBoxEditingDidEnd",
            "void",
            [("cocos2d::extension::CCEditBox*", "editBox")],
        ),
        (
            "editBoxTextChanged",
            "void",
            [
                ("cocos2d::extension::CCEditBox*", "editBox"),
                ("gd::string const&", "text"),
            ],
        ),
        ("editBoxReturn", "void", [("cocos2d::extension::CCEditBox*", "editBox")]),
    ],
}

METHOD_CXX_ARGS: dict[tuple[str, str], list[tuple[str, str]]] = {
    (
        "cocos2d::extension::CCEditBoxDelegate",
        "editBoxTextChanged",
    ): [
        ("cocos2d::extension::CCEditBox*", "editBox"),
        ("gd::string const&", "text"),
    ],
}

SKIP_METHODS = frozenset(
    {
        ("TriggerEffectDelegate", "checkSpawnAbuse"),
    }
)

SKIP = frozenset(
    {
        "AppDelegate",
        "cocos2d::AppDelegate",
        "cocos2d::CCScriptEngineProtocol",
        "cocos2d::EGLTouchDelegate",
        "EGLTouchDelegate",
    }
)

LUA_TYPES: dict[str, str] = {
    "void": "()",
    "bool": "boolean",
    "int": "number",
    "float": "number",
    "double": "number",
    "unsigned int": "number",
    "unsigned char": "number",
    "char": "number",
    "enumKeyCodes": "number",
    "cocos2d::enumKeyCodes": "number",
    "cocos2d::CCObject*": "CCObject",
    "CCObject*": "CCObject",
    "cocos2d::CCNode*": "CCNode",
    "cocos2d::CCTouch*": "CCTouch",
    "cocos2d::CCEvent*": "CCEvent",
    "cocos2d::CCSet*": "CCSet",
    "cocos2d::CCArray*": "CCArray",
    "cocos2d::CCAcceleration*": "CCAcceleration",
    "cocos2d::CCTextFieldTTF*": "CCTextFieldTTF",
    "cocos2d::extension::CCEditBox*": "CCEditBox",
    "char const*": "string",
    "const char*": "string",
    "gd::string": "string",
    "std::string": "string",
    "cocos2d::ccColor3B": "RGBColor",
    "cocos2d::ccColor4B": "RGBAColor",
    "cocos2d::CCPoint": "CCPoint",
    "cocos2d::CCSize": "CCSize",
    "cocos2d::CCRect": "CCRect",
    "IconType": "number",
    "UnlockType": "number",
    "SearchType": "number",
    "GJHttpType": "number",
    "LikeItemType": "number",
    "UserListType": "number",
    "GJRewardType": "number",
    "GJTimedLevelType": "number",
    "GJMusicAction": "number",
    "GJActionCommand": "number",
    "GJSongError": "number",
    "ColorSelectType": "number",
    "BoomListType": "number",
    "AudioGuidelinesType": "number",
    "cocos2d::ccHSVValue": "HSVValue",
    "CCIndexPath": "CCIndexPath",
    "TableView": "TableView",
    "TableViewCell": "TableViewCell",
}

ENUM_SUFFIXES = ("Type", "Error", "Action", "Command", "Mode")
PRIMITIVE_POINTER_BASES = frozenset(
    {
        "bool",
        "char",
        "double",
        "float",
        "int",
        "long",
        "short",
        "unsigned char",
        "unsigned int",
        "unsigned long",
        "unsigned short",
    }
)


@dataclass
class DelegateMethod:
    name: str
    ret: str
    args: list[tuple[str, str]]


@dataclass
class DelegateSpec:
    cxx_type: str
    lua_name: str
    cpp_class: str
    methods: list[DelegateMethod] = field(default_factory=list)


def lua_for(cxx: str) -> str | None:
    n = strip_ref(cxx)
    if n in LUA_TYPES:
        return LUA_TYPES[n]
    if n.endswith("*"):
        base = n[:-1].strip()
        short = base.split("::")[-1]
        if base in PRIMITIVE_POINTER_BASES or short.endswith(("Delegate", "Protocol")):
            return None
        return short or None
    if any(n.endswith(s) for s in ENUM_SUFFIXES):
        return "number"
    return None


def resolve_bindings_dir(bindings_dir: Path | str | None = None) -> Path:
    if bindings_dir is not None:
        return Path(bindings_dir)
    fallback = fallback_bindings_dir()
    if fallback.is_dir() and any(fallback.glob("*.bro")):
        return fallback
    raise FileNotFoundError(
        "delegate bindings dir required, pass --bindings or add "
        "tests/luau_codegen/fixtures/delegate_bindings"
    )


def parse_broma(
    bindings_dir: Path | str | None = None,
) -> dict[str, list[DelegateMethod]]:
    from luau_codegen.parse.broma_delegates import parse_delegate_classes

    return parse_delegate_classes(
        resolve_bindings_dir(bindings_dir),
        norm=strip_ref,
        delegate_method_cls=DelegateMethod,
    )


def collect(bindings_dir: Path | str | None = None) -> dict[str, DelegateSpec]:
    from luau_codegen.parse.broma_delegates import collect_delegate_ptrs

    bro_dir = resolve_bindings_dir(bindings_dir)
    broma = parse_broma(bro_dir)
    ptrs = collect_delegate_ptrs(bro_dir, norm=strip_ref)
    specs: dict[str, DelegateSpec] = {}
    for ptr in sorted(ptrs):
        if ptr in SKIP or ptr.split("::")[-1] in SKIP:
            continue
        short = ptr.split("::")[-1]
        keys = [ptr, short, f"cocos2d::{short}"]
        methods = None
        cxx = ptr
        for k in keys:
            if k in COCOS_DELEGATES:
                methods = [DelegateMethod(n, r, a) for n, r, a in COCOS_DELEGATES[k]]
                cxx = k
                break
            if k in broma:
                methods = broma[k]
                cxx = k
                break
        if not methods:
            continue
        specs[cxx] = DelegateSpec(cxx, short, f"Lua{short}", methods)
    return specs


def method_args(spec: DelegateSpec, m: DelegateMethod) -> list[tuple[str, str]]:
    for key in ((spec.cxx_type, m.name), (spec.lua_name, m.name)):
        if key in METHOD_CXX_ARGS:
            return METHOD_CXX_ARGS[key]
    return m.args


def unique_cxx_args(args: list[tuple[str, str]]) -> list[tuple[str, str]]:
    out: list[tuple[str, str]] = []
    seen: set[str] = set()
    for i, (param_type, name) in enumerate(args):
        base = name.strip() or f"arg{i}"
        if re.match(r"^[A-Za-z_]\w*$", base) is None:
            base = f"arg{i}"
        candidate = base
        suffix = i
        while candidate in seen:
            candidate = f"{base}{suffix}"
            suffix += 1
        seen.add(candidate)
        out.append((param_type, candidate))
    return out


def should_emit_method(spec: DelegateSpec, m: DelegateMethod) -> bool:
    if (spec.lua_name, m.name) in SKIP_METHODS or (
        spec.cxx_type,
        m.name,
    ) in SKIP_METHODS:
        return False
    args = method_args(spec, m)
    return lua_for(m.ret) is not None and all(lua_for(a[0]) for a in args)


def cpp_emit_supported(spec: DelegateSpec, m: DelegateMethod) -> bool:
    if not should_emit_method(spec, m):
        return False
    nret = strip_ref(m.ret)
    if nret in ("void", "bool", "int", "float", "double"):
        return True
    if nret in ("gd::string", "char const*", "const char*", "std::string"):
        return True
    if nret.endswith("*"):
        return lua_for(m.ret) is not None
    return False


def cxx_ctx_type(param_type: str) -> str:
    n = strip_ref(param_type)
    if n in ("enumKeyCodes", "cocos2d::enumKeyCodes"):
        return "cocos2d::enumKeyCodes"
    if n == "CCIndexPath":
        return "CCIndexPath"
    if n == "gd::string":
        return "gd::string"
    return n


def cxx_emit_param(param_type: str, name: str) -> str:
    n = strip_ref(param_type)
    if n in ("enumKeyCodes", "cocos2d::enumKeyCodes"):
        return f"cocos2d::enumKeyCodes {name}"
    if n == "CCIndexPath":
        if is_reference_type(param_type):
            return f"CCIndexPath& {name}"
        return f"CCIndexPath {name}"
    if "gd::string" in param_type and param_type.strip().endswith("&"):
        return f"gd::string const& {name}"
    return f"{cxx_ctx_type(param_type)} {name}"


def push_stmt(t: str, expr: str) -> str:
    n = strip_ref(t)
    if n == "cocos2d::CCAcceleration*":
        return textwrap.dedent(
            f"""\
            if ({expr} == nullptr) lua_pushnil(L);
            else {{
                lua_createtable(L, 0, 4);
                lua_pushnumber(L, {expr}->x); lua_setfield(L, -2, "x");
                lua_pushnumber(L, {expr}->y); lua_setfield(L, -2, "y");
                lua_pushnumber(L, {expr}->z); lua_setfield(L, -2, "z");
                lua_pushnumber(L, {expr}->timestamp); lua_setfield(L, -2, "timestamp");
            }}"""
        )
    if n in ("char const*", "const char*"):
        return f"luax::push(L, {expr} ? std::string({expr}) : std::string())"
    if n == "gd::string":
        return f"luax::push(L, std::string({expr}.c_str()))"
    if "CCIndexPath" in n and not n.endswith("*"):
        return (
            f"lua_createtable(L, 0, 2);\n"
            f'            lua_pushnumber(L, {expr}.m_section); lua_setfield(L, -2, "section");\n'
            f'            lua_pushnumber(L, {expr}.m_row); lua_setfield(L, -2, "row");'
        )
    if "ccHSVValue" in n:
        return f"luax::push(L, {expr})"
    if n.endswith("*"):
        obj = n[:-1].strip()
        return f"luax::Usertype<{obj}>::pushBorrowed(L, {expr})"
    if n in ("enumKeyCodes", "cocos2d::enumKeyCodes") or any(n.endswith(s) for s in ENUM_SUFFIXES):
        return f"lua_pushnumber(L, static_cast<double>(static_cast<int>({expr})))"
    return f"luax::push(L, {expr})"


def push_lambda_body(args: list[tuple[str, str]]) -> str:
    lines: list[str] = []
    for i, (t, _) in enumerate(args):
        stmt_lines = push_stmt(t, f"c->p{i}").splitlines()
        for j, line in enumerate(stmt_lines):
            stripped = line.rstrip()
            needs_suffix = j == len(stmt_lines) - 1 and not stripped.endswith(";")
            suffix = ";" if needs_suffix else ""
            lines.append(f"                {stripped}{suffix}")
    return "\n".join(lines)


def emit_specs_py(specs: dict[str, DelegateSpec]) -> str:
    lines = [
        '"""Delegate specs for LuauAPI codegen (generated)."""',
        "from __future__ import annotations",
        "from dataclasses import dataclass",
        "from typing import Dict, FrozenSet, Optional, Tuple",
        "",
        "@dataclass(frozen=True)",
        "class DelegateMethodSpec:",
        "    name: str",
        "    ret_lua: str",
        "    args_lua: Tuple[str, ...]",
        "",
        "@dataclass(frozen=True)",
        "class DelegateSpec:",
        "    cxx_type: str",
        "    lua_name: str",
        "    cpp_class: str",
        "    create_fn: str",
        "    methods: Tuple[DelegateMethodSpec, ...]",
        "",
        "DELEGATE_SPECS: Dict[str, DelegateSpec] = {",
    ]
    count = 0
    for cxx, spec in sorted(specs.items()):
        entries = []
        for m in spec.methods:
            if not cpp_emit_supported(spec, m):
                continue
            rl, al = lua_for(m.ret), [lua_for(a[0]) for a in method_args(spec, m)]
            if rl is None or any(x is None for x in al):
                continue
            entries.append(f'            DelegateMethodSpec("{m.name}", "{rl}", {tuple(al)!r}),')
        if not entries:
            continue
        count += 1
        lines += [
            f'    "{cxx}": DelegateSpec(',
            f'        cxx_type="{cxx}", lua_name="{spec.lua_name}",',
            f'        cpp_class="{spec.cpp_class}", create_fn="{spec.cpp_class}::create",',
            "        methods=(",
            *entries,
            "        ),",
            "    ),",
        ]
    lines += [
        "}",
        "DELEGATE_CXX_TYPES: FrozenSet[str] = frozenset(DELEGATE_SPECS.keys())",
        "",
        "def lookup_delegate(cxx_ptr_type: str) -> Optional[DelegateSpec]:",
        "    n = cxx_ptr_type.strip().removesuffix('*').strip()",
        "    return DELEGATE_SPECS.get(n) or DELEGATE_SPECS.get(n.split('::')[-1])",
        "",
    ]
    return "\n".join(lines)


def emit_gen_hpp(specs: dict[str, DelegateSpec]) -> str:
    classes = []
    for spec in specs.values():
        methods = []
        for m in spec.methods:
            if not cpp_emit_supported(spec, m):
                continue
            o = emit_override(spec, m)
            if o:
                methods.append(textwrap.indent(o, "        "))
        if not methods:
            continue
        classes.append(
            f"    class {spec.cpp_class} : public {spec.cxx_type}, public cocos2d::CCObject {{\n"
            f"    public:\n"
            f"        static {spec.cpp_class}* create(lua_State* L, int tableIndex);\n"
            f"        ~{spec.cpp_class}() override {{\n"
            f"            LuaDelegateBase::unregisterInterface(static_cast<{spec.cxx_type}*>(this));\n"
            f"        }}\n" + "\n".join(methods) + "\n    private:\n"
            "        std::shared_ptr<LuaRef> m_table;\n"
            "    };"
        )
    return (
        "#pragma once\n\n"
        '#include "framework/callback/LuaDelegate.hpp"\n'
        '#include "framework/stack/Stack.hpp"\n'
        '#include "framework/stack/Types.hpp"\n'
        '#include "framework/usertype/Usertype.hpp"\n\n'
        "namespace luax {\n" + "\n\n".join(classes) + "\n}\n"
    )


def emit_override(spec: DelegateSpec, m: DelegateMethod) -> str:
    if not cpp_emit_supported(spec, m):
        return ""
    args = unique_cxx_args(method_args(spec, m))
    params = ", ".join(cxx_emit_param(t, n) for t, n in args)
    ctx_fields = "; ".join(f"{cxx_ctx_type(t)} p{i}" for i, (t, _) in enumerate(args))
    ctx_init = ", ".join(n for _, n in args)
    pushes = push_lambda_body(args)
    ctx_block = ""
    if args:
        ctx_block = f"struct Ctx {{ {ctx_fields}; }};\n        Ctx ctx{{ {ctx_init} }};\n        "
    push_fn = f"+[](lua_State* L, void* raw) {{\n            auto* c = static_cast<Ctx*>(raw);\n{pushes}\n        }}"
    push_args = f", {push_fn}, &ctx" if args else ""
    nret = strip_ref(m.ret)
    if m.ret == "void":
        return textwrap.dedent(
            f"""\
            void {m.name}({params}) override {{
                {ctx_block}LuaDelegateBase::invokeTableVoid(m_table, "{m.name}", "{spec.lua_name}.{m.name}", {len(args)}{push_args});
            }}"""
        )
    if m.ret == "bool":
        return textwrap.dedent(
            f"""\
            bool {m.name}({params}) override {{
                {ctx_block}return LuaDelegateBase::invokeTableValue<bool>(m_table, "{m.name}", false, "{spec.lua_name}.{m.name}", {len(args)}{push_args});
            }}"""
        )
    if m.ret == "int":
        return textwrap.dedent(
            f"""\
            int {m.name}({params}) override {{
                {ctx_block}return LuaDelegateBase::invokeTableValue<int>(m_table, "{m.name}", 0, "{spec.lua_name}.{m.name}", {len(args)}{push_args});
            }}"""
        )
    if nret in ("float", "double"):
        default = "0.f" if nret == "float" else "0.0"
        return textwrap.dedent(
            f"""\
            {nret} {m.name}({params}) override {{
                {ctx_block}return LuaDelegateBase::invokeTableValue<{nret}>(m_table, "{m.name}", {default}, "{spec.lua_name}.{m.name}", {len(args)}{push_args});
            }}"""
        )
    if nret in ("gd::string", "char const*", "const char*", "std::string"):
        return textwrap.dedent(
            f"""\
            gd::string {m.name}({params}) override {{
                {ctx_block}return gd::string(LuaDelegateBase::invokeTableValue<std::string>(m_table, "{m.name}", std::string(), "{spec.lua_name}.{m.name}", {len(args)}{push_args}).c_str());
            }}"""
        )
    if nret.endswith("*"):
        obj = nret[:-1].strip()
        return textwrap.dedent(
            f"""\
            {obj}* {m.name}({params}) override {{
                {ctx_block}return LuaDelegateBase::invokeTableObject<{obj}>(m_table, "{m.name}", nullptr, "{spec.lua_name}.{m.name}", {len(args)}{push_args});
            }}"""
        )
    return ""


def emit_gen_cpp(specs: dict[str, DelegateSpec]) -> str:
    parts = [
        '#include "framework/callback/LuaDelegates.gen.hpp"',
        "",
        "namespace luax {",
    ]
    for spec in specs.values():
        if not any(cpp_emit_supported(spec, m) for m in spec.methods):
            continue
        parts.append(f"{spec.cpp_class}* {spec.cpp_class}::create(lua_State* L, int tableIndex) {{")
        parts.append("    LuaDelegateBase::checkDelegateTable(L, tableIndex);")
        parts.append(f"    auto* self = new {spec.cpp_class}();")
        parts.append("    self->m_table = LuaDelegateBase::captureTable(L, tableIndex);")
        parts.append(
            f"    LuaDelegateBase::registerInterface(static_cast<{spec.cxx_type}*>(self), self->m_table);"
        )
        parts.append("    self->autorelease();")
        parts.append("    return self;")
        parts.append("}")
        parts.append("")
    parts.append("}")
    return "\n".join(parts) + "\n"


def _warn_unsupported_emitters(specs: dict[str, DelegateSpec]) -> None:
    for spec in specs.values():
        for m in spec.methods:
            if should_emit_method(spec, m) and not cpp_emit_supported(spec, m):
                print(
                    f"warning: {spec.lua_name}.{m.name} has Luau types but no C++ emitter "
                    f"(ret={m.ret!r})",
                    file=sys.stderr,
                )


def emit_delegate_artifacts(
    bindings_dir: Path | str,
    *,
    specs_out: Path | str,
    gen_out: Path | str | None = None,
    install_module: bool = True,
    preserve_existing_on_empty: bool = False,
) -> dict[str, DelegateSpec]:
    specs = collect(bindings_dir)
    _warn_unsupported_emitters(specs)
    _write_if_changed(str(specs_out), emit_specs_py(specs))
    if install_module:
        install_delegate_specs_module(
            specs,
            specs_path=specs_out,
            preserve_existing_on_empty=preserve_existing_on_empty,
        )
    if gen_out is not None:
        gen_root = Path(gen_out)
        gen_files = {
            DELEGATE_GEN_REL_PATHS[0]: emit_gen_hpp(specs),
            DELEGATE_GEN_REL_PATHS[1]: emit_gen_cpp(specs),
        }
        for rel, content in gen_files.items():
            _write_if_changed(str(gen_root / rel), content)
    return specs
