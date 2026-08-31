from __future__ import annotations

import json
from collections import defaultdict
from dataclasses import dataclass
from itertools import combinations
from pathlib import Path
from typing import Dict, Iterable, List, Tuple

import unreal


MESH_ROOT = "/Game/Migrated/QuirkySeriesUltimate/Meshes"
ANIM_ROOT = "/Game/Migrated/QuirkySeriesUltimate/Animations"
REPORT_PATH = Path(r"C:\Users\KGH\Gitfork\UnrealSecond\UnrealTeamProject\Saved\QuirkySeriesMigration\quirky_skeleton_compatibility.json")


@dataclass(frozen=True)
class SkeletonEntry:
    path: str
    kind: str
    name: str
    skeleton: unreal.Skeleton


def log(message: str) -> None:
    unreal.log(f"[QuirkyCompat] {message}")


def warn(message: str) -> None:
    unreal.log_warning(f"[QuirkyCompat] {message}")


def get_bone_tree_signature(skeleton: unreal.Skeleton) -> Tuple:
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

    def serialize(index: int) -> Tuple:
        child_signatures = tuple(sorted(serialize(child_index) for child_index in children.get(index, [])))
        return names[index], child_signatures

    return tuple(sorted(serialize(root_index) for root_index in roots))


def load_entries() -> List[SkeletonEntry]:
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    entries: List[SkeletonEntry] = []
    seen_paths: set[str] = set()

    for kind, root in (("mesh", MESH_ROOT), ("anim", ANIM_ROOT)):
        for asset_data in registry.get_assets_by_path(root, recursive=True):
            asset = asset_data.get_asset()
            if kind == "mesh" and not isinstance(asset, unreal.SkeletalMesh):
                continue
            if kind == "anim" and not isinstance(asset, unreal.AnimSequence):
                continue

            try:
                skeleton = asset.get_editor_property("skeleton")
            except Exception:
                skeleton = None
            if not skeleton:
                continue

            path = skeleton.get_path_name()
            if path in seen_paths:
                continue
            seen_paths.add(path)
            entries.append(SkeletonEntry(path=path, kind=kind, name=skeleton.get_name(), skeleton=skeleton))

    return entries


def pair_is_registered(lhs: unreal.Skeleton, rhs: unreal.Skeleton) -> bool:
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


def add_compatibility_pair(lhs: unreal.Skeleton, rhs: unreal.Skeleton) -> bool:
    changed = False
    if not pair_is_registered(lhs, rhs):
        lhs.add_compatible_skeleton(rhs)
        changed = True
    if not pair_is_registered(rhs, lhs):
        rhs.add_compatible_skeleton(lhs)
        changed = True
    return changed


def save_skeletons(skeletons: Iterable[unreal.Skeleton]) -> int:
    saved = 0
    for skeleton in skeletons:
        try:
            if unreal.EditorAssetLibrary.save_loaded_asset(skeleton):
                saved += 1
        except Exception as exc:
            warn(f"Failed to save {skeleton.get_name()}: {exc}")
    return saved


def main() -> None:
    entries = load_entries()
    by_signature: Dict[Tuple, List[SkeletonEntry]] = defaultdict(list)
    for entry in entries:
        by_signature[get_bone_tree_signature(entry.skeleton)].append(entry)

    changed_paths: set[str] = set()
    changed_skeletons: Dict[str, unreal.Skeleton] = {}
    applied_groups: List[dict] = []
    pair_count = 0

    for signature, group in sorted(by_signature.items(), key=lambda item: (-len(item[1]), len(item[0]), item[1][0].name if item[1] else "")):
        mesh_members = [entry for entry in group if entry.kind == "mesh"]
        anim_members = [entry for entry in group if entry.kind == "anim"]
        if not mesh_members or not anim_members:
            continue

        group_pairs = []
        unique_skeletons = {entry.path: entry.skeleton for entry in group}
        for left, right in combinations(unique_skeletons.values(), 2):
            if add_compatibility_pair(left, right):
                changed_paths.add(left.get_path_name())
                changed_paths.add(right.get_path_name())
                changed_skeletons[left.get_path_name()] = left
                changed_skeletons[right.get_path_name()] = right
                pair_count += 1
                group_pairs.append([left.get_name(), right.get_name()])

        if group_pairs:
            applied_groups.append(
                {
                    "signature": [str(item) for item in signature],
                    "mesh_skeletons": sorted(entry.name for entry in mesh_members),
                    "anim_skeletons": sorted(entry.name for entry in anim_members),
                    "pair_count": len(group_pairs),
                    "pairs": group_pairs[:20],
                }
            )

    saved_count = save_skeletons(sorted(changed_skeletons.values(), key=lambda skel: skel.get_path_name()))

    report = {
        "skeleton_count": len(entries),
        "changed_skeleton_count": len(changed_paths),
        "saved_skeleton_count": saved_count,
        "compatibility_pair_count": pair_count,
        "applied_group_count": len(applied_groups),
        "applied_groups": applied_groups,
    }

    REPORT_PATH.parent.mkdir(parents=True, exist_ok=True)
    REPORT_PATH.write_text(json.dumps(report, indent=2), encoding="utf-8")
    log(
        f"entries={len(entries)} changed={len(changed_paths)} "
        f"pairs={pair_count} groups={len(applied_groups)} saved={saved_count}"
    )
    log(f"report={REPORT_PATH}")


if __name__ == "__main__":
    main()
