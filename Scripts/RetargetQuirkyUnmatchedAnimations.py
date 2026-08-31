from __future__ import annotations

import json
from collections import defaultdict
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, Iterable, List, Optional, Sequence, Tuple

import unreal


VALIDATION_PATH = Path(r"C:\Users\KGH\Gitfork\UnrealSecond\UnrealTeamProject\Saved\QuirkySeriesMigration\quirky_migration_validation.json")
CANDIDATE_PATH = Path(r"C:\Users\KGH\Gitfork\UnrealSecond\UnrealTeamProject\Saved\QuirkySeriesMigration\quirky_retarget_candidates.json")
REPORT_PATH = Path(r"C:\Users\KGH\Gitfork\UnrealSecond\UnrealTeamProject\Saved\QuirkySeriesMigration\quirky_retarget_run.json")

ANIM_ROOT = "/Game/Migrated/QuirkySeriesUltimate/Animations"
GAME_ROOT = "/Game"
WORK_ROOT = "/Game/Migrated/QuirkySeriesUltimate/Retargeted"
RIG_ROOT = f"{WORK_ROOT}/_Rigs"

MIN_SCORE = 0.75


@dataclass(frozen=True)
class ChainDef:
    name: str
    start_bone: str
    end_bone: str


def log(message: str) -> None:
    unreal.log(f"[QuirkyRetargetRun] {message}")


def warn(message: str) -> None:
    unreal.log_warning(f"[QuirkyRetargetRun] {message}")


def sanitize(segment: str) -> str:
    import re

    cleaned = re.sub(r"[^A-Za-z0-9_]+", "_", segment.strip())
    cleaned = re.sub(r"_+", "_", cleaned).strip("_")
    return cleaned or "Unnamed"


def get_bones_from_record(record: dict) -> List[str]:
    return [str(name) for name in record.get("bones", [])]


def first_existing(bones: Sequence[str], candidates: Sequence[str]) -> Optional[str]:
    for candidate in candidates:
        if candidate in bones:
            return candidate
    return None


def last_existing(bones: Sequence[str], candidates: Sequence[str]) -> Optional[str]:
    for candidate in candidates:
        if candidate in bones:
            return candidate
    return None


def tail_chain(bones: Sequence[str]) -> Optional[ChainDef]:
    start = first_existing(bones, ["tail_01", "tail"])
    end = last_existing(bones, ["tail_03", "tail_02", "tail_01", "tail"])
    if not start or not end:
        return None
    return ChainDef("Tail", start, end)


def body_chain(bones: Sequence[str]) -> Optional[ChainDef]:
    if "body" not in bones:
        return None
    return ChainDef("Body", "body", "body")


def head_chain(bones: Sequence[str]) -> Optional[ChainDef]:
    if "head" not in bones:
        return None
    return ChainDef("Head", "head", "head")


def wing_chain(side: str, bones: Sequence[str]) -> Optional[ChainDef]:
    bone = f"wing_{side}"
    if bone not in bones:
        return None
    return ChainDef(f"Wing_{side.upper()}", bone, bone)


def leg_chain(side: str, bones: Sequence[str]) -> Optional[ChainDef]:
    bone = f"leg_{side}"
    if bone not in bones:
        return None
    return ChainDef(f"Leg_{side.upper()}", bone, bone)


def quadruped_leg_chain(prefix: str, side: str, bones: Sequence[str]) -> Optional[ChainDef]:
    bone = f"leg_{prefix}_{side}"
    if bone not in bones:
        return None
    return ChainDef(f"Leg_{prefix.upper()}_{side.upper()}", bone, bone)


def ear_chain(side: str, bones: Sequence[str]) -> Optional[ChainDef]:
    bone = f"ear_{side}"
    if bone not in bones:
        return None
    return ChainDef(f"Ear_{side.upper()}", bone, bone)


def neck_chain_crow_source(record_name: str, bones: Sequence[str]) -> Optional[ChainDef]:
    if record_name == "Shoebill_Skeleton":
        if "neck" in bones and "head" in bones:
            return ChainDef("Neck", "neck", "head")
        return None
    return None


