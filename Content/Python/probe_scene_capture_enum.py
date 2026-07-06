import json
import os
import unreal

enum_names = {}
for enum_cls_name in ("SceneCaptureSource", "SceneCaptureUnlitViewmode", "CameraProjectionMode"):
    enum_cls = getattr(unreal, enum_cls_name)
    enum_names[enum_cls_name] = [name for name in dir(enum_cls) if name.isupper() or name.startswith("SCS_")]

out_path = os.path.join(unreal.Paths.project_saved_dir(), "scene_capture_enum_probe.json")
with open(out_path, "w", encoding="utf-8") as f:
    json.dump(enum_names, f, indent=2)
unreal.log("SCENE_CAPTURE_ENUM_PROBE " + out_path)
