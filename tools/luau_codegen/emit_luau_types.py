from __future__ import annotations

import re
from typing import Dict, List, Sequence, Tuple

from broma_parser import Class, Field, Function, Method, Root
from fields import bindable_field
from model import lua_namespace, short_name
from plan import EmitPlan, collect_plan
from type_map import TypeInfo, classify_arg, classify_return, enum_lua_names

TYPES_FILE = "geode.d.luau"
cocos_namespace = "geode.cocos2d"
gd_namespace = "geode.gd"
geode_namespace = "geode"

_DUMMY_CLS = Class(name="")

LUAU_KEYWORDS = frozenset(
    {
        "and",
        "break",
        "do",
        "else",
        "elseif",
        "end",
        "false",
        "for",
        "function",
        "if",
        "in",
        "local",
        "nil",
        "not",
        "or",
        "repeat",
        "return",
        "then",
        "true",
        "until",
        "while",
        "continue",
        "export",
        "type",
        "typeof",
        "declare",
    }
)


def _method_return_type(cls: Class, m: Method, objects: Dict[str, Class]) -> str:
    ret = classify_return(m.ret, objects)
    assert ret is not None
    out_types: List[str] = []
    if ret.kind != "void":
        out_types.append(ret.lua_type)
    if ret.kind == "void":
        for arg in m.args:
            info = classify_arg(arg.type, objects)
            assert info is not None
            if info.is_out:
                out_types.append(info.lua_type)
    if not out_types:
        return "()"
    if len(out_types) == 1:
        return out_types[0]
    return "(" + ", ".join(out_types) + ")"


def _method_type(cls: Class, methods: List[Method], objects: Dict[str, Class]) -> str:
    parts = []
    for m in methods:
        args = _classify_input_args(m, objects)
        ret_type = _method_return_type(cls, m, objects)
        params = []
        if not m.is_static:
            params.append(f"self: {cls.name}")
        for i, arg in enumerate(args, start=1):
            params.append(f"arg{i}: {arg.lua_type}")
        parts.append(f"({', '.join(params)}) -> {ret_type}")
    return " & ".join(f"({p})" for p in parts)


def _widened_method_type(
    cls: Class, methods: List[Method], objects: Dict[str, Class], *, static: bool
) -> str:
    arg_lists = [_classify_input_args(m, objects) for m in methods]
    prefix: List[str] = []
    for i in range(min(len(a) for a in arg_lists)):
        types_at_i = {a[i].lua_type for a in arg_lists}
        if len(types_at_i) == 1:
            prefix.append(arg_lists[0][i].lua_type)
        else:
            break
    returns = {_method_return_type(cls, m, objects) for m in methods}
    ret = returns.pop() if len(returns) == 1 else "any?"
    params: List[str] = []
    if not static:
        params.append(f"self: {cls.name}")
    params += [f"arg{i}: {t}" for i, t in enumerate(prefix, start=1)]
    params.append("...any")
    return f"({', '.join(params)}) -> {ret}"


def _classify_input_args(m: Method, objects: Dict[str, Class]) -> List[TypeInfo]:
    ret = classify_return(m.ret, objects)
    assert ret is not None
    args: List[TypeInfo] = []
    for arg in m.args:
        info = classify_arg(arg.type, objects)
        assert info is not None
        if ret.kind == "void" and info.is_out:
            continue
        args.append(info)
    return args


_VALUE_STUB_BODY: Dict[str, str] = {
    "RGBColor": "export type RGBColor = { r: number, g: number, b: number }\n",
    "CCSize": "export type CCSize = { width: number, height: number }\n",
    "CCPoint": "export type CCPoint = { x: number, y: number }\n",
    "CCRect": "export type CCRect = { origin: CCPoint, size: CCSize }\n",
    "UIButtonConfig": (
        "export type UIButtonConfig = {\n"
        "    width: number,\n"
        "    height: number,\n"
        "    deadzone: number,\n"
        "    scale: number,\n"
        "    opacity: number,\n"
        "    radius: number,\n"
        "    modeB: boolean,\n"
        "    snap: boolean,\n"
        "    position: CCPoint,\n"
        "    oneButton: boolean,\n"
        "    player2: boolean,\n"
        "    split: boolean,\n"
        "}\n"
    ),
}

_VALUE_STUB_ORDER = ("CCPoint", "CCSize", "RGBColor", "CCRect", "UIButtonConfig")

