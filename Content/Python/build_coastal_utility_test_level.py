import math
import unreal


LEVEL_PATH = "/Game/Variant_Strategy/LVL_CoastalUtilityRefined"
CELL_SIZE = 100.0
GRID_W = 52
GRID_H = 52
ORIGIN_X = -2600.0
ORIGIN_Y = -2600.0


def load_asset(path):
    asset = unreal.EditorAssetLibrary.load_asset(path)
    if not asset:
        unreal.log_warning(f"Missing asset: {path}")
    return asset


def ensure_material(name, color, roughness=0.85):
    package_path = "/Game/Variant_Strategy/Generated"
    asset_path = f"{package_path}/{name}"
    if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        existing = unreal.EditorAssetLibrary.load_asset(asset_path)
        if existing:
            return existing

    unreal.EditorAssetLibrary.make_directory(package_path)
    material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        name, package_path, unreal.Material, unreal.MaterialFactoryNew()
    )

    base = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionVectorParameter, -420, 0
    )
    base.set_editor_property("parameter_name", "BaseColor")
    base.set_editor_property("default_value", color)
    unreal.MaterialEditingLibrary.connect_material_property(
        base, "", unreal.MaterialProperty.MP_BASE_COLOR
    )

    rough = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionScalarParameter, -420, 180
    )
    rough.set_editor_property("parameter_name", "Roughness")
    rough.set_editor_property("default_value", roughness)
    unreal.MaterialEditingLibrary.connect_material_property(
        rough, "", unreal.MaterialProperty.MP_ROUGHNESS
    )

    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material)
    return material


def ensure_texture_material(name, texture_path, tint, roughness=0.82, tiling=6.0):
    package_path = "/Game/Variant_Strategy/Generated"
    asset_path = f"{package_path}/{name}"
    if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        existing = unreal.EditorAssetLibrary.load_asset(asset_path)
        if existing:
            return existing

    texture = load_asset(texture_path)
    if not texture:
        return ensure_material(name, tint, roughness)

    unreal.EditorAssetLibrary.make_directory(package_path)
    material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        name, package_path, unreal.Material, unreal.MaterialFactoryNew()
    )

    tex = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionTextureSampleParameter2D, -620, -80
    )
    tex.set_editor_property("parameter_name", "BaseTexture")
    tex.set_editor_property("texture", texture)

    try:
        coord = unreal.MaterialEditingLibrary.create_material_expression(
            material, unreal.MaterialExpressionTextureCoordinate, -860, -80
        )
        coord.set_editor_property("u_tiling", tiling)
        coord.set_editor_property("v_tiling", tiling)
        unreal.MaterialEditingLibrary.connect_material_expressions(coord, "", tex, "Coordinates")
    except Exception as exc:
        unreal.log_warning(f"{name}: texture coordinate tiling skipped: {exc}")

    tint_expr = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionVectorParameter, -620, 120
    )
    tint_expr.set_editor_property("parameter_name", "Tint")
    tint_expr.set_editor_property("default_value", tint)

    multiply = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionMultiply, -330, -20
    )
    unreal.MaterialEditingLibrary.connect_material_expressions(tex, "RGB", multiply, "A")
    unreal.MaterialEditingLibrary.connect_material_expressions(tint_expr, "", multiply, "B")
    unreal.MaterialEditingLibrary.connect_material_property(
        multiply, "", unreal.MaterialProperty.MP_BASE_COLOR
    )

    rough = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionScalarParameter, -330, 170
    )
    rough.set_editor_property("parameter_name", "Roughness")
    rough.set_editor_property("default_value", roughness)
    unreal.MaterialEditingLibrary.connect_material_property(
        rough, "", unreal.MaterialProperty.MP_ROUGHNESS
    )

    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material)
    return material


MATS = {}
MESHES = {}


def mat(name):
    return MATS[name]


def world_from_cell(cx, cy, z=0.0):
    return (
        ORIGIN_X + cx * CELL_SIZE + CELL_SIZE * 0.5,
        ORIGIN_Y + cy * CELL_SIZE + CELL_SIZE * 0.5,
        z,
    )


def spawn_actor(actor_class, name, loc, rot=(0, 0, 0), scale=(1, 1, 1)):
    actor = unreal.EditorLevelLibrary.spawn_actor_from_class(
        actor_class, unreal.Vector(*loc), unreal.Rotator(*rot)
    )
    actor.set_actor_label(name)
    actor.set_actor_scale3d(unreal.Vector(*scale))
    return actor


def safe_set(actor, property_name, value):
    try:
        actor.set_editor_property(property_name, value)
    except Exception as exc:
        label = actor.get_actor_label() if hasattr(actor, "get_actor_label") else str(actor)
        unreal.log_warning(f"{label}: could not set {property_name}: {exc}")