def neck_chain_crow_target(bones: Sequence[str]) -> Optional[ChainDef]:
    if "head" not in bones:
        return None
    return ChainDef("Neck", "body" if "body" in bones else "head", "head")


def neck_chain_emu_source(record_name: str, bones: Sequence[str]) -> Optional[ChainDef]:
    if record_name == "Swan_Skeleton":
        start = first_existing(bones, ["neck_01", "neck"])
        end = first_existing(bones, ["neck_02", "head"])
        if start and end:
            return ChainDef("Neck", start, end)
    return None


def neck_chain_emu_target(bones: Sequence[str]) -> Optional[ChainDef]:
    start = first_existing(bones, ["neck_01", "neck"])
    end = first_existing(bones, ["neck_02", "head"])
    if start and end:
        return ChainDef("Neck", start, end)
    return None


def hair_chain(record_name: str, bones: Sequence[str]) -> Optional[ChainDef]:
    if "hair_01" not in bones:
        return None
    if record_name == "Woodpecker_Skeleton":
        return ChainDef("Hair", "hair_01", "hair_01")
    if "hair_02" in bones:
        return ChainDef("Hair", "hair_01", "hair_02")
    return ChainDef("Hair", "hair_01", "hair_01")


def squid_chain(name: str, source_bones: Sequence[str], target_bones: Sequence[str]) -> Optional[Tuple[ChainDef, ChainDef]]:
    source_map = {
        "Tentacle_1_L": ("tentacle_01_01_L", "tentacle_01_02_L"),
        "Tentacle_2_L": ("tentacle_02_01_L", "tentacle_02_02_L"),
        "Tentacle_3_L": ("tentacle_03_01_L", "tentacle_03_02_L"),
        "Tentacle_4_L": ("tentacle_04_01_L", "tentacle_04_02_L"),
        "Tentacle_1_R": ("tentacle_01_01_R", "tentacle_01_02_R"),
        "Tentacle_2_R": ("tentacle_02_01_R", "tentacle_02_02_R"),
        "Tentacle_3_R": ("tentacle_03_01_R", "tentacle_03_02_R"),
        "Tentacle_4_R": ("tentacle_04_01_R", "tentacle_04_02_R"),
    }
    target_map = {
        "Tentacle_1_L": ("tentacle_L_01", "tentacle_L_05"),
        "Tentacle_2_L": ("tentacle_L_02", "tentacle_L_06"),
        "Tentacle_3_L": ("tentacle_L_03", "tentacle_L_07"),
        "Tentacle_4_L": ("tentacle_L_04", "tentacle_L_08"),
        "Tentacle_1_R": ("tentacle_R_01", "tentacle_R_05"),
        "Tentacle_2_R": ("tentacle_R_02", "tentacle_R_06"),
        "Tentacle_3_R": ("tentacle_R_03", "tentacle_R_07"),
        "Tentacle_4_R": ("tentacle_R_04", "tentacle_R_08"),
    }
    src = source_map.get(name)
    tgt = target_map.get(name)
    if not src or not tgt:
        return None
    if src[0] not in source_bones or src[1] not in source_bones:
        return None
    if tgt[0] not in target_bones or tgt[1] not in target_bones:
        return None
    return ChainDef(name, src[0], src[1]), ChainDef(name, tgt[0], tgt[1])


