from __future__ import annotations

import json
import re
from dataclasses import dataclass, asdict
from pathlib import Path
from typing import Dict, Iterable, List, Optional, Tuple

import unreal


SOURCE_ROOT = Path(r"C:\Users\KGH\Gitfork\UnrealSecond\UnrealTeamProject\Content\Imported\Quirky Series Ultimate")
PROJECT_ROOT = Path(r"C:\Users\KGH\Gitfork\UnrealSecond\UnrealTeamProject")
REPORT_DIR = PROJECT_ROOT / "Saved" / "QuirkySeriesMigration"
DEST_ROOT = "/Game/Migrated/QuirkySeriesUltimate"

TEXTURE_EXTS = {".png", ".tga", ".jpg", ".jpeg", ".bmp", ".dds", ".exr", ".tif", ".tiff", ".psd", ".gif"}
CATEGORY_MAP = {
    "textures": "Textures",
    "materials": "Materials",
    "models": "Meshes",
    "animations": "Animations",
}

GUID_RE = re.compile(r"^guid:\s*([0-9a-f]+)\s*$", re.MULTILINE)
ASSET_PATH_RE = re.compile(r"^assetPath:\s*(.+?)\s*$", re.MULTILINE)
MAT_NAME_RE = re.compile(r"^\s*m_Name:\s*(.+?)\s*$", re.MULTILINE)
FLOAT_RE = re.compile(r"^\s*-\s*(_[A-Za-z0-9]+):\s*([+-]?\d+(?:\.\d+)?)\s*$", re.MULTILINE)
COLOR_RE = re.compile(
    r"^\s*-\s*(_[A-Za-z0-9]+):\s*\{r:\s*([+-]?\d+(?:\.\d+)?),\s*g:\s*([+-]?\d+(?:\.\d+)?),\s*b:\s*([+-]?\d+(?:\.\d+)?),\s*a:\s*([+-]?\d+(?:\.\d+)?)\}\s*$",
    re.MULTILINE,
)
TEXTURE_REF_RE = re.compile(
    r"^\s*-\s*(_[A-Za-z0-9]+):\s*\n\s*m_Texture:\s*\{fileID:\s*\d+,\s*guid:\s*([0-9a-f]+),\s*type:\s*\d+\}",
    re.MULTILINE,
)
LOD_RE = re.compile(r"^(?P<base>.+)_LOD(?P<lod>\d+)$", re.IGNORECASE)
ANIM_RE = re.compile(r"^(?P<base>.+)_Animations$", re.IGNORECASE)


@dataclass
class MaterialSpec:
    name: str
    source_path: str
    texture_guid: Optional[str]
    texture_source: Optional[str]
    color: Tuple[float, float, float, float]
    emission: float


@dataclass
class ImportStats:
    textures_imported: int = 0
    materials_created: int = 0
    mesh_bases_imported: int = 0
    lods_imported: int = 0
    animation_assets_imported: int = 0
    skipped_files: int = 0
    unresolved_textures: int = 0
    unresolved_material_textures: int = 0
    errors: int = 0


def log(message: str) -> None:
    unreal.log(f"[QuirkyMigration] {message}")


def warn(message: str) -> None:
    unreal.log_warning(f"[QuirkyMigration] {message}")


def err(message: str) -> None:
    unreal.log_error(f"[QuirkyMigration] {message}")


def sanitize_segment(segment: str) -> str:
    cleaned = re.sub(r"[^A-Za-z0-9_]+", "_", segment.strip())
    cleaned = re.sub(r"_+", "_", cleaned).strip("_")
    return cleaned or "Unnamed"


def sanitize_name(name: str) -> str:
    return sanitize_segment(name)


def asset_path_for(category: str, rel_parts: Iterable[str]) -> str:
    pieces = [sanitize_segment(p) for p in rel_parts if p]
    if pieces:
        return f"{DEST_ROOT}/{category}/" + "/".join(pieces)
    return f"{DEST_ROOT}/{category}"


def ensure_directory(asset_path: str) -> None:
    if not unreal.EditorAssetLibrary.does_directory_exist(asset_path):
        unreal.EditorAssetLibrary.make_directory(asset_path)


