import math
import unreal


LEVEL_PATH = "/Game/Variant_Strategy/LVL_CoastalCache"
CELL_SIZE = 100.0
GRID_W = 34
GRID_H = 24
ORIGIN_X = -1700.0
ORIGIN_Y = -1200.0


def load_asset(path):
    asset = unreal.EditorAssetLibrary.load_asset(path)
    if not asset:
        unreal.log_warning(f"Missing asset: {path}")
    return asset


def ensure_material(name, color, roughness=0.82):
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


MATS = {}
ENV_MESHES = {}


def mat(name):
    return MATS[name]


def world_from_cell(cx, cy, z=0.0):
    return (ORIGIN_X + cx * CELL_SIZE + CELL_SIZE * 0.5, ORIGIN_Y + cy * CELL_SIZE + CELL_SIZE * 0.5, z)


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
        unreal.log_warning(f"{actor.get_actor_label()}: could not set {property_name}: {exc}")


def spawn_mesh(mesh, name, loc, rot=(0, 0, 0), scale=(1, 1, 1), material=None, collision=True):
    actor = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.StaticMeshActor, unreal.Vector(*loc), unreal.Rotator(*rot)
    )
    actor.set_actor_label(name)
    actor.set_actor_scale3d(unreal.Vector(*scale))
    comp = actor.get_component_by_class(unreal.StaticMeshComponent)
    comp.set_static_mesh(mesh)
    if material:
        comp.set_material(0, material)
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


def spawn_fit_mesh(mesh, name, center_xy, footprint_cells, height, yaw=0, z_base=0, collision=True):
    desired_x = max(footprint_cells[0] * CELL_SIZE, 1.0)
    desired_y = max(footprint_cells[1] * CELL_SIZE, 1.0)
    size, bounds_center, bounds_min = mesh_bounds_size(mesh)
    if size and size.x > 1.0 and size.y > 1.0 and size.z > 1.0:
        uniform_xy = min(desired_x / size.x, desired_y / size.y)
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
    return spawn_mesh(mesh, name, loc, (0, yaw, 0), scale, None, collision)


def cube(name, loc, scale, material, rot=(0, 0, 0), collision=True):
    return spawn_mesh(BASIC_CUBE, name, loc, rot, scale, material, collision)


def cyl(name, loc, scale, material, rot=(0, 0, 0), collision=True):
    return spawn_mesh(BASIC_CYLINDER, name, loc, rot, scale, material, collision)


def add_text(name, text, loc, size=70, color=unreal.Color(255, 230, 120, 255)):
    actor = spawn_actor(unreal.TextRenderActor, name, loc, (62, 0, 0))
    comp = actor.get_component_by_class(unreal.TextRenderComponent)
    comp.set_text(text)
    comp.set_world_size(size)
    comp.set_text_render_color(color)
    return actor


def add_floor_rect(name, x0, y0, w, h, z, material_name, collision=True):
    x = ORIGIN_X + (x0 + w * 0.5) * CELL_SIZE
    y = ORIGIN_Y + (y0 + h * 0.5) * CELL_SIZE
    cube(name, (x, y, z - 5), (w, h, 0.10), mat(material_name), collision=collision)


def add_grid_rect(prefix, x0, y0, w, h, z=4):
    # Thin physical line strips, used instead of GridManager debug lines so it only appears on walkable blocks.
    line_mat = mat("grid_line")
    for ix in range(w + 1):
        x = ORIGIN_X + (x0 + ix) * CELL_SIZE
        y = ORIGIN_Y + (y0 + h * 0.5) * CELL_SIZE
        cube(f"{prefix}_grid_v_{ix}", (x, y, z), (0.012, h, 0.012), line_mat, collision=False)
    for iy in range(h + 1):
        x = ORIGIN_X + (x0 + w * 0.5) * CELL_SIZE
        y = ORIGIN_Y + (y0 + iy) * CELL_SIZE
        cube(f"{prefix}_grid_h_{iy}", (x, y, z), (w, 0.012, 0.012), line_mat, collision=False)