def spawn_mesh(mesh, name, loc, rot=(0, 0, 0), scale=(1, 1, 1), material=None, collision=True):
    if not mesh:
        return None
    if not isinstance(mesh, unreal.StaticMesh):
        unreal.log_warning(f"{name}: skipped non-static mesh asset {mesh}")
        return None
    actor = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.StaticMeshActor, unreal.Vector(*loc), unreal.Rotator(*rot)
    )
    actor.set_actor_label(name)
    actor.set_actor_scale3d(unreal.Vector(*scale))
    comp = actor.get_component_by_class(unreal.StaticMeshComponent)
    try:
        comp.set_static_mesh(mesh)
    except Exception as exc:
        unreal.log_warning(f"{name}: could not use mesh {mesh}: {exc}")
        unreal.EditorLevelLibrary.destroy_actor(actor)
        return None
    if material:
        comp.set_material(0, material)
    comp.set_collision_enabled(
        unreal.CollisionEnabled.QUERY_AND_PHYSICS if collision else unreal.CollisionEnabled.NO_COLLISION
    )
    return actor


def spawn_skeletal_mesh(mesh, name, loc, rot=(0, 0, 0), scale=(1, 1, 1), collision=True):
    if not mesh or not isinstance(mesh, unreal.SkeletalMesh):
        return None
    actor = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.SkeletalMeshActor, unreal.Vector(*loc), unreal.Rotator(*rot)
    )
    actor.set_actor_label(name)
    actor.set_actor_scale3d(unreal.Vector(*scale))
    comp = actor.get_component_by_class(unreal.SkeletalMeshComponent)
    try:
        comp.set_skeletal_mesh_asset(mesh)
    except Exception:
        try:
            comp.set_skeletal_mesh(mesh)
        except Exception as exc:
            unreal.log_warning(f"{name}: could not use skeletal mesh {mesh}: {exc}")
            unreal.EditorLevelLibrary.destroy_actor(actor)
            return None
    comp.set_collision_enabled(
        unreal.CollisionEnabled.QUERY_AND_PHYSICS if collision else unreal.CollisionEnabled.NO_COLLISION
    )
    return actor


def mesh_bounds_size(mesh):
    try:
        box = mesh.get_bounding_box()
        return box.max - box.min, (box.max + box.min) * 0.5, box.min
    except Exception:
        return None, None, None


def spawn_fit_mesh(mesh, name, center_xy, footprint_cells, height, yaw=0, z_base=0, material=None, collision=True):
    if not mesh:
        return None
    is_static = isinstance(mesh, unreal.StaticMesh)
    is_skeletal = isinstance(mesh, unreal.SkeletalMesh)
    if not is_static and not is_skeletal:
        unreal.log_warning(f"{name}: skipped non-mesh asset {mesh}")
        return None
    desired_x = max(footprint_cells[0] * CELL_SIZE, 1.0)
    desired_y = max(footprint_cells[1] * CELL_SIZE, 1.0)
    size, bounds_center, bounds_min = mesh_bounds_size(mesh)
    if size and size.x > 1.0 and size.y > 1.0 and size.z > 1.0:
        scale_x = desired_x / size.x
        scale_y = desired_y / size.y
        uniform_xy = min(scale_x, scale_y)
        scale_z = height / size.z if height else uniform_xy
        scale = (uniform_xy, uniform_xy, scale_z)
        loc = (
            center_xy[0] - bounds_center.x * scale[0],
            center_xy[1] - bounds_center.y * scale[1],
            z_base - bounds_min.z * scale[2],
        )
    else:
        scale = (1, 1, 1)
        loc = (center_xy[0], center_xy[1], z_base)
    if is_skeletal:
        return spawn_skeletal_mesh(mesh, name, loc, (0, yaw, 0), scale, collision)
    return spawn_mesh(mesh, name, loc, (0, yaw, 0), scale, material, collision)


def cube(name, loc, scale, material, rot=(0, 0, 0), collision=True):
    return spawn_mesh(MESHES["cube"], name, loc, rot, scale, material, collision)


def cyl(name, loc, scale, material, rot=(0, 0, 0), collision=True):
    return spawn_mesh(MESHES["cylinder"], name, loc, rot, scale, material, collision)


def floor_rect(name, x0, y0, w, h, z, material_name, collision=True):
    x = ORIGIN_X + (x0 + w * 0.5) * CELL_SIZE
    y = ORIGIN_Y + (y0 + h * 0.5) * CELL_SIZE
    cube(name, (x, y, z - 5), (w, h, 0.10), mat(material_name), collision=collision)


def grid_rect(prefix, x0, y0, w, h, z=4):
    # Kept subtle: useful for tactics readability, but not a decorative object field.
    for ix in range(w + 1):
        x = ORIGIN_X + (x0 + ix) * CELL_SIZE
        y = ORIGIN_Y + (y0 + h * 0.5) * CELL_SIZE
        cube(f"{prefix}_grid_v_{ix}", (x, y, z), (0.010, h, 0.010), mat("grid_line"), collision=False)
    for iy in range(h + 1):
        x = ORIGIN_X + (x0 + w * 0.5) * CELL_SIZE
        y = ORIGIN_Y + (y0 + iy) * CELL_SIZE
        cube(f"{prefix}_grid_h_{iy}", (x, y, z), (w, 0.010, 0.010), mat("grid_line"), collision=False)


def rect_center(x0, y0, w, h):
    return (
        ORIGIN_X + (x0 + w * 0.5) * CELL_SIZE,
        ORIGIN_Y + (y0 + h * 0.5) * CELL_SIZE,
    )


