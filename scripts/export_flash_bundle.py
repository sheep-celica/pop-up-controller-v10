#!/usr/bin/env python3

import argparse
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
import zipfile
from datetime import datetime, timezone
from pathlib import Path


BUILD_VERSION_PATTERN = re.compile(r'(?m)^#define\s+BUILD_VERSION\s+"([^"]+)"\s*$')
BUILD_TIMESTAMP_PATTERN = re.compile(r'(?m)^#define\s+BUILD_TIMESTAMP\s+"([^"]+)"\s*$')

FLASH_LAYOUT = [
    ("0x1000", "bootloader.bin"),
    ("0x8000", "partitions.bin"),
    ("0xE000", "boot_app0.bin"),
    ("0x10000", "firmware.bin"),
]


def parse_build_info(main_cpp_path: Path):
    text = main_cpp_path.read_text(encoding="utf-8")

    version_match = BUILD_VERSION_PATTERN.search(text)
    timestamp_match = BUILD_TIMESTAMP_PATTERN.search(text)

    return {
        "build_version": version_match.group(1) if version_match else None,
        "build_timestamp": timestamp_match.group(1) if timestamp_match else None,
    }


def default_archive_name(build_version: str | None) -> str:
    if build_version:
        return f"pop_up_controller_v10_firmware_v_{build_version}.zip"
    return "pop_up_controller_v10_firmware.zip"


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


def run_build(project_root: Path, env_name: str, platformio_exe: Path):
    command = [str(platformio_exe), "run", "-e", env_name]
    print("[flash-bundle] Running:", " ".join(command), flush=True)
    subprocess.run(command, cwd=project_root, check=True)


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


def copy_required_files(project_root: Path, env_name: str, staging_dir: Path):
    build_dir = project_root / ".pio" / "build" / env_name
    source_map = {
        "bootloader.bin": build_dir / "bootloader.bin",
        "partitions.bin": build_dir / "partitions.bin",
        "firmware.bin": build_dir / "firmware.bin",
        "boot_app0.bin": find_boot_app0(),
    }

    staging_dir.mkdir(parents=True, exist_ok=True)

    for _, filename in FLASH_LAYOUT:
        source = source_map[filename]
        if not source.is_file():
            raise FileNotFoundError(f"Required artifact not found: {source}")

        destination = staging_dir / filename
        shutil.copy2(source, destination)
        print(f"[flash-bundle] Copied {source} -> {destination}", flush=True)


def write_manifest(project_root: Path, env_name: str, staging_dir: Path):
    build_info = parse_build_info(project_root / "src" / "main.cpp")

    manifest = {
        "project": project_root.name,
        "environment": env_name,
        "generated_at_utc": datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ"),
        "build_version": build_info["build_version"],
        "build_timestamp": build_info["build_timestamp"],
        "flash_files": [
            {"offset": offset, "file": filename} for offset, filename in FLASH_LAYOUT
        ],
    }

    manifest_path = staging_dir / "flash_manifest.json"
    manifest_path.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    print(f"[flash-bundle] Wrote {manifest_path}", flush=True)


def write_esptool_command(staging_dir: Path):
    command = (
        "python -m esptool --chip esp32 --baud 460800 write_flash "
        "0x1000 bootloader.bin "
        "0x8000 partitions.bin "
        "0xE000 boot_app0.bin "
        "0x10000 firmware.bin"
    )

    content = (
        "Run this from the directory where you extracted this archive:\n"
        f"{command}\n"
    )

    command_path = staging_dir / "esptool_command.txt"
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
        for file_path in sorted(staging_dir.iterdir()):
            archive.write(file_path, arcname=file_path.name)

    print(f"[flash-bundle] Wrote {archive_path}", flush=True)


def main():
    parser = argparse.ArgumentParser(
        description="Build the firmware and export a flashable bundle into the repo root."
    )
    parser.add_argument(
        "--env",
        default="esp32doit-devkit-v1",
        help="PlatformIO environment name to build",
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
    args = parser.parse_args()

    project_root = Path(__file__).resolve().parents[1]
    output_dir = (project_root / args.output_dir).resolve()
    platformio_exe = find_platformio_executable()
    build_info = parse_build_info(project_root / "src" / "main.cpp")
    archive_name = args.archive_name or default_archive_name(build_info["build_version"])

    if not args.skip_build:
        run_build(project_root, args.env, platformio_exe)

    clean_output_directory(output_dir)

    with tempfile.TemporaryDirectory(prefix="flash_bundle_", dir=project_root) as temp_dir:
        staging_dir = Path(temp_dir)
        copy_required_files(project_root, args.env, staging_dir)
        write_manifest(project_root, args.env, staging_dir)
        write_esptool_command(staging_dir)
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