def add_route_floor():
    # Three readable lanes: open beach, village cover route, and raised dock/roof route.
    add_floor_rect("Walkable_OpenRoute_Beach", 2, 3, 17, 7, 0, "ground")
    add_floor_rect("Walkable_VillageRoute", 6, 9, 20, 8, 0, "ground")
    add_floor_rect("Walkable_ObjectiveArea", 21, 13, 10, 7, 0, "ground")
    add_floor_rect("Walkable_HighRoute_Dock", 5, 17, 19, 4, 0, "ground")
    add_floor_rect("Walkable_PlayerStart", 1, 1, 8, 5, 0, "ground_safe")
    add_floor_rect("Visual_CoastalWater_West_NoNav", -1, 15, 6, 8, -18, "water", collision=False)
    add_floor_rect("Visual_CoastalWater_North_NoNav", 15, 21, 19, 4, -18, "water", collision=False)

    add_grid_rect("Grid_PlayerStart", 1, 1, 8, 5)
    add_grid_rect("Grid_OpenRoute", 2, 3, 17, 7)
    add_grid_rect("Grid_VillageRoute", 6, 9, 20, 8)
    add_grid_rect("Grid_ObjectiveArea", 21, 13, 10, 7)
    add_grid_rect("Grid_HighRouteDock", 5, 17, 19, 4)


def add_wall_cells(prefix, cells, height_cells=1.7, material_name="wall"):
    for i, (cx, cy) in enumerate(cells):
        x, y, _ = world_from_cell(cx, cy, height_cells * 50)
        cube(f"{prefix}_{i}", (x, y, height_cells * 50), (0.92, 0.92, height_cells), mat(material_name))


def add_low_cover(name, cx, cy, w=1, h=1, z=0, material_name="cover_low"):
    x = ORIGIN_X + (cx + w * 0.5) * CELL_SIZE
    y = ORIGIN_Y + (cy + h * 0.5) * CELL_SIZE
    cube(name, (x, y, z + 42), (w * 0.88, h * 0.88, 0.84), mat(material_name))


def add_full_cover(name, cx, cy, w=1, h=1, z=0, material_name="cover_full"):
    x = ORIGIN_X + (cx + w * 0.5) * CELL_SIZE
    y = ORIGIN_Y + (cy + h * 0.5) * CELL_SIZE
    cube(name, (x, y, z + 115), (w * 0.92, h * 0.92, 2.30), mat(material_name))


def add_building(name, x0, y0, w, h, roof_access=False):
    x = ORIGIN_X + (x0 + w * 0.5) * CELL_SIZE
    y = ORIGIN_Y + (y0 + h * 0.5) * CELL_SIZE
    visual_key = "utility_2" if (w >= 5 or "objective" in name) else "utility_1"
    visual_mesh = ENV_MESHES.get(visual_key)
    if visual_mesh:
        spawn_fit_mesh(visual_mesh, f"{name}_visual", (x, y), (w, h), 290, yaw=0, z_base=0)
    else:
        cube(f"{name}_box", (x, y, 145), (w, h, 2.9), mat("building"))
    if roof_access:
        add_floor_rect(f"{name}_WalkableRoof", x0, y0, w, h, 312, "roof_walk")
        add_grid_rect(f"{name}_RoofGrid", x0, y0, w, h, 322)


def add_ramp(name, cx, cy, w, h, z0, z1, yaw=0):
    x = ORIGIN_X + (cx + w * 0.5) * CELL_SIZE
    y = ORIGIN_Y + (cy + h * 0.5) * CELL_SIZE
    pitch = -math.degrees(math.atan2(abs(z1 - z0), max(w * CELL_SIZE, 1.0))) if z1 > z0 else math.degrees(math.atan2(abs(z1 - z0), max(w * CELL_SIZE, 1.0)))
    cube(name, (x, y, (z0 + z1) * 0.5), (w, h, 0.18), mat("ramp"), (pitch, yaw, 0))


def add_container(name, cx, cy, w=3, h=1, yaw=0):
    x = ORIGIN_X + (cx + w * 0.5) * CELL_SIZE
    y = ORIGIN_Y + (cy + h * 0.5) * CELL_SIZE
    cube(f"{name}_body", (x, y, 95), (w * 0.95, h * 0.92, 1.9), mat("container"), (0, yaw, 0))
    cube(f"{name}_top", (x, y, 194), (w * 0.98, h * 0.95, 0.08), mat("container_trim"), (0, yaw, 0))


def add_boat_cover(name, cx, cy, w=4, h=1, yaw=0):
    x = ORIGIN_X + (cx + w * 0.5) * CELL_SIZE
    y = ORIGIN_Y + (cy + h * 0.5) * CELL_SIZE
    cube(f"{name}_hull", (x, y, 52), (w * 0.95, h * 0.70, 0.55), mat("boat"), (0, yaw, 0))
    cube(f"{name}_left_side", (x, y + 38, 92), (w * 0.95, 0.10, 0.80), mat("boat_dark"), (0, yaw, 10))
    cube(f"{name}_right_side", (x, y - 38, 92), (w * 0.95, 0.10, 0.80), mat("boat_dark"), (0, yaw, -10))


