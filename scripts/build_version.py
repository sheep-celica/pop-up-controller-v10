Import("env")

import json
import re
from datetime import datetime, timezone
from pathlib import Path


DEFAULT_VERSION = "1.0.0"


def parse_version(version_text):
    parts = version_text.strip().split(".")
    if len(parts) != 3:
        raise ValueError("Expected semantic version format MAJOR.MINOR.PATCH")
    major, minor, patch = (int(part) for part in parts)
    if major < 0 or minor < 0 or patch < 0:
        raise ValueError("Version parts must be non-negative")
    return major, minor, patch


def bump_patch(version_text):
    major, minor, patch = parse_version(version_text)
    return f"{major}.{minor}.{patch + 1}"


def load_next_version(state_file):
    if not state_file.exists():
        return DEFAULT_VERSION

    try:
        data = json.loads(state_file.read_text(encoding="utf-8"))
        version_text = str(data.get("next_version", DEFAULT_VERSION))
        parse_version(version_text)
        return version_text
    except Exception:
        return DEFAULT_VERSION


def save_next_version(state_file, next_version):
    state_file.parent.mkdir(parents=True, exist_ok=True)
    state_file.write_text(
        json.dumps({"next_version": next_version}, indent=2) + "\n",
        encoding="utf-8",
    )


def update_main_cpp_build_info(main_cpp_path, build_version, build_timestamp):
    text = main_cpp_path.read_text(encoding="utf-8")

    version_line = f'#define BUILD_VERSION "{build_version}"'
    timestamp_line = f'#define BUILD_TIMESTAMP "{build_timestamp}"'

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
            "Could not find BUILD_VERSION/BUILD_TIMESTAMP defines in src/main.cpp"
        )

    if text_new != text:
        main_cpp_path.write_text(text_new, encoding="utf-8")


project_dir = Path(env.subst("$PROJECT_DIR"))
state_file = project_dir / ".pio" / "build_version_state.json"
main_cpp_file = project_dir / "src" / "main.cpp"

build_version = load_next_version(state_file)
build_timestamp = datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")

update_main_cpp_build_info(main_cpp_file, build_version, build_timestamp)
save_next_version(state_file, bump_patch(build_version))

print(f"[build-version] BUILD_VERSION={build_version} BUILD_TIMESTAMP={build_timestamp}")