_VALUE_STUB_DEPS: Dict[str, Tuple[str, ...]] = {
    "CCRect": ("CCPoint", "CCSize"),
    "UIButtonConfig": ("CCPoint",),
}


def _expand_value_refs(names: set[str]) -> set[str]:
    out = set(names)
    for name in names:
        out.update(_VALUE_STUB_DEPS.get(name, ()))
    return {n for n in out if n in _VALUE_STUB_BODY}


def _value_refs_in_text(text: str) -> set[str]:
    return {name for name in _VALUE_STUB_BODY if re.search(rf"\b{name}\b", text)}


def _emit_value_stub_block(names: set[str]) -> str:
    expanded = _expand_value_refs(names)
    if not expanded:
        return ""
    return "".join(_VALUE_STUB_BODY[n] for n in _VALUE_STUB_ORDER if n in expanded)


def _free_fn_supported(fn: Function, objects: Dict[str, Class]) -> bool:
    if classify_return(fn.ret, objects) is None:
        return False
    return all(classify_arg(arg.type, objects) is not None for arg in fn.args)


def _build_function_tree(functions: List[Function], objects: Dict[str, Class]) -> dict:
    tree: dict = {"children": {}, "functions": {}}
    for fn in functions:
        if not _free_fn_supported(fn, objects):
            continue
        node = tree
        for seg in fn.lua_path.split(".")[1:]:
            node = node["children"].setdefault(seg, {"children": {}, "functions": {}})
        node["functions"].setdefault(fn.name, []).append(fn)
    return tree


def _emit_function_tree(
    node: dict, objects: Dict[str, Class], indent: int
) -> List[str]:
    pad = "    " * indent
    lines: List[str] = []
    for name in sorted(node["functions"]):
        methods = [
            Method(name=fn.name, ret=fn.ret, args=fn.args, is_static=True)
            for fn in node["functions"][name]
        ]
        lines.append(f"{pad}{name}: {_method_type(_DUMMY_CLS, methods, objects)},\n")
    for seg in sorted(node["children"]):
        lines.append(f"{pad}{seg}: {{\n")
        lines.extend(_emit_function_tree(node["children"][seg], objects, indent + 1))
        lines.append(f"{pad}}},\n")
    return lines


def _enum_block(namespace: str) -> str:
    names = sorted(enum_lua_names(namespace))
    if not names:
        return ""
    return "".join(f"export type {name} = number\n" for name in names)


def _header(label: str) -> List[str]:
    return [
        "-- Generated by LuauAPI codegen\n",
        f"-- {label}\n",
        "\n",
    ]


def _emit_factory_records(
    factories: Dict[str, Dict[str, List[Method]]],
    objects: Dict[str, Class],
) -> List[str]:
    lines: List[str] = []
    for cls_name in sorted(factories):
        methods = factories[cls_name]
        lines.append(f"export type {cls_name}Factory = {{\n")
        for name, overloads in sorted(methods.items()):
            if len(overloads) > 1:
                type_str = _widened_method_type(
                    objects[cls_name], overloads, objects, static=True
                )
            else:
                type_str = _method_type(objects[cls_name], overloads, objects)
            lines.append(f"    {name}: {type_str},\n")
        lines.append("}\n\n")
    return lines


def _factory_field_lines(factories: Dict[str, Dict[str, List[Method]]]) -> List[str]:
    return [f"    {cls_name}: {cls_name}Factory,\n" for cls_name in sorted(factories)]


def _emit_factories(
    factories: Dict[str, Dict[str, List[Method]]],
    objects: Dict[str, Class],
    namespace: str,
) -> List[str]:
    lines = _emit_factory_records(factories, objects)
    lines.append(f"export type {_namespace_type_name(namespace)} = {{\n")
    lines.extend(_factory_field_lines(factories))
    lines.append("}\n\n")
    return lines


def _namespace_type_name(namespace: str) -> str:
    if namespace == "geode.cocos2d":
        return "Cocos2dNamespace"
    return "GDNamespace"


def _collect_factories(
    classes: Sequence[Class],
    grouped_by_class: Dict[str, Dict[str, List[Method]]],
    skipped_classes: set,
    namespace: str,
) -> Dict[str, Dict[str, List[Method]]]:
    factories: Dict[str, Dict[str, List[Method]]] = {}
    for cls in classes:
        if cls.name in skipped_classes or lua_namespace(cls) != namespace:
            continue
        grouped = grouped_by_class[cls.name]
        static_methods = {
            k: v
            for k, v in grouped.items()
            if v[0].is_static and k not in LUAU_KEYWORDS
        }
        if static_methods:
            factories[cls.name] = static_methods
    return factories