def build_source_chain_defs(record_name: str, bones: Sequence[str], family: str) -> List[ChainDef]:
    chains: List[ChainDef] = []
    if family == "crow":
        for chain in [body_chain(bones), head_chain(bones), wing_chain("L", bones), wing_chain("R", bones), leg_chain("L", bones), leg_chain("R", bones), tail_chain(bones)]:
            if chain:
                chains.append(chain)
        neck = neck_chain_crow_source(record_name, bones)
        if neck:
            chains.append(neck)
        hair = hair_chain(record_name, bones)
        if hair:
            chains.append(hair)
    elif family == "emu":
        for chain in [body_chain(bones), head_chain(bones), wing_chain("L", bones), wing_chain("R", bones), leg_chain("L", bones), leg_chain("R", bones), tail_chain(bones)]:
            if chain:
                chains.append(chain)
        neck = neck_chain_emu_source(record_name, bones)
        if neck:
            chains.append(neck)
    elif family == "bandicoot":
        for chain in [
            body_chain(bones),
            head_chain(bones),
            quadruped_leg_chain("f", "L", bones),
            quadruped_leg_chain("b", "L", bones),
            quadruped_leg_chain("f", "R", bones),
            quadruped_leg_chain("b", "R", bones),
            ear_chain("L", bones),
            ear_chain("R", bones),
        ]:
            if chain:
                chains.append(chain)
    elif family == "squid":
        if "body" in bones:
            chains.append(ChainDef("Body", "body", "body"))
        if "head" in bones:
            chains.append(ChainDef("Head", "head", "head"))
        for name in ["Tentacle_1_L", "Tentacle_2_L", "Tentacle_3_L", "Tentacle_4_L", "Tentacle_1_R", "Tentacle_2_R", "Tentacle_3_R", "Tentacle_4_R"]:
            pair = squid_chain(name, bones, bones)
            if pair:
                chains.append(pair[0])
    return chains


def build_target_chain_defs(family: str, bones: Sequence[str]) -> List[ChainDef]:
    chains: List[ChainDef] = []
    if family == "crow":
        for chain in [body_chain(bones), head_chain(bones), wing_chain("L", bones), wing_chain("R", bones), leg_chain("L", bones), leg_chain("R", bones), tail_chain(bones)]:
            if chain:
                chains.append(chain)
        neck = neck_chain_crow_target(bones)
        if neck:
            chains.append(neck)
        if "hair_01" in bones:
            if "hair_02" in bones:
                chains.append(ChainDef("Hair", "hair_01", "hair_02"))
            else:
                chains.append(ChainDef("Hair", "hair_01", "hair_01"))
    elif family == "emu":
        for chain in [body_chain(bones), head_chain(bones), wing_chain("L", bones), wing_chain("R", bones), leg_chain("L", bones), leg_chain("R", bones), tail_chain(bones)]:
            if chain:
                chains.append(chain)
        neck = neck_chain_emu_target(bones)
        if neck:
            chains.append(neck)
    elif family == "bandicoot":
        for chain in [
            body_chain(bones),
            head_chain(bones),
            quadruped_leg_chain("f", "L", bones),
            quadruped_leg_chain("b", "L", bones),
            quadruped_leg_chain("f", "R", bones),
            quadruped_leg_chain("b", "R", bones),
            ear_chain("L", bones),
            ear_chain("R", bones),
        ]:
            if chain:
                chains.append(chain)
    elif family == "squid":
        if "body" in bones:
            chains.append(ChainDef("Body", "body", "body"))
        for name in ["Tentacle_1_L", "Tentacle_2_L", "Tentacle_3_L", "Tentacle_4_L", "Tentacle_1_R", "Tentacle_2_R", "Tentacle_3_R", "Tentacle_4_R"]:
            pair = squid_chain(name, bones, bones)
            if pair:
                chains.append(pair[1])
    return chains


def create_or_update_ikrig(asset_path: str, asset_name: str, mesh: unreal.SkeletalMesh, chain_defs: List[ChainDef]) -> unreal.IKRigDefinition:
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        unreal.EditorAssetLibrary.delete_asset(asset_path)
    package_path = asset_path.rsplit("/", 1)[0]
    rig = asset_tools.create_asset(asset_name, package_path, unreal.IKRigDefinition, unreal.IKRigDefinitionFactory())
    if not rig:
        raise RuntimeError(f"Could not create IK Rig: {asset_path}")

    controller = unreal.IKRigController.get_controller(rig)
    if not controller.set_skeletal_mesh(mesh):
        raise RuntimeError(f"IK Rig rejected skeletal mesh: {mesh.get_name()}")

    root_bone = "root" if "root" in [str(name) for name in mesh.get_editor_property("skeleton").get_reference_pose().get_bone_names()] else "body"
    controller.set_retarget_root(root_bone)

    for chain_def in chain_defs:
        created_name = controller.add_retarget_chain(chain_def.name, "", "", "")
        controller.set_retarget_chain_start_bone(created_name, chain_def.start_bone)
        controller.set_retarget_chain_end_bone(created_name, chain_def.end_bone)

    unreal.EditorAssetLibrary.save_loaded_asset(rig)
    return rig


