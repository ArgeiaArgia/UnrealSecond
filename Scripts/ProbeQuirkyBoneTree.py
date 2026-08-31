from __future__ import annotations

import unreal


MESH_PATH = "/Game/Migrated/QuirkySeriesUltimate/Meshes/FREE_Pack/Colobus"


def main() -> None:
    mesh = unreal.EditorAssetLibrary.load_asset(MESH_PATH)
    if not mesh:
        unreal.log_error(f"[QuirkyProbe] Missing mesh: {MESH_PATH}")
        return

    skeleton = mesh.get_editor_property("skeleton")
    bone_tree = skeleton.get_editor_property("bone_tree")
    unreal.log(f"[QuirkyProbe] bone_tree length: {len(bone_tree)}")
    if len(bone_tree) == 0:
        return

    first = bone_tree[0]
    names = [name for name in dir(first) if "bone" in name.lower() or "parent" in name.lower() or "name" in name.lower() or "ref" in name.lower() or "index" in name.lower()]
    unreal.log(f"[QuirkyProbe] first bone tree members: {names}")
    for prop in names:
        try:
            value = getattr(first, prop)
            unreal.log(f"[QuirkyProbe] first.{prop} => {value}")
        except Exception as exc:
            unreal.log(f"[QuirkyProbe] first.{prop} unavailable: {exc}")

    try:
        ref_pose = skeleton.get_reference_pose()
        pose_names = [name for name in dir(ref_pose) if "bone" in name.lower() or "pose" in name.lower() or "transform" in name.lower() or "index" in name.lower()]
        unreal.log(f"[QuirkyProbe] reference pose type: {type(ref_pose)}")
        unreal.log(f"[QuirkyProbe] reference pose members: {pose_names}")
        for prop in pose_names:
            try:
                value = getattr(ref_pose, prop)
                unreal.log(f"[QuirkyProbe] pose.{prop} => {value}")
            except Exception as exc:
                unreal.log(f"[QuirkyProbe] pose.{prop} unavailable: {exc}")

        try:
            bone_names = list(ref_pose.get_bone_names())
            unreal.log(f"[QuirkyProbe] bone names ({len(bone_names)}): {bone_names}")
            sample = bone_names[:3]
            for bone_name in sample:
                try:
                    bone_pose = ref_pose.get_ref_bone_pose(bone_name)
                    unreal.log(f"[QuirkyProbe] ref pose for {bone_name}: {bone_pose}")
                except Exception as exc:
                    unreal.log(f"[QuirkyProbe] ref pose for {bone_name} unavailable: {exc}")
        except Exception as exc:
            unreal.log_error(f"[QuirkyProbe] get_bone_names failed: {exc}")
    except Exception as exc:
        unreal.log_error(f"[QuirkyProbe] get_reference_pose failed: {exc}")


if __name__ == "__main__":
    main()