def add_cover_clusters():
    # Player start: safe cover facing first contact.
    add_low_cover("player_start_low_wall_left", 3, 3, 2, 1)
    add_low_cover("player_start_crates_center", 6, 4, 2, 1, material_name="crate")
    add_full_cover("player_start_full_container", 1, 5, 2, 1, material_name="container")

    # Open route: exposed but never empty for more than a few cells.
    add_low_cover("open_route_low_concrete_1", 10, 5, 2, 1)
    add_low_cover("open_route_low_concrete_2", 15, 6, 2, 1)
    add_boat_cover("open_route_beached_boat_cover", 12, 8, 4, 1, -8)
    add_full_cover("open_route_broken_wall", 18, 8, 1, 3)

    # Safer village route with alleys and meaningful full-cover corners.
    add_building("village_house_south", 7, 10, 4, 3, False)
    add_building("village_house_mid_roof", 13, 11, 5, 4, True)
    add_building("village_store_north", 7, 15, 5, 3, False)
    add_container("village_container_lane", 19, 10, 3, 1)
    add_low_cover("village_crate_cluster_a", 11, 13, 2, 1, material_name="crate")
    add_low_cover("village_crate_cluster_b", 18, 15, 2, 1, material_name="crate")
    add_full_cover("village_corner_wall_a", 6, 13, 1, 2)
    add_full_cover("village_corner_wall_b", 22, 12, 1, 2)

    # High route: dock/platform and roof with cover.
    add_floor_rect("high_ground_1_platform", 15, 18, 6, 3, 285, "platform")
    add_grid_rect("high_ground_1_platform_grid", 15, 18, 6, 3, 295)
    add_ramp("ramp_to_high_ground_1", 12, 17, 4, 2, 15, 285)
    add_low_cover("high_ground_1_rooftop_crates", 16, 19, 2, 1, 310, "crate")
    add_low_cover("high_ground_1_low_wall", 19, 20, 2, 1, 310)

    # Objective zone and enemy high ground.
    add_building("objective_cache_building", 25, 15, 4, 3, True)
    add_floor_rect("high_ground_2_enemy_platform", 27, 18, 4, 2, 260, "platform_enemy")
    add_grid_rect("high_ground_2_enemy_platform_grid", 27, 18, 4, 2, 270)
    add_ramp("ramp_to_high_ground_2", 24, 18, 3, 2, 10, 260)
    add_low_cover("objective_low_wall_west", 23, 15, 2, 1)
    add_low_cover("objective_low_wall_south", 25, 13, 2, 1)
    add_full_cover("objective_full_cover_container", 29, 14, 2, 1, material_name="container")

    # Boundaries/fences communicate lanes without sealing the whole map.
    add_wall_cells("north_dock_fence", [(x, 21) for x in range(6, 24, 2)], 1.25, "fence")
    add_wall_cells("south_rock_boundary", [(x, 0) for x in range(0, 11)], 1.6, "wall")
    add_wall_cells("east_village_fence", [(32, y) for y in range(12, 20)], 1.25, "fence")


def add_objective():
    add_low_cover("objective_weapon_cache_crate_a", 27, 16, 1, 1, 312, "objective")
    add_low_cover("objective_weapon_cache_crate_b", 28, 16, 1, 1, 312, "objective")
    x, y, _ = world_from_cell(27.5, 16.0, 345)
    cyl("objective_yellow_marker", (x, y, 350), (0.58, 0.58, 0.08), mat("objective"), collision=False)
    add_text("objective", "objective", (x, y - 95, 470), 60, unreal.Color(255, 210, 80, 255))


def add_debug_labels():
    labels = [
        ("player_start", 4.5, 2.0, 80, unreal.Color(80, 170, 255, 255)),
        ("high_ground_1", 18.0, 19.5, 430, unreal.Color(180, 230, 255, 255)),
        ("high_ground_2", 29.0, 19.0, 405, unreal.Color(255, 120, 95, 255)),
        ("zombie_spawn_group_1", 17.0, 10.5, 90, unreal.Color(255, 95, 80, 255)),
        ("zombie_spawn_group_2", 28.0, 14.0, 90, unreal.Color(255, 95, 80, 255)),
    ]
    for name, cx, cy, z, color in labels:
        x, y, _ = world_from_cell(cx, cy, z)
        add_text(name, name, (x, y, z), 48, color)


