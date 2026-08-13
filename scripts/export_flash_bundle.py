#!/usr/bin/env python3

import argparse
import json
import os
import shutil
import subprocess
import sys
import tempfile
import zipfile
from datetime import datetime, timezone
from dataclasses import dataclass
from pathlib import Path

from build_info import default_archive_name, resolve_build_info

FLASH_LAYOUT = [
    ("0x1000", "bootloader.bin"),
    ("0x8000", "partitions.bin"),
    ("0xE000", "boot_app0.bin"),
    ("0x10000", "firmware.bin"),
]

MANIFEST_FORMAT_VERSION = 2


@dataclass(frozen=True)
class BoardTarget:
    env_name: str
    board_id: str
    display_name: str
    chip: str


BOARD_TARGETS = {
    "pop-up-controller-v10-rev-c": BoardTarget(
        env_name="pop-up-controller-v10-rev-c",
        board_id="pop-up-controller-v10-rev-c",
        display_name="Pop-up Controller V10 Revision C",
        chip="esp32",
    ),
    "pop-up-controller-v10-rev-e": BoardTarget(
        env_name="pop-up-controller-v10-rev-e",
        board_id="pop-up-controller-v10-rev-e",
        display_name="Pop-up Controller V10 Revision E",
        chip="esp32",
    ),
    "pop-up-controller-v10-rev-e-esp32-s3": BoardTarget(
        env_name="pop-up-controller-v10-rev-e-esp32-s3",
        board_id="pop-up-controller-v10-rev-e-esp32-s3",
        display_name="Pop-up Controller V10 Revision E ESP32-S3",
        chip="esp32s3",
    ),
}

DEFAULT_ENV_NAMES = tuple(BOARD_TARGETS.keys())


def candidate_platformio_paths():
    override = os.environ.get("PLATFORMIO_EXE")
    if override:
        yield Path(override)

    for executable in ("platformio", "pio"):
        resolved = shutil.which(executable)
        if resolved:
            yield Path(resolved)

    home = Path.home()
    yield home / ".platformio" / "penv" / "Scripts" / "platformio.exe"
    yield home / ".platformio" / "penv" / "Scripts" / "pio.exe"
    yield home / ".platformio" / "penv" / "bin" / "platformio"
    yield home / ".platformio" / "penv" / "bin" / "pio"


def find_platformio_executable() -> Path:
    for candidate in candidate_platformio_paths():
        if candidate.is_file():
            return candidate

    raise FileNotFoundError(
        "Could not find a PlatformIO executable. "
        "Set PLATFORMIO_EXE or install PlatformIO."
    )


def run_build(
    project_root: Path,
    target: BoardTarget,
    platformio_exe: Path,
    build_info: dict[str, str],
):
    command = [str(platformio_exe), "run", "-e", target.env_name]
    print("[flash-bundle] Running:", " ".join(command), flush=True)
    build_env = os.environ.copy()
    build_env["POP_UP_BUILD_VERSION"] = build_info["build_version"]
    build_env["POP_UP_BUILD_TIMESTAMP"] = build_info["build_timestamp"]
    subprocess.run(command, cwd=project_root, check=True, env=build_env)


def find_boot_app0() -> Path:
    candidate = (
        Path.home()
        / ".platformio"
        / "packages"
        / "framework-arduinoespressif32"
        / "tools"
        / "partitions"
        / "boot_app0.bin"
    )
    if candidate.is_file():
        return candidate

    raise FileNotFoundError(
        "Could not find boot_app0.bin in the PlatformIO framework package."
    )


def board_directory_name(target: BoardTarget) -> str:
    return target.board_id


def copy_required_files(project_root: Path, target: BoardTarget, board_dir: Path):
    build_dir = project_root / ".pio" / "build" / target.env_name
    source_map = {
        "bootloader.bin": build_dir / "bootloader.bin",
        "partitions.bin": build_dir / "partitions.bin",
        "firmware.bin": build_dir / "firmware.bin",
        "boot_app0.bin": find_boot_app0(),
    }

    board_dir.mkdir(parents=True, exist_ok=True)

    for _, filename in FLASH_LAYOUT:
        source = source_map[filename]
        if not source.is_file():
            raise FileNotFoundError(f"Required artifact not found: {source}")

        destination = board_dir / filename
        shutil.copy2(source, destination)
        print(f"[flash-bundle] Copied {source} -> {destination}", flush=True)


def manifest_entry(
    target: BoardTarget,
    build_info: dict[str, str],
):
    board_path = f"boards/{board_directory_name(target)}"
    return {
        "board_id": target.board_id,
        "display_name": target.display_name,
        "environment": target.env_name,
        "chip": target.chip,
        "path": board_path,
        "build_version": build_info["build_version"],
        "build_timestamp": build_info["build_timestamp"],
        "flash_files": [
            {
                "offset": offset,
                "file": filename,
                "path": f"{board_path}/{filename}",
            }
            for offset, filename in FLASH_LAYOUT
        ],
    }