def delete_matching_assets(folder_path: str, base_name: str) -> None:
    assets = unreal.EditorAssetLibrary.list_assets(folder_path, recursive=False, include_folder=False)
    for asset_path in assets:
        asset = unreal.EditorAssetLibrary.load_asset(asset_path)
        if not asset:
            continue
        name = asset.get_name()
        if name == base_name or name.startswith(f"{base_name}_"):
            unreal.EditorAssetLibrary.delete_asset(asset_path)


def read_text(path: Path) -> str:
    return path.read_text(encoding="utf-8", errors="ignore")


def parse_meta_guid_map() -> Dict[str, Path]:
    mapping: Dict[str, Path] = {}
    for meta in SOURCE_ROOT.rglob("*.meta"):
        text = read_text(meta)
        m = GUID_RE.search(text)
        if not m:
            continue
        guid = m.group(1)
        source = meta.with_suffix("")
        mapping[guid] = source
    return mapping


def parse_material_specs(guid_map: Dict[str, Path]) -> List[MaterialSpec]:
    specs: List[MaterialSpec] = []
    for mat in SOURCE_ROOT.rglob("*.mat"):
        text = read_text(mat)
        name_match = MAT_NAME_RE.search(text)
        name = name_match.group(1).strip() if name_match else mat.stem

        tex_match = TEXTURE_REF_RE.search(text)
        texture_guid = tex_match.group(2) if tex_match else None
        texture_source = str(guid_map[texture_guid]) if texture_guid in guid_map else None

        color_match = COLOR_RE.search(text)
        if color_match:
            color = (
                float(color_match.group(2)),
                float(color_match.group(3)),
                float(color_match.group(4)),
                float(color_match.group(5)),
            )
        else:
            color = (1.0, 1.0, 1.0, 1.0)

        float_match = FLOAT_RE.search(text)
        emission = 0.0
        if float_match and float_match.group(1) == "_Emission":
            emission = float(float_match.group(2))
        else:
            emission_search = re.search(r"^\s*-\s*_Emission:\s*([+-]?\d+(?:\.\d+)?)\s*$", text, re.MULTILINE)
            if emission_search:
                emission = float(emission_search.group(1))

        specs.append(
            MaterialSpec(
                name=name,
                source_path=str(mat),
                texture_guid=texture_guid,
                texture_source=texture_source,
                color=color,
                emission=emission,
            )
        )
    return specs


def category_and_context(relative_path: Path) -> Tuple[Optional[str], List[str]]:
    parts = list(relative_path.parts)
    lower_parts = [p.lower() for p in parts]
    for index, part in enumerate(lower_parts):
        if part in CATEGORY_MAP:
            category = CATEGORY_MAP[part]
            context = list(parts[:index] + parts[index + 1 : -1])
            return category, context
    return None, []


def import_tasks(tasks: List[unreal.AssetImportTask]) -> None:
    if not tasks:
        return
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)


def import_textures() -> Dict[str, str]:
    guid_map = parse_meta_guid_map()
    texture_asset_paths: Dict[str, str] = {}
    tasks: List[unreal.AssetImportTask] = []

    texture_files = [p for p in SOURCE_ROOT.rglob("*") if p.is_file() and p.suffix.lower() in TEXTURE_EXTS]
    texture_files.sort()

    for src in texture_files:
        rel = src.relative_to(SOURCE_ROOT)
        category, context = category_and_context(rel)
        if category != "Textures":
            continue

        asset_name = sanitize_name(src.stem)
        dest_path = asset_path_for("Textures", context)
        ensure_directory(dest_path)

        task = unreal.AssetImportTask()
        task.filename = str(src)
        task.destination_path = dest_path
        task.destination_name = asset_name
        task.automated = True
        task.save = True
        task.replace_existing = True
        task.replace_existing_settings = True
        tasks.append(task)

        texture_asset_paths[src.as_posix().lower()] = f"{dest_path}/{asset_name}"

    import_tasks(tasks)
    return texture_asset_paths


