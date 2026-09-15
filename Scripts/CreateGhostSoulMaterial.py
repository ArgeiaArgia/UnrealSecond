import unreal


ASSET_PATH = "/Game/_Art/QuirkyMinimal/Ghost/Materials"
ASSET_NAME = "M_GhostSoul"


def create_or_load_material():
    unreal.EditorAssetLibrary.make_directory(ASSET_PATH)

    asset = unreal.EditorAssetLibrary.load_asset(f"{ASSET_PATH}/{ASSET_NAME}")
    if asset and isinstance(asset, unreal.Material):
        material = asset
        unreal.MaterialEditingLibrary.delete_all_material_expressions(material)
    else:
        factory = unreal.MaterialFactoryNew()
        material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            ASSET_NAME,
            ASSET_PATH,
            unreal.Material,
            factory,
        )
        if not material:
            raise RuntimeError("Failed to create material asset.")

    # Masked shading writes depth, preventing the back side of the mesh from
    # blending through the front side like translucent shading does.
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
    material.set_editor_property("two_sided", True)
    material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    material.set_editor_property("screen_space_reflections", False)
    material.set_editor_property("allow_front_layer_translucency", True)
    material.set_editor_property("opacity_mask_clip_value", 0.1)

    return material


def create_scalar(material, name, value, x, y, group="Ghost"):
    node = unreal.MaterialEditingLibrary.create_material_expression(
        material,
        unreal.MaterialExpressionScalarParameter,
        x,
        y,
    )
    node.set_editor_property("parameter_name", name)
    node.set_editor_property("default_value", float(value))
    node.set_editor_property("group", group)
    return node


def create_vector(material, name, value, x, y, group="Ghost"):
    node = unreal.MaterialEditingLibrary.create_material_expression(
        material,
        unreal.MaterialExpressionVectorParameter,
        x,
        y,
    )
    node.set_editor_property("parameter_name", name)
    node.set_editor_property("default_value", value)
    node.set_editor_property("group", group)
    return node


def create_texture(material, name, x, y, group="Ghost"):
    node = unreal.MaterialEditingLibrary.create_material_expression(
        material,
        unreal.MaterialExpressionTextureSampleParameter2D,
        x,
        y,
    )
    node.set_editor_property("parameter_name", name)
    node.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
    node.set_editor_property("group", group)
    return node


def resolve_output_name(expression, requested_name):
    if requested_name:
        return requested_name

    try:
        outputs = expression.get_editor_property("outputs")
    except Exception:
        outputs = []

    for output in outputs:
        try:
            output_name = output.get_editor_property("output_name")
        except Exception:
            output_name = ""

        if output_name:
            return str(output_name)

    return ""


def resolve_input_name(expression, requested_name):
    if requested_name:
        return requested_name

    try:
        inputs = expression.get_editor_property("inputs")
    except Exception:
        inputs = []

    for input_pin in inputs:
        try:
            input_name = input_pin.get_editor_property("input_name")
        except Exception:
            input_name = ""

        if input_name:
            return str(input_name)

    return ""


def connect(material, source, output_name, target, input_name):
    resolved_output = resolve_output_name(source, output_name)
    resolved_input = resolve_input_name(target, input_name)
    if not unreal.MaterialEditingLibrary.connect_material_expressions(source, resolved_output, target, resolved_input):
        raise RuntimeError(f"Failed to connect {source.get_name()} -> {target.get_name()}")


def connect_property(expression, output_name, property_name):
    resolved_output = resolve_output_name(expression, output_name)
    if not unreal.MaterialEditingLibrary.connect_material_property(expression, resolved_output, property_name):
        raise RuntimeError(f"Failed to connect expression to {property_name}")