def write_manifest(
    project_root: Path,
    staging_dir: Path,
    targets: list[BoardTarget],
    build_info: dict[str, str],
):
    manifest = {
        "manifest_format": MANIFEST_FORMAT_VERSION,
        "project": project_root.name,
        "generated_at_utc": datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ"),
        "build_version": build_info["build_version"],
        "build_timestamp": build_info["build_timestamp"],
        "boards": [manifest_entry(target, build_info) for target in targets],
    }

    manifest_path = staging_dir / "flash_manifest.json"
    manifest_path.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    print(f"[flash-bundle] Wrote {manifest_path}", flush=True)


def write_esptool_command(board_dir: Path, target: BoardTarget):
    command = (
        f"python -m esptool --chip {target.chip} --baud 460800 write_flash "
        "0x1000 bootloader.bin "
        "0x8000 partitions.bin "
        "0xE000 boot_app0.bin "
        "0x10000 firmware.bin"
    )

    content = (
        "Run this from the directory where you extracted this archive:\n"
        f"{command}\n"
    )

    command_path = board_dir / "esptool_command.txt"
    command_path.write_text(content, encoding="utf-8")
    print(f"[flash-bundle] Wrote {command_path}", flush=True)


def clean_output_directory(output_dir: Path):
    output_dir.mkdir(parents=True, exist_ok=True)

    for child in output_dir.iterdir():
        if child.is_dir():
            shutil.rmtree(child)
        else:
            child.unlink()


def create_archive(staging_dir: Path, archive_path: Path):
    with zipfile.ZipFile(archive_path, "w", compression=zipfile.ZIP_DEFLATED) as archive:
        for file_path in sorted(path for path in staging_dir.rglob("*") if path.is_file()):
            archive.write(file_path, arcname=file_path.relative_to(staging_dir))

    print(f"[flash-bundle] Wrote {archive_path}", flush=True)


def resolve_targets(env_names: list[str] | None) -> list[BoardTarget]:
    requested_envs = env_names or list(DEFAULT_ENV_NAMES)
    targets = []
    unknown_envs = []

    for env_name in requested_envs:
        target = BOARD_TARGETS.get(env_name)
        if not target:
            unknown_envs.append(env_name)
            continue

        targets.append(target)

    if unknown_envs:
        known = ", ".join(BOARD_TARGETS.keys())
        unknown = ", ".join(unknown_envs)
        raise ValueError(f"Unknown board environment(s): {unknown}. Known environments: {known}")

    return targets


def main():
    parser = argparse.ArgumentParser(
        description="Build the firmware and export a flashable bundle into the repo root."
    )
    parser.add_argument(
        "--env",
        action="append",
        dest="env_names",
        help=(
            "PlatformIO environment name to build. "
            "May be provided more than once. Defaults to all release board environments."
        ),
    )
    parser.add_argument(
        "--output-dir",
        default="firmware_builds",
        help="Output directory relative to the project root",
    )
    parser.add_argument(
        "--archive-name",
        default=None,
        help="Zip file name to create inside the output directory",
    )
    parser.add_argument(
        "--skip-build",
        action="store_true",
        help="Reuse the existing .pio build output instead of rebuilding",
    )
    parser.add_argument(
        "--release-tag",
        default=None,
        help="Release tag in the format vMAJOR.MINOR.PATCH",
    )
    parser.add_argument(
        "--build-version",
        default=None,
        help="Explicit firmware version to use for the build metadata",
    )
    parser.add_argument(
        "--build-timestamp",
        default=None,
        help="Explicit UTC timestamp to use for the build metadata",
    )
    args = parser.parse_args()

    project_root = Path(__file__).resolve().parents[1]
    output_dir = (project_root / args.output_dir).resolve()
    platformio_exe = find_platformio_executable()
    targets = resolve_targets(args.env_names)
    build_info = resolve_build_info(
        project_root,
        release_tag=args.release_tag,
        build_version=args.build_version,
        build_timestamp=args.build_timestamp,
    )
    archive_name = args.archive_name or default_archive_name(build_info["build_version"])

    if not args.skip_build:
        for target in targets:
            run_build(project_root, target, platformio_exe, build_info)

    clean_output_directory(output_dir)

    with tempfile.TemporaryDirectory(prefix="flash_bundle_", dir=project_root) as temp_dir:
        staging_dir = Path(temp_dir)
        for target in targets:
            board_dir = staging_dir / "boards" / board_directory_name(target)
            copy_required_files(project_root, target, board_dir)
            write_esptool_command(board_dir, target)

        write_manifest(project_root, staging_dir, targets, build_info)
        create_archive(staging_dir, output_dir / archive_name)

    print(f"[flash-bundle] Bundle ready at: {output_dir}", flush=True)


if __name__ == "__main__":
    try:
        main()
    except subprocess.CalledProcessError as exc:
        sys.exit(exc.returncode)
    except Exception as exc:
        print(f"[flash-bundle] ERROR: {exc}", file=sys.stderr)
        sys.exit(1)