def _should_emit_type_class(
    cls: Class,
    objects: Dict[str, Class],
    field_targets: list[tuple[Class, Field]],
    skipped_classes: set,
) -> bool:
    if cls.name not in skipped_classes:
        return True
    bound = {field.name for _, field in field_targets}
    for field in cls.fields:
        ok, reason, _, ret = bindable_field(field, objects, cls)
        if ok and field.name in bound and ret:
            return True
        if reason:
            return True
    return False


def _emitted_base_name(
    cls: Class, objects: Dict[str, Class], skipped_classes: set
) -> str | None:
    for b in cls.bases:
        b_cls = objects.get(short_name(b))
        if b_cls and b_cls.name not in skipped_classes:
            return b_cls.name
    return None


def _emit_class(
    cls: Class,
    grouped: Dict[str, List[Method]],
    field_targets: list[tuple[Class, Field]],
    objects: Dict[str, Class],
    skipped_classes: set,
) -> List[str]:
    lines: List[str] = []
    base_name = _emitted_base_name(cls, objects, skipped_classes)
    base = f" extends {base_name}" if base_name else ""
    instance_methods = {
        k: v
        for k, v in grouped.items()
        if not v[0].is_static and k not in LUAU_KEYWORDS
    }
    bound_field_names = {field.name for _, field in field_targets}
    field_lines: List[str] = []
    if _is_ccnode_descendant(cls, objects, skipped_classes):
        field_lines.append("    m_fields: { [string]: any }\n")
    for field in cls.fields:
        ok, reason, _, ret = bindable_field(field, objects, cls)
        if ok and field.name in bound_field_names and ret:
            field_lines.append(f"    {field.name}: {ret.lua_type}\n")
        elif reason:
            field_lines.append(f"    -- skipped {field.name}: {reason}\n")

    if not instance_methods and not field_lines:
        lines.append(f"declare class {cls.name}{base} end\n\n")
    else:
        lines.append(f"declare class {cls.name}{base}\n")
        lines.extend(field_lines)
        for name, methods in sorted(instance_methods.items()):
            if len(methods) == 1:
                m = methods[0]
                args = _classify_input_args(m, objects)
                ret_type = _method_return_type(cls, m, objects)
                arg_text = ", ".join(
                    f"arg{i}: {arg.lua_type}" for i, arg in enumerate(args, start=1)
                )
                joiner = ", " if arg_text else ""
                self_prefix = "" if m.is_static else f"self{joiner}"
                if m.is_static:
                    lines.append(
                        f"    function {name}({arg_text}){': ' + ret_type if ret_type != '()' else ''}\n"
                    )
                else:
                    lines.append(
                        f"    function {name}({self_prefix}{arg_text}){': ' + ret_type if ret_type != '()' else ''}\n"
                    )
            else:
                widened = _widened_method_type(cls, methods, objects, static=False)
                lines.append(f"    {name}: {widened}\n")
        lines.append("end\n\n")
    return lines


def _is_ccnode_descendant(
    cls: Class,
    objects: Dict[str, Class],
    skipped_classes: set,
    seen: set[str] | None = None,
) -> bool:
    if cls.name == "CCNode":
        return True
    if cls.name in skipped_classes:
        return False
    seen = seen or set()
    if cls.name in seen:
        return False
    seen.add(cls.name)
    for base in cls.bases:
        base_cls = objects.get(short_name(base))
        if base_cls and _is_ccnode_descendant(base_cls, objects, skipped_classes, seen):
            return True
    return False


def _object_type_name(info: TypeInfo) -> str:
    if info.class_name:
        return info.class_name
    return info.lua_type.removesuffix("?")


def _refs_from_method(method: Method, objects: Dict[str, Class]) -> set[str]:
    refs: set[str] = set()
    for arg in method.args:
        info = classify_arg(arg.type, objects)
        if info and info.kind == "object":
            refs.add(_object_type_name(info))
    ret = classify_return(method.ret, objects)
    if ret and ret.kind == "object":
        refs.add(_object_type_name(ret))
    return refs