def create_or_update_retargeter(asset_path: str, asset_name: str, source_rig: unreal.IKRigDefinition, target_rig: unreal.IKRigDefinition, source_mesh: unreal.SkeletalMesh, target_mesh: unreal.SkeletalMesh) -> unreal.IKRetargeter:
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        unreal.EditorAssetLibrary.delete_asset(asset_path)
    package_path = asset_path.rsplit("/", 1)[0]
    rtg = asset_tools.create_asset(asset_name, package_path, unreal.IKRetargeter, unreal.IKRetargetFactory())
    if not rtg:
        raise RuntimeError(f"Could not create IK Retargeter: {asset_path}")

    controller = unreal.IKRetargeterController.get_controller(rtg)
    controller.set_ik_rig(unreal.RetargetSourceOrTarget.SOURCE, source_rig)
    controller.set_ik_rig(unreal.RetargetSourceOrTarget.TARGET, target_rig)
    controller.set_preview_mesh(unreal.RetargetSourceOrTarget.SOURCE, source_mesh)
    controller.set_preview_mesh(unreal.RetargetSourceOrTarget.TARGET, target_mesh)
    controller.auto_map_chains(unreal.AutoMapChainType.EXACT, True)
    unreal.EditorAssetLibrary.save_loaded_asset(rtg)
    return rtg


def get_unique_meshes() -> Dict[str, unreal.SkeletalMesh]:
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    meshes: Dict[str, unreal.SkeletalMesh] = {}
    for asset_data in registry.get_assets_by_path(GAME_ROOT, recursive=True):
        asset = asset_data.get_asset()
        if isinstance(asset, unreal.SkeletalMesh):
            meshes[asset.get_path_name()] = asset
    return meshes


def find_mesh_by_skeleton_path(meshes: Dict[str, unreal.SkeletalMesh], skeleton_path: str) -> Optional[unreal.SkeletalMesh]:
    for mesh in meshes.values():
        try:
            skeleton = mesh.get_editor_property("skeleton")
            if skeleton and skeleton.get_path_name() == skeleton_path:
                return mesh
        except Exception:
            continue
    return None


def find_mesh_by_skeleton_name(meshes: Dict[str, unreal.SkeletalMesh], skeleton_name: str) -> Optional[unreal.SkeletalMesh]:
    for mesh in meshes.values():
        try:
            skeleton = mesh.get_editor_property("skeleton")
            if skeleton and skeleton.get_name() == skeleton_name:
                return mesh
        except Exception:
            continue
    return None


def get_anim_assets_for_skeleton_path(skeleton_path: str) -> List[unreal.AssetData]:
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    result: List[unreal.AssetData] = []
    for asset_data in registry.get_assets_by_path(ANIM_ROOT, recursive=True):
        asset = asset_data.get_asset()
        if not isinstance(asset, unreal.AnimSequence):
            continue
        try:
            anim_skel = asset.get_editor_property("skeleton")
        except Exception:
            anim_skel = None
        if anim_skel and anim_skel.get_path_name() == skeleton_path:
            result.append(asset_data)
    return result


def ensure_directory(path: str) -> None:
    if not unreal.EditorAssetLibrary.does_directory_exist(path):
        unreal.EditorAssetLibrary.make_directory(path)