def create_material_asset(material_spec: MaterialSpec, texture_asset_path: Optional[str]) -> Optional[unreal.Material]:
    mat_path = Path(material_spec.source_path)
    rel = mat_path.relative_to(SOURCE_ROOT)
    _, context = category_and_context(rel)
    dest_path = asset_path_for("Materials", context)
    ensure_directory(dest_path)

    asset_name = sanitize_name(material_spec.name)
    factory = unreal.MaterialFactoryNew()
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    material = asset_tools.create_asset(asset_name, dest_path, unreal.Material, factory)
    if not material:
        warn(f"Failed to create material asset for {material_spec.name}")
        return None

    unreal.MaterialEditingLibrary.delete_all_material_expressions(material)
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_OPAQUE)

    tex_node = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionTextureSampleParameter2D, -800, 0
    )
    tex_node.set_editor_property("parameter_name", "MainTex")
    if texture_asset_path:
        texture_obj = unreal.EditorAssetLibrary.load_asset(texture_asset_path)
        if texture_obj:
            tex_node.set_editor_property("texture", texture_obj)

    tint_node = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionVectorParameter, -800, 220
    )
    tint_node.set_editor_property("parameter_name", "TintColor")
    tint_node.set_editor_property("default_value", unreal.LinearColor(*material_spec.color))

    tex_tint_mul = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionMultiply, -450, 60
    )
    unreal.MaterialEditingLibrary.connect_material_expressions(tex_node, "", tex_tint_mul, "A")
    unreal.MaterialEditingLibrary.connect_material_expressions(tint_node, "", tex_tint_mul, "B")
    unreal.MaterialEditingLibrary.connect_material_property(tex_tint_mul, "", unreal.MaterialProperty.MP_BASE_COLOR)

    emission_node = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionScalarParameter, -800, 420
    )
    emission_node.set_editor_property("parameter_name", "EmissionStrength")
    emission_node.set_editor_property("default_value", float(material_spec.emission))

    emissive_mul = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionMultiply, -450, 340
    )
    unreal.MaterialEditingLibrary.connect_material_expressions(tex_tint_mul, "", emissive_mul, "A")
    unreal.MaterialEditingLibrary.connect_material_expressions(emission_node, "", emissive_mul, "B")
    unreal.MaterialEditingLibrary.connect_material_property(emissive_mul, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

    unreal.MaterialEditingLibrary.layout_material_expressions(material)
    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material)
    return material


def import_materials(material_specs: List[MaterialSpec], texture_asset_paths: Dict[str, str]) -> Dict[str, str]:
    material_asset_paths: Dict[str, str] = {}
    for spec in material_specs:
        texture_path = None
        if spec.texture_source:
            texture_path = texture_asset_paths.get(Path(spec.texture_source).as_posix().lower())
        material = create_material_asset(spec, texture_path)
        if material:
            material_asset_paths[spec.name] = material.get_path_name()
    return material_asset_paths


def parse_mesh_groups() -> Dict[Tuple[str, str], Dict[str, Path]]:
    groups: Dict[Tuple[str, str], Dict[str, Path]] = {}
    for fbx in SOURCE_ROOT.rglob("*.fbx"):
        rel = fbx.relative_to(SOURCE_ROOT)
        category, context = category_and_context(rel)
        if category not in {"Meshes", "Animations"}:
            continue

        stem = fbx.stem
        lod_match = LOD_RE.match(stem)
        anim_match = ANIM_RE.match(stem)
        if lod_match:
            base_name = lod_match.group("base")
            lod_index = lod_match.group("lod")
            key = ("Meshes", "/".join(sanitize_segment(p) for p in context))
            groups.setdefault(key, {})[f"lod{lod_index}"] = fbx
        elif anim_match:
            base_name = anim_match.group("base")
            key = ("Animations", "/".join(sanitize_segment(p) for p in context))
            groups.setdefault(key, {})[base_name] = fbx
    return groups