def _refs_from_fields(
    cls: Class, fields: Sequence[Field], objects: Dict[str, Class]
) -> set[str]:
    refs: set[str] = set()
    for field in fields:
        ok, _, _, ret = bindable_field(field, objects, cls)
        if ok and ret and ret.kind == "object":
            refs.add(_object_type_name(ret))
    return refs


def _base_type_refs(
    cls: Class, objects: Dict[str, Class], skipped_classes: set
) -> set[str]:
    refs: set[str] = set()
    for base in cls.bases:
        base_cls = objects.get(short_name(base))
        if base_cls and base_cls.name not in skipped_classes:
            refs.add(base_cls.name)
    return refs


def _refs_from_classes(
    class_names: set[str],
    grouped_by_class: Dict[str, Dict[str, List[Method]]],
    objects: Dict[str, Class],
    skipped_classes: set,
) -> set[str]:
    refs: set[str] = set()
    for name in class_names:
        cls = objects.get(name)
        if not cls or name in skipped_classes:
            continue
        refs.update(_base_type_refs(cls, objects, skipped_classes))
        refs.update(_refs_from_fields(cls, cls.fields, objects))
        for methods in grouped_by_class.get(name, {}).values():
            for method in methods:
                refs.update(_refs_from_method(method, objects))
    return refs


def _factory_object_refs(
    factories: Dict[str, Dict[str, List[Method]]],
    objects: Dict[str, Class],
) -> set[str]:
    refs: set[str] = set()
    for methods in factories.values():
        for overloads in methods.values():
            for method in overloads:
                refs.update(_refs_from_method(method, objects))
    return refs


def _refs_from_text(content: str) -> set[str]:
    refs: set[str] = set()
    for match in re.finditer(r"extends (\w+)", content):
        refs.add(match.group(1))
    for match in re.finditer(
        r":\s*(\w+)\??(?:\s*[,&)]|\s*$|\s*->)", content, re.MULTILINE
    ):
        name = match.group(1)
        if name[0].isupper():
            refs.add(name)
    return refs


def _emit_orphan_stubs(names: set[str]) -> str:
    if not names:
        return ""
    lines = [
        "-- Forward declarations for referenced classes without bindable members\n",
        "\n",
    ]
    for name in sorted(names):
        lines.append(f"declare class {name} end\n\n")
    return "".join(lines)


def _topo_sort_chunks(
    chunks: List[Tuple[str, str]], base_of: Dict[str, str | None]
) -> List[Tuple[str, str]]:
    chunk_map = dict(chunks)
    present = set(chunk_map)
    placed: set[str] = set()
    ordered: List[str] = []

    def visit(name: str, stack: frozenset[str]) -> None:
        if name in placed:
            return
        base = base_of.get(name)
        if base in present and base not in placed and name not in stack:
            visit(base, stack | {name})
        placed.add(name)
        ordered.append(name)

    for name in sorted(chunk_map):
        visit(name, frozenset())
    return [(n, chunk_map[n]) for n in ordered]


