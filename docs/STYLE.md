# Documentation style

## Summary

Use this when you edit or make new pages.
Keep docs plain, short, and easy to scan (not everyone can read).
If docs and code disagree, just use the code as reference.

This is mostly just my writing style, but I want to make sure everyone understands the docs.

## Page shape

Most pages use this shape:

1. `#` title.
2. `## Summary`.
3. Body sections.
4. `## Related`.
5. `## Source`.

Use `## Next` on tutorials. Use `## Limits` only when a page needs caps.
Link to [Limits and errors](reference/cpp/limits-and-errors.md) instead of copying stuff.

## Writing

- Use ASCII prose.
- No semicolons in prose.
- Keep sentences short.
- Keep one idea per sentence when you can.
- Use plain words. Prefer "run on the main thread" over jargon.
- Code blocks, API names, file paths, and error strings can use any characters they need.

## Links

- Link to files only.
- Use relative paths like `../reference/lua/globals.md`.
- Do not link to headings.
- Use the page title or a short label.
- Link outside the repo when this repo does not own the topic.
- Put shared facts in one place.

## Headings and sections

- One `#` title per page.
- Use sentence case.
- Do not skip levels.
- API reference pages may use API names as titles.
- Put three to ten links in `## Related`.
- Put closest links first.
- Include the parent hub for deep pages.
- Use backtick paths in `## Source`.
- Omit `## Source` on pure policy pages.

## Central sources

Do not repeat these topics. Link to the central page.

| Topic | Canonical page |
| --- | --- |
| Lua module index | [globals](reference/lua/globals.md) |
| Caps, deadlines, error strings | [Limits and errors](reference/cpp/limits-and-errors.md) |
| Codegen overview | [Codegen](contributor/codegen/codegen.md) |
| Test map | [Testing](contributor/testing.md) |
| Mod policy overlay | [LuauAPI mod guidelines](mod_guidelines.md) |
| Main-thread and runtime basics | [Getting started](getting-started/overview.md) |
| C++ host API surface | [C++ API reference](reference/cpp/api-reference.md) |
| Hook usage in scripts | [hooks](reference/lua/hooks.md) |
| Native crash Luau context | [Crash sidecar](contributor/internals/crash-sidecar.md) |
| Game object lifetime and refs | [game objects](reference/lua/game-objects.md) Ownership |

Keep hook policy in [LuauAPI mod guidelines](mod_guidelines.md).

## Truth sources

When unsure, check these first:

- Generated Luau stubs in `types/geode.d.luau` after a build.
- Handwritten stubs in `tools/luau_codegen/extra_bindings/`.
- C++ under `src/` and generated bindings under `build/luauapi-gen/src/`.
- Codegen under `tools/luau_codegen/`.
- Limits in `src/core/Config.hpp`.

If behavior changes there then update the docs too.
