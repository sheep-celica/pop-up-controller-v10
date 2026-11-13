"""Run esptool while handling the expected Windows USB reset disconnect."""

import re
import subprocess
import sys
import time

from serial.tools import list_ports


ESPRESSIF_USB_VID = 0x303A
REENUMERATION_TIMEOUT_S = 10.0
POLL_INTERVAL_S = 0.1


def _normalize_serial(value):
    return re.sub(r"[^0-9A-F]", "", (value or "").upper())


def _argument_value(arguments, option):
    try:
        return arguments[arguments.index(option) + 1]
    except (ValueError, IndexError):
        return None


def _port_named(device):
    device_lower = (device or "").lower()
    return next(
        (port for port in list_ports.comports() if port.device.lower() == device_lower),
        None,
    )


def _wait_for_reenumerated_controller(original_device, serial_number):
    normalized_serial = _normalize_serial(serial_number)
    deadline = time.monotonic() + REENUMERATION_TIMEOUT_S

    while time.monotonic() < deadline:
        for port in list_ports.comports():
            if port.vid != ESPRESSIF_USB_VID:
                continue
            if normalized_serial and _normalize_serial(port.serial_number) != normalized_serial:
                continue
            if port.device.lower() != original_device.lower():
                return port.device
        time.sleep(POLL_INTERVAL_S)

    return None


def main():
    if len(sys.argv) < 2:
        print("Missing esptool path.", file=sys.stderr)
        return 2

    esptool_path = sys.argv[1]
    esptool_arguments = sys.argv[2:]
    original_device = _argument_value(esptool_arguments, "--port")
    original_port = _port_named(original_device)
    original_serial = original_port.serial_number if original_port else None
    watchdog_reset_started = False

    process = subprocess.Popen(
        [sys.executable, esptool_path] + esptool_arguments,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        errors="replace",
        bufsize=1,
    )

    assert process.stdout is not None
    for line in process.stdout:
        print(line, end="", flush=True)
        if "Hard resetting with a watchdog" in line:
            watchdog_reset_started = True

    return_code = process.wait()
    if return_code == 0 or not watchdog_reset_started or not original_device:
        return return_code

    application_port = _wait_for_reenumerated_controller(
        original_device,
        original_serial,
    )
    if not application_port:
        return return_code

    print(
        "Watchdog reset completed; controller re-enumerated on %s. "
        "Ignoring the expected Windows pySerial disconnect error." % application_port
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
