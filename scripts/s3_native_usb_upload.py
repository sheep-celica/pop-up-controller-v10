"""Prepare an ESP32-S3 native USB CDC port for an esptool upload."""

import time
import re

from serial import Serial, SerialException
from serial.tools import list_ports

Import("env")


ESPRESSIF_USB_VID = 0x303A
BOOTLOADER_BAUD = 1200
DISCONNECT_TIMEOUT_S = 3.0
REENUMERATION_TIMEOUT_S = 12.0
POLL_INTERVAL_S = 0.1


def _ports():
    return list(list_ports.comports())


def _port_named(device):
    device_lower = device.lower()
    return next((port for port in _ports() if port.device.lower() == device_lower), None)


def _select_initial_port(upload_port):
    if upload_port:
        selected = _port_named(upload_port)
        if selected:
            return selected

    espressif_ports = [port for port in _ports() if port.vid == ESPRESSIF_USB_VID]
    if len(espressif_ports) == 1:
        return espressif_ports[0]

    if not espressif_ports:
        raise RuntimeError("No Espressif native USB serial port was found.")

    devices = ", ".join(port.device for port in espressif_ports)
    raise RuntimeError(
        "Multiple Espressif USB serial ports were found (%s); set upload_port explicitly."
        % devices
    )


def _matches_controller(port, original):
    if port.vid != ESPRESSIF_USB_VID:
        return False
    if original.serial_number and port.serial_number:
        normalize = lambda value: re.sub(r"[^0-9A-F]", "", value.upper())
        return normalize(port.serial_number) == normalize(original.serial_number)
    return True


def _request_bootloader(original):
    print(
        "Requesting ESP32-S3 download mode from %s using the 1200-baud CDC trigger..."
        % original.device
    )
    try:
        with Serial(
            original.device,
            baudrate=BOOTLOADER_BAUD,
            timeout=0.1,
            write_timeout=0.1,
        ):
            time.sleep(0.1)
    except SerialException as exc:
        # Windows can invalidate the open handle as soon as the controller
        # accepts the trigger. A disappearing port is the expected outcome.
        print("The USB port closed during the bootloader request: %s" % exc)


def _wait_for_bootloader(original):
    disconnect_deadline = time.monotonic() + DISCONNECT_TIMEOUT_S
    while time.monotonic() < disconnect_deadline:
        if _port_named(original.device) is None:
            break
        time.sleep(POLL_INTERVAL_S)
    else:
        print(
            "%s did not re-enumerate; assuming it is already in download mode."
            % original.device
        )
        return original.device

    reenumeration_deadline = time.monotonic() + REENUMERATION_TIMEOUT_S
    while time.monotonic() < reenumeration_deadline:
        matches = [port for port in _ports() if _matches_controller(port, original)]
        if matches:
            return matches[0].device
        time.sleep(POLL_INTERVAL_S)

    raise RuntimeError(
        "The ESP32-S3 USB port disconnected but did not re-enumerate within %.0f seconds."
        % REENUMERATION_TIMEOUT_S
    )


def prepare_native_usb_upload(source, target, env):
    configured_port = env.subst("$UPLOAD_PORT").strip('"')
    original = _select_initial_port(configured_port)
    _request_bootloader(original)
    bootloader_port = _wait_for_bootloader(original)
    env.Replace(UPLOAD_PORT=bootloader_port)
    env.Replace(
        UPLOADCMD=(
            '"$PYTHONEXE" "$PROJECT_DIR/scripts/esptool_native_usb_wrapper.py" '
            '"$UPLOADER" $UPLOADERFLAGS $ESP32_APP_OFFSET $SOURCE'
        )
    )
    print("Using ESP32-S3 download port %s." % bootloader_port)


env.AddPreAction("upload", prepare_native_usb_upload)