def low_cover(name, x0, y0, w=1, h=1, z=0, material_name="concrete"):
    x, y = rect_center(x0, y0, w, h)
    cube(name, (x, y, z + 42), (w * 0.88, h * 0.88, 0.84), mat(material_name))


def full_cover(name, x0, y0, w=1, h=1, z=0, material_name="concrete"):
    x, y = rect_center(x0, y0, w, h)
    cube(name, (x, y, z + 115), (w * 0.92, h * 0.92, 2.30), mat(material_name))


def crate_stack(name, x0, y0, z=0, rotation=0):
    x, y = rect_center(x0, y0, 1, 1)
    cube(f"{name}_crate_low", (x - 18, y - 12, z + 34), (0.58, 0.52, 0.68), mat("wood"), (0, rotation, 0))
    cube(f"{name}_crate_high", (x + 20, y + 16, z + 74), (0.48, 0.46, 0.56), mat("wood"), (0, rotation + 90, 0))


def concrete_barrier(name, x0, y0, w=2, h=1, z=0, yaw=0):
    x, y = rect_center(x0, y0, w, h)
    cube(name, (x, y, z + 46), (w * 0.86, h * 0.28, 0.92), mat("concrete"), (0, yaw, 0))


def shipping_container(name, x0, y0, yaw=0):
    x, y = rect_center(x0, y0, 4, 2)
    frame = MESHES.get("container_frame")
    side_a = MESHES.get("container_side_a")
    side_b = MESHES.get("container_side_b")
    door_a = MESHES.get("container_door_a")
    door_b = MESHES.get("container_door_b")
    if frame:
        spawn_fit_mesh(frame, f"{name}_asset_frame", (x, y), (4.2, 1.75), 230, yaw=yaw, z_base=0)
    else:
        cube(f"{name}_fallback_body", (x, y, 110), (4.1, 1.65, 2.20), mat("metal"), (0, yaw, 0))
    if side_a:
        spawn_fit_mesh(side_a, f"{name}_asset_left_side", (x, y - 38), (4.25, 0.35), 220, yaw=yaw, z_base=6, collision=False)
    if side_b:
        spawn_fit_mesh(side_b, f"{name}_asset_right_side", (x, y + 38), (4.25, 0.35), 220, yaw=yaw, z_base=6, collision=False)
    if door_a or door_b:
        end_x = x + math.cos(math.radians(yaw)) * 205
        end_y = y + math.sin(math.radians(yaw)) * 205
        if door_a:
            spawn_fit_mesh(door_a, f"{name}_asset_left_door", (end_x, end_y - 22), (0.9, 0.30), 210, yaw=yaw, z_base=8, collision=False)
        if door_b:
            spawn_fit_mesh(door_b, f"{name}_asset_right_door", (end_x, end_y + 22), (0.9, 0.30), 210, yaw=yaw, z_base=8, collision=False)


def vent(name, x0, y0, z):
    x, y = rect_center(x0, y0, 1, 1)
    cube(f"{name}_base", (x, y, z + 22), (0.65, 0.42, 0.44), mat("metal"))
    cyl(f"{name}_cap", (x + 18, y, z + 58), (0.16, 0.16, 0.36), mat("metal"), collision=False)


def roof_walls(prefix, x0, y0, w, h, z):
    concrete_barrier(f"{prefix}_north_low_wall", x0, y0 + h - 1, w, 1, z)
    concrete_barrier(f"{prefix}_south_low_wall", x0, y0, w, 1, z)
    concrete_barrier(f"{prefix}_west_low_wall", x0, y0 + 1, h - 2, 1, z, 90)
    concrete_barrier(f"{prefix}_east_low_wall", x0 + w - 1, y0 + 1, h - 2, 1, z, 90)


def add_roof_detail(prefix, x0, y0, w, h, z):
    roof_walls(prefix, x0, y0, w, h, z)
    vent(f"{prefix}_air_vent_a", x0 + 1, y0 + h - 2, z)
    if w > 5:
        vent(f"{prefix}_air_vent_b", x0 + w - 2, y0 + 1, z)
    crate_stack(f"{prefix}_roof_utility_boxes", x0 + max(1, w - 3), y0 + 2, z, 12)


def add_ladder(name, cx, cy, z_top, yaw=0):
    x, y, _ = world_from_cell(cx, cy, 0)
    ladder = MESHES.get("ladder")
    if ladder:
        spawn_fit_mesh(ladder, f"{name}_visual_ladder", (x, y), (0.65, 2.1), z_top, yaw=yaw, z_base=0, collision=False)
    else:
        cube(f"{name}_visual_ladder_fallback", (x, y, z_top * 0.5), (0.18, 0.10, z_top / 100.0), mat("wood"), (0, yaw, 0), False)
    ramp_len = max(3.0, z_top / 100.0)
    pitch = -math.degrees(math.atan2(z_top, ramp_len * CELL_SIZE))
    cube(
        f"{name}_nav_ramp",
        (x, y + 105, z_top * 0.5),
        (0.85, ramp_len, 0.12),
        mat("ramp"),
        (pitch, yaw, 0),
    )


