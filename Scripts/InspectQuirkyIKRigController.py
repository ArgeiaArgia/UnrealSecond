from __future__ import annotations

import json
from pathlib import Path

import unreal


IKRIG_PATH = "/Game/Migrated/QuirkySeriesUltimate/_TempRetargetProbe/Crow_IKRig_Probe"
REPORT_PATH = Path(r"C:\Users\KGH\Gitfork\UnrealSecond\UnrealTeamProject\Saved\QuirkySeriesMigration\quirkyrig_probe.json")


def main() -> None:
    rig = unreal.EditorAssetLibrary.load_asset(IKRIG_PATH)
    if not rig:
        unreal.log_error(f"[QuirkyIKRigInspect] Missing rig: {IKRIG_PATH}")
        return

    controller = unreal.IKRigController.get_controller(rig)
    report = {
        "rig_class": rig.get_class().get_name(),
        "controller_class": controller.get_class().get_name(),
        "controller_dir": [name for name in dir(controller) if "chain" in name.lower() or "retarget" in name.lower() or "skeleton" in name.lower() or "mesh" in name.lower()],
    }

    chains = []
    try:
        chain_items = list(controller.get_retarget_chains() or [])
        for item in chain_items:
            item_report = {"type": type(item).__name__}
            for prop in ["name", "chain_name", "start_bone", "end_bone", "goal_bone"]:
                try:
                    item_report[prop] = str(item.get_editor_property(prop))
                except Exception:
                    try:
                        item_report[prop] = str(getattr(item, prop))
                    except Exception:
                        pass
            chains.append(item_report)
    except Exception as exc:
        report["chain_error"] = str(exc)

    report["chains"] = chains

    try:
        report["retarget_root"] = str(controller.get_retarget_root())
    except Exception as exc:
        report["retarget_root_error"] = str(exc)

    REPORT_PATH.parent.mkdir(parents=True, exist_ok=True)
    REPORT_PATH.write_text(json.dumps(report, indent=2), encoding="utf-8")
    unreal.log(f"[QuirkyIKRigInspect] report={REPORT_PATH}")


if __name__ == "__main__":
    main()
