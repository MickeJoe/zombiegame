import re
from collections import Counter, defaultdict

import unreal


ROOT = "/Game/EnvironmentAssets"

BASE_KEYS = ("basecolor", "base_color", "basecolo", "diffuse", "_d", "albedo")
NORMAL_KEYS = ("normal", "_n", "bump")
ROUGHNESS_KEYS = ("roughness", "_r")
METALLIC_KEYS = ("metallic", "metalness", "_m")
AO_KEYS = ("occlusion", "occlusio", "_ao", "ambientocclusion")
SPECULAR_KEYS = ("specular", "_s")

TOKEN_RE = re.compile(r"[^a-z0-9]+")


def norm(value):
    return TOKEN_RE.sub("_", value.lower()).strip("_")


def package_path(path):
    head, tail = path.rsplit("/", 1)
    if "." in tail:
        tail = tail.split(".", 1)[0]
    return f"{head}/{tail}"


def basename(path):
    return package_path(path).rsplit("/", 1)[-1]


def folder(path):
    return package_path(path).rsplit("/", 1)[0]


def load(path):
    return unreal.EditorAssetLibrary.load_asset(package_path(path))


def is_instance(obj, class_name):
    return obj and obj.get_class().get_name() == class_name


def asset_class(obj):
    return obj.get_class().get_name() if obj else ""


def score_name(candidate, material_name, group_name):
    c = norm(candidate)
    m = norm(material_name)
    g = norm(group_name)
    score = 0
    if m and m in c:
        score += 120
    if c and c in m:
        score += 60
    for part in m.split("_"):
        if len(part) > 2 and part in c:
            score += 12
    if g and g in c:
        score += 35
    for part in g.split("_"):
        if len(part) > 3 and part in c:
            score += 8
    return score


def texture_kind(asset_name):
    n = norm(asset_name)
    checks = (
        ("base", BASE_KEYS),
        ("normal", NORMAL_KEYS),
        ("roughness", ROUGHNESS_KEYS),
        ("metallic", METALLIC_KEYS),
        ("ao", AO_KEYS),
        ("specular", SPECULAR_KEYS),
    )
    for kind, keys in checks:
        for key in keys:
            if key.startswith("_"):
                if n.endswith(key) or f"{key}_" in n or f"_{key[1:]}_" in n:
                    return kind
            elif key in n:
                return kind
    return None


def texture_prefix(asset_name, kind):
    n = norm(asset_name)
    suffixes = {
        "base": BASE_KEYS,
        "normal": NORMAL_KEYS,
        "roughness": ROUGHNESS_KEYS,
        "metallic": METALLIC_KEYS,
        "ao": AO_KEYS,
        "specular": SPECULAR_KEYS,
    }[kind]
    best = n
    for suffix in suffixes:
        s = suffix.strip("_")
        patterns = (f"_{s}", s)
        for pattern in patterns:
            idx = n.rfind(pattern)
            if idx > 0:
                best = n[:idx].strip("_")
                return best
    return best


def collect_assets():
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    registry.scan_paths_synchronous([ROOT], force_rescan=True, ignore_deny_list_scan_filters=True)
    paths = unreal.EditorAssetLibrary.list_assets(ROOT, recursive=True, include_folder=False)
    assets = {}
    for path in paths:
        path = package_path(path)
        asset = load(path)
        if asset:
            assets[path] = asset
    return assets


def group_textures(texture_paths):
    groups = defaultdict(lambda: defaultdict(list))
    for path in texture_paths:
        name = basename(path)
        kind = texture_kind(name)
        if not kind:
            continue
        prefix = texture_prefix(name, kind)
        groups[folder(path)][prefix].append((kind, path))
    return groups


