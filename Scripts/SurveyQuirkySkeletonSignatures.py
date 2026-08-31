from __future__ import annotations

import json
from collections import defaultdict
from pathlib import Path

import unreal


MESH_ROOT = "/Game/Migrated/QuirkySeriesUltimate/Meshes"
ANIM_ROOT = "/Game/Migrated/QuirkySeriesUltimate/Animations"
REPORT_PATH = Path(r"C:\Users\KGH\Gitfork\UnrealSecond\UnrealTeamProject\Saved\QuirkySeriesMigration\quirky_skeleton_signatures.json")


def bones_for_skeleton(skeleton) -> list[str]:
    pose = skeleton.get_reference_pose()
    return [str(name) for name in pose.get_bone_names()]


def main() -> None:
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    roots = [MESH_ROOT, ANIM_ROOT]
    unique: dict[str, dict[str, object]] = {}
    groups: dict[tuple[str, ...], list[str]] = defaultdict(list)

    for root in roots:
        for asset_data in registry.get_assets_by_path(root, recursive=True):
            asset = asset_data.get_asset()
            if not asset:
                continue
            try:
                skeleton = asset.get_editor_property("skeleton")
            except Exception:
                skeleton = None
            if not skeleton:
                continue
            skel_name = skeleton.get_name()
            if skel_name in unique:
                continue
            bones = bones_for_skeleton(skeleton)
            unique[skel_name] = {
                "bone_count": len(bones),
                "bones": bones,
            }
            groups[tuple(bones)].append(skel_name)

    report = {
        "unique_skeleton_count": len(unique),
        "exact_signature_groups": [
            {"size": len(names), "skeletons": sorted(names), "bone_count": len(sig)}
            for sig, names in sorted(groups.items(), key=lambda item: (-len(item[1]), len(item[0]), item[1][0]))
        ],
        "sample_skeletons": dict(list(unique.items())[:20]),
    }

    REPORT_PATH.parent.mkdir(parents=True, exist_ok=True)
    REPORT_PATH.write_text(json.dumps(report, indent=2), encoding="utf-8")
    unreal.log(f"[QuirkySig] unique_skeletons={len(unique)} groups={len(groups)} report={REPORT_PATH}")


if __name__ == "__main__":
    main()
