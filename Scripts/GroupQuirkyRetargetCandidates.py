from __future__ import annotations

import json
from collections import defaultdict
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, Iterable, List, Tuple

import unreal


MESH_ROOT = "/Game/Migrated/QuirkySeriesUltimate/Meshes"
ANIM_ROOT = "/Game/Migrated/QuirkySeriesUltimate/Animations"
VALIDATION_REPORT_PATH = Path(r"C:\Users\KGH\Gitfork\UnrealSecond\UnrealTeamProject\Saved\QuirkySeriesMigration\quirky_migration_validation.json")
REPORT_PATH = Path(r"C:\Users\KGH\Gitfork\UnrealSecond\UnrealTeamProject\Saved\QuirkySeriesMigration\quirky_retarget_candidates.json")


@dataclass(frozen=True)
class SkeletonRecord:
    name: str
    path: str
    bones: Tuple[str, ...]
    asset_count: int = 0


def log(message: str) -> None:
    unreal.log(f"[QuirkyRetarget] {message}")


def warn(message: str) -> None:
    unreal.log_warning(f"[QuirkyRetarget] {message}")


def get_bones(skeleton: unreal.Skeleton) -> Tuple[str, ...]:
    try:
        return tuple(str(name) for name in skeleton.get_reference_pose().get_bone_names())
    except Exception:
        return ()


def get_unique_skeletons(root: str) -> Dict[str, SkeletonRecord]:
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    result: Dict[str, SkeletonRecord] = {}
    counts: Dict[str, int] = defaultdict(int)

    for asset_data in registry.get_assets_by_path(root, recursive=True):
        asset = asset_data.get_asset()
        if not isinstance(asset, (unreal.SkeletalMesh, unreal.AnimSequence)):
            continue

        try:
            skeleton = asset.get_editor_property("skeleton")
        except Exception:
            skeleton = None
        if not skeleton:
            continue

        bones = get_bones(skeleton)
        if not bones:
            continue

        path = skeleton.get_path_name()
        counts[path] += 1
        if path not in result:
            result[path] = SkeletonRecord(name=skeleton.get_name(), path=path, bones=bones, asset_count=0)

    for path, record in list(result.items()):
        result[path] = SkeletonRecord(name=record.name, path=record.path, bones=record.bones, asset_count=counts[path])

    return result


def jaccard(lhs: Iterable[str], rhs: Iterable[str]) -> float:
    left = set(lhs)
    right = set(rhs)
    union = left | right
    if not union:
        return 0.0
    return len(left & right) / len(union)


def best_mesh_candidate(anim: SkeletonRecord, meshes: List[SkeletonRecord]) -> dict:
    anim_set = set(anim.bones)
    best = None
    for mesh in meshes:
        mesh_set = set(mesh.bones)
        overlap = len(anim_set & mesh_set)
        score = jaccard(anim.bones, mesh.bones)
        candidate = (score, overlap, -abs(len(anim.bones) - len(mesh.bones)), mesh.name, mesh)
        if best is None or candidate > best:
            best = candidate

    assert best is not None
    score, overlap, _, _, mesh = best
    return {
        "mesh_name": mesh.name,
        "mesh_path": mesh.path,
        "score": score,
        "overlap": overlap,
        "anim_bone_count": len(anim.bones),
        "mesh_bone_count": len(mesh.bones),
    }


def main() -> None:
    validation = json.loads(VALIDATION_REPORT_PATH.read_text(encoding="utf-8"))
    unmatched_names = {
        name
        for group in validation.get("unmatched_signature_groups", [])
        for name in group.get("anim_skeletons", [])
    }

    mesh_skeletons = get_unique_skeletons(MESH_ROOT)
    anim_registry = unreal.AssetRegistryHelpers.get_asset_registry()

    # Count all animation assets per unique skeleton path.
    anim_counts: Dict[str, int] = defaultdict(int)
    anim_records: Dict[str, SkeletonRecord] = {}
    mesh_signatures = {record.bones for record in mesh_skeletons.values()}

    for asset_data in anim_registry.get_assets_by_path(ANIM_ROOT, recursive=True):
        asset = asset_data.get_asset()
        if not isinstance(asset, unreal.AnimSequence):
            continue

        try:
            skeleton = asset.get_editor_property("skeleton")
        except Exception:
            skeleton = None
        if not skeleton:
            continue

        bones = get_bones(skeleton)
        if not bones:
            continue

        path = skeleton.get_path_name()
        anim_counts[path] += 1
        if path not in anim_records:
            anim_records[path] = SkeletonRecord(name=skeleton.get_name(), path=path, bones=bones, asset_count=0)

    anim_records = {
        path: SkeletonRecord(name=record.name, path=record.path, bones=record.bones, asset_count=anim_counts[path])
        for path, record in anim_records.items()
    }

    unresolved: List[SkeletonRecord] = []
    candidate_groups: Dict[str, dict] = {}
    total_unmatched_assets = 0

    for record in sorted(anim_records.values(), key=lambda item: (-item.asset_count, item.name)):
        if record.name not in unmatched_names:
            continue

        total_unmatched_assets += record.asset_count
        candidate = best_mesh_candidate(record, list(mesh_skeletons.values()))
        bucket = candidate_groups.setdefault(
            candidate["mesh_name"],
            {
                "mesh_name": candidate["mesh_name"],
                "mesh_path": candidate["mesh_path"],
                "score": candidate["score"],
                "overlap": candidate["overlap"],
                "anim_skeletons": [],
                "asset_count": 0,
                "anim_bone_counts": {},
            },
        )
        bucket["anim_skeletons"].append(record.name)
        bucket["asset_count"] += record.asset_count
        bucket["anim_bone_counts"][record.name] = len(record.bones)
        unresolved.append(record)

    sorted_groups = sorted(candidate_groups.values(), key=lambda item: (-item["asset_count"], item["mesh_name"]))
    for group in sorted_groups:
        group["anim_skeletons"] = sorted(group["anim_skeletons"])
        group["anim_bone_counts"] = dict(sorted(group["anim_bone_counts"].items(), key=lambda item: item[0]))
        group["score"] = round(float(group["score"]), 4)

    report = {
        "mesh_skeleton_count": len(mesh_skeletons),
        "anim_skeleton_count": len(anim_records),
        "unmatched_asset_count": total_unmatched_assets,
        "unmatched_skeleton_count": len(unresolved),
        "candidate_groups": sorted_groups,
        "unresolved_anim_skeletons": [
            {
                "anim_name": record.name,
                "anim_path": record.path,
                "asset_count": record.asset_count,
                "bone_count": len(record.bones),
                "bones": list(record.bones),
                "best_candidate": best_mesh_candidate(record, list(mesh_skeletons.values())),
            }
            for record in sorted(unresolved, key=lambda item: (-item.asset_count, item.name))
        ],
    }

    REPORT_PATH.parent.mkdir(parents=True, exist_ok=True)
    REPORT_PATH.write_text(json.dumps(report, indent=2), encoding="utf-8")

    log(
        f"mesh_skeletons={report['mesh_skeleton_count']} anim_skeletons={report['anim_skeleton_count']} "
        f"unmatched_assets={report['unmatched_asset_count']} groups={len(sorted_groups)}"
    )
    log(f"report={REPORT_PATH}")


if __name__ == "__main__":
    main()
