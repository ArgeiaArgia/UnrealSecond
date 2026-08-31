from __future__ import annotations

import sys
from pathlib import Path

import unreal


SCRIPT_DIR = Path(r"C:\Users\KGH\Gitfork\UnrealSecond\UnrealTeamProject\Scripts")
if str(SCRIPT_DIR) not in sys.path:
    sys.path.append(str(SCRIPT_DIR))

import MigrateQuirkySeriesUltimate as mig  # noqa: E402


def build_skeleton_lookup_from_content() -> dict[str, unreal.Object]:
    lookup: dict[str, unreal.Object] = {}
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    assets = registry.get_assets_by_path("/Game/Migrated/QuirkySeriesUltimate/Meshes", recursive=True)
    for asset_data in assets:
        asset = asset_data.get_asset()
        if not asset:
            continue
        try:
            skeleton = asset.get_editor_property("skeleton")
        except Exception:
            skeleton = None
        if skeleton:
            lookup[asset.get_name()] = skeleton
    return lookup


def main() -> None:
    target_dir = "/Game/Migrated/QuirkySeriesUltimate/Animations"
    if unreal.EditorAssetLibrary.does_directory_exist(target_dir):
        unreal.EditorAssetLibrary.delete_directory(target_dir)

    lookup = build_skeleton_lookup_from_content()
    stats = mig.ImportStats()
    mig.import_animation_assets(stats, lookup)
    unreal.log(f"[QuirkyMigration] Refreshed animations: {stats.animation_assets_imported} assets")


if __name__ == "__main__":
    main()