def add_door_window_set(prefix, x0, y0, w, h, z_base=0):
    door = MESHES.get("door_alt") if (len(prefix) % 2 == 0 and MESHES.get("door_alt")) else MESHES.get("door")
    win_a = MESHES.get("window_1")
    win_b = MESHES.get("window_2")
    front_y = ORIGIN_Y + y0 * CELL_SIZE - 8
    center_x = ORIGIN_X + (x0 + w * 0.5) * CELL_SIZE
    if door:
        spawn_fit_mesh(door, f"{prefix}_metal_door", (center_x, front_y), (0.9, 0.22), 170, yaw=0, z_base=z_base, material=mat("metal"), collision=False)
    else:
        cube(f"{prefix}_metal_door", (center_x, front_y, z_base + 85), (0.8, 0.08, 1.7), mat("metal"), collision=False)
    for i, wx in enumerate([x0 + 1.2, x0 + w - 1.2]):
        x = ORIGIN_X + wx * CELL_SIZE
        mesh = win_a if i == 0 else win_b
        if mesh:
            spawn_fit_mesh(mesh, f"{prefix}_glass_window_{i}", (x, front_y), (0.85, 0.18), 110, yaw=0, z_base=z_base + 95, material=None, collision=False)
        else:
            cube(f"{prefix}_glass_window_{i}", (x, front_y, z_base + 155), (0.75, 0.06, 0.8), mat("glass"), collision=False)


def wall_segment(name, x, y, z, sx, sy, sz):
    cube(name, (x, y, z), (sx, sy, sz), mat("brick"))


def blockout_building(name, x0, y0, w, h, height_cells, roof_z, dominant=False):
    x, y = rect_center(x0, y0, w, h)
    wall_z = height_cells * 50
    t = 0.26
    front_y = ORIGIN_Y + y0 * CELL_SIZE + t * CELL_SIZE * 0.5
    back_y = ORIGIN_Y + (y0 + h) * CELL_SIZE - t * CELL_SIZE * 0.5
    left_x = ORIGIN_X + x0 * CELL_SIZE + t * CELL_SIZE * 0.5
    right_x = ORIGIN_X + (x0 + w) * CELL_SIZE - t * CELL_SIZE * 0.5
    door_gap = 1.35
    side_w = max((w - door_gap) * 0.5, 0.7)
    wall_segment(f"{name}_front_wall_left", ORIGIN_X + (x0 + side_w * 0.5) * CELL_SIZE, front_y, wall_z, side_w, t, height_cells)
    wall_segment(f"{name}_front_wall_right", ORIGIN_X + (x0 + w - side_w * 0.5) * CELL_SIZE, front_y, wall_z, side_w, t, height_cells)
    wall_segment(f"{name}_back_wall", x, back_y, wall_z, w, t, height_cells)
    wall_segment(f"{name}_left_wall", left_x, y, wall_z, t, h, height_cells)
    wall_segment(f"{name}_right_wall", right_x, y, wall_z, t, h, height_cells)
    floor_rect(f"{name}_walkable_roof", x0, y0, w, h, roof_z, "roof")
    grid_rect(f"{name}_roof_grid", x0, y0, w, h, roof_z + 8)
    add_roof_detail(name, x0, y0, w, h, roof_z)
    add_door_window_set(name, x0, y0, w, h)
    concrete_barrier(f"{name}_entry_step", x0 + int(w * 0.5), y0 - 1, 1, 1, 0)
    if dominant:
        concrete_barrier(f"{name}_dominant_overlook_cover", x0 + 2, y0 + h - 2, 2, 1, roof_z)


def utility_building(name, mesh_key, x0, y0, w, h, roof_z, yaw=0):
    x, y = rect_center(x0, y0, w, h)
    mesh = MESHES.get(mesh_key)
    if mesh:
        spawn_fit_mesh(mesh, f"{name}_asset_{mesh_key}", (x, y), (w, h), roof_z - 5, yaw=yaw, z_base=0)
    else:
        cube(f"{name}_brick_shell", (x, y, roof_z * 0.5), (w, h, roof_z / 100.0), mat("brick"))
    floor_rect(f"{name}_walkable_roof", x0, y0, w, h, roof_z, "roof")
    grid_rect(f"{name}_roof_grid", x0, y0, w, h, roof_z + 8)
    add_roof_detail(name, x0, y0, w, h, roof_z)
    add_door_window_set(name, x0, y0, w, h)


def concrete_fence(name, x0, y0, length, horizontal=True):
    mesh = MESHES.get("fence")
    for i in range(length):
        cx = x0 + i if horizontal else x0
        cy = y0 if horizontal else y0 + i
        x, y, _ = world_from_cell(cx, cy, 0)
        if mesh and isinstance(mesh, unreal.StaticMesh):
            yaw = 90 if horizontal else 0
            spawned = spawn_fit_mesh(mesh, f"{name}_{i}", (x, y), (1.0, 0.35), 130, yaw=yaw, z_base=0, material=None)
            if spawned:
                continue
        cube(f"{name}_{i}", (x, y, 65), (1.0 if horizontal else 0.28, 0.28 if horizontal else 1.0, 1.30), mat("concrete"))


