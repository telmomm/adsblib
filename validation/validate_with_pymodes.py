"""Cross-check adsblib frames against the independent pyModeS decoder."""

from __future__ import annotations

import argparse
import ctypes
import math
import random
from pathlib import Path
from typing import Any

import pyModeS as pms


ADSB_FRAME_BYTES = 14
ADSB_CALLSIGN_LEN = 8
CPR_EVEN = 0
CPR_ODD = 1


class ADSBIdentification(ctypes.Structure):
    _fields_ = [
        ("icao", ctypes.c_uint32),
        ("callsign", ctypes.c_char * (ADSB_CALLSIGN_LEN + 1)),
    ]


class ADSBPosition(ctypes.Structure):
    _fields_ = [
        ("icao", ctypes.c_uint32),
        ("latitude_deg", ctypes.c_double),
        ("longitude_deg", ctypes.c_double),
        ("altitude_ft", ctypes.c_int32),
        ("cpr_format", ctypes.c_int),
    ]


class ADSBSurfacePosition(ctypes.Structure):
    _fields_ = [
        ("icao", ctypes.c_uint32),
        ("latitude_deg", ctypes.c_double),
        ("longitude_deg", ctypes.c_double),
        ("ground_speed_kt", ctypes.c_double),
        ("ground_track_deg", ctypes.c_double),
        ("cpr_format", ctypes.c_int),
    ]


class ADSBVelocity(ctypes.Structure):
    _fields_ = [
        ("icao", ctypes.c_uint32),
        ("ground_speed_kt", ctypes.c_double),
        ("track_deg", ctypes.c_double),
        ("vertical_rate_fpm", ctypes.c_int32),
    ]


class ADSBEmergency(ctypes.Structure):
    _fields_ = [
        ("icao", ctypes.c_uint32),
        ("emergency_state", ctypes.c_int),
        ("mode_a_code", ctypes.c_uint16),
    ]


class ADSBTargetState(ctypes.Structure):
    _fields_ = [
        ("icao", ctypes.c_uint32),
        ("selected_altitude_ft", ctypes.c_int32),
        ("selected_altitude_source", ctypes.c_int),
        ("barometric_pressure_mbar", ctypes.c_double),
        ("selected_heading_valid", ctypes.c_bool),
        ("selected_heading_deg", ctypes.c_double),
        ("nac_p", ctypes.c_uint8),
        ("nic_baro", ctypes.c_bool),
        ("sil", ctypes.c_uint8),
        ("mode_status", ctypes.c_bool),
        ("autopilot_engaged", ctypes.c_bool),
        ("vnav_mode", ctypes.c_bool),
        ("altitude_hold_mode", ctypes.c_bool),
        ("approach_mode", ctypes.c_bool),
        ("tcas_operational", ctypes.c_bool),
        ("lnav_mode", ctypes.c_bool),
    ]


class ADSBOperationalStatus(ctypes.Structure):
    _fields_ = [
        ("icao", ctypes.c_uint32),
        ("subtype", ctypes.c_uint8),
        ("capability_class", ctypes.c_uint16),
        ("operational_mode", ctypes.c_uint16),
        ("adsb_version", ctypes.c_uint8),
        ("nic_supplement_a", ctypes.c_bool),
        ("nac_p", ctypes.c_uint8),
        ("sil", ctypes.c_uint8),
        ("nic_baro", ctypes.c_bool),
        ("heading_reference_magnetic", ctypes.c_bool),
        ("sil_supplement", ctypes.c_bool),
    ]


def frame_to_hex(frame: Any) -> str:
    return "".join(f"{byte:02X}" for byte in frame)


def payload_bits(message: str, start: int, width: int) -> int:
    payload = int(message[8:22], 16)
    return (payload >> (56 - start - width)) & ((1 << width) - 1)


def movement_reference_speed(movement: int) -> float | None:
    if movement == 0 or movement > 124:
        return None
    if movement == 1:
        return 0.0
    if movement == 124:
        return 175.0

    lower_bounds = [2, 9, 13, 39, 94, 109, 124]
    speeds = [0.125, 1, 2, 15, 70, 100, 175]
    steps = [0.125, 0.25, 0.5, 1, 2, 5]
    index = next(index for index, lower_bound in enumerate(lower_bounds) if lower_bound > movement)
    return speeds[index - 1] + (movement - lower_bounds[index - 1]) * steps[index - 1]


