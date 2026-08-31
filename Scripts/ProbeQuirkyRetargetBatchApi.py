from __future__ import annotations

import unreal


def main() -> None:
    func = unreal.IKRetargetBatchOperation.duplicate_and_retarget
    unreal.log(str(func))
    unreal.log(repr(func))
    try:
        unreal.log(func.__doc__ or "<no doc>")
    except Exception as exc:
        unreal.log_warning(f"doc unavailable: {exc}")
    try:
        unreal.log(str(unreal.IKRetargetBatchOperation.run_batch_retarget))
        unreal.log(unreal.IKRetargetBatchOperation.run_batch_retarget.__doc__ or "<no doc>")
    except Exception as exc:
        unreal.log_warning(f"run_batch_retarget unavailable: {exc}")


if __name__ == "__main__":
    main()
