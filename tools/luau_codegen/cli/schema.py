from __future__ import annotations

import json
import os

from luau_codegen.parse.broma import Root
from luau_codegen.emit.plan import EmitPlan, ambiguous_overloads
from luau_codegen.policy.fields import field_key
from luau_codegen.model.domain import status_for
from luau_codegen.cli.io import _write_if_changed


def _emit_schema(root: Root, path: str, plan: EmitPlan) -> None:
    bound_fields = {
        field_key(cls, field)
        for targets in plan.field_targets_by_class.values()
        for cls, field in targets
    }
    classes = []
    for cls in root.classes:
        classes.append(
            {
                "name": cls.name,
                "qualifiedName": cls.qualified_name,
                "bases": cls.bases,
                "attributes": cls.attributes,
                "source": os.path.basename(cls.source),
                "line": cls.line,
                "methods": [
                    {
                        "name": m.name,
                        "ret": m.ret,
                        "args": [{"type": a.type, "name": a.name} for a in m.args],
                        "static": m.is_static,
                        "virtual": m.is_virtual,
                        "const": m.is_const,
                        "ctor": m.is_ctor,
                        "dtor": m.is_dtor,
                        "platforms": m.platforms,
                        "status": status_for(m.platforms),
                        "line": m.line,
                    }
                    for m in cls.methods
                ],
                "fields": [
                    {
                        "name": f.name,
                        "type": f.type,
                        "count": f.count,
                        "line": f.line,
                        "bound": field_key(cls, f) in bound_fields,
                    }
                    for f in cls.fields
                ],
            }
        )
    ambiguous = [
        {"class": cls, "method": method, "reason": reason}
        for cls, method, reason in ambiguous_overloads(plan)
    ]
    _write_if_changed(
        path,
        json.dumps(
            {
                "classes": classes,
                "ambiguousOverloads": ambiguous,
            },
            indent=2,
        )
        + "\n",
    )
