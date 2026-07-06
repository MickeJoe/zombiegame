import json
import os
import unreal

classes = [
    "RenderingLibrary",
    "GeometryScriptRenderCaptureTextures",
    "GeometryScriptRenderCaptureCamera",
    "GeometryScriptRenderCaptureCamerasForBoxOptions",
    "GeometryScript_SceneUtils",
    "TextureRenderTarget2D",
    "SceneCapture2D",
    "SceneCaptureComponent2D",
    "StaticMeshActor",
    "SkeletalMeshActor",
    "EditorLevelLibrary",
    "EditorActorSubsystem",
    "EditorLoadingAndSavingUtils",
    "AutomationLibrary",
    "AutomationEditorTask",
]

data = {}
for cls_name in classes:
    cls = getattr(unreal, cls_name, None)
    if cls is None:
        data[cls_name] = None
        continue
    data[cls_name] = [name for name in dir(cls) if not name.startswith("_")]

out_path = os.path.join(unreal.Paths.project_saved_dir(), "render_methods_probe.json")
with open(out_path, "w", encoding="utf-8") as f:
    json.dump(data, f, indent=2, sort_keys=True)

unreal.log("RENDER_METHODS_PROBE " + out_path)
