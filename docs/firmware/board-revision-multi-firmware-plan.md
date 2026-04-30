# Board Revision Multi-Firmware Plan

This document captures a practical path for supporting multiple PCB revisions without forcing a large runtime board-detection refactor inside the firmware.

## Status Snapshot

As of April 23, 2026:

- compile-time board selection has now started
- the old single `include/config.h` pin map has been split behind `include/board_config.h`
- Revision C and Revision D board headers exist
- PlatformIO now has separate Revision C and Revision D firmware environments
- build info now exposes a compile-time board id and board name
- multi-board release bundles are not implemented yet
- app-side automatic firmware selection is not implemented yet

## Goal

Support at least two controller board revisions from one repository while keeping:

- one shared firmware codebase
- separate board-specific builds
- one release bundle that the desktop app can inspect
- automatic firmware selection in the app based on the connected controller's reported board identity

## Recommended Direction

Build a separate firmware image for each supported board revision, then package those images into one release archive with a manifest.

This is preferred over runtime board selection inside the firmware because the current codebase uses compile-time configuration heavily through `include/config.h`, and many hardware-dependent objects are created before `setup()` runs.

## Important Clarification

The target should be one release file for the app, not one flashable firmware binary containing all board variants.

A single archive can contain:

- board-specific `firmware.bin`
- matching `bootloader.bin`
- matching `partitions.bin`
- matching `boot_app0.bin`
- a manifest that tells the app which set belongs to which board revision

That gives the app one downloadable artifact while keeping the firmware itself simple and board-specific.

## Scope Assumptions

This plan assumes:

- the desktop app can already identify the connected controller board through serial commands
- the new board revision will still be ESP32-based
- most differences are pin mapping, I2C addresses, expander channel assignment, or other hardware wiring changes
- the application, not the firmware, will decide which board-specific image to flash

If the new board later changes MCU family, flash layout, partitioning, or bootloader requirements, revisit the bundle format before implementation.

## High-Level Architecture

### 1. Separate board config headers

Split hardware-specific configuration out of `include/config.h` into board-specific headers, for example:

- `include/boards/pop_up_controller_v10_rev_c.h`
- `include/boards/pop_up_controller_v10_rev_d.h`

Then keep one small shared selection layer, for example:

- `include/board_config.h`

That selection layer would include the correct board definition based on a compile-time macro set by the PlatformIO environment.

### 2. Separate PlatformIO environments

Add one environment per supported board revision in `platformio.ini`, for example:

- `env:pop-up-controller-v10-rev-c`
- `env:pop-up-controller-v10-rev-d`

Each environment should define:

- the board revision macro used by `board_config.h`
- any board-specific build flags
- any future board-specific upload or partition settings if needed

### 3. Shared firmware code

Keep the service code shared as much as possible.

Only board-specific hardware definitions should vary unless the new PCB revision truly requires different behavior.

### 4. Multi-image release bundle

Extend the current flash-bundle export flow so one archive can contain multiple board-specific image sets plus a top-level manifest describing:

- supported board revision ids
- build version and timestamp
- image file names and flash offsets for each board revision
- optional compatibility metadata for the desktop app

### 5. App-side automatic selection

The app should:

1. query the connected controller for its board identity
2. read the release manifest from the downloaded archive
3. choose the matching firmware set
4. flash that set only

## Implementation Phases

### Phase 0: Define the board identity contract

Before touching build tooling, define the identifier that links the controller, the firmware bundle, and the app.

Decide and document:

- the board identifier string for Revision C
- the board identifier string for Revision D
- whether the identifier comes from manufacturing data, a command response, or a compile-time constant
- whether firmware version responses should also include board revision

Preferred outcome:

- one stable compile-time board id per hardware revision
- one serial command response format that the app can rely on

Recommended decision for this repo:

- treat manufacturing `board_revision` as traceability data only
- do not use manufacturing data as the app's firmware-selection contract
- use compile-time board identity in firmware responses instead

### Phase 1: Refactor config structure for board-specific headers

Status: started

Refactor `include/config.h` into:

- shared constants that are common to all boards
- board-specific pin and hardware mapping
- optional board-specific electrical or timing defaults if needed

Suggested split:

- keep generic behavior defaults in shared config
- move GPIO pins, I2C addresses, expander mapping, ADC channels, and hardware options into board headers

Goal:

- changing board wiring should not require editing service code

### Phase 2: Add multiple build environments

