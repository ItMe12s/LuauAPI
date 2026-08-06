from __future__ import annotations

import unittest

from luau_codegen.model.codegen_context import CodegenContext  # type: ignore[import-unresolved]
from luau_codegen.model.delegate_specs import (  # type: ignore[import-unresolved]
    DelegateCatalog,
    DelegateSpec,
)
from luau_codegen.model.value_types import ValueTypeSpec  # type: ignore[import-unresolved]


class CodegenContextIsolationTests(unittest.TestCase):
    def test_value_specs_do_not_leak_between_contexts(self) -> None:
        derived = CodegenContext.static().with_value_specs(
            (ValueTypeSpec(lua_name="SyntheticValue", cxx_type="SyntheticValue"),)
        )
        empty = CodegenContext.static()

        self.assertEqual(derived.value_types.types["SyntheticValue"], "SyntheticValue")
        self.assertNotIn("SyntheticValue", empty.value_types.types)

    def test_delegate_specs_do_not_leak_between_contexts(self) -> None:
        spec = DelegateSpec(
            cxx_type="SyntheticDelegate",
            lua_name="SyntheticDelegateTable",
            cpp_class="LuaSyntheticDelegate",
            create_fn="LuaSyntheticDelegate::create",
            methods=(),
        )
        base = CodegenContext.static()
        populated = base.with_catalogs(
            value_types=base.value_types,
            delegates=DelegateCatalog.from_specs({spec.cxx_type: spec}),
        )
        empty = CodegenContext.static()

        self.assertIs(populated.delegates.lookup("SyntheticDelegate*"), spec)
        self.assertIsNone(empty.delegates.lookup("SyntheticDelegate*"))


if __name__ == "__main__":
    unittest.main()
