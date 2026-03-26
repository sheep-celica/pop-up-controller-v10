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

## Developer Path

If you want to build from source, this is a PlatformIO project targeting `esp32doit-devkit-v1`.

Typical commands:

```bash
pio run
pio run -t upload
pio device monitor -b 115200
```

Local developer builds do not require any manual version-bump step. Release builds take their version from the GitHub release tag, while local builds fall back to `dev` unless you override the build metadata through environment variables.

## Flash Bundle Export Script

This repo includes [scripts/export_flash_bundle.py](../../scripts/export_flash_bundle.py), which packages the files needed for ESP32 flashing:

- `bootloader.bin`
- `partitions.bin`
- `boot_app0.bin`
- `firmware.bin`

It also writes a manifest and an `esptool` command file to help with manual flashing workflows.

## Creating A Release Build

Create or publish the release manually on GitHub with a tag in the format `vMAJOR.MINOR.PATCH`, for example `v1.1.0`.

That workflow will:

1. read the GitHub release tag
2. set the firmware version to that tag without the leading `v`
3. generate a fresh UTC build timestamp
4. build the firmware bundle
5. attach the generated zip to the published GitHub release