def build_graph(material):
    base_tex = create_texture(material, "BaseTexture", -800, -200)
    ghost_tint = create_vector(material, "GhostTint", unreal.LinearColor(0.55, 0.92, 1.0, 1.0), -800, 100)
    glow_tint = create_vector(material, "GlowTint", unreal.LinearColor(0.70, 1.0, 1.0, 1.0), -800, 320)

    base_intensity = create_scalar(material, "BaseIntensity", 1.0, -540, -40)
    glow_intensity = create_scalar(material, "GlowIntensity", 1.25, -540, 260)
    ghost_opacity = create_scalar(material, "GhostOpacity", 0.68, -540, 560)

    pulse_base = create_scalar(material, "PulseBase", 1.0, -540, 760)
    pulse_speed = create_scalar(material, "PulseSpeed", 1.6, -800, 760)
    pulse_strength = create_scalar(material, "PulseStrength", 0.08, -540, 960)

    time_node = unreal.MaterialEditingLibrary.create_material_expression(
        material,
        unreal.MaterialExpressionTime,
        -1100,
        760,
    )
    sine_node = unreal.MaterialEditingLibrary.create_material_expression(
        material,
        unreal.MaterialExpressionSine,
        -300,
        760,
    )
    time_speed_mul = unreal.MaterialEditingLibrary.create_material_expression(
        material,
        unreal.MaterialExpressionMultiply,
        -900,
        760,
    )
    pulse_mul = unreal.MaterialEditingLibrary.create_material_expression(
        material,
        unreal.MaterialExpressionMultiply,
        -100,
        900,
    )
    pulse_add = unreal.MaterialEditingLibrary.create_material_expression(
        material,
        unreal.MaterialExpressionAdd,
        120,
        840,
    )

    fresnel = unreal.MaterialEditingLibrary.create_material_expression(
        material,
        unreal.MaterialExpressionFresnel,
        -520,
        340,
    )
    base_tint_mul = unreal.MaterialEditingLibrary.create_material_expression(
        material,
        unreal.MaterialExpressionMultiply,
        -280,
        -140,
    )
    base_intensity_mul = unreal.MaterialEditingLibrary.create_material_expression(
        material,
        unreal.MaterialExpressionMultiply,
        -60,
        -140,
    )
    glow_mul = unreal.MaterialEditingLibrary.create_material_expression(
        material,
        unreal.MaterialExpressionMultiply,
        -280,
        280,
    )
    glow_intensity_mul = unreal.MaterialEditingLibrary.create_material_expression(
        material,
        unreal.MaterialExpressionMultiply,
        -60,
        280,
    )
    emissive_add = unreal.MaterialEditingLibrary.create_material_expression(
        material,
        unreal.MaterialExpressionAdd,
        160,
        40,
    )
    emissive_final = unreal.MaterialEditingLibrary.create_material_expression(
        material,
        unreal.MaterialExpressionMultiply,
        380,
        40,
    )

    connect(material, time_node, None, time_speed_mul, "A")
    connect(material, pulse_speed, None, time_speed_mul, "B")
    # Sine exposes a single input whose Python pin name varies by UE version.
    connect(material, time_speed_mul, None, sine_node, None)
    connect(material, sine_node, None, pulse_mul, "A")
    connect(material, pulse_strength, None, pulse_mul, "B")
    connect(material, pulse_mul, None, pulse_add, "A")
    connect(material, pulse_base, None, pulse_add, "B")

    connect(material, base_tex, "RGB", base_tint_mul, "A")
    connect(material, ghost_tint, None, base_tint_mul, "B")
    connect(material, base_tint_mul, None, base_intensity_mul, "A")
    connect(material, base_intensity, None, base_intensity_mul, "B")

    connect(material, fresnel, None, glow_mul, "A")
    connect(material, glow_tint, None, glow_mul, "B")
    connect(material, glow_mul, None, glow_intensity_mul, "A")
    connect(material, glow_intensity, None, glow_intensity_mul, "B")

    connect(material, base_intensity_mul, None, emissive_add, "A")
    connect(material, glow_intensity_mul, None, emissive_add, "B")
    connect(material, emissive_add, None, emissive_final, "A")
    connect(material, pulse_add, None, emissive_final, "B")

    connect_property(emissive_final, None, unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    connect_property(base_tex, "A", unreal.MaterialProperty.MP_OPACITY_MASK)

    unreal.MaterialEditingLibrary.layout_material_expressions(material)
    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material)


def main():
    material = create_or_load_material()
    build_graph(material)
    print(f"Created ghost material: {material.get_path_name()}")


if __name__ == "__main__":
    main()
