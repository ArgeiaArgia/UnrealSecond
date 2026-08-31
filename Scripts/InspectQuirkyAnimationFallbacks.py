from __future__ import annotations

import json
from pathlib import Path

import unreal


ANIM_ROOT = "/Game/Migrated/QuirkySeriesUltimate/Animations"
REPORT_PATH = Path(
    r"C:\Users\KGH\Gitfork\UnrealSecond\UnrealTeamProject\Saved\QuirkySeriesMigration\quirky_animation_fallbacks.json"
)


def main() -> None:
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    assets = registry.get_assets_by_path(ANIM_ROOT, recursive=True)

    anim_sequences: list[str] = []
    static_meshes: list[str] = []
    others: list[str] = []

    for asset_data in assets:
        asset = asset_data.get_asset()
        if isinstance(asset, unreal.AnimSequence):
            anim_sequences.append(asset.get_name())
        elif isinstance(asset, unreal.StaticMesh):
            static_meshes.append(asset.get_name())
        else:
            others.append(asset.get_name())

    report = {
        "anim_sequence_count": len(anim_sequences),
        "static_mesh_count": len(static_meshes),
        "other_count": len(others),
        "static_mesh_samples": static_meshes[:100],
        "other_samples": others[:20],
    }

    REPORT_PATH.parent.mkdir(parents=True, exist_ok=True)
    REPORT_PATH.write_text(json.dumps(report, indent=2), encoding="utf-8")

    unreal.log(f"[QuirkyFallbacks] anim_sequences={len(anim_sequences)} static_meshes={len(static_meshes)} others={len(others)}")
    unreal.log(f"[QuirkyFallbacks] report={REPORT_PATH}")


if __name__ == "__main__":
    main()