def add_ground():
    floor_rect("Ground_Base_35x35_dirt_utility_lot", 0, 0, 35, 35, 0, "dirt")
    floor_rect("MainRoad_T_Asphalt_north_south", 13, 0, 8, 35, 2, "asphalt")
    floor_rect("RoadBend_T_Asphalt_east", 20, 22, 13, 6, 3, "asphalt")
    floor_rect("OpenCourtyard_T_Concrete_center", 7, 10, 17, 13, 4, "concrete")
    floor_rect("DirtArea_T_Dirt_service_yard", 23, 4, 10, 11, 5, "dirt")
    floor_rect("BackYard_T_Concrete_northwest", 3, 22, 10, 8, 5, "concrete")
    floor_rect("PlayerApproach_T_Concrete_southwest", 2, 2, 9, 8, 5, "concrete")
    grid_rect("Ground_Playable_Grid", 0, 0, 35, 35, 9)


def add_buildings():
    utility_building("UtilityBuilding01_BackYardControl", "utility_1", 4, 23, 6, 5, 330)
    utility_building("UtilityBuilding02_ServiceYard", "utility_2", 25, 7, 6, 5, 310)
    blockout_building("BrickPumpHouse_Southwest", 4, 7, 5, 5, 3.0, 305)
    blockout_building("BrickControlRoom_DominantRoof", 15, 14, 6, 6, 5.0, 505, dominant=True)

    add_ladder("Ladder_BackYardUtility_RoofAccess", 10, 25, 330, 90)
    add_ladder("Ladder_ServiceYardUtility_RoofAccess", 24, 9, 310, -90)
    add_ladder("Ladder_PumpHouse_RoofAccess", 9, 8, 305, 90)
    add_ladder("Ladder_DominantRoof_SouthAccess", 17, 13, 505, 0)

    floor_rect("RooftopPath_MetalCatwalk_BackYardToCenter", 10, 24, 6, 1, 325, "metal")
    floor_rect("RooftopPath_MetalCatwalk_CenterToRoad", 20, 20, 4, 1, 325, "metal")
    concrete_barrier("RooftopPath_catwalk_cover_backyard", 12, 24, 2, 1, 325)
    concrete_barrier("RooftopPath_catwalk_cover_road", 21, 20, 2, 1, 325)


def add_cover_and_paths():
    # Main road cover breaks long sightlines every 5-6 meters.
    concrete_barrier("Road_Median_ConcreteCover_01", 15, 4, 2, 1)
    concrete_barrier("Road_Median_ConcreteCover_02", 18, 10, 2, 1)
    concrete_barrier("Road_Median_ConcreteCover_03", 14, 20, 2, 1)
    concrete_barrier("Road_Bend_ConcreteCover", 22, 24, 3, 1)

    car = MESHES.get("junk_car")
    if car:
        spawn_fit_mesh(car, "JunkCar01_AbandonedCover_MainRoad", rect_center(15, 29, 3, 2), (3, 2), 135, yaw=18, z_base=0)
        spawn_fit_mesh(car, "JunkCar01_AbandonedCover_SideStreet", rect_center(25, 24, 3, 2), (3, 2), 135, yaw=-28, z_base=0)
    else:
        full_cover("JunkCar01_AbandonedCover_MainRoad_fallback", 15, 29, 3, 2, material_name="metal")
        full_cover("JunkCar01_AbandonedCover_SideStreet_fallback", 25, 24, 3, 2, material_name="metal")

    shipping_container("ShippingContainer_Rusted_DirtYard_LOSBlocker", 27, 13, 90)
    shipping_container("ShippingContainer_Rusted_BackYard_FlankGate", 5, 29, 0)

    # Courtyard has multiple cover islands and two flanking exits.
    concrete_barrier("Courtyard_LowWall_North", 10, 21, 4, 1)
    concrete_barrier("Courtyard_LowWall_South", 9, 10, 3, 1)
    crate_stack("Courtyard_CrateCluster_West", 9, 15, 0, 8)
    crate_stack("Courtyard_CrateCluster_East", 22, 16, 0, -12)
    concrete_barrier("Courtyard_Center_ConcreteCover", 13, 17, 2, 1, 0, 90)
    concrete_barrier("Courtyard_East_ConcreteCover", 21, 12, 2, 1, 0, 90)

    # Narrow alleys.
    concrete_fence("SideAlley_West_BrokenFence_LOS", 10, 7, 5, False)
    crate_stack("SideAlley_West_Crate", 6, 13, 0, 0)
    concrete_fence("SideAlley_East_BrokenFence_LOS", 23, 20, 5, False)
    crate_stack("SideAlley_East_Crate", 27, 18, 0, 90)

    # Dirt/service yard.
    crate_stack("DirtYard_WoodenCrates_01", 24, 5, 0, 10)
    crate_stack("DirtYard_WoodenCrates_02", 31, 9, 0, -8)
    concrete_barrier("DirtYard_ConcreteCover", 31, 13, 2, 1, 0, 90)

    # Outer and interior fences with deliberate gaps.
    concrete_fence("OuterFence_SouthWest", 0, 0, 10, True)
    concrete_fence("OuterFence_NorthBack", 3, 34, 24, True)
    concrete_fence("OuterFence_EastService", 34, 8, 20, False)
    concrete_fence("InteriorFence_DirtYard_West", 22, 4, 8, False)
    concrete_fence("InteriorFence_BackYard_Screen", 3, 22, 8, True)
    concrete_fence("InteriorFence_Courtyard_North", 8, 23, 7, True)
    concrete_fence("InteriorFence_RoadBend_Screen", 27, 27, 4, True)

    # Readable blue player setup and green extraction, matching the reference image language.
    floor_rect("PlayerStartArea_Blue", 2, 2, 6, 5, 8, "player_blue", collision=False)
    crate_stack("PlayerStartArea_StartCover_A", 3, 3, 10, 0)
    concrete_barrier("PlayerStartArea_StartCover_B", 5, 4, 2, 1, 10)
    floor_rect("ExtractionPoint_Green", 29, 29, 4, 4, 8, "extract_green", collision=False)
    crate_stack("ExtractionPoint_SupplyBox", 30, 30, 10, 0)


