from __future__ import annotations

import unreal


MESH_PATH = "/Game/Migrated/QuirkySeriesUltimate/Meshes/Mega_Pack_Vol_1/Forest_Vol_1/Crow"
TEMP_DIR = "/Game/Migrated/QuirkySeriesUltimate/_TempRetargetProbe"


def main() -> None:
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    if unreal.EditorAssetLibrary.does_directory_exist(TEMP_DIR):
        unreal.EditorAssetLibrary.delete_directory(TEMP_DIR)
    unreal.EditorAssetLibrary.make_directory(TEMP_DIR)

    mesh = unreal.EditorAssetLibrary.load_asset(MESH_PATH)
    if not mesh:
        unreal.log_error(f"[QuirkyProbeRig] Missing mesh: {MESH_PATH}")
        return

    ikr = asset_tools.create_asset(
        asset_name="Crow_IKRig_Probe",
        package_path=TEMP_DIR,
        asset_class=unreal.IKRigDefinition,
        factory=unreal.IKRigDefinitionFactory(),
    )
    if not ikr:
        unreal.log_error("[QuirkyProbeRig] Could not create IK Rig asset")
        return

    controller = unreal.IKRigController.get_controller(ikr)
    controller.set_skeletal_mesh(mesh)
    try:
        controller.apply_auto_generated_retarget_definition()
    except Exception as exc:
        unreal.log_warning(f"[QuirkyProbeRig] auto generation failed: {exc}")

    try:
        root = controller.get_retarget_root()
        unreal.log(f"[QuirkyProbeRig] retarget root: {root}")
    except Exception as exc:
        unreal.log_warning(f"[QuirkyProbeRig] root query failed: {exc}")

    try:
        chains = controller.get_retarget_chains()
        unreal.log(f"[QuirkyProbeRig] chain count: {len(chains)}")
        for chain in chains:
            try:
                name = str(chain.get_editor_property("name"))
            except Exception:
                name = str(chain)
            try:
                start = controller.get_retarget_chain_start_bone(name)
                end = controller.get_retarget_chain_end_bone(name)
                goal = controller.get_retarget_chain_goal(name)
            except Exception as exc:
                start = end = goal = f"ERR:{exc}"
            unreal.log(f"[QuirkyProbeRig] chain {name}: {start} -> {end} goal={goal}")
    except Exception as exc:
        unreal.log_warning(f"[QuirkyProbeRig] chain query failed: {exc}")

    try:
        unreal.EditorAssetLibrary.save_loaded_asset(ikr)
    except Exception as exc:
        unreal.log_warning(f"[QuirkyProbeRig] save failed: {exc}")


if __name__ == "__main__":
    main()
