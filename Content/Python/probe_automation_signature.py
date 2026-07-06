import inspect
import json
import os
import unreal

names = [
    "take_high_res_screenshot",
    "take_automation_screenshot_at_camera",
    "finish_loading_before_screenshot",
]

data = {}
for name in names:
    fn = getattr(unreal.AutomationLibrary, name)
    try:
        sig = str(inspect.signature(fn))
    except Exception as exc:
        sig = "ERR " + str(exc)
    data[name] = {
        "signature": sig,
        "doc": getattr(fn, "__doc__", ""),
    }

out_path = os.path.join(unreal.Paths.project_saved_dir(), "automation_signature_probe.json")
with open(out_path, "w", encoding="utf-8") as f:
    json.dump(data, f, indent=2)
unreal.log("AUTOMATION_SIGNATURE_PROBE " + out_path)