def best_texture_set(material_path, material_name, texture_groups):
    candidates = []
    mat_folder = folder(material_path)
    for group_folder, groups in texture_groups.items():
        if group_folder == mat_folder or group_folder.startswith(mat_folder + "/") or mat_folder.startswith(group_folder + "/"):
            proximity = 40 if group_folder == mat_folder else 10
            for group_name, entries in groups.items():
                by_kind = {}
                for kind, path in entries:
                    current = by_kind.get(kind)
                    if not current or len(path) < len(current):
                        by_kind[kind] = path
                score = proximity + score_name(group_name, material_name, basename(group_folder))
                score += 25 * int("base" in by_kind)
                score += 18 * int("normal" in by_kind)
                score += 10 * int("roughness" in by_kind)
                score += 8 * int("metallic" in by_kind)
                score += 6 * int("ao" in by_kind)
                candidates.append((score, group_name, by_kind))
    if not candidates:
        return None, {}
    candidates.sort(key=lambda item: item[0], reverse=True)
    best_score, group_name, textures = candidates[0]
    if best_score < 45:
        return None, {}
    return group_name, textures


def set_texture_import_properties(texture, kind):
    if kind != "base":
        try:
            texture.set_editor_property("srgb", False)
        except Exception:
            pass
    if kind == "normal":
        try:
            texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
        except Exception:
            pass
    unreal.EditorAssetLibrary.save_loaded_asset(texture)


def add_texture_sample(material, texture, x, y):
    node = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionTextureSample, x, y
    )
    node.set_editor_property("texture", texture)
    return node


def connect(material, node, output_name, prop):
    try:
        unreal.MaterialEditingLibrary.connect_material_property(node, output_name, prop)
    except Exception:
        unreal.log_warning(f"Could not connect {node.get_name()}:{output_name} to {prop} on {material.get_name()}")


def rebuild_material(material, textures):
    unreal.MaterialEditingLibrary.delete_all_material_expressions(material)

    if "base" in textures:
        tex = load(textures["base"])
        set_texture_import_properties(tex, "base")
        node = add_texture_sample(material, tex, -560, -220)
        connect(material, node, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)

    if "normal" in textures:
        tex = load(textures["normal"])
        set_texture_import_properties(tex, "normal")
        node = add_texture_sample(material, tex, -560, 20)
        connect(material, node, "RGB", unreal.MaterialProperty.MP_NORMAL)

    if "roughness" in textures:
        tex = load(textures["roughness"])
        set_texture_import_properties(tex, "roughness")
        node = add_texture_sample(material, tex, -560, 260)
        connect(material, node, "R", unreal.MaterialProperty.MP_ROUGHNESS)

    if "metallic" in textures:
        tex = load(textures["metallic"])
        set_texture_import_properties(tex, "metallic")
        node = add_texture_sample(material, tex, -560, 500)
        connect(material, node, "R", unreal.MaterialProperty.MP_METALLIC)

    if "ao" in textures:
        tex = load(textures["ao"])
        set_texture_import_properties(tex, "ao")
        node = add_texture_sample(material, tex, -560, 740)
        connect(material, node, "R", unreal.MaterialProperty.MP_AMBIENT_OCCLUSION)

    if "specular" in textures:
        tex = load(textures["specular"])
        set_texture_import_properties(tex, "specular")
        node = add_texture_sample(material, tex, -560, 980)
        connect(material, node, "R", unreal.MaterialProperty.MP_SPECULAR)

    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material)


def ensure_parent_material(instance_path, textures):
    instance_name = basename(instance_path)
    package_path = f"{folder(instance_path)}/GeneratedMaterials"
    asset_name = f"M_{instance_name}_Textured"
    asset_path = f"{package_path}/{asset_name}"

    unreal.EditorAssetLibrary.make_directory(package_path)
    if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        material = load(asset_path)
    else:
        material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            asset_name, package_path, unreal.Material, unreal.MaterialFactoryNew()
        )
    rebuild_material(material, textures)
    return material


