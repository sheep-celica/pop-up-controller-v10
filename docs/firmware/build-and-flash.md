# Build and Flash

There are two main ways to work with the firmware.

## End-User Path

For most users, the normal path is:

1. download a released firmware bundle from the firmware repo Releases page
2. use the [Pop-up Controller V10 Application](https://github.com/sheep-celica/Pop-up-controller-V10-Application) to flash it

Related pages:

- [Application](../application.md)
- [Firmware Releases](https://github.com/sheep-celica/pop-up-controller-v10/releases)

Release bundles are produced automatically when a GitHub release is published. The release workflow reads the release tag, turns a tag like `v1.1.0` into firmware version `1.1.0`, generates a fresh UTC build timestamp, builds the PlatformIO firmware, exports the flash bundle, and uploads the resulting zip to that GitHub release.

Release bundles contain firmware images for every supported board environment. The application should identify the connected controller by board ID, read the bundle manifest, and flash the matching board image.

## Developer Path

If you want to build from source, this is a PlatformIO project with one environment per supported board target.

Current board environments:

- `pop-up-controller-v10-rev-c`
- `pop-up-controller-v10-rev-d`
- `pop-up-controller-v10-rev-d-esp32-s3`

Typical commands:

```bash
pio run
pio run -e pop-up-controller-v10-rev-c
pio run -e pop-up-controller-v10-rev-d
pio run -e pop-up-controller-v10-rev-d-esp32-s3
pio run -e pop-up-controller-v10-rev-d-esp32-s3 -t upload
pio device monitor -b 115200
```

`pio run` uses the default environment from [platformio.ini](../../platformio.ini). Use `-e` when you need to build or upload a specific board target.

Local developer builds do not require any manual version-bump step. Release builds take their version from the GitHub release tag, while local builds fall back to `dev` unless you override the build metadata through environment variables.

## Flash Bundle Export Script

This repo includes [scripts/export_flash_bundle.py](../../scripts/export_flash_bundle.py), which builds board-specific firmware images and packages the files needed for ESP32-family flashing:

- `bootloader.bin`
- `partitions.bin`
- `boot_app0.bin`
- `firmware.bin`

It also writes a top-level manifest and per-board `esptool` command files to help with application-driven and manual flashing workflows.

By default, the export script builds all release board environments:

```bash
python scripts/export_flash_bundle.py --build-version dev
```

To export one board target for local work:

```bash
python scripts/export_flash_bundle.py --env pop-up-controller-v10-rev-d-esp32-s3 --build-version dev
```

The ESP32-S3 target uses `--chip esp32s3` in its generated manual flashing command.

## Release Bundle Layout

Release archives use one top-level manifest plus one board folder per firmware image set:

```text
flash_manifest.json
boards/
  pop-up-controller-v10-rev-c/
    bootloader.bin
    partitions.bin
    boot_app0.bin
    firmware.bin
    esptool_command.txt
  pop-up-controller-v10-rev-d/
    bootloader.bin
    partitions.bin
    boot_app0.bin
    firmware.bin
    esptool_command.txt
  pop-up-controller-v10-rev-d-esp32-s3/
    bootloader.bin
    partitions.bin
    boot_app0.bin
    firmware.bin
    esptool_command.txt
```

Each manifest board entry includes:

- board ID
- display name
- PlatformIO environment name
- esptool chip type, such as `esp32` or `esp32s3`
- build version
- build timestamp
- flash files and offsets

The board ID is the app-facing selection key and should match the firmware's compile-time `config::board::ID`.

## Manual Flashing

Manual flashing is still possible from an extracted release archive:

1. choose the folder whose name matches the controller board ID
2. open that folder
3. run the command in `esptool_command.txt`

For example, Revision D ESP32-S3 bundles generate a command using:

```bash
python -m esptool --chip esp32s3 --baud 460800 write_flash ...
```

## Creating A Release Build

Create or publish the release manually on GitHub with a tag in the format `vMAJOR.MINOR.PATCH`, for example `v1.1.0`.

That workflow will:

1. read the GitHub release tag
2. set the firmware version to that tag without the leading `v`
3. generate a fresh UTC build timestamp
4. build each supported board environment
5. export one multi-board firmware bundle
6. attach the generated zip to the published GitHub release

## Compatibility Notes

Older tooling may expect a single-board bundle with firmware files at the archive root. New release bundles are manifest-driven and store board files under `boards/<board-id>/`.

During the transition, flashing tools should support both shapes where practical:

- legacy single-board bundles with root-level flash files
- current multi-board bundles with a top-level `boards` manifest

Manual flashing should use the generated per-board `esptool_command.txt` file so the correct chip type and offsets are used.
