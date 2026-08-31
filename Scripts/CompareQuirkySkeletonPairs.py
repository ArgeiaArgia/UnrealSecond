from __future__ import annotations

import unreal


PAIRS = [
    (
        "/Game/Migrated/QuirkySeriesUltimate/Meshes/FREE_Pack/Sparrow",
        "/Game/Migrated/QuirkySeriesUltimate/Animations/FREE_Pack/Sparrow_Animations_Anim_Walk",
    ),
    (
        "/Game/Migrated/QuirkySeriesUltimate/Meshes/Mega_Pack_Vol_4/Sea_Vol_2/Prawn",
        "/Game/Migrated/QuirkySeriesUltimate/Animations/Mega_Pack_Vol_4/Sea_Vol_2/Prawn_Animations_Anim_Walk",
    ),
    (
        "/Game/Migrated/QuirkySeriesUltimate/Meshes/Mega_Pack_Vol_4/Sea_Vol_2/Octopus",
        "/Game/Migrated/QuirkySeriesUltimate/Animations/Mega_Pack_Vol_4/Sea_Vol_2/Octopus_Animations_Anim_Walk",
    ),
]


def get_bones(asset):
    skeleton = asset.get_editor_property("skeleton")
    pose = skeleton.get_reference_pose()
    return [str(name) for name in pose.get_bone_names()]


def main() -> None:
    for mesh_path, anim_path in PAIRS:
        mesh = unreal.EditorAssetLibrary.load_asset(mesh_path)
        anim = unreal.EditorAssetLibrary.load_asset(anim_path)
        if not mesh or not anim:
            unreal.log_error(f"[QuirkyCompare] Missing pair: {mesh_path} / {anim_path}")
            continue

        mesh_bones = get_bones(mesh)
        anim_bones = get_bones(anim)
        unreal.log(f"[QuirkyCompare] {mesh.get_name()} mesh={mesh_bones}")
        unreal.log(f"[QuirkyCompare] {anim.get_name()} anim={anim_bones}")
        unreal.log(f"[QuirkyCompare] equal={mesh_bones == anim_bones}")


if __name__ == "__main__":
    main()