def mode_a_gillham(mode_a_code: int) -> int:
    digit_a, digit_b, digit_c, digit_d = [
        (mode_a_code >> shift) & 0x7 for shift in (9, 6, 3, 0)
    ]
    return (
        ((digit_a & 4) << 5)
        | ((digit_a & 2) << 8)
        | ((digit_a & 1) << 11)
        | ((digit_b & 4) >> 1)
        | ((digit_b & 2) << 2)
        | ((digit_b & 1) << 5)
        | ((digit_c & 4) << 6)
        | ((digit_c & 2) << 9)
        | ((digit_c & 1) << 12)
        | ((digit_d & 4) >> 2)
        | ((digit_d & 2) >> 1)
        | ((digit_d & 1) << 4)
    )


def load_library(path: Path) -> ctypes.CDLL:
    library = ctypes.CDLL(str(path))
    structures = {
        "adsb_encode_identification": ADSBIdentification,
        "adsb_encode_position": ADSBPosition,
        "adsb_encode_surface_position": ADSBSurfacePosition,
        "adsb_encode_velocity": ADSBVelocity,
        "adsb_encode_emergency": ADSBEmergency,
        "adsb_encode_target_state": ADSBTargetState,
        "adsb_encode_operational_status": ADSBOperationalStatus,
    }
    for function_name, structure in structures.items():
        function = getattr(library, function_name)
        function.argtypes = [ctypes.POINTER(structure), ctypes.POINTER(ctypes.c_uint8)]
        function.restype = ctypes.c_int
    return library


def encode(library: ctypes.CDLL, function_name: str, message: Any) -> tuple[ctypes.Array, str]:
    frame = (ctypes.c_uint8 * ADSB_FRAME_BYTES)()
    status = getattr(library, function_name)(ctypes.byref(message), frame)
    if status != 0:
        raise AssertionError(f"{function_name} returned status {status}")
    return frame, frame_to_hex(frame)


def check_crc(library: ctypes.CDLL, message: str) -> None:
    assert pms.crc(message) == 0
    frame = (ctypes.c_uint8 * ADSB_FRAME_BYTES)(*bytes.fromhex(message))
    assert library.adsb_verify_crc(frame)


def test_identification(library: ctypes.CDLL, random_generator: random.Random) -> None:
    for _ in range(1000):
        length = random_generator.randint(1, ADSB_CALLSIGN_LEN)
        callsign = "".join(random_generator.choice("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789") for _ in range(length))
        message = ADSBIdentification(random_generator.randint(0, 0xFFFFFF), callsign.encode())
        _, frame_hex = encode(library, "adsb_encode_identification", message)
        decoded = pms.adsb.callsign(frame_hex).replace("_", "").strip()
        assert decoded == callsign
        check_crc(library, frame_hex)


def test_position(library: ctypes.CDLL, random_generator: random.Random) -> None:
    for _ in range(200):
        latitude = random_generator.uniform(-80.0, 80.0)
        longitude = random_generator.uniform(-180.0, 180.0)
        altitude = random_generator.randint(0, 40000)
        even = ADSBPosition(0xABCDEF, latitude, longitude, altitude, CPR_EVEN)
        odd = ADSBPosition(0xABCDEF, latitude, longitude, altitude, CPR_ODD)
        _, even_hex = encode(library, "adsb_encode_position", even)
        _, odd_hex = encode(library, "adsb_encode_position", odd)
        decoded = pms.adsb.position(even_hex, odd_hex, 0.0, 1.0)
        assert decoded is not None
        decoded_latitude, decoded_longitude = decoded
        assert abs(decoded_latitude - latitude) < 0.01
        assert abs(decoded_longitude - longitude) < 0.01
        assert abs(pms.adsb.altitude(even_hex) - altitude) <= 25
        check_crc(library, even_hex)
        check_crc(library, odd_hex)