def add_spawn_marker(name, cx, cy, color_name):
    floor_rect(f"{name}_{color_name}_marker", cx + 0.18, cy + 0.18, 0.64, 0.64, 12, color_name, collision=False)


def add_spawn(name, cx, cy, side, order, yaw, z=50):
    spawn_class = unreal.load_class(None, "/Script/ZombieGame.StrategySpawnPoint")
    if not spawn_class:
        unreal.log_warning("StrategySpawnPoint class not found")
        return None
    actor = spawn_actor(spawn_class, name, world_from_cell(cx, cy, z), (0, yaw, 0), (1, 1, 1))
    side_type = type(actor.get_editor_property("side"))
    side_value = side_type.PLAYER if side == "Player" else side_type.ENEMY
    actor.set_editor_property("side", side_value)
    actor.set_editor_property("spawn_order", order)
    return actor


def add_spawns_and_labels():
    for order, (cx, cy, yaw) in enumerate([(3, 3, 35), (5, 3, 25), (4, 5, 10), (6, 5, 5)]):
        add_spawn(f"PlayerSpawn_{order}_BlueStartArea", cx, cy, "Player", order, yaw)

    enemy_spawns = [
        (27, 6, 145, "behind_service_building"),
        (31, 13, 180, "dirt_fence_corner"),
        (30, 21, -150, "container_shadow"),
        (30, 27, -125, "road_bend"),
        (26, 30, -155, "north_back_fence"),
        (22, 19, -135, "dominant_roof_shadow"),
        (12, 22, -80, "courtyard_corner"),
    ]
    for order, (cx, cy, yaw, group) in enumerate(enemy_spawns):
        add_spawn(f"EnemySpawn_{order}_{group}", cx, cy, "Enemy", order, yaw)
        add_spawn_marker(f"ZombieSpawn_{order}_{group}", cx, cy, "enemy_red")


def add_text(name, text, loc, size=55, color=unreal.Color(255, 230, 120, 255)):
    actor = spawn_actor(unreal.TextRenderActor, name, loc, (62, 0, 0))
    comp = actor.get_component_by_class(unreal.TextRenderComponent)
    comp.set_text(text)
    comp.set_world_size(size)
    comp.set_text_render_color(color)
    return actor


def configure_world():
    world = unreal.EditorLevelLibrary.get_editor_world()
    settings = world.get_world_settings()
    game_mode = unreal.load_class(None, "/Game/Variant_Strategy/Blueprints/BP_StrategyGameMode.BP_StrategyGameMode_C")
    if game_mode:
        settings.set_editor_property("default_game_mode", game_mode)


def create_or_clear_level():
    if unreal.EditorAssetLibrary.does_asset_exist(LEVEL_PATH):
        unreal.EditorLevelLibrary.load_level(LEVEL_PATH)
        for actor in unreal.EditorLevelLibrary.get_all_level_actors():
            unreal.EditorLevelLibrary.destroy_actor(actor)
    else:
        unreal.EditorLevelLibrary.new_level(LEVEL_PATH)


