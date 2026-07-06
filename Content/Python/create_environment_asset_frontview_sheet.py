import math
import os
import re

import unreal


OUT_FILE = os.path.join(unreal.Paths.project_dir(), "BuildOutput", "EnvironmentAssets_FrontViews.png")
TITLE = "EnvironmentAssets"


def asset_class_name(asset_data):
    class_path = getattr(asset_data, "asset_class_path", None)
    if class_path:
        return str(class_path.asset_name)
    return str(asset_data.asset_class)


def collect_mesh_assets():
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    registry.scan_paths_synchronous(["/Game/EnvironmentAssets"], True)
    assets = registry.get_assets_by_path("/Game/EnvironmentAssets", recursive=True)
    meshes = []
    for data in assets:
        cls = asset_class_name(data)
        if cls not in ("StaticMesh", "SkeletalMesh"):
            continue
        package = str(data.package_name)
        if "/GeneratedMaterials/" in package:
            continue
        meshes.append((package, cls))
    return sorted(meshes, key=lambda item: item[0].lower())


def clear_level():
    subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    for actor in list(subsystem.get_all_level_actors()):
        subsystem.destroy_actor(actor)


def spawn_mesh(package, cls):
    asset = unreal.load_asset(package)
    label = package.replace("/Game/EnvironmentAssets/", "")
    if cls == "StaticMesh":
        actor = unreal.EditorLevelLibrary.spawn_actor_from_class(
            unreal.StaticMeshActor, unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0)
        )
        actor.static_mesh_component.set_static_mesh(asset)
        actor.static_mesh_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    else:
        actor = unreal.EditorLevelLibrary.spawn_actor_from_class(
            unreal.SkeletalMeshActor, unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0)
        )
        actor.skeletal_mesh_component.set_skeletal_mesh_asset(asset)
        actor.skeletal_mesh_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    actor.set_actor_label(label)
    return actor


def recenter_and_fit(actor, cell_center, max_width, max_height):
    origin, extent = actor.get_actor_bounds(False)
    visible_width = max(extent.y * 2.0, 1.0)
    visible_height = max(extent.z * 2.0, 1.0)
    scale = min(max_width / visible_width, max_height / visible_height)
    scale = min(scale, 4.0)
    actor.set_actor_scale3d(unreal.Vector(scale, scale, scale))
    origin, extent = actor.get_actor_bounds(False)
    actor.add_actor_world_offset(cell_center - origin, False, True)


def add_text(label, location, size=34):
    actor = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.TextRenderActor, location, unreal.Rotator(0, 0, 0)
    )
    text = label.replace("/Game/EnvironmentAssets/", "")
    text = re.sub(r"^(.{42}).+$", r"\1...", text)
    comp = actor.text_render
    comp.set_text(text)
    comp.set_horizontal_alignment(unreal.HorizTextAligment.EHTA_CENTER)
    comp.set_vertical_alignment(unreal.VerticalTextAligment.EVRTA_TEXT_CENTER)
    comp.set_world_size(size)
    comp.set_editor_property("text_render_color", unreal.Color(25, 31, 34, 255))
    actor.set_actor_label("label_" + text)
    return actor


def add_lighting():
    unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.DirectionalLight, unreal.Vector(900, -700, 1200), unreal.Rotator(-35, 150, 0)
    ).light_component.set_editor_property("intensity", 14.0)
    sky = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.SkyLight, unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0)
    )
    sky.light_component.set_editor_property("intensity", 6.0)
    fill = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.PointLight, unreal.Vector(1800, 0, 700), unreal.Rotator(0, 0, 0)
    )
    fill.point_light_component.set_editor_property("intensity", 90000.0)
    fill.point_light_component.set_editor_property("attenuation_radius", 6000.0)


def build_scene():
    os.makedirs(os.path.dirname(OUT_FILE), exist_ok=True)
    unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
    clear_level()
    add_lighting()

    meshes = collect_mesh_assets()
    cols = 3
    cell_w = 920.0
    cell_h = 650.0
    rows = int(math.ceil(len(meshes) / cols))
    total_w = cols * cell_w
    total_h = rows * cell_h + 360.0
    start_y = -total_w / 2.0 + cell_w / 2.0
    top_z = total_h / 2.0 - 520.0

    for i, (package, cls) in enumerate(meshes):
        col = i % cols
        row = i // cols
        y = start_y + col * cell_w
        z = top_z - row * cell_h
        center = unreal.Vector(0, y, z)
        actor = spawn_mesh(package, cls)
        recenter_and_fit(actor, center + unreal.Vector(0, 0, 50), cell_w * 0.78, cell_h * 0.62)

    unreal.SystemLibrary.execute_console_command(None, "r.DefaultFeature.AutoExposure 0")
    unreal.SystemLibrary.execute_console_command(None, "r.EyeAdaptationQuality 0")

    camera = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.CameraActor,
        unreal.Vector(4200, 0, 0),
        unreal.Rotator(0, 180, 0),
    )
    comp = camera.camera_component
    comp.set_editor_property("projection_mode", unreal.CameraProjectionMode.ORTHOGRAPHIC)
    comp.set_editor_property("ortho_width", total_h)
    comp.set_editor_property("aspect_ratio", 4200.0 / 5600.0)
    camera.set_actor_label("EnvironmentAssets_FrontView_Camera")
    return camera


camera_actor = build_scene()
unreal.AutomationLibrary.finish_loading_before_screenshot()
task = unreal.AutomationLibrary.take_high_res_screenshot(
    4200,
    6200,
    OUT_FILE,
    camera_actor,
    False,
    False,
    unreal.ComparisonTolerance.LOW,
    "",
    1.0,
    True,
)


def wait_for_screenshot(delta_seconds):
    if not task.is_valid_task() or task.is_task_done():
        unreal.log("FRONTVIEW_SHEET_DONE " + OUT_FILE)
        unreal.unregister_slate_post_tick_callback(wait_for_screenshot.handle)
        unreal.SystemLibrary.quit_editor()


wait_for_screenshot.handle = unreal.register_slate_post_tick_callback(wait_for_screenshot)