def import_base_meshes_and_lods(stats: ImportStats) -> Dict[str, str]:
    mesh_asset_paths: Dict[str, str] = {}

    mesh_files = [p for p in SOURCE_ROOT.rglob("*.fbx") if p.is_file()]
    mesh_files.sort()

    base_mesh_sources: List[Path] = []
    for src in mesh_files:
        rel = src.relative_to(SOURCE_ROOT)
        category, context = category_and_context(rel)
        if category != "Meshes":
            continue
        stem = src.stem
        lod_match = LOD_RE.match(stem)
        if not lod_match:
            continue
        base_name = lod_match.group("base")
        lod_index = int(lod_match.group("lod"))
        if lod_index == 0:
            base_mesh_sources.append(src)

    base_mesh_sources.sort()
    for src in base_mesh_sources:
        rel = src.relative_to(SOURCE_ROOT)
        _, context = category_and_context(rel)
        stem = src.stem
        base_name = LOD_RE.match(stem).group("base") if LOD_RE.match(stem) else stem
        dest_path = asset_path_for("Meshes", context)
        ensure_directory(dest_path)
        delete_matching_assets(dest_path, sanitize_name(base_name))

        task = unreal.AssetImportTask()
        task.filename = str(src)
        task.destination_path = dest_path
        task.destination_name = sanitize_name(base_name)
        task.automated = True
        task.save = True
        task.replace_existing = True
        task.replace_existing_settings = True

        options = unreal.FbxImportUI()
        options.set_editor_property("automated_import_should_detect_type", False)
        options.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_SKELETAL_MESH)
        options.set_editor_property("import_as_skeletal", True)
        options.set_editor_property("import_mesh", True)
        options.set_editor_property("import_animations", False)
        options.set_editor_property("import_materials", False)
        options.set_editor_property("import_textures", False)
        options.set_editor_property("skeleton", None)

        skeletal_data = options.get_editor_property("skeletal_mesh_import_data")
        if skeletal_data:
            if hasattr(skeletal_data, "set_editor_property"):
                skeletal_data.set_editor_property("convert_scene", True)
                skeletal_data.set_editor_property("convert_scene_unit", True)
                skeletal_data.set_editor_property("force_front_x_axis", True)
                if hasattr(skeletal_data, "set_editor_property"):
                    try:
                        skeletal_data.set_editor_property("import_mesh_lods", False)
                    except Exception:
                        pass
                try:
                    skeletal_data.set_editor_property("material_search_location", unreal.MaterialSearchLocation.ALL_ASSETS)
                except Exception:
                    pass
                try:
                    # Import both geometry and skin weights so the skeletal mesh keeps valid deformation data.
                    skeletal_data.set_editor_property("import_content_type", unreal.FBXImportContentType.FBXICT_ALL)
                except Exception:
                    pass
        task.options = options
        import_tasks([task])

        mesh_asset_path = f"{dest_path}/{sanitize_name(base_name)}"
        mesh_asset_paths[mesh_asset_path] = mesh_asset_path
        stats.mesh_bases_imported += 1

        mesh_obj = unreal.EditorAssetLibrary.load_asset(mesh_asset_path)
        if not mesh_obj:
            warn(f"Imported mesh asset not found at {mesh_asset_path}")
            continue

        unreal.EditorAssetLibrary.save_loaded_asset(mesh_obj)

    return mesh_asset_paths


def import_animation_assets(stats: ImportStats, skeleton_by_species: Dict[str, unreal.Object]) -> None:
    anim_files = [p for p in SOURCE_ROOT.rglob("*.fbx") if p.is_file()]
    anim_files.sort()
    for src in anim_files:
        rel = src.relative_to(SOURCE_ROOT)
        category, context = category_and_context(rel)
        if category != "Animations":
            continue

        stem = src.stem
        anim_match = ANIM_RE.match(stem)
        if not anim_match:
            continue
        base_name = anim_match.group("base")
        species_key = sanitize_name(base_name)
        skeleton = skeleton_by_species.get(species_key)
        if not skeleton:
            warn(f"No skeleton found for animation source {src}")
            stats.errors += 1
            continue

        dest_path = asset_path_for("Animations", context)
        ensure_directory(dest_path)

        task = unreal.AssetImportTask()
        task.filename = str(src)
        task.destination_path = dest_path
        task.destination_name = sanitize_name(base_name)
        task.automated = True
        task.save = True
        task.replace_existing = True
        task.replace_existing_settings = True

        options = unreal.FbxImportUI()
        options.set_editor_property("automated_import_should_detect_type", False)
        options.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_ANIMATION)
        options.set_editor_property("import_as_skeletal", True)
        options.set_editor_property("import_mesh", False)
        options.set_editor_property("import_animations", True)
        options.set_editor_property("import_materials", False)
        options.set_editor_property("import_textures", False)
        options.set_editor_property("skeleton", skeleton)

        skeletal_data = options.get_editor_property("skeletal_mesh_import_data")
        if skeletal_data:
            try:
                skeletal_data.set_editor_property("convert_scene", True)
                skeletal_data.set_editor_property("convert_scene_unit", True)
                skeletal_data.set_editor_property("force_front_x_axis", True)
            except Exception:
                pass

        task.options = options
        try:
            import_tasks([task])
            stats.animation_assets_imported += 1
            log(f"Imported animation FBX for {base_name}")
        except Exception as exc:
            warn(f"Animation import failed for {base_name}: {exc}. Falling back to static mesh import.")
            fallback_task = unreal.AssetImportTask()
            fallback_task.filename = str(src)
            fallback_task.destination_path = dest_path
            fallback_task.destination_name = sanitize_name(base_name)
            fallback_task.automated = True
            fallback_task.save = True
            fallback_task.replace_existing = True
            fallback_task.replace_existing_settings = True

            fallback_options = unreal.FbxImportUI()
            fallback_options.set_editor_property("automated_import_should_detect_type", False)
            fallback_options.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_STATIC_MESH)
            fallback_options.set_editor_property("import_mesh", True)
            fallback_options.set_editor_property("import_as_skeletal", False)
            fallback_options.set_editor_property("import_animations", False)
            fallback_options.set_editor_property("import_materials", False)
            fallback_options.set_editor_property("import_textures", False)
            fallback_task.options = fallback_options
            import_tasks([fallback_task])
            stats.errors += 1
            warn(f"Static mesh fallback imported for {base_name}")


