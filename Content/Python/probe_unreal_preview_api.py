import json
import os
import unreal

names = dir(unreal)
keywords = [
    "thumbnail",
    "screenshot",
    "capture",
    "render",
    "scene",
    "camera",
    "asset",
]

matches = {}
for keyword in keywords:
    matches[keyword] = [name for name in names if keyword.lower() in name.lower()]

out_path = os.path.join(unreal.Paths.project_saved_dir(), "unreal_preview_api_probe.json")
with open(out_path, "w", encoding="utf-8") as f:
    json.dump(matches, f, indent=2, sort_keys=True)

unreal.log("PREVIEW_API_PROBE " + out_path)
