import unreal


BLUEPRINT_PATH = "/Game/Blueprints/Characters"
PUZZLE_PATH = "/Game/Blueprints/Puzzles"


def ensure_directory(path):
    if not unreal.EditorAssetLibrary.does_directory_exist(path):
        unreal.EditorAssetLibrary.make_directory(path)


def create_blueprint(asset_path, parent_class):
    existing = unreal.EditorAssetLibrary.load_asset(asset_path)
    if existing:
        return existing

    package_path, asset_name = asset_path.rsplit("/", 1)
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", parent_class)
    blueprint = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        asset_name,
        package_path,
        unreal.Blueprint,
        factory,
    )
    if not blueprint:
        raise RuntimeError(f"Failed to create Blueprint: {asset_path}")
    return blueprint


def set_default(blueprint, property_name, value):
    generated_class = blueprint.generated_class()
    cdo = unreal.get_default_object(generated_class)
    cdo.set_editor_property(property_name, value)


def main():
    ensure_directory(BLUEPRINT_PATH)
    ensure_directory(PUZZLE_PATH)

    turtle_class = unreal.load_class(None, "/Script/UnrealTeamProject.UTPTurtleCharacter")
    plate_class = unreal.load_class(None, "/Script/UnrealTeamProject.UTPPressurePlate")
    if not turtle_class or not plate_class:
        raise RuntimeError("The UnrealTeamProject module must be compiled before creating turtle assets.")

    turtle_blueprint = create_blueprint(
        f"{BLUEPRINT_PATH}/BP_SnappingTurtle",
        turtle_class,
    )
    set_default(turtle_blueprint, "bAllowSoulPossession", True)
    set_default(turtle_blueprint, "bDisableMovementWhenUnpossessed", True)
    set_default(turtle_blueprint, "BraceMass", 100.0)
    unreal.EditorAssetLibrary.save_loaded_asset(turtle_blueprint)

    plate_blueprint = create_blueprint(
        f"{PUZZLE_PATH}/BP_PressurePlate",
        plate_class,
    )
    set_default(plate_blueprint, "RequiredWeight", 80.0)
    unreal.EditorAssetLibrary.save_loaded_asset(plate_blueprint)

    print("Created turtle prototype assets:")
    print(turtle_blueprint.get_path_name())
    print(plate_blueprint.get_path_name())


if __name__ == "__main__":
    main()
