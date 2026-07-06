import json
import math
import os
import re
import time

import unreal


OUT_DIR = os.path.join(unreal.Paths.project_saved_dir(), "EnvironmentAssetFrontViews")
SIZE = 1024


def safe_name(value):
    return re.sub(r"[^A-Za-z0-9_.-]+", "_", value)


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
    only = os.environ.get("ENV_ASSET_PREVIEW_ONLY")
    if only:
        meshes = [item for item in meshes if only.lower() in item[0].lower()]
    meshes.sort(key=lambda item: item[0].lower())
    return meshes


def clear_level():
    subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    for actor in list(subsystem.get_all_level_actors()):
        subsystem.destroy_actor(actor)


def spawn_mesh(package, cls):
    asset = unreal.load_asset(package)
    if not asset:
        raise RuntimeError("Could not load " + package)
    if cls == "StaticMesh":
        actor = unreal.EditorLevelLibrary.spawn_actor_from_class(
            unreal.StaticMeshActor,
            unreal.Vector(0, 0, 0),
            unreal.Rotator(0, 0, 0),
        )
        actor.static_mesh_component.set_static_mesh(asset)
        actor.static_mesh_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    else:
        actor = unreal.EditorLevelLibrary.spawn_actor_from_class(
            unreal.SkeletalMeshActor,
            unreal.Vector(0, 0, 0),
            unreal.Rotator(0, 0, 0),
        )
        actor.skeletal_mesh_component.set_skeletal_mesh_asset(asset)
        actor.skeletal_mesh_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    actor.set_actor_label(os.path.basename(package))
    return actor


def look_at_rotation(from_location, to_location):
    direction = to_location - from_location
    yaw = math.degrees(math.atan2(direction.y, direction.x))
    dist_xy = math.sqrt(direction.x * direction.x + direction.y * direction.y)
    pitch = math.degrees(math.atan2(direction.z, dist_xy))
    return unreal.Rotator(pitch, yaw, 0)


def setup_lighting(center, extent):
    sun = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.DirectionalLight,
        unreal.Vector(-600, -700, 900),
        unreal.Rotator(-38, -35, 0),
    )
    sun.light_component.set_editor_property("intensity", 4.0)

    fill = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.PointLight,
        unreal.Vector(-350, 450, center.z + max(extent.z, 120)),
        unreal.Rotator(0, 0, 0),
    )
    fill.point_light_component.set_editor_property("intensity", 5000.0)
    fill.point_light_component.set_editor_property("attenuation_radius", 2200.0)


def render_mesh(package, cls):
    clear_level()
    actor = spawn_mesh(package, cls)

    unreal.SystemLibrary.flush_persistent_debug_lines(actor)
    origin, extent = actor.get_actor_bounds(False)
    max_dim = max(extent.x, extent.y, extent.z, 1.0)
    setup_lighting(origin, extent)

    capture_actor = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.SceneCapture2D,
        unreal.Vector(origin.x - max_dim * 3.4, origin.y, origin.z),
        unreal.Rotator(0, 0, 0),
    )
    capture_actor.set_actor_rotation(
        look_at_rotation(capture_actor.get_actor_location(), origin),
        False,
    )

    rt = unreal.RenderingLibrary.create_render_target2d(
        capture_actor,
        SIZE,
        SIZE,
        unreal.TextureRenderTargetFormat.RTF_RGBA8,
        unreal.LinearColor(0.78, 0.78, 0.74, 1.0),
        False,
    )

    comp = capture_actor.capture_component2d
    comp.set_editor_property("texture_target", rt)
    comp.set_editor_property("capture_source", unreal.SceneCaptureSource.SCS_BASE_COLOR)
    comp.set_editor_property("unlit_viewmode", unreal.SceneCaptureUnlitViewmode.CAPTURE)
    comp.set_editor_property("projection_type", unreal.CameraProjectionMode.ORTHOGRAPHIC)
    comp.set_editor_property("ortho_width", max(extent.y * 2.45, extent.z * 2.45, max_dim * 1.35, 120.0))
    comp.set_editor_property("capture_every_frame", False)
    comp.set_editor_property("capture_on_movement", False)

    comp.set_editor_property("capture_gpu_next_render", True)
    comp.capture_scene()
    time.sleep(0.2)
    comp.capture_scene()

    file_name = safe_name(package.replace("/Game/EnvironmentAssets/", "").replace("/", "__")) + ".png"
    unreal.RenderingLibrary.export_render_target(capture_actor, rt, OUT_DIR, file_name)

    return {
        "package": package,
        "class": cls,
        "name": os.path.basename(package),
        "image": os.path.join(OUT_DIR, file_name),
        "extent": [extent.x, extent.y, extent.z],
    }


def main():
    os.makedirs(OUT_DIR, exist_ok=True)
    unreal.EditorLoadingAndSavingUtils.new_blank_map(False)

    results = []
    failures = []
    for package, cls in collect_mesh_assets():
        try:
            result = render_mesh(package, cls)
            results.append(result)
            unreal.log("FRONTVIEW_RENDERED " + result["image"])
        except Exception as exc:
            failures.append({"package": package, "class": cls, "error": str(exc)})
            unreal.log_error("FRONTVIEW_FAILED {} {}".format(package, exc))

    index_path = os.path.join(OUT_DIR, "index.json")
    with open(index_path, "w", encoding="utf-8") as f:
        json.dump({"renders": results, "failures": failures}, f, indent=2)
    unreal.log("FRONTVIEW_INDEX " + index_path)


main()
