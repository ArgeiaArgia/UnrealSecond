from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
from typing import Dict, Iterable, Optional, Tuple

import unreal


SOURCE_ROOT = Path(r"C:\Users\KGH\Gitfork\UnrealSecond\UnrealTeamProject\Content\Imported\Quirky Series Ultimate")
ANIM_ROOT = "/Game/Migrated/QuirkySeriesUltimate/Animations"
RETRY_ROOT = "/Game/Migrated/QuirkySeriesUltimate/_FallbackRetry"


def sanitize_segment(segment: str) -> str:
    import re

    cleaned = re.sub(r"[^A-Za-z0-9_]+", "_", segment.strip())
    cleaned = re.sub(r"_+", "_", cleaned).strip("_")
    return cleaned or "Unnamed"


def sanitize_name(name: str) -> str:
    return sanitize_segment(name)


def log(message: str) -> None:
    unreal.log(f"[QuirkyRetry] {message}")


def warn(message: str) -> None:
    unreal.log_warning(f"[QuirkyRetry] {message}")


def build_source_lookup() -> Dict[Tuple[str, str], Path]:
    lookup: Dict[Tuple[str, str], Path] = {}
    for src in SOURCE_ROOT.rglob("*.fbx"):
        rel = src.relative_to(SOURCE_ROOT)
        parts = list(rel.parts)
        lower_parts = [p.lower() for p in parts]
        if "animations" not in lower_parts:
            continue
        index = lower_parts.index("animations")
        context = "/".join(parts[:index] + parts[index + 1 : -1])
        stem = src.stem
        if stem.endswith("_Animations"):
            base_name = stem[: -len("_Animations")]
        else:
            base_name = stem
        lookup[(sanitize_segment(context), sanitize_name(base_name))] = src
    return lookup


def build_import_options() -> unreal.FbxImportUI:
    options = unreal.FbxImportUI()
    options.set_editor_property("automated_import_should_detect_type", False)
    options.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_ANIMATION)
    options.set_editor_property("import_as_skeletal", True)
    options.set_editor_property("import_mesh", False)
    options.set_editor_property("import_animations", True)
    options.set_editor_property("import_materials", False)
    options.set_editor_property("import_textures", False)
    options.set_editor_property("skeleton", None)

    anim_data = options.get_editor_property("anim_sequence_import_data")
    if anim_data:
        try:
            anim_data.set_editor_property("animation_length", unreal.FBXAnimationLengthImportType.FBXALIT_EXPORTED_TIME)
        except Exception:
            pass
        try:
            anim_data.set_editor_property("import_custom_attribute", False)
        except Exception:
            pass
    return options


def import_fbx(src: Path, dest_path: str, dest_name: str) -> None:
    task = unreal.AssetImportTask()
    task.filename = str(src)
    task.destination_path = dest_path
    task.destination_name = dest_name
    task.automated = True
    task.save = True
    task.replace_existing = True
    task.replace_existing_settings = True
    task.options = build_import_options()
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])


def delete_matching_assets(folder_path: str, base_name: str) -> None:
    assets = unreal.EditorAssetLibrary.list_assets(folder_path, recursive=False, include_folder=False)
    for asset_path in assets:
        asset = unreal.EditorAssetLibrary.load_asset(asset_path)
        if not asset:
            continue
        name = asset.get_name()
        if name == base_name or name.startswith(f"{base_name}_"):
            unreal.EditorAssetLibrary.delete_asset(asset_path)


def main() -> None:
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    assets = registry.get_assets_by_path(ANIM_ROOT, recursive=True)
    lookup = build_source_lookup()

    retry_count = 0
    missing_sources: list[str] = []
    failed_imports: list[str] = []

    for asset_data in assets:
        asset = asset_data.get_asset()
        if not isinstance(asset, unreal.StaticMesh):
            continue

        asset_path = asset.get_path_name()
        package_path = asset_path.rsplit(".", 1)[0]
        rel = package_path[len(ANIM_ROOT) + 1 :]
        rel_parts = rel.split("/")
        if len(rel_parts) < 2:
            continue

        context_key = sanitize_segment("/".join(rel_parts[:-1]))
        base_name = sanitize_name(rel_parts[-1])
        src = lookup.get((context_key, base_name))
        if not src:
            missing_sources.append(asset_path)
            warn(f"No source FBX found for {asset_path}")
            continue

        target_parent = package_path.rsplit("/", 1)[0]
        temp_parent = f"{RETRY_ROOT}/{rel_parts[:-1][0]}" if len(rel_parts[:-1]) == 1 else f"{RETRY_ROOT}/{'/'.join(rel_parts[:-1])}"
        temp_path = f"{temp_parent}/{base_name}"

        if unreal.EditorAssetLibrary.does_directory_exist(temp_path):
            unreal.EditorAssetLibrary.delete_directory(temp_path)
        unreal.EditorAssetLibrary.make_directory(temp_path)

        try:
            import_fbx(src, temp_path, base_name)
            temp_assets = unreal.EditorAssetLibrary.list_assets(temp_path, recursive=True, include_folder=False)
            imported_anim_count = 0
            for temp_asset_path in temp_assets:
                temp_asset = unreal.EditorAssetLibrary.load_asset(temp_asset_path)
                if isinstance(temp_asset, unreal.AnimSequence):
                    imported_anim_count += 1
            if imported_anim_count == 0:
                raise RuntimeError("No AnimSequence assets were created in the temp import")

            delete_matching_assets(target_parent, base_name)
            if unreal.EditorAssetLibrary.does_directory_exist(temp_path):
                if not unreal.EditorAssetLibrary.rename_directory(temp_path, target_parent):
                    raise RuntimeError(f"Failed to move temp assets into {target_parent}")
            retry_count += 1
            log(f"Retried as AnimSequence source: {src.name} -> {asset_path} ({imported_anim_count} anims)")
        except Exception as exc:
            failed_imports.append(f"{asset_path}: {exc}")
            warn(f"Retry failed for {asset_path}: {exc}")
            if unreal.EditorAssetLibrary.does_directory_exist(temp_path):
                unreal.EditorAssetLibrary.delete_directory(temp_path)

    log(f"Retry complete: {retry_count} assets")
    if missing_sources:
        warn(f"Missing sources: {len(missing_sources)}")
    if failed_imports:
        warn(f"Failed imports: {len(failed_imports)}")


if __name__ == "__main__":
    main()
