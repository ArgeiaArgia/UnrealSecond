from __future__ import annotations

import json
from collections import defaultdict
from pathlib import Path

import unreal


ROOTS = {
    "meshes": "/Game/Migrated/QuirkySeriesUltimate/Meshes",
    "animations": "/Game/Migrated/QuirkySeriesUltimate/Animations",
}
REPORT_PATH = Path(r"C:\Users\KGH\Gitfork\UnrealSecond\UnrealTeamProject\Saved\QuirkySeriesMigration\quirky_skeleton_data.json")


def get_bones(skeleton) -> list[str]:
    pose = skeleton.get_reference_pose()
    return [str(name) for name in pose.get_bone_names()]


def main() -> None:
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    by_kind: dict[str, dict[str, list[str]]] = {kind: {} for kind in ROOTS}
    seen: dict[str, str] = {}

    for kind, root in ROOTS.items():
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
            if skel_name in seen:
                continue
            seen[skel_name] = kind
            by_kind[kind][skel_name] = get_bones(skeleton)

    signature_groups: dict[str, list[str]] = defaultdict(list)
    for kind, skeletons in by_kind.items():
        for skel_name, bones in skeletons.items():
            signature_groups["|".join(bones)].append(f"{kind}:{skel_name}")

    report = {
        "meshes": by_kind["meshes"],
        "animations": by_kind["animations"],
        "exact_signature_groups": [
            {"signature": sig.split("|"), "members": sorted(members)}
            for sig, members in sorted(signature_groups.items(), key=lambda item: (-len(item[1]), len(item[0]), item[1][0]))
        ],
    }

    REPORT_PATH.parent.mkdir(parents=True, exist_ok=True)
    REPORT_PATH.write_text(json.dumps(report, indent=2), encoding="utf-8")
    unreal.log(f"[QuirkyDump] meshes={len(by_kind['meshes'])} animations={len(by_kind['animations'])} report={REPORT_PATH}")


if __name__ == "__main__":
    main()
