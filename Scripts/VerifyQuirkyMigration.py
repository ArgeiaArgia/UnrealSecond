from __future__ import annotations

import json
import math
from dataclasses import dataclass, asdict
from collections import defaultdict
from pathlib import Path
from typing import Dict, List

import unreal


MESH_ROOT = "/Game/Migrated/QuirkySeriesUltimate/Meshes"
ANIM_ROOT = "/Game/Migrated/QuirkySeriesUltimate/Animations"
REPORT_PATH = Path(r"C:\Users\KGH\Gitfork\UnrealSecond\UnrealTeamProject\Saved\QuirkySeriesMigration\quirky_migration_validation.json")


@dataclass
class MeshCheck:
    name: str
    bone_count: int
    root_name: str
    second_name: str
    root_translation_len: float
    root_scale_avg: float
    root_ok: bool


def vec_len(vec) -> float:
    return math.sqrt(float(vec.x) ** 2 + float(vec.y) ** 2 + float(vec.z) ** 2)


def avg_scale(scale) -> float:
    return (abs(float(scale.x)) + abs(float(scale.y)) + abs(float(scale.z))) / 3.0


def get_bone_tree_signature(skeleton: unreal.Skeleton) -> tuple:
    bone_tree = list(skeleton.get_editor_property("bone_tree") or [])
    if not bone_tree:
        return ()

    try:
        names = [str(name) for name in skeleton.get_reference_pose().get_bone_names()]
    except Exception:
        return ()

    if len(names) != len(bone_tree):
        names = names[: len(bone_tree)]

    parents: List[int] = []
    for node in bone_tree[: len(names)]:
        try:
            parent_index = int(node.get_editor_property("parent_index"))
        except Exception:
            parent_index = int(getattr(node, "parent_index", -1))
        parents.append(parent_index)

    children: Dict[int, List[int]] = defaultdict(list)
    roots: List[int] = []
    for index, parent_index in enumerate(parents):
        if parent_index < 0:
            roots.append(index)
        else:
            children[parent_index].append(index)

    def serialize(index: int) -> tuple:
        child_signatures = tuple(sorted(serialize(child_index) for child_index in children.get(index, [])))
        return names[index], child_signatures

    return tuple(sorted(serialize(root_index) for root_index in roots))


def skeleton_is_compatible(lhs: unreal.Skeleton, rhs: unreal.Skeleton) -> bool:
    try:
        compatible = list(lhs.get_editor_property("compatible_skeletons") or [])
    except Exception:
        return False

    rhs_path = rhs.get_path_name()
    rhs_name = rhs.get_name()
    for item in compatible:
        if not item:
            continue
        try:
            if item.get_path_name() == rhs_path or item.get_name() == rhs_name:
                return True
        except Exception:
            continue
    return False