def test_velocity(library: ctypes.CDLL, random_generator: random.Random) -> None:
    for speed in [50, 250, 600, 1100, 2000, 4000]:
        for track in [0.0, 90.0, 180.0, 270.0]:
            message = ADSBVelocity(0xABCDEF, speed, track, random_generator.randint(-3000, 3000))
            _, frame_hex = encode(library, "adsb_encode_velocity", message)
            decoded_speed, decoded_track, decoded_vertical_rate, _ = pms.adsb.velocity(frame_hex)
            assert abs(decoded_speed - speed) <= 8
            assert abs((decoded_track - track + 180.0) % 360.0 - 180.0) <= 2
            assert abs(decoded_vertical_rate - message.vertical_rate_fpm) <= 64
            expected_subtype = 2 if speed >= 1100 else 1
            assert payload_bits(frame_hex, 5, 3) == expected_subtype
            check_crc(library, frame_hex)


def test_surface_position(library: ctypes.CDLL) -> None:
    for speed in [0.0, 0.5, 5.0, 25.0, 99.0, 174.0, 175.0, 250.0]:
        for track in [0.0, 45.0, 90.0, 200.0, 359.9]:
            message = ADSBSurfacePosition(0xABCDEF, 37.5, -122.1, speed, track, CPR_EVEN)
            _, frame_hex = encode(library, "adsb_encode_surface_position", message)
            movement = payload_bits(frame_hex, 5, 7)
            decoded_speed = movement_reference_speed(movement)
            expected_speed = 0.0 if speed < 0.125 else 175.0 if speed >= 175.0 else speed
            assert decoded_speed is not None
            assert abs(decoded_speed - expected_speed) <= 6.0
            decoded_track = payload_bits(frame_hex, 13, 7) * 360.0 / 128.0
            assert abs((decoded_track - track + 180.0) % 360.0 - 180.0) <= 360.0 / 128.0
            check_crc(library, frame_hex)


def test_status_messages(library: ctypes.CDLL) -> None:
    emergency = ADSBEmergency(0xABCDEF, 5, 0o7500)
    _, emergency_hex = encode(library, "adsb_encode_emergency", emergency)
    assert pms.adsb.typecode(emergency_hex) == 28
    assert payload_bits(emergency_hex, 5, 3) == 1
    assert payload_bits(emergency_hex, 8, 3) == 5
    assert payload_bits(emergency_hex, 11, 13) == mode_a_gillham(0o7500)
    check_crc(library, emergency_hex)

    target = ADSBTargetState(
        0xABCDEF, 32000, 1, 1013.2, True, 90.0,
        9, True, 2, True, True, True, True, False, True, True,
    )
    _, target_hex = encode(library, "adsb_encode_target_state", target)
    assert pms.adsb.typecode(target_hex) == 29
    assert payload_bits(target_hex, 5, 2) == 1
    assert payload_bits(target_hex, 9, 11) == 1001
    assert payload_bits(target_hex, 20, 9) == 268
    assert payload_bits(target_hex, 30, 9) == 128
    check_crc(library, target_hex)

    operational = ADSBOperationalStatus(
        0xABCDEF, 1, 0xA55A, 0x5AA5, 2,
        True, 12, 3, True, True, True,
    )
    _, operational_hex = encode(library, "adsb_encode_operational_status", operational)
    assert pms.adsb.typecode(operational_hex) == 31
    assert payload_bits(operational_hex, 5, 3) == 1
    assert payload_bits(operational_hex, 8, 16) == 0xA55A
    assert payload_bits(operational_hex, 24, 16) == 0x5AA5
    assert payload_bits(operational_hex, 40, 3) == 2
    assert payload_bits(operational_hex, 44, 4) == 12
    assert payload_bits(operational_hex, 50, 2) == 3
    check_crc(library, operational_hex)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--library", type=Path, help="Path to adsblib.so or adsblib.dylib")
    arguments = parser.parse_args()

    default_name = "adsblib.dylib" if __import__("platform").system() == "Darwin" else "adsblib.so"
    library_path = arguments.library or Path.cwd() / default_name
    if not library_path.exists():
        raise FileNotFoundError(f"Shared library not found: {library_path}")

    library = load_library(library_path)
    random_generator = random.Random(0xAD5)
    test_identification(library, random_generator)
    test_position(library, random_generator)
    test_velocity(library, random_generator)
    test_surface_position(library)
    test_status_messages(library)
    print("pyModeS integration validation passed")


if __name__ == "__main__":
    main()