def add_managers():
    grid_class = unreal.load_class(None, "/Script/ZombieGame.GridManager")
    highlight_class = unreal.load_class(None, "/Game/UI/BP_GridHighlightActor.BP_GridHighlightActor_C")
    if not highlight_class:
        highlight_class = unreal.load_class(None, "/Script/ZombieGame.GridHighlightActor")
    sight_class = unreal.load_class(None, "/Script/ZombieGame.SightManager")
    fog_class = unreal.load_class(None, "/Script/ZombieGame.FogOfWarActor")
    bounds_class = unreal.load_class(None, "/Script/ZombieGame.GridBoundsActor")
    nav_class = unreal.load_class(None, "/Script/NavigationSystem.NavMeshBoundsVolume")

    if grid_class:
        grid = spawn_actor(grid_class, "GridManager_logic_only_52x52", (ORIGIN_X, ORIGIN_Y, 0))
        safe_set(grid, "grid_origin", unreal.Vector(ORIGIN_X, ORIGIN_Y, 0))
        safe_set(grid, "cell_size", CELL_SIZE)
        safe_set(grid, "grid_width", GRID_W)
        safe_set(grid, "grid_height", GRID_H)
        safe_set(grid, "show_grid_in_editor", False)
        safe_set(grid, "show_grid_in_game", False)
        safe_set(grid, "debug_grid_disabled", True)

    if highlight_class:
        highlight = spawn_actor(highlight_class, "GridHighlight_RuntimeVisuals", (0, 0, 40))
        safe_set(highlight, "reachable_decal_material", load_asset("/Game/UI/Material/M_Reachable.M_Reachable"))
        safe_set(highlight, "overwatch_decal_material", load_asset("/Game/UI/Material/M_Overwatch.M_Overwatch"))
        safe_set(highlight, "half_cover_material", load_asset("/Game/UI/Material/M_HalfCoverShield1.M_HalfCoverShield1"))
        safe_set(highlight, "full_cover_material", load_asset("/Game/UI/Material/M_FullCoverShield.M_FullCoverShield"))
        safe_set(highlight, "cover_icon_mesh", MESHES["plane"])
        safe_set(highlight, "selected_cell_line_color", unreal.LinearColor(0.0, 0.9, 1.0, 1.0))
        safe_set(highlight, "selected_cell_line_thickness", 12.0)
        safe_set(highlight, "selected_cell_line_height_offset", 28.0)
        safe_set(highlight, "movement_path_line_thickness", 8.0)
        safe_set(highlight, "decal_size", unreal.Vector(120.0, 58.0, 58.0))

    if sight_class:
        spawn_actor(sight_class, "SightManager", (0, 0, 80))

    if fog_class:
        fog = spawn_actor(fog_class, "FogOfWar_GridTiles", (0, 0, 0))
        safe_set(fog, "fog_tile_mesh", MESHES["plane"])
        safe_set(fog, "unexplored_material", load_asset("/Game/UI/Material/M_Fog_Unexplored.M_Fog_Unexplored"))
        safe_set(fog, "explored_material", load_asset("/Game/UI/Material/M_Fog_Explored.M_Fog_Explored"))

    if bounds_class:
        center_x = ORIGIN_X + GRID_W * CELL_SIZE * 0.5
        center_y = ORIGIN_Y + GRID_H * CELL_SIZE * 0.5
        bounds = spawn_actor(
            bounds_class,
            "GridBounds_52x52_coastal_utility_test",
            (center_x, center_y, 60),
            (0, 0, 0),
            (GRID_W, GRID_H, 1.0),
        )
        bounds.set_actor_hidden_in_game(True)

    if nav_class:
        center_x = ORIGIN_X + GRID_W * CELL_SIZE * 0.5
        center_y = ORIGIN_Y + GRID_H * CELL_SIZE * 0.5
        spawn_actor(nav_class, "NavMeshBounds_CoastalUtilityTest", (center_x, center_y, 260), (0, 0, 0), (56, 56, 8))


def add_lighting_camera():
    sun = spawn_actor(unreal.DirectionalLight, "GameplayTest_Sun", (-900, -1200, 1800), (-48, -35, 0))
    sky = spawn_actor(unreal.SkyLight, "GameplayTest_Sky", (0, 0, 500))
    sun_comp = sun.get_component_by_class(unreal.DirectionalLightComponent)
    sky_comp = sky.get_component_by_class(unreal.SkyLightComponent)
    if sun_comp:
        safe_set(sun_comp, "intensity", 2.4)
    if sky_comp:
        safe_set(sky_comp, "intensity", 0.45)
    camera = spawn_actor(unreal.CineCameraActor, "Overview_Camera_52m_TestMap", (-3300, -4100, 4300), (-58, 40, 0))
    camera.get_cine_camera_component().set_editor_property("current_focal_length", 24.0)