def main() -> None:
    registry = unreal.AssetRegistryHelpers.get_asset_registry()

    mesh_assets = registry.get_assets_by_path(MESH_ROOT, recursive=True)
    anim_assets = registry.get_assets_by_path(ANIM_ROOT, recursive=True)

    mesh_checks: List[MeshCheck] = []
    skeleton_lookup: Dict[str, unreal.Skeleton] = {}
    root_name_counts: Dict[str, int] = {}
    root_ok_count = 0
    mesh_signature_counts: Dict[tuple, int] = defaultdict(int)
    mesh_signature_groups: Dict[tuple, List[str]] = defaultdict(list)

    for asset_data in mesh_assets:
        asset = asset_data.get_asset()
        if not isinstance(asset, unreal.SkeletalMesh):
            continue

        skeleton = asset.get_editor_property("skeleton")
        if not skeleton:
            continue

        pose = skeleton.get_reference_pose()
        bone_names = list(pose.get_bone_names())
        if not bone_names:
            continue

        root_name = str(bone_names[0])
        second_name = str(bone_names[1]) if len(bone_names) > 1 else ""
        root_pose = pose.get_ref_bone_pose(bone_names[1] if len(bone_names) > 1 else bone_names[0])

        root_translation_len = vec_len(root_pose.translation)
        root_scale = avg_scale(root_pose.scale3d)
        root_ok = len(bone_names) >= 2 and root_translation_len <= 0.05 and abs(root_scale - 1.0) <= 0.05

        mesh_checks.append(
            MeshCheck(
                name=asset.get_name(),
                bone_count=len(bone_names),
                root_name=root_name,
                second_name=second_name,
                root_translation_len=root_translation_len,
                root_scale_avg=root_scale,
                root_ok=root_ok,
            )
        )
        skeleton_lookup[asset.get_name()] = skeleton
        root_name_counts[root_name] = root_name_counts.get(root_name, 0) + 1
        if root_ok:
            root_ok_count += 1
        signature = get_bone_tree_signature(skeleton)
        mesh_signature_counts[signature] += 1
        mesh_signature_groups[signature].append(skeleton.get_name())

    anim_sequence_count = 0
    static_fallback_count = 0
    anim_skeleton_mismatch: List[str] = []
    anim_skeleton_missing: List[str] = []
    anim_signature_matched: List[str] = []
    anim_signature_unmatched: List[str] = []
    anim_signature_groups: Dict[tuple, List[str]] = defaultdict(list)
    base_name_compatible = 0
    base_name_exact = 0

    for asset_data in anim_assets:
        asset = asset_data.get_asset()
        if isinstance(asset, unreal.AnimSequence):
            anim_sequence_count += 1
            anim_skel = asset.get_editor_property("skeleton")
            if not anim_skel:
                anim_skeleton_missing.append(asset.get_name())
                continue

            signature = get_bone_tree_signature(anim_skel)
            anim_signature_groups[signature].append(anim_skel.get_name())
            if signature in mesh_signature_counts:
                anim_signature_matched.append(asset.get_name())
            else:
                anim_signature_unmatched.append(asset.get_name())

            base_name = asset.get_name().split("_Animations", 1)[0]
            mesh_skel = skeleton_lookup.get(base_name)
            if mesh_skel:
                if anim_skel.get_name() == mesh_skel.get_name():
                    base_name_exact += 1
                elif skeleton_is_compatible(mesh_skel, anim_skel) or skeleton_is_compatible(anim_skel, mesh_skel):
                    base_name_compatible += 1
                else:
                    anim_skeleton_mismatch.append(asset.get_name())
            else:
                anim_skeleton_mismatch.append(asset.get_name())
        elif isinstance(asset, unreal.StaticMesh):
            static_fallback_count += 1

    shared_signature_groups = [
        {
            "signature": [str(item) for item in signature],
            "mesh_skeletons": sorted(set(mesh_signature_groups[signature])),
            "anim_skeletons": sorted(set(names)),
        }
        for signature, names in anim_signature_groups.items()
        if signature in mesh_signature_groups
    ]
    unmatched_signature_groups = [
        {"signature": [str(item) for item in signature], "anim_skeletons": sorted(set(names))}
        for signature, names in anim_signature_groups.items()
        if signature not in mesh_signature_groups
    ]

    report = {
        "mesh_count": len(mesh_checks),
        "root_ok_count": root_ok_count,
        "root_name_counts": dict(sorted(root_name_counts.items(), key=lambda item: (-item[1], item[0]))),
        "anim_sequence_count": anim_sequence_count,
        "static_fallback_count": static_fallback_count,
        "anim_skeleton_missing_count": len(anim_skeleton_missing),
        "anim_skeleton_mismatch_count": len(anim_skeleton_mismatch),
        "anim_base_name_exact_count": base_name_exact,
        "anim_base_name_compatible_count": base_name_compatible,
        "anim_signature_matched_count": len(anim_signature_matched),
        "anim_signature_unmatched_count": len(anim_signature_unmatched),
        "mismatched_anim_samples": anim_skeleton_mismatch[:20],
        "missing_anim_samples": anim_skeleton_missing[:20],
        "signature_matched_anim_samples": anim_signature_matched[:20],
        "signature_unmatched_anim_samples": anim_signature_unmatched[:20],
        "shared_signature_groups": shared_signature_groups,
        "unmatched_signature_groups": unmatched_signature_groups,
        "sample_meshes": [asdict(m) for m in mesh_checks[:5]],
    }

    REPORT_PATH.parent.mkdir(parents=True, exist_ok=True)
    REPORT_PATH.write_text(json.dumps(report, indent=2), encoding="utf-8")

    unreal.log(f"[QuirkyVerify] meshes={report['mesh_count']} root_ok={root_ok_count}")
    unreal.log(f"[QuirkyVerify] anim_sequences={anim_sequence_count} static_fallback={static_fallback_count}")
    unreal.log(
        f"[QuirkyVerify] anim_missing={len(anim_skeleton_missing)} anim_mismatch={len(anim_skeleton_mismatch)} "
        f"signature_match={len(anim_signature_matched)} signature_unmatched={len(anim_signature_unmatched)}"
    )
    unreal.log(
        f"[QuirkyVerify] base_exact={base_name_exact} base_compatible={base_name_compatible} "
        f"shared_signature_groups={len(shared_signature_groups)}"
    )
    unreal.log(f"[QuirkyVerify] root names={report['root_name_counts']}")
    unreal.log(f"[QuirkyVerify] report={REPORT_PATH}")


if __name__ == "__main__":
    main()