def emit(
    root: Root, target_platform: str = "win", plan: EmitPlan | None = None
) -> Dict[str, str]:
    if plan is None:
        plan = collect_plan(root, target_platform)
    classes = plan.classes
    objects = plan.objects
    grouped_by_class = plan.supported_by_class
    skipped_classes = plan.skipped_classes

    cocos_chunks: List[Tuple[str, str]] = []
    gd_chunks: List[Tuple[str, str]] = []
    for cls in classes:
        field_targets = plan.field_targets_by_class.get(cls.name, [])
        if not _should_emit_type_class(cls, objects, field_targets, skipped_classes):
            continue
        grouped = grouped_by_class[cls.name]
        chunk = "".join(
            _emit_class(cls, grouped, field_targets, objects, skipped_classes)
        )
        if lua_namespace(cls) == cocos_namespace:
            cocos_chunks.append((cls.name, chunk))
        else:
            gd_chunks.append((cls.name, chunk))

    base_of = {
        name: _emitted_base_name(objects[name], objects, skipped_classes)
        for name in {n for n, _ in cocos_chunks} | {n for n, _ in gd_chunks}
        if name in objects
    }
    cocos_chunks = _topo_sort_chunks(cocos_chunks, base_of)
    gd_chunks = _topo_sort_chunks(gd_chunks, base_of)

    cocos_body = "".join(chunk for _, chunk in cocos_chunks)
    gd_body = "".join(chunk for _, chunk in gd_chunks)

    cocos_factories = _collect_factories(
        classes, grouped_by_class, skipped_classes, cocos_namespace
    )
    gd_factories = _collect_factories(
        classes, grouped_by_class, skipped_classes, gd_namespace
    )
    geode_factories = _collect_factories(
        classes, grouped_by_class, skipped_classes, geode_namespace
    )
    cocos_factory_text = "".join(
        _emit_factories(cocos_factories, objects, cocos_namespace)
    )
    gd_factory_text = "".join(_emit_factories(gd_factories, objects, gd_namespace))

    geode_factory_text = "".join(_emit_factory_records(geode_factories, objects))

    function_tree = _build_function_tree(root.functions, objects)
    function_field_lines = _emit_function_tree(function_tree, objects, 1)

    defined = {name for name, _ in cocos_chunks} | {name for name, _ in gd_chunks}
    refs = _refs_from_classes(defined, grouped_by_class, objects, skipped_classes)
    refs |= _factory_object_refs(cocos_factories, objects)
    refs |= _factory_object_refs(gd_factories, objects)
    refs |= _factory_object_refs(geode_factories, objects)
    refs |= _refs_from_text(
        cocos_body + gd_body + cocos_factory_text + gd_factory_text + geode_factory_text
    )
    refs |= _refs_from_text("".join(function_field_lines))
    orphan_names = {
        name
        for name in refs
        if name not in defined and name in objects and name not in _VALUE_STUB_BODY
    }

    lines = _header("Geode type stubs")

    value_block = _emit_value_stub_block(
        _value_refs_in_text(cocos_body + gd_body + cocos_factory_text + gd_factory_text)
    )
    if value_block:
        lines.append(value_block)
        lines.append("\n")

    for namespace in (cocos_namespace, gd_namespace, geode_namespace):
        enum_block = _enum_block(namespace)
        if enum_block:
            lines.append(enum_block)
            lines.append("\n")

    orphan_block = _emit_orphan_stubs(orphan_names)
    if orphan_block:
        lines.append(orphan_block)

    lines.append(cocos_body)
    lines.append(gd_body)

    lines.append(cocos_factory_text)
    lines.append(gd_factory_text)
    lines.append(geode_factory_text)

    lines.append("export type HookHandle = {\n")
    lines.append("    enable: (self: HookHandle) -> (boolean, string?),\n")
    lines.append("    disable: (self: HookHandle) -> (boolean, string?),\n")
    lines.append("    remove: (self: HookHandle) -> boolean,\n")
    lines.append("    isEnabled: (self: HookHandle) -> boolean,\n")
    lines.append("}\n\n")
    lines.append("export type HookCallbackTable = {\n")
    lines.append("    before: ((...any) -> any?)?,\n")
    lines.append("    after: ((...any) -> any?)?,\n")
    lines.append("    priority: number?,\n")
    lines.append("}\n\n")

    lines.append("export type ModNamespace = {\n")
    lines.append("    getSavedValue: (key: string) -> any,\n")
    lines.append("    setSavedValue: (key: string, value: any) -> (),\n")
    lines.append("    getSettingValue: (key: string) -> any,\n")
    lines.append("    hasSetting: (key: string) -> boolean,\n")
    lines.append("    getID: () -> string,\n")
    lines.append("    getName: () -> string,\n")
    lines.append("    getVersion: () -> string,\n")
    lines.append("    getResourcesDir: () -> string,\n")
    lines.append("    getSaveDir: () -> string,\n")
    lines.append("    getConfigDir: () -> string,\n")
    lines.append("}\n\n")

    lines.append("export type GeodeNamespace = {\n")
    lines.append("    cocos2d: Cocos2dNamespace,\n")
    lines.append("    gd: GDNamespace,\n")
    lines.append("    Mod: ModNamespace,\n")
    lines.extend(_factory_field_lines(geode_factories))
    lines.extend(function_field_lines)
    lines.append(
        "    hook: (target: string, callback: HookCallbackTable) -> HookHandle,\n"
    )
    lines.append("    skip: (value: any?) -> any,\n")
    lines.append("    fields: (self: CCNode) -> { [string]: any },\n")
    lines.append("}\n\n")
    lines.append("declare geode: GeodeNamespace\n")

    return {TYPES_FILE: "".join(lines)}
