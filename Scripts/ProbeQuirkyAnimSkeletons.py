from __future__ import annotations

import unreal


MESH_PATHS = [
    "/Game/Migrated/QuirkySeriesUltimate/Meshes/FREE_Pack/Sparrow",
    "/Game/Migrated/QuirkySeriesUltimate/Meshes/Mega_Pack_Vol_4/Sea_Vol_2/Prawn",
    "/Game/Migrated/QuirkySeriesUltimate/Meshes/Mega_Pack_Vol_4/Sea_Vol_2/Octopus",
]

ANIM_PATHS = [
    "/Game/Migrated/QuirkySeriesUltimate/Animations/FREE_Pack/Sparrow_Animations_Anim_Walk",
    "/Game/Migrated/QuirkySeriesUltimate/Animations/Mega_Pack_Vol_4/Sea_Vol_2/Prawn_Animations_Anim_Walk",
    "/Game/Migrated/QuirkySeriesUltimate/Animations/Mega_Pack_Vol_4/Sea_Vol_2/Octopus_Animations_Anim_Walk",
]


def main() -> None:
    for mesh_path, anim_path in zip(MESH_PATHS, ANIM_PATHS):
        mesh = unreal.EditorAssetLibrary.load_asset(mesh_path)
        anim = unreal.EditorAssetLibrary.load_asset(anim_path)
        if not mesh or not anim:
            unreal.log_error(f"[QuirkyProbe] Missing pair: {mesh_path} / {anim_path}")
            continue

        mesh_skel = mesh.get_editor_property("skeleton")
        anim_skel = anim.get_editor_property("skeleton")
        unreal.log(
            f"[QuirkyProbe] {mesh.get_name()} mesh_skel={mesh_skel.get_name() if mesh_skel else 'None'} "
            f"anim_skel={anim_skel.get_name() if anim_skel else 'None'}"
        )
        if anim_skel:
            pose = anim_skel.get_reference_pose()
            unreal.log(f"[QuirkyProbe] {anim.get_name()} bones={list(pose.get_bone_names())[:8]}")


if __name__ == "__main__":
    main()
