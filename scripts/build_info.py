#!/usr/bin/env python3

from __future__ import annotations

import argparse
import os
import re
import subprocess
from datetime import datetime, timezone
from pathlib import Path


SEMVER_PATTERN = re.compile(r"^\d+\.\d+\.\d+$")
RELEASE_TAG_PATTERN = re.compile(r"^v(\d+\.\d+\.\d+)$")
DEFAULT_BUILD_VERSION = "dev"


def default_project_root() -> Path:
    return Path(__file__).resolve().parents[1]


def default_archive_name(build_version: str) -> str:
    return f"pop_up_controller_v10_firmware_v_{build_version}.zip"


def current_utc_timestamp() -> str:
    return datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")


def normalize_release_tag(tag: str) -> str:
    match = RELEASE_TAG_PATTERN.fullmatch(tag.strip())
    if not match:
        raise ValueError(
            "Expected release tag format vMAJOR.MINOR.PATCH, for example v1.1.0."
        )

    return match.group(1)


def validate_build_version(build_version: str) -> str:
    version = build_version.strip()
    if version == DEFAULT_BUILD_VERSION:
        return version

    if not SEMVER_PATTERN.fullmatch(version):
        raise ValueError(
            "Expected build version format MAJOR.MINOR.PATCH, or 'dev' for local builds."
        )

    return version


def git_exact_tag(project_root: Path) -> str | None:
    try:
        result = subprocess.run(
            ["git", "describe", "--tags", "--exact-match"],
            cwd=project_root,
            check=True,
            capture_output=True,
            text=True,
        )
    except (OSError, subprocess.CalledProcessError):
        return None

    tag = result.stdout.strip()
    if not tag:
        return None

    return tag


def resolve_build_info(
    project_root: Path | None = None,
    *,
    release_tag: str | None = None,
    build_version: str | None = None,
    build_timestamp: str | None = None,
) -> dict[str, str]:
    project_root = project_root or default_project_root()

    if release_tag and build_version:
        raise ValueError("Provide either release_tag or build_version, not both.")

    version_from_env = os.environ.get("POP_UP_BUILD_VERSION")
    timestamp_from_env = os.environ.get("POP_UP_BUILD_TIMESTAMP")

    if release_tag:
        resolved_build_version = normalize_release_tag(release_tag)
    elif build_version:
        resolved_build_version = validate_build_version(build_version)
    elif version_from_env:
        resolved_build_version = validate_build_version(version_from_env)
    else:
        exact_tag = git_exact_tag(project_root)
        if exact_tag:
            try:
                resolved_build_version = normalize_release_tag(exact_tag)
            except ValueError:
                resolved_build_version = DEFAULT_BUILD_VERSION
        else:
            resolved_build_version = DEFAULT_BUILD_VERSION

    resolved_build_timestamp = (
        build_timestamp
        or timestamp_from_env
        or current_utc_timestamp()
    )

    return {
        "build_version": resolved_build_version,
        "build_timestamp": resolved_build_timestamp,
        "archive_name": default_archive_name(resolved_build_version),
    }


def write_github_output(build_info: dict[str, str]) -> None:
    github_output = os.environ.get("GITHUB_OUTPUT")
    if not github_output:
        raise RuntimeError("--github-output was requested but GITHUB_OUTPUT is not set.")

    with open(github_output, "a", encoding="utf-8") as output_file:
        output_file.write(f"build_version={build_info['build_version']}\n")
        output_file.write(f"build_timestamp={build_info['build_timestamp']}\n")
        output_file.write(f"archive_name={build_info['archive_name']}\n")


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Resolve firmware build metadata for local and release builds."
    )
    parser.add_argument(
        "--tag",
        dest="release_tag",
        help="Release tag in the format vMAJOR.MINOR.PATCH",
    )
    parser.add_argument(
        "--build-version",
        help="Explicit firmware version to use instead of deriving it from a release tag",
    )
    parser.add_argument(
        "--build-timestamp",
        help="Explicit UTC build timestamp to use",
    )
    parser.add_argument(
        "--github-output",
        action="store_true",
        help="Write resolved metadata to the GitHub Actions output file",
    )
    args = parser.parse_args()

    build_info = resolve_build_info(
        default_project_root(),
        release_tag=args.release_tag,
        build_version=args.build_version,
        build_timestamp=args.build_timestamp,
    )

    if args.github_output:
        write_github_output(build_info)

    print(
        "[build-info] "
        f"BUILD_VERSION={build_info['build_version']} "
        f"BUILD_TIMESTAMP={build_info['build_timestamp']}"
    )


if __name__ == "__main__":
    main()
