from __future__ import annotations

import glob
import os


def _write_if_changed(path: str, content: str) -> None:
    try:
        with open(path, "r", encoding="utf-8") as f:
            if f.read() == content:
                return
    except FileNotFoundError:
        pass
    parent = os.path.dirname(path)
    if parent:
        os.makedirs(parent, exist_ok=True)
    tmp_path = f"{path}.tmp.{os.getpid()}"
    try:
        with open(tmp_path, "w", encoding="utf-8", newline="\n") as f:
            f.write(content)
        os.replace(tmp_path, path)
    except OSError:
        try:
            if os.path.exists(tmp_path):
                os.remove(tmp_path)
        finally:
            raise


def _cleanup_orphans(out_dir: str, current_files: set[str]) -> None:
    src_dir = os.path.join(out_dir, "src")
    legacy = os.path.join(src_dir, "luauapi_generated_bindings.cpp")
    if os.path.exists(legacy):
        os.remove(legacy)

    if not os.path.isdir(src_dir):
        return

    for path in glob.glob(os.path.join(src_dir, "bindings_*.cpp")):
        rel = os.path.relpath(path, out_dir).replace("\\", "/")
        if rel not in current_files:
            os.remove(path)

    legacy_hpp = os.path.join(src_dir, "bindings_internal.hpp")
    if "src/bindings_internal.hpp" not in current_files and os.path.exists(legacy_hpp):
        os.remove(legacy_hpp)