def cleanup_temp_assets(prefix: str = "RT_") -> int:
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    deleted = 0
    for asset_data in registry.get_assets_by_path(GAME_ROOT, recursive=True):
        try:
            asset_name = str(asset_data.asset_name)
            if not asset_name.startswith(prefix):
                continue
            asset_path = str(asset_data.object_path)
            if unreal.EditorAssetLibrary.delete_asset(asset_path):
                deleted += 1
        except Exception:
            continue
    return deleted


def main() -> None:
    validation = json.loads(VALIDATION_PATH.read_text(encoding="utf-8"))
    candidate = json.loads(CANDIDATE_PATH.read_text(encoding="utf-8"))

    mesh_assets = get_unique_meshes()
    target_mesh_by_name = {mesh.get_editor_property("skeleton").get_name(): mesh for mesh in mesh_assets.values() if mesh.get_editor_property("skeleton")}

    allowed_records: List[dict] = []
    for record in candidate.get("unresolved_anim_skeletons", []):
        score = float(record["best_candidate"]["score"])
        if score < MIN_SCORE:
            continue
        if record["best_candidate"]["mesh_name"] == "Beluga_Skeleton":
            continue
        allowed_records.append(record)

    ensure_directory(WORK_ROOT)
    ensure_directory(RIG_ROOT)
    deleted_temp = cleanup_temp_assets("RT_")
    if deleted_temp:
        log(f"cleaned_temp_assets={deleted_temp}")

    target_rig_cache: Dict[str, unreal.IKRigDefinition] = {}
    source_rig_cache: Dict[str, unreal.IKRigDefinition] = {}
    retargeter_cache: Dict[str, unreal.IKRetargeter] = {}

    created_outputs: List[str] = []
    skipped_records: List[dict] = []

    for record in allowed_records:
        record_name = record["anim_name"]
        source_skeleton_path = record["anim_path"]
        source_bones = get_bones_from_record(record)
        target_family_name = record["best_candidate"]["mesh_name"]

        if target_family_name not in {"Crow_Skeleton", "Bandicoot_Skeleton", "Emu_Skeleton", "Jellyfish_Skeleton"}:
            skipped_records.append({"record": record_name, "reason": f"Unsupported target family: {target_family_name}"})
            continue

        source_mesh = find_mesh_by_skeleton_path(mesh_assets, source_skeleton_path)
        if not source_mesh:
            source_mesh = find_mesh_by_skeleton_name(mesh_assets, record_name)
        target_mesh = target_mesh_by_name.get(target_family_name)
        if not source_mesh:
            skipped_records.append({"record": record_name, "reason": "Missing source skeletal mesh"})
            continue
        if not target_mesh:
            skipped_records.append({"record": record_name, "reason": f"Missing target skeletal mesh: {target_family_name}"})
            continue

        target_family = "crow" if target_family_name == "Crow_Skeleton" else "bandicoot" if target_family_name == "Bandicoot_Skeleton" else "emu" if target_family_name == "Emu_Skeleton" else "squid"
        target_bones = [str(name) for name in target_mesh.get_editor_property("skeleton").get_reference_pose().get_bone_names()]
        source_chain_defs = build_source_chain_defs(record_name, source_bones, target_family)
        target_chain_defs = build_target_chain_defs(target_family, target_bones)

        source_rig_path = f"{RIG_ROOT}/Sources/{sanitize(record_name)}_{sanitize(source_skeleton_path.split('/')[-1])}_IKRig"
        target_rig_path = f"{RIG_ROOT}/Targets/{sanitize(target_family_name)}_IKRig"
        retargeter_path = f"{RIG_ROOT}/Retargeters/{sanitize(record_name)}_to_{sanitize(target_family_name)}_RTG"

        ensure_directory(f"{RIG_ROOT}/Sources")
        ensure_directory(f"{RIG_ROOT}/Targets")
        ensure_directory(f"{RIG_ROOT}/Retargeters")

        source_rig = source_rig_cache.get(source_rig_path)
        if not source_rig:
            source_rig = create_or_update_ikrig(source_rig_path, source_rig_path.split("/")[-1], source_mesh, source_chain_defs)
            source_rig_cache[source_rig_path] = source_rig
        else:
            source_rig = create_or_update_ikrig(source_rig_path, source_rig_path.split("/")[-1], source_mesh, source_chain_defs)
            source_rig_cache[source_rig_path] = source_rig

        target_rig = target_rig_cache.get(target_rig_path)
        if not target_rig:
            target_rig = create_or_update_ikrig(target_rig_path, target_rig_path.split("/")[-1], target_mesh, target_chain_defs)
            target_rig_cache[target_rig_path] = target_rig
        else:
            target_rig = create_or_update_ikrig(target_rig_path, target_rig_path.split("/")[-1], target_mesh, target_chain_defs)
            target_rig_cache[target_rig_path] = target_rig

        retargeter = retargeter_cache.get(retargeter_path)
        if not retargeter:
            retargeter = create_or_update_retargeter(retargeter_path, retargeter_path.split("/")[-1], source_rig, target_rig, source_mesh, target_mesh)
            retargeter_cache[retargeter_path] = retargeter
        else:
            retargeter = create_or_update_retargeter(retargeter_path, retargeter_path.split("/")[-1], source_rig, target_rig, source_mesh, target_mesh)
            retargeter_cache[retargeter_path] = retargeter

        assets_to_retarget = get_anim_assets_for_skeleton_path(source_skeleton_path)
        if not assets_to_retarget:
            skipped_records.append({"record": record_name, "reason": "No animation assets found for source skeleton"})
            continue

        output_path = f"{WORK_ROOT}/{target_family_name.replace('_Skeleton', '')}/{sanitize(record_name)}"
        ensure_directory(output_path)

        temp_prefix = f"RT_{sanitize(record_name)}_"
        try:
            created = unreal.IKRetargetBatchOperation.duplicate_and_retarget(
                assets_to_retarget,
                source_mesh,
                target_mesh,
                retargeter,
                search="",
                replace="",
                prefix=temp_prefix,
                suffix="",
                include_referenced_assets=False,
            )

            moved_outputs: List[str] = []
            for source_item, created_item in zip(assets_to_retarget, created):
                source_asset = source_item.get_asset()
                created_asset = created_item.get_asset()
                if not source_asset or not created_asset:
                    continue
                final_name = source_asset.get_name()
                current_path = created_asset.get_path_name()
                final_path = f"{output_path}/{final_name}.{final_name}"
                if unreal.EditorAssetLibrary.does_asset_exist(final_path):
                    unreal.EditorAssetLibrary.delete_asset(final_path)
                if unreal.EditorAssetLibrary.rename_asset(current_path, final_path):
                    moved_outputs.append(final_path)
                else:
                    warn(f"Failed to move retargeted asset {current_path} -> {final_path}")

            created_outputs.extend(moved_outputs)
            log(f"retargeted {record_name} -> {target_family_name}: assets={len(assets_to_retarget)} outputs={len(moved_outputs)}")
        except Exception as exc:
            skipped_records.append({"record": record_name, "reason": f"Retarget failed: {exc}"})
            warn(f"{record_name} retarget failed: {exc}")

    report = {
        "allowed_record_count": len(allowed_records),
        "created_output_count": len(created_outputs),
        "created_outputs": created_outputs[:200],
        "skipped_records": skipped_records,
        "source_rig_count": len(source_rig_cache),
        "target_rig_count": len(target_rig_cache),
        "retargeter_count": len(retargeter_cache),
    }

    REPORT_PATH.parent.mkdir(parents=True, exist_ok=True)
    REPORT_PATH.write_text(json.dumps(report, indent=2), encoding="utf-8")
    log(f"report={REPORT_PATH}")
    log(
        f"allowed={report['allowed_record_count']} created_outputs={report['created_output_count']} "
        f"source_rigs={report['source_rig_count']} target_rigs={report['target_rig_count']}"
    )
    try:
        unreal.SystemLibrary.quit_editor()
    except Exception:
        pass


if __name__ == "__main__":
    main()
