# Build and Flash

There are two main ways to work with the firmware.

## End-User Path

For most users, the normal path is:

1. download a released firmware bundle from the firmware repo Releases page
2. use the [Pop-up Controller V10 Application](https://github.com/sheep-celica/Pop-up-controller-V10-Application) to flash it

Related pages:

- [Application](../application.md)
- [Firmware Releases](https://github.com/sheep-celica/pop-up-controller-v10/releases)

## Developer Path

If you want to build from source, this is a PlatformIO project targeting `esp32doit-devkit-v1`.

Typical commands:

```bash
pio run
pio run -t upload
pio device monitor -b 115200
```

## Flash Bundle Export Script

This repo includes [scripts/export_flash_bundle.py](../../scripts/export_flash_bundle.py), which packages the files needed for ESP32 flashing:

- `bootloader.bin`
- `partitions.bin`
- `boot_app0.bin`
- `firmware.bin`

It also writes a manifest and an `esptool` command file to help with manual flashing workflows.
