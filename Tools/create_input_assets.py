import unreal


ROOT = "/Game/Input"
IMC_PATH = ROOT + "/IMC_Player"


def get_or_create_asset(name, path, asset_class):
    full_path = path + "/" + name
    asset = unreal.load_asset(full_path)
    if asset:
        return asset

    factory = unreal.DataAssetFactory()
    factory.set_editor_property("data_asset_class", asset_class)
    asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        name, path, asset_class, factory
    )
    if not asset:
        raise RuntimeError("Could not create " + full_path)
    return asset


def create_action(name, value_type):
    action = get_or_create_asset(name, ROOT, unreal.InputAction)
    action.set_editor_property("value_type", value_type)
    unreal.EditorAssetLibrary.save_loaded_asset(action, only_if_is_dirty=False)
    return action


def key(name):
    input_key = unreal.Key()
    input_key.set_editor_property("key_name", name)
    return input_key


def map_key(context, action, key_name, negate=False):
    mapping = context.map_key(action, key(key_name))
    modifiers = []
    if negate:
        modifiers.append(unreal.InputModifierNegate())
    if key_name in ("D", "A"):
        swizzle = unreal.InputModifierSwizzleAxis()
        swizzle.set_editor_property("order", unreal.InputAxisSwizzle.YXZ)
        modifiers.append(swizzle)
    if key_name == "MouseY":
        swizzle = unreal.InputModifierSwizzleAxis()
        swizzle.set_editor_property("order", unreal.InputAxisSwizzle.YXZ)
        modifiers.append(swizzle)
    if modifiers:
        mapping.modifiers = modifiers


try:
    axis1d = unreal.InputActionValueType.AXIS1D
    axis2d = unreal.InputActionValueType.AXIS2D
    boolean = unreal.InputActionValueType.BOOLEAN

    actions = {
        "IA_Move": create_action("IA_Move", axis2d),
        "IA_Look": create_action("IA_Look", axis2d),
        "IA_MoveUp": create_action("IA_MoveUp", axis1d),
        "IA_Turn": create_action("IA_Turn", axis1d),
        "IA_LookUp": create_action("IA_LookUp", axis1d),
        "IA_SoulAscend": create_action("IA_SoulAscend", axis1d),
        "IA_SoulDescend": create_action("IA_SoulDescend", axis1d),
        "IA_Jump": create_action("IA_Jump", boolean),
        "IA_PossessTarget": create_action("IA_PossessTarget", boolean),
        "IA_ReleasePossession": create_action("IA_ReleasePossession", boolean),
        "IA_Interact": create_action("IA_Interact", boolean),
        "IA_AnimalAbility": create_action("IA_AnimalAbility", boolean),
        "IA_Pause": create_action("IA_Pause", boolean),
        "IA_RestartCheckpoint": create_action("IA_RestartCheckpoint", boolean),
    }

    context = get_or_create_asset("IMC_Player", ROOT, unreal.InputMappingContext)
    context.unmap_all()

    for action_name, key_name, negate in [
        ("IA_Move", "W", False),
        ("IA_Move", "S", True),
        ("IA_Move", "D", False),
        ("IA_Move", "A", True),
        ("IA_Move", "Gamepad_LeftX", False),
        ("IA_Move", "Gamepad_LeftY", False),
        ("IA_Look", "MouseX", False),
        ("IA_Look", "MouseY", True),
        ("IA_Look", "Gamepad_RightX", False),
        ("IA_Look", "Gamepad_RightY", False),
        ("IA_Jump", "SpaceBar", False),
        ("IA_Jump", "Gamepad_FaceButton_Bottom", False),
        ("IA_SoulAscend", "SpaceBar", False),
        ("IA_SoulAscend", "Gamepad_FaceButton_Bottom", False),
        ("IA_SoulDescend", "LeftControl", False),
        ("IA_SoulDescend", "Gamepad_LeftTriggerAxis", False),
        ("IA_PossessTarget", "E", False),
        ("IA_PossessTarget", "Gamepad_FaceButton_Right", False),
        ("IA_ReleasePossession", "Q", False),
        ("IA_ReleasePossession", "Tab", False),
        ("IA_ReleasePossession", "Gamepad_FaceButton_Left", False),
        ("IA_Interact", "F", False),
        ("IA_Interact", "Gamepad_FaceButton_Left", False),
        ("IA_AnimalAbility", "LeftShift", False),
        ("IA_AnimalAbility", "Gamepad_RightShoulder", False),
        ("IA_Pause", "Escape", False),
        ("IA_Pause", "Gamepad_Special_Right", False),
        ("IA_RestartCheckpoint", "R", False),
    ]:
        map_key(context, actions[action_name], key_name, negate)
    unreal.EditorAssetLibrary.save_loaded_asset(context, only_if_is_dirty=False)
    for old_action in ("IA_MoveForward", "IA_MoveRight", "IA_Turn", "IA_LookUp"):
        old_path = ROOT + "/" + old_action
        if unreal.EditorAssetLibrary.does_asset_exist(old_path):
            unreal.EditorAssetLibrary.delete_asset(old_path)
    unreal.log("Created Enhanced Input assets under " + ROOT)
except Exception as error:
    unreal.log_error("Input asset generation failed: {}".format(error))
    raise
finally:
    unreal.SystemLibrary.quit_editor()
