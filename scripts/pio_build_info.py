Import("env")

import sys
from pathlib import Path


project_dir = Path(env.subst("$PROJECT_DIR"))
scripts_dir = project_dir / "scripts"
if str(scripts_dir) not in sys.path:
    sys.path.insert(0, str(scripts_dir))

from build_info import resolve_build_info


build_info = resolve_build_info(project_dir)

env.Append(
    CPPDEFINES=[
        ("BUILD_VERSION", env.StringifyMacro(build_info["build_version"])),
        ("BUILD_TIMESTAMP", env.StringifyMacro(build_info["build_timestamp"])),
    ]
)

print(
    "[build-info] "
    f"BUILD_VERSION={build_info['build_version']} "
    f"BUILD_TIMESTAMP={build_info['build_timestamp']}",
    flush=True,
)