def fix_material_instances(material_instance_paths, texture_groups):
    fixed = []
    skipped = []
    for instance_path in sorted(material_instance_paths):
        instance = load(instance_path)
        group_name, textures = best_texture_set(instance_path, basename(instance_path), texture_groups)
        if not textures:
            skipped.append(instance_path)
            continue
        parent = ensure_parent_material(instance_path, textures)
        instance.set_editor_property("parent", parent)
        unreal.EditorAssetLibrary.save_loaded_asset(instance)
        fixed.append((instance_path, parent.get_path_name(), group_name, textures))
    return fixed, skipped


def assign_materials_to_meshes(mesh_paths, material_paths):
    mats_by_folder = defaultdict(list)
    for path in material_paths:
        mats_by_folder[folder(path)].append(load(path))

    changed = 0
    for path in mesh_paths:
        mesh = load(path)
        if not mesh:
            continue
        mesh_folder = folder(path)
        candidates = mats_by_folder.get(mesh_folder, [])
        if not candidates:
            continue
        try:
            slots = mesh.get_editor_property("static_materials")
        except Exception:
            continue
        updated = False
        for i, slot in enumerate(slots):
            current = slot.get_editor_property("material_interface")
            if current:
                continue
            if i < len(candidates):
                slot.set_editor_property("material_interface", candidates[i])
                updated = True
        if updated:
            mesh.set_editor_property("static_materials", slots)
            unreal.EditorAssetLibrary.save_loaded_asset(mesh)
            changed += 1
    return changed


def main():
    assets = collect_assets()
    class_counts = Counter(asset_class(asset) for asset in assets.values())
    material_paths = []
    material_instance_paths = []
    texture_paths = []
    static_mesh_paths = []

    for path, asset in assets.items():
        if isinstance(asset, unreal.Material):
            material_paths.append(path)
        elif isinstance(asset, unreal.MaterialInstanceConstant):
            material_instance_paths.append(path)
        elif isinstance(asset, unreal.Texture2D):
            texture_paths.append(path)
        elif isinstance(asset, unreal.StaticMesh):
            static_mesh_paths.append(path)

    unreal.log(f"Environment assets scanned: {len(assets)}")
    unreal.log(f"  Classes: {dict(sorted(class_counts.items()))}")
    unreal.log(f"  Materials: {len(material_paths)}")
    unreal.log(f"  Material instances: {len(material_instance_paths)}")
    unreal.log(f"  Textures: {len(texture_paths)}")
    unreal.log(f"  Static meshes: {len(static_mesh_paths)}")

    texture_groups = group_textures(texture_paths)
    fixed = []
    skipped = []

    for material_path in sorted(material_paths):
        material = load(material_path)
        group_name, textures = best_texture_set(material_path, basename(material_path), texture_groups)
        if not textures:
            skipped.append(material_path)
            continue
        rebuild_material(material, textures)
        fixed.append((material_path, group_name, textures))

    fixed_instances, skipped_instances = fix_material_instances(material_instance_paths, texture_groups)

    meshes_changed = assign_materials_to_meshes(static_mesh_paths, material_paths + material_instance_paths)
    unreal.EditorAssetLibrary.save_directory(ROOT, only_if_is_dirty=True, recursive=True)

    unreal.log("Environment texture fix complete")
    unreal.log(f"Materials fixed: {len(fixed)}")
    for material_path, group_name, textures in fixed:
        unreal.log(f"  {material_path} <- {group_name}: {sorted(textures.keys())}")
    unreal.log(f"Materials skipped (no confident texture set): {len(skipped)}")
    for material_path in skipped:
        unreal.log(f"  skipped {material_path}")
    unreal.log(f"Material instances fixed: {len(fixed_instances)}")
    for instance_path, parent_path, group_name, textures in fixed_instances:
        unreal.log(f"  {instance_path} parent={parent_path} <- {group_name}: {sorted(textures.keys())}")
    unreal.log(f"Material instances skipped (no confident texture set): {len(skipped_instances)}")
    for instance_path in skipped_instances:
        unreal.log(f"  skipped {instance_path}")
    unreal.log(f"Meshes with missing material slots filled: {meshes_changed}")


main()
