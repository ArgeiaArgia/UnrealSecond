from __future__ import annotations

import unreal


MESH_PATHS = [
    "/Game/Migrated/QuirkySeriesUltimate/Meshes/FREE_Pack/Sparrow",
    "/Game/Migrated/QuirkySeriesUltimate/Meshes/Mega_Pack_Vol_4/Sea_Vol_2/Whale",
    "/Game/Migrated/QuirkySeriesUltimate/Meshes/Mega_Pack_Vol_4/Sea_Vol_2/Tuna",
    "/Game/Migrated/QuirkySeriesUltimate/Meshes/Mega_Pack_Vol_4/Island_Vol_2/FlyingFox",
]


def main() -> None:
    for mesh_path in MESH_PATHS:
        mesh = unreal.EditorAssetLibrary.load_asset(mesh_path)
        if not mesh:
            unreal.log_error(f"[QuirkyProbe] Missing mesh: {mesh_path}")
            continue

        skeleton = mesh.get_editor_property("skeleton")
        if not skeleton:
            unreal.log_error(f"[QuirkyProbe] Missing skeleton: {mesh_path}")
            continue

        pose = skeleton.get_reference_pose()
        bone_names = [str(name) for name in pose.get_bone_names()]
        unreal.log(f"[QuirkyProbe] {mesh.get_name()} bones ({len(bone_names)}): {bone_names}")


if __name__ == "__main__":
    main()
