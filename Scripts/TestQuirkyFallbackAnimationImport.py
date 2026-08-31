from __future__ import annotations

from pathlib import Path

import unreal


SOURCE = Path(
    r"C:\Users\KGH\Gitfork\UnrealSecond\UnrealTeamProject\Content\Imported\Quirky Series Ultimate\FREE Pack\Animations\Sparrow_Animations.fbx"
)
DEST_PATH = "/Game/Migrated/QuirkySeriesUltimate/_TempFallbackTest/Sparrow"


def main() -> None:
    if unreal.EditorAssetLibrary.does_directory_exist(DEST_PATH):
        unreal.EditorAssetLibrary.delete_directory(DEST_PATH)
    unreal.EditorAssetLibrary.make_directory(DEST_PATH)

    task = unreal.AssetImportTask()
    task.filename = str(SOURCE)
    task.destination_path = DEST_PATH
    task.destination_name = "Sparrow"
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
    options.set_editor_property("skeleton", None)

    task.options = options
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    assets = unreal.EditorAssetLibrary.list_assets(DEST_PATH, recursive=True, include_folder=False)
    for asset_path in assets:
        asset = unreal.EditorAssetLibrary.load_asset(asset_path)
        if asset:
            unreal.log(f"[QuirkyTest] {asset_path} => {asset.get_class().get_name()}")


if __name__ == "__main__":
    main()
