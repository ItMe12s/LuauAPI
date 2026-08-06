from __future__ import annotations


def emit_arity_dispatcher(
    fn_name: str,
    top_expr: str,
    cases: list[tuple[int, str]],
    error_label: str,
) -> str:
    out = [f"    int {fn_name}(lua_State* L) {{\n", f"        switch ({top_expr}) {{\n"]
    for arity, target in cases:
        out.append(f"            case {arity}: return {target}(L);\n")
    out.append("            default: break;\n")
    out.append("        }\n")
    out.append(f'        luaL_error(L, "{error_label} unsupported overload arity");\n')
    out.append("    }\n\n")
    return "".join(out)
