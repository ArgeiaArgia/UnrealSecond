from __future__ import annotations

import unreal


MESH_PATH = "/Game/Migrated/QuirkySeriesUltimate/Meshes/FREE_Pack/Colobus"


def main() -> None:
    mesh = unreal.EditorAssetLibrary.load_asset(MESH_PATH)
    if not mesh:
        unreal.log_error(f"[QuirkyProbe] Missing mesh: {MESH_PATH}")
        return

    unreal.log(f"[QuirkyProbe] Mesh class: {mesh.get_class().get_name()}")
    names = [name for name in dir(mesh) if "bone" in name.lower() or "skeleton" in name.lower() or "ref" in name.lower()]
    unreal.log(f"[QuirkyProbe] Mesh methods/properties: {names}")

    try:
        skeleton = mesh.get_editor_property("skeleton")
    except Exception as exc:
        unreal.log_error(f"[QuirkyProbe] Could not read mesh skeleton: {exc}")
        return

    if not skeleton:
        unreal.log_error("[QuirkyProbe] Mesh skeleton is null")
        return

    unreal.log(f"[QuirkyProbe] Skeleton class: {skeleton.get_class().get_name()}")
    names = [name for name in dir(skeleton) if "bone" in name.lower() or "skeleton" in name.lower() or "ref" in name.lower()]
    unreal.log(f"[QuirkyProbe] Skeleton methods/properties: {names}")

    for prop in ["ref_skeleton", "reference_skeleton", "bone_tree", "bone_names"]:
        try:
            value = skeleton.get_editor_property(prop)
            unreal.log(f"[QuirkyProbe] skeleton.{prop} => {type(value)}")
        except Exception as exc:
            unreal.log(f"[QuirkyProbe] skeleton.{prop} unavailable: {exc}")


if __name__ == "__main__":
    main()