def build_skeleton_lookup(mesh_asset_paths: Dict[str, str]) -> Dict[str, unreal.Object]:
    lookup: Dict[str, unreal.Object] = {}
    for asset_path in mesh_asset_paths.values():
        mesh = unreal.EditorAssetLibrary.load_asset(asset_path)
        if not mesh:
            continue
        species_name = Path(asset_path).name
        skeleton = None
        try:
            skeleton = mesh.get_editor_property("skeleton")
        except Exception:
            skeleton = None
        if skeleton:
            lookup[species_name] = skeleton
    return lookup


def scan_skipped_files() -> List[str]:
    skipped: List[str] = []
    for path in SOURCE_ROOT.rglob("*"):
        if not path.is_file():
            continue
        suffix = path.suffix.lower()
        rel = path.relative_to(SOURCE_ROOT)
        category, _ = category_and_context(rel)
        if suffix in TEXTURE_EXTS:
            if category != "Textures":
                skipped.append(str(path))
            continue
        if suffix == ".fbx":
            if category not in {"Meshes", "Animations"}:
                skipped.append(str(path))
            continue
        if suffix in {".mat"}:
            if category != "Materials":
                skipped.append(str(path))
            continue
        if suffix in {".meta"}:
            continue
        skipped.append(str(path))
    return skipped


def main() -> None:
    REPORT_DIR.mkdir(parents=True, exist_ok=True)
    stats = ImportStats()
    report: Dict[str, object] = {}

    if not SOURCE_ROOT.exists():
        err(f"Source root does not exist: {SOURCE_ROOT}")
        return

    log(f"Source root: {SOURCE_ROOT}")
    log(f"Destination root: {DEST_ROOT}")

    guid_map = parse_meta_guid_map()
    material_specs = parse_material_specs(guid_map)
    texture_asset_paths = import_textures()
    stats.textures_imported = len(texture_asset_paths)

    material_asset_paths = import_materials(material_specs, texture_asset_paths)
    stats.materials_created = len(material_asset_paths)

    mesh_asset_paths = import_base_meshes_and_lods(stats)
    skeleton_lookup = build_skeleton_lookup(mesh_asset_paths)

    import_animation_assets(stats, skeleton_lookup)

    skipped = scan_skipped_files()
    stats.skipped_files = len(skipped)
    stats.unresolved_textures = sum(1 for spec in material_specs if spec.texture_guid and not spec.texture_source)
    stats.unresolved_material_textures = sum(1 for spec in material_specs if spec.texture_source and spec.texture_source.lower() not in texture_asset_paths)

    report = {
        "source_root": str(SOURCE_ROOT),
        "destination_root": DEST_ROOT,
        "stats": asdict(stats),
        "material_count": len(material_specs),
        "texture_count": len(texture_asset_paths),
        "material_assets": material_asset_paths,
        "skipped_files": skipped,
    }
    (REPORT_DIR / "quirky_series_migration_report.json").write_text(json.dumps(report, indent=2), encoding="utf-8")
    log(f"Report written to {REPORT_DIR / 'quirky_series_migration_report.json'}")
    log(f"Stats: {json.dumps(asdict(stats), indent=2)}")


if __name__ == "__main__":
    try:
        main()
    except Exception as exc:
        err(f"Migration failed: {exc}")
        raise