Status: started

Update `platformio.ini` so each board revision builds independently from the same source tree.

Expected result:

- local builds can target one board at a time
- CI and release workflows can build all supported boards
- a regression on one board revision does not silently affect the other build artifact

### Phase 3: Improve firmware self-identification

Status: partially started

Make sure the firmware exposes enough metadata for the app to make a safe choice.

Minimum metadata:

- firmware version
- build timestamp
- board revision id

Nice to have:

- hardware family
- manifest compatibility version

This may be implemented by extending an existing status or version command rather than adding a brand-new command, if that keeps the app integration simpler.

Current direction:

- `printBuildInfo` should expose the compile-time board id and display name
- a later app-facing command or structured response can build on that once the desktop app side is ready

### Phase 4: Extend bundle export tooling

Update `scripts/export_flash_bundle.py` or add a companion script so it can:

- build multiple PlatformIO environments in one run
- stage each board's image set in a separate folder
- write a manifest that maps board id to flash files and offsets
- produce one zip archive containing all supported board revisions

Suggested archive layout:

```text
flash_manifest.json
boards/popup-controller-v10/bootloader.bin
boards/popup-controller-v10/partitions.bin
boards/popup-controller-v10/boot_app0.bin
boards/popup-controller-v10/firmware.bin
boards/popup-controller-v11/bootloader.bin
boards/popup-controller-v11/partitions.bin
boards/popup-controller-v11/boot_app0.bin
boards/popup-controller-v11/firmware.bin
```

Suggested manifest fields:

- release version
- generated timestamp
- manifest format version
- list of supported board ids
- per-board flash layout
- optional minimum app version

### Phase 5: Update release automation

Update the GitHub release workflow so a tagged release builds all supported board environments and publishes one combined archive.

Expected result:

- one GitHub release asset per firmware release
- the app downloads one archive and chooses the correct board image

### Phase 6: Update the desktop app

The app will need to:

- read the controller board id
- parse the multi-board manifest
- choose the correct firmware set
- show a clear error if the connected board is not present in the bundle

Useful UX behavior:

- show detected board revision before flashing
- show selected firmware target before write begins
- fail safely if the bundle metadata is incomplete or inconsistent

### Phase 7: Documentation and support

When implementation starts, update:

- build documentation
- flashing documentation
- serial command documentation if board identity is exposed there
- release notes and support instructions

The final customer-facing message should be simple: download one release bundle, connect the controller, and let the app pick the correct image automatically.

## Refactor Notes For This Repo

The current firmware is a good fit for separate board builds, but true runtime board selection would require more invasive changes because hardware config is used in global object construction.

Areas likely to need careful review during the board-header refactor:

- `src/services/pop_up_control/pop_up_control.cpp`
- `src/services/io/io_expanders.cpp`
- `src/services/inputs/logic/*`
- `src/services/inputs/types/*`
- `src/helpers/pop_up.cpp`
- `src/services/io/i2c_bus.cpp`

Even with separate builds, it is still worth centralizing hardware definitions cleanly so future board revisions remain low-friction.

## Open Questions To Answer For Revision D

- Which pins changed on Revision D, exactly?
- Which brand-new features exist only on Revision D?
- Should any Revision D-only hardware start disabled behind feature flags until the shared code is ready?
- Will the new board use the same ESP32 module and flash layout?
- Will the new board keep the same I2C devices and addresses?
- Will any input polarity or motor drive polarity change?
- Will LEDC channel usage stay the same?
- Will analog scaling, battery divider ratio, or calibration assumptions change?
- Will the new board need different default timing constants or only different pin mapping?
- How exactly will the firmware report its board identity to the app?

## Revised First Milestone For Revision D

Now that the new board exists, the best next steps are:

1. fill in the full Revision D pin map and device/address differences in `include/boards/pop_up_controller_v10_rev_d.h`
2. list the Revision D-only features and add compile-time feature flags for them in the board header
3. decide which existing commands should expose board identity to the app in a stable format
4. make sure both Revision C and Revision D builds compile cleanly
5. extend the export bundle format to support more than one board image set

That sequence should de-risk the work early without requiring the new board logic to be fully finished.

## Non-Goal For The First Iteration

Do not try to make one running firmware binary dynamically adapt to either PCB revision on boot unless there is a strong future requirement for that.

That approach is possible, but it would require additional abstraction work and would add more complexity to startup, hardware initialization, and testing than the separate-build approach.
