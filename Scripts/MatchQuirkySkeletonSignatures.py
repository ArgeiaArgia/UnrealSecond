from __future__ import annotations

import json
from collections import defaultdict
from pathlib import Path

import unreal


MESH_ROOT = "/Game/Migrated/QuirkySeriesUltimate/Meshes"
ANIM_ROOT = "/Game/Migrated/QuirkySeriesUltimate/Animations"
REPORT_PATH = Path(r"C:\Users\KGH\Gitfork\UnrealSecond\UnrealTeamProject\Saved\QuirkySeriesMigration\quirky_skeleton_signature_matches.json")


def bone_signature(skeleton) -> tuple[str, ...]:
    pose = skeleton.get_reference_pose()
    return tuple(str(name) for name in pose.get_bone_names())


def main() -> None:
    registry = unreal.AssetRegistryHelpers.get_asset_registry()

    mesh_by_sig: dict[tuple[str, ...], list[str]] = defaultdict(list)
    anim_by_sig: dict[tuple[str, ...], list[str]] = defaultdict(list)
    unique_skeletons: dict[str, tuple[str, ...]] = {}

    for root, bucket in ((MESH_ROOT, mesh_by_sig), (ANIM_ROOT, anim_by_sig)):
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
            if skel_name in unique_skeletons:
                continue
            sig = bone_signature(skeleton)
            unique_skeletons[skel_name] = sig
            bucket[sig].append(skel_name)

    mesh_sigs = set(mesh_by_sig.keys())
    matched_anim_groups = []
    unmatched_anim_groups = []
    matched_anim_skeletons = 0
    unmatched_anim_skeletons = 0

    for sig, names in sorted(anim_by_sig.items(), key=lambda item: (-len(item[1]), len(item[0]), item[1][0])):
        if sig in mesh_sigs:
            matched_anim_groups.append({"signature": list(sig), "skeletons": sorted(names), "mesh_skeletons": sorted(mesh_by_sig[sig])})
            matched_anim_skeletons += len(names)
        else:
            unmatched_anim_groups.append({"signature": list(sig), "skeletons": sorted(names)})
            unmatched_anim_skeletons += len(names)

    report = {
        "mesh_group_count": len(mesh_by_sig),
        "anim_group_count": len(anim_by_sig),
        "matched_anim_skeleton_count": matched_anim_skeletons,
        "unmatched_anim_skeleton_count": unmatched_anim_skeletons,
        "matched_anim_groups": matched_anim_groups,
        "unmatched_anim_groups": unmatched_anim_groups,
    }

    REPORT_PATH.parent.mkdir(parents=True, exist_ok=True)
    REPORT_PATH.write_text(json.dumps(report, indent=2), encoding="utf-8")
    unreal.log(
        f"[QuirkyMatch] mesh_groups={len(mesh_by_sig)} anim_groups={len(anim_by_sig)} "
        f"matched_anim_skeletons={matched_anim_skeletons} unmatched_anim_skeletons={unmatched_anim_skeletons}"
    )
    unreal.log(f"[QuirkyMatch] report={REPORT_PATH}")


if __name__ == "__main__":
    main()
