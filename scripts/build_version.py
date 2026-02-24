Import("env")

import json
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


def cpp_string_literal(value):
    escaped = value.replace("\\", "\\\\").replace('"', '\\"')
    # For SCons/PlatformIO CPPDEFINES, the quotes need to be escaped so the
    # compiler receives a real C string literal (e.g. \"1.0.0\").
    return f'\\"{escaped}\\"'


project_dir = Path(env.subst("$PROJECT_DIR"))
state_file = project_dir / ".pio" / "build_version_state.json"

build_version = load_next_version(state_file)
build_timestamp = datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")

save_next_version(state_file, bump_patch(build_version))

env.Append(
    CPPDEFINES=[
        ("BUILD_VERSION", cpp_string_literal(build_version)),
        ("BUILD_TIMESTAMP", cpp_string_literal(build_timestamp)),
    ]
)

print(f"[build-version] BUILD_VERSION={build_version} BUILD_TIMESTAMP={build_timestamp}")