def add_spawn(name, cx, cy, side, order, yaw, z=50):
    spawn_class = unreal.load_class(None, "/Script/ZombieGame.StrategySpawnPoint")
    actor = spawn_actor(spawn_class, name, world_from_cell(cx, cy, z), (0, yaw, 0), (1, 1, 1))
    side_type = type(actor.get_editor_property("side"))
    side_value = side_type.PLAYER if side == "Player" else side_type.ENEMY
    actor.set_editor_property("side", side_value)
    actor.set_editor_property("spawn_order", order)
    return actor


def add_spawns():
    for order, (cx, cy, yaw) in enumerate([(3, 3, 35), (5, 3, 20), (4, 5, 10), (7, 4, 5)]):
        add_spawn(f"PlayerSpawn_{order}_player_start", cx, cy, "Player", order, yaw)

    enemy_spawns = [
        (15, 9, -120, 0, "zombie_spawn_group_1"),
        (18, 10, -135, 0, "zombie_spawn_group_1"),
        (21, 12, -150, 0, "zombie_spawn_group_1"),
        (28, 14, -155, 0, "zombie_spawn_group_2"),
        (30, 16, -165, 0, "zombie_spawn_group_2"),
        (29, 18, -145, 285, "zombie_high_ground"),
        (24, 18, -145, 0, "objective_patrol"),
        (12, 17, -100, 0, "north_patrol"),
    ]
    for order, (cx, cy, yaw, z, group) in enumerate(enemy_spawns):
        add_spawn(f"EnemySpawn_{order}_{group}", cx, cy, "Enemy", order, yaw, 50 + z)
        x, y, _ = world_from_cell(cx, cy, 18 + z)
        cyl(f"Enemy_Danger_Tile_{order}", (x, y, 18 + z), (0.42, 0.42, 0.025), mat("enemy"), collision=False)


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
    sight_class = unreal.load_class(None, "/Script/ZombieGame.SightManager")
    fog_class = unreal.load_class(None, "/Script/ZombieGame.FogOfWarActor")
    bounds_class = unreal.load_class(None, "/Script/ZombieGame.GridBoundsActor")
    nav_class = unreal.load_class(None, "/Script/NavigationSystem.NavMeshBoundsVolume")

    if grid_class:
        grid = spawn_actor(grid_class, "GridManager_logic_only_34x24", (ORIGIN_X, ORIGIN_Y, 0))
        safe_set(grid, "grid_origin", unreal.Vector(ORIGIN_X, ORIGIN_Y, 0))
        safe_set(grid, "cell_size", CELL_SIZE)
        safe_set(grid, "grid_width", GRID_W)
        safe_set(grid, "grid_height", GRID_H)
        safe_set(grid, "show_grid_in_editor", False)
        safe_set(grid, "show_grid_in_game", False)
        safe_set(grid, "debug_grid_disabled", True)
        safe_set(grid, "grid_color", unreal.Color(70, 120, 140, 60))
        safe_set(grid, "grid_line_thickness", 0.35)

    if sight_class:
        spawn_actor(sight_class, "SightManager", (0, 0, 80))

    if fog_class:
        fog = spawn_actor(fog_class, "FogOfWar_GridTiles", (0, 0, 0))
        safe_set(fog, "fog_tile_mesh", BASIC_PLANE)
        safe_set(fog, "unexplored_material", load_asset("/Game/UI/Material/M_Fog_Unexplored.M_Fog_Unexplored"))
        safe_set(fog, "explored_material", load_asset("/Game/UI/Material/M_Fog_Explored.M_Fog_Explored"))

    if bounds_class:
        center_x = ORIGIN_X + GRID_W * CELL_SIZE * 0.5
        center_y = ORIGIN_Y + GRID_H * CELL_SIZE * 0.5
        bounds = spawn_actor(
            bounds_class,
            "GridBounds_34x24_tactical_blockout",
            (center_x, center_y, 60),
            (0, 0, 0),
            (GRID_W, GRID_H, 1.0),
        )
        bounds.set_actor_hidden_in_game(True)

    if nav_class:
        center_x = ORIGIN_X + GRID_W * CELL_SIZE * 0.5
        center_y = ORIGIN_Y + GRID_H * CELL_SIZE * 0.5
        spawn_actor(nav_class, "NavMeshBounds_TacticalCoast", (center_x, center_y, 240), (0, 0, 0), (36, 26, 6))


