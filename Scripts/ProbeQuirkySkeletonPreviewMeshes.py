from __future__ import annotations

import json
from pathlib import Path

import unreal


ANIM_ROOT = "/Game/Migrated/QuirkySeriesUltimate/Animations"
OUT_PATH = Path(r"C:\Users\KGH\Gitfork\UnrealSecond\UnrealTeamProject\Saved\QuirkySeriesMigration\quirky_skeleton_preview_meshes.json")


def main() -> None:
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    rows = []

    for asset_data in registry.get_assets_by_path(ANIM_ROOT, recursive=True):
        asset = asset_data.get_asset()
        if not isinstance(asset, unreal.AnimSequence):
            continue
        try:
            skeleton = asset.get_editor_property("skeleton")
        except Exception:
            skeleton = None
        if not skeleton:
            continue

        preview_mesh = None
        preview_mesh_path = None
        try:
            preview_mesh = skeleton.get_editor_property("preview_mesh")
        except Exception:
            preview_mesh = None
        try:
            if hasattr(skeleton, "get_preview_mesh"):
                preview_mesh = skeleton.get_preview_mesh()
        except Exception:
            pass
        if preview_mesh:
            try:
                preview_mesh_path = preview_mesh.get_path_name()
            except Exception:
                preview_mesh_path = str(preview_mesh)

        rows.append(
            {
                "anim_name": asset.get_name(),
                "anim_path": asset.get_path_name(),
                "skeleton_name": skeleton.get_name(),
                "skeleton_path": skeleton.get_path_name(),
                "preview_mesh_path": preview_mesh_path,
            }
        )

    OUT_PATH.parent.mkdir(parents=True, exist_ok=True)
    OUT_PATH.write_text(json.dumps(rows, indent=2), encoding="utf-8")
    unreal.log(f"[QuirkyPreview] wrote {OUT_PATH} rows={len(rows)}")


if __name__ == "__main__":
    main()