def load_assets_and_materials():
    MESHES.update({
        "cube": load_asset("/Engine/BasicShapes/Cube.Cube"),
        "cylinder": load_asset("/Engine/BasicShapes/Cylinder.Cylinder"),
        "plane": load_asset("/Engine/BasicShapes/Plane.Plane"),
        "utility_1": load_asset("/Game/EnvironmentAssets/UtilityBuilding1/UtilityBuilding001.UtilityBuilding001"),
        "utility_2": load_asset("/Game/EnvironmentAssets/UtilityBuilding2/UtilityBuilding002.UtilityBuilding002"),
        "fence": load_asset("/Game/EnvironmentAssets/classic-reinforced-concrete-fence/mur_beton_u1_v1.mur_beton_u1_v1"),
        "door": load_asset("/Game/EnvironmentAssets/Door/Door.Door"),
        "door_alt": load_asset("/Game/EnvironmentAssets/ModernDoor/Door.Door"),
        "window_1": load_asset("/Game/EnvironmentAssets/FramedWoodenWindow/Framed_Wooden_Window_vmgjfabdw_Mid.Framed_Wooden_Window_vmgjfabdw_Mid"),
        "window_2": load_asset("/Game/EnvironmentAssets/OldWoodenWindow/Old_Wooden_Window_vmfwcabdw_Mid.Old_Wooden_Window_vmfwcabdw_Mid"),
        "ladder": load_asset("/Game/EnvironmentAssets/MuddyLadder/Muddy_Ladder_FBX.Muddy_Ladder_FBX"),
        "junk_car": load_asset("/Game/EnvironmentAssets/AbandonCar/SM_JUNKCAR1_DEFORMED2.SM_JUNKCAR1_DEFORMED2"),
        "container_frame": load_asset("/Game/EnvironmentAssets/freight-shipping-container-rusted/frame1.frame1"),
        "container_side_a": load_asset("/Game/EnvironmentAssets/freight-shipping-container-rusted/left_side.left_side"),
        "container_side_b": load_asset("/Game/EnvironmentAssets/freight-shipping-container-rusted/right_side.right_side"),
        "container_door_a": load_asset("/Game/EnvironmentAssets/freight-shipping-container-rusted/left_door.left_door"),
        "container_door_b": load_asset("/Game/EnvironmentAssets/freight-shipping-container-rusted/right_door.right_door"),
    })

    MATS.update({
        "concrete": ensure_texture_material(
            "M_CoastalTexture_Concrete",
            "/Game/EnvironmentalTextures/T_Convrete.T_Convrete",
            unreal.LinearColor(0.82, 0.80, 0.74, 1),
            0.88,
            7.5,
        ),
        "brick": ensure_texture_material(
            "M_CoastalTexture_Brick",
            "/Game/EnvironmentalTextures/T_Brick.T_Brick",
            unreal.LinearColor(0.70, 0.57, 0.48, 1),
            0.86,
            4.0,
        ),
        "asphalt": ensure_texture_material(
            "M_CoastalTexture_Asphalt",
            "/Game/EnvironmentalTextures/T_Asphalt.T_Asphalt",
            unreal.LinearColor(0.62, 0.62, 0.58, 1),
            0.94,
            8.5,
        ),
        "dirt": ensure_texture_material(
            "M_CoastalTexture_Dirt",
            "/Game/EnvironmentalTextures/T_Dirt.T_Dirt",
            unreal.LinearColor(0.70, 0.56, 0.42, 1),
            0.96,
            7.0,
        ),
        "metal": ensure_texture_material(
            "M_CoastalTexture_Metal",
            "/Game/EnvironmentalTextures/T_Metal.T_Metal",
            unreal.LinearColor(0.78, 0.78, 0.72, 1),
            0.58,
            4.0,
        ),
        "wood": ensure_texture_material(
            "M_CoastalTexture_Wood",
            "/Game/EnvironmentalTextures/T_Wood.T_Wood",
            unreal.LinearColor(0.78, 0.66, 0.52, 1),
            0.82,
            3.5,
        ),
        "glass": ensure_texture_material(
            "M_CoastalTexture_Glass",
            "/Game/EnvironmentalTextures/T_Glass.T_Glass",
            unreal.LinearColor(0.70, 0.82, 0.86, 1),
            0.22,
            2.0,
        ),
        "roof": ensure_texture_material(
            "M_CoastalTexture_Roof",
            "/Game/EnvironmentalTextures/T_Roof.T_Roof",
            unreal.LinearColor(0.62, 0.62, 0.58, 1),
            0.82,
            5.0,
        ),
        "roof_wall": ensure_texture_material(
            "M_CoastalTexture_RoofLowWall",
            "/Game/EnvironmentalTextures/T_Convrete.T_Convrete",
            unreal.LinearColor(0.64, 0.63, 0.58, 1),
            0.9,
            5.0,
        ),
        "interior_dark": ensure_material("M_CoastalUtility_InteriorDark", unreal.LinearColor(0.025, 0.022, 0.020, 1), 0.95),
        "grid_line": ensure_material("M_CoastalUtility_DiscreteGrid", unreal.LinearColor(0.09, 0.10, 0.10, 1), 0.95),
        "ramp": ensure_material("M_CoastalUtility_LadderNavRamp", unreal.LinearColor(0.22, 0.24, 0.22, 1), 0.88),
        "player_blue": ensure_material("M_CoastalUtility_PlayerStartBlue", unreal.LinearColor(0.08, 0.24, 0.85, 1), 0.45),
        "extract_green": ensure_material("M_CoastalUtility_ExtractionGreen", unreal.LinearColor(0.05, 0.68, 0.18, 1), 0.45),
        "enemy_red": ensure_material("M_CoastalUtility_ZombieSpawnRed", unreal.LinearColor(0.80, 0.03, 0.05, 1), 0.55),
        "objective": ensure_material("M_CoastalUtility_ObjectiveAmber", unreal.LinearColor(1.00, 0.62, 0.08, 1), 0.50),
    })


def main():
    create_or_clear_level()
    configure_world()
    load_assets_and_materials()
    add_ground()
    add_buildings()
    add_cover_and_paths()
    add_spawns_and_labels()
    add_managers()
    add_lighting_camera()
    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    unreal.log(f"Built compact tactical zombie test level {LEVEL_PATH}")


if __name__ == "__main__":
    main()