def add_lighting_camera():
    spawn_actor(unreal.DirectionalLight, "Blockout_Sun", (-700, -500, 1600), (-45, -38, 0))
    spawn_actor(unreal.SkyLight, "Blockout_Sky", (0, 0, 500))
    camera = spawn_actor(unreal.CineCameraActor, "Overview_Camera", (-1600, -2050, 2200), (-56, 39, 0))
    camera.get_cine_camera_component().set_editor_property("current_focal_length", 23.0)


def main():
    create_or_clear_level()
    configure_world()

    global BASIC_CUBE, BASIC_CYLINDER, BASIC_PLANE
    BASIC_CUBE = load_asset("/Engine/BasicShapes/Cube.Cube")
    BASIC_CYLINDER = load_asset("/Engine/BasicShapes/Cylinder.Cylinder")
    BASIC_PLANE = load_asset("/Engine/BasicShapes/Plane.Plane")
    ENV_MESHES.update({
        "utility_1": load_asset("/Game/EnvironmentAssets/UtilityBuilding1/UtilityBuilding001.UtilityBuilding001"),
        "utility_2": load_asset("/Game/EnvironmentAssets/UtilityBuilding2/UtilityBuilding002.UtilityBuilding002"),
    })

    MATS.update({
        "ground": ensure_material("M_Blockout_Ground_BeigeGray", unreal.LinearColor(0.47, 0.46, 0.42, 1)),
        "ground_safe": ensure_material("M_Blockout_PlayerStart_MutedBlue", unreal.LinearColor(0.30, 0.40, 0.46, 1)),
        "grid_line": ensure_material("M_Blockout_DiscreteGrid", unreal.LinearColor(0.22, 0.27, 0.27, 1), 0.95),
        "water": ensure_material("M_Blockout_Water_Dark", unreal.LinearColor(0.03, 0.10, 0.14, 1), 0.45),
        "cover_low": ensure_material("M_Blockout_HalfCover_DarkGray", unreal.LinearColor(0.24, 0.23, 0.21, 1)),
        "cover_full": ensure_material("M_Blockout_FullCover_Charcoal", unreal.LinearColor(0.11, 0.11, 0.10, 1)),
        "crate": ensure_material("M_Blockout_Crate_Brown", unreal.LinearColor(0.36, 0.25, 0.15, 1)),
        "building": ensure_material("M_Blockout_Building_Wood", unreal.LinearColor(0.27, 0.20, 0.15, 1)),
        "roof": ensure_material("M_Blockout_Roof_Dark", unreal.LinearColor(0.08, 0.085, 0.08, 1)),
        "roof_walk": ensure_material("M_Blockout_Roof_Walkable", unreal.LinearColor(0.19, 0.20, 0.19, 1)),
        "door": ensure_material("M_Blockout_Door_Opening", unreal.LinearColor(0.015, 0.015, 0.012, 1)),
        "platform": ensure_material("M_Blockout_Platform_BlueGray", unreal.LinearColor(0.24, 0.32, 0.36, 1)),
        "platform_enemy": ensure_material("M_Blockout_Platform_RedGray", unreal.LinearColor(0.35, 0.25, 0.23, 1)),
        "ramp": ensure_material("M_Blockout_Ramp", unreal.LinearColor(0.33, 0.31, 0.27, 1)),
        "container": ensure_material("M_Blockout_Container", unreal.LinearColor(0.08, 0.17, 0.24, 1)),
        "container_trim": ensure_material("M_Blockout_ContainerTrim", unreal.LinearColor(0.035, 0.04, 0.04, 1)),
        "boat": ensure_material("M_Blockout_Boat_Wood", unreal.LinearColor(0.34, 0.25, 0.18, 1)),
        "boat_dark": ensure_material("M_Blockout_Boat_Dark", unreal.LinearColor(0.16, 0.12, 0.09, 1)),
        "wall": ensure_material("M_Blockout_Wall_Stone", unreal.LinearColor(0.22, 0.21, 0.19, 1)),
        "fence": ensure_material("M_Blockout_Fence", unreal.LinearColor(0.30, 0.22, 0.15, 1)),
        "objective": ensure_material("M_Blockout_Objective_Yellow", unreal.LinearColor(1.0, 0.62, 0.08, 1), 0.48),
        "enemy": ensure_material("M_Blockout_Enemy_Red", unreal.LinearColor(0.80, 0.05, 0.04, 1), 0.58),
    })

    add_route_floor()
    add_cover_clusters()
    add_objective()
    add_debug_labels()
    add_managers()
    add_spawns()
    add_lighting_camera()

    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    unreal.log(f"Built tactical blockout level {LEVEL_PATH}")


if __name__ == "__main__":
    main()
