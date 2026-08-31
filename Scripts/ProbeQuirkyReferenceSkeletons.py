from __future__ import annotations

import unreal


MESH_PATHS = [
    "/Game/Migrated/QuirkySeriesUltimate/Meshes/Mega_Pack_Vol_3/Farm_Vol_2/Turkey",
    "/Game/Migrated/QuirkySeriesUltimate/Meshes/Mega_Pack_Vol_3/Arctic_Vol_2/Beluga",
    "/Game/Migrated/QuirkySeriesUltimate/Meshes/Mega_Pack_Vol_4/Island_Vol_2/Bandicoot",
    "/Game/Migrated/QuirkySeriesUltimate/Meshes/Mega_Pack_Vol_2/Island_Vol_1/Koala",
    "/Game/Migrated/QuirkySeriesUltimate/Meshes/Mega_Pack_Vol_4/River_Vol_2/Shoebill",
    "/Game/Migrated/QuirkySeriesUltimate/Meshes/Mega_Pack_Vol_1/Farm_Vol_1/Rooster",
    "/Game/Migrated/QuirkySeriesUltimate/Meshes/Mega_Pack_Vol_1/Pets_Vol_1/Pigeon",
    "/Game/Migrated/QuirkySeriesUltimate/Meshes/Mega_Pack_Vol_4/Island_Vol_2/Quail",
    "/Game/Migrated/QuirkySeriesUltimate/Meshes/Mega_Pack_Vol_1/Forest_Vol_1/Crow",
    "/Game/Migrated/QuirkySeriesUltimate/Meshes/Mega_Pack_Vol_4/Sea_Vol_2/Jellyfish",
]


def main() -> None:
    for mesh_path in MESH_PATHS:
        mesh = unreal.EditorAssetLibrary.load_asset(mesh_path)
        if not mesh:
            unreal.log_error(f"[QuirkyRef] Missing mesh: {mesh_path}")
            continue
        skeleton = mesh.get_editor_property("skeleton")
        if not skeleton:
            unreal.log_error(f"[QuirkyRef] Missing skeleton: {mesh_path}")
            continue
        pose = skeleton.get_reference_pose()
        bones = [str(name) for name in pose.get_bone_names()]
        unreal.log(f"[QuirkyRef] {mesh.get_name()} bones={bones}")


if __name__ == "__main__":
    main()
