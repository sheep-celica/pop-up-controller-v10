#!/usr/bin/env python3

import argparse
import re
from datetime import datetime, timezone
from pathlib import Path


VERSION_PATTERN = re.compile(r'(?m)^#define\s+BUILD_VERSION\s+"([^"]+)"\s*$')
TIMESTAMP_PATTERN = re.compile(r'(?m)^#define\s+BUILD_TIMESTAMP\s+"([^"]+)"\s*$')


def parse_version(version_text: str):
    parts = version_text.strip().split(".")
    if len(parts) != 3:
        raise ValueError("Expected semantic version format MAJOR.MINOR.PATCH")

    major, minor, patch = (int(part) for part in parts)
    if major < 0 or minor < 0 or patch < 0:
        raise ValueError("Version parts must be non-negative")

    return major, minor, patch


def bump_patch(version_text: str) -> str:
    major, minor, patch = parse_version(version_text)
    return f"{major}.{minor}.{patch + 1}"


def update_main_cpp_build_info(main_cpp_path: Path):
    text = main_cpp_path.read_text(encoding="utf-8")

    version_match = VERSION_PATTERN.search(text)
    timestamp_match = TIMESTAMP_PATTERN.search(text)
    if not version_match or not timestamp_match:
        raise RuntimeError(
            "Could not find BUILD_VERSION/BUILD_TIMESTAMP defines in src/main.cpp"
        )

    previous_version = version_match.group(1)
    next_version = bump_patch(previous_version)
    next_timestamp = datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")

    version_line = f'#define BUILD_VERSION "{next_version}"'
    timestamp_line = f'#define BUILD_TIMESTAMP "{next_timestamp}"'

    text_new, version_count = re.subn(
        r'(?m)^#define\s+BUILD_VERSION\b.*$',
        version_line,
        text,
        count=1,
    )
    text_new, timestamp_count = re.subn(
        r'(?m)^#define\s+BUILD_TIMESTAMP\b.*$',
        timestamp_line,
        text_new,
        count=1,
    )

    if version_count != 1 or timestamp_count != 1:
        raise RuntimeError(
            "Could not update BUILD_VERSION/BUILD_TIMESTAMP defines in src/main.cpp"
        )

    if text_new != text:
        main_cpp_path.write_text(text_new, encoding="utf-8")

    return previous_version, next_version, next_timestamp


def main():
    parser = argparse.ArgumentParser(
        description="Bump patch version and update BUILD_TIMESTAMP in src/main.cpp"
    )
    parser.add_argument(
        "--main",
        type=Path,
        default=(Path(__file__).resolve().parents[1] / "src" / "main.cpp"),
        help="Path to src/main.cpp",
    )
    args = parser.parse_args()

    previous_version, next_version, next_timestamp = update_main_cpp_build_info(
        args.main
    )
    print(
        f"[build-version] BUILD_VERSION {previous_version} -> {next_version}; "
        f"BUILD_TIMESTAMP={next_timestamp}"
    )


if __name__ == "__main__":
    main()
