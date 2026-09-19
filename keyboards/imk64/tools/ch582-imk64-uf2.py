#!/usr/bin/env python3
# Copyright 2026 bhe
# SPDX-License-Identifier: GPL-2.0-or-later
"""Build and verify the qmk_port_ch582 imk64 MCUboot/UF2 update image."""

from __future__ import annotations

import argparse
import hashlib
import struct
from pathlib import Path

APP_ADDRESS = 0x00013000
SLOT_ADDRESS = 0x00012000
HEADER_SIZE = 0x1000
SLOT_SIZE = 0x5E000
FAMILY_ID = 0x1AA143C7
VERSION = (1, 0, 0, 0)

IMAGE_MAGIC = 0x96F3B83D
TLV_INFO_MAGIC = 0x6907
TLV_SHA256 = 0x10
TLV_TOTAL_SIZE = 4 + 4 + hashlib.sha256().digest_size

UF2_MAGIC_START0 = 0x0A324655
UF2_MAGIC_START1 = 0x9E5D5157
UF2_MAGIC_END = 0x0AB16F30
UF2_FLAG_FAMILY_ID_PRESENT = 0x00002000
UF2_BLOCK_SIZE = 512
UF2_PAYLOAD_SIZE = 256
UF2_DATA_CAPACITY = 476

# imgtool v2.1.0 checks that the image plus the default swap trailer fits the
# slot even though --pad is not used by qmk_port_ch582.
MCUBOOT_ALIGN = 4
MCUBOOT_MAX_ALIGN = 8
MCUBOOT_MAX_SECTORS = 128


def trailer_size() -> int:
    magic_aligned = (16 + MCUBOOT_MAX_ALIGN - 1) & ~(MCUBOOT_MAX_ALIGN - 1)
    return (MCUBOOT_MAX_SECTORS * 3 * MCUBOOT_ALIGN + MCUBOOT_MAX_ALIGN * 4 + magic_aligned)


def build_mcuboot_image(application: bytes) -> bytes:
    if not application:
        raise ValueError("application binary is empty")

    image = bytearray(b"\xff" * HEADER_SIZE)
    image.extend(application)
    header = struct.pack(
        "<IIHHIIBBHII",
        IMAGE_MAGIC,
        0,  # load address: execute in place
        HEADER_SIZE,
        0,  # protected TLV size
        len(application),
        0,  # image flags
        VERSION[0],
        VERSION[1],
        VERSION[2],
        VERSION[3],
        0,  # header padding
    )
    if len(header) != 32:
        raise AssertionError("unexpected MCUboot header size")
    image[:len(header)] = header

    digest = hashlib.sha256(image).digest()
    image.extend(struct.pack("<HH", TLV_INFO_MAGIC, TLV_TOTAL_SIZE))
    image.extend(struct.pack("<BBH", TLV_SHA256, 0, len(digest)))
    image.extend(digest)

    if len(image) + trailer_size() > SLOT_SIZE:
        raise ValueError(f"MCUboot image (0x{len(image):x}) plus trailer "
                         f"(0x{trailer_size():x}) exceeds slot 0x{SLOT_SIZE:x}")
    return bytes(image)


def verify_mcuboot_image(image: bytes, application: bytes) -> None:
    if len(image) != HEADER_SIZE + len(application) + TLV_TOTAL_SIZE:
        raise ValueError("unexpected MCUboot image length")

    header = struct.unpack("<IIHHIIBBHII", image[:32])
    expected_header = (
        IMAGE_MAGIC,
        0,
        HEADER_SIZE,
        0,
        len(application),
        0,
        *VERSION,
        0,
    )
    if header != expected_header:
        raise ValueError("MCUboot header does not match qmk_port_ch582")
    if image[32:HEADER_SIZE] != b"\xff" * (HEADER_SIZE - 32):
        raise ValueError("MCUboot header padding is not erased flash")
    if image[HEADER_SIZE:HEADER_SIZE + len(application)] != application:
        raise ValueError("MCUboot image does not contain the application verbatim")

    tlv_offset = HEADER_SIZE + len(application)
    info_magic, total = struct.unpack("<HH", image[tlv_offset:tlv_offset + 4])
    tlv_type, reserved, digest_size = struct.unpack("<BBH", image[tlv_offset + 4:tlv_offset + 8])
    if (info_magic, total, tlv_type, reserved, digest_size) != (
        TLV_INFO_MAGIC,
        TLV_TOTAL_SIZE,
        TLV_SHA256,
        0,
        hashlib.sha256().digest_size,
    ):
        raise ValueError("MCUboot SHA-256 TLV is malformed")
    expected_digest = hashlib.sha256(image[:tlv_offset]).digest()
    if image[tlv_offset + 8:] != expected_digest:
        raise ValueError("MCUboot SHA-256 TLV does not authenticate the image")


def encode_uf2(image: bytes) -> bytes:
    block_count = (len(image) + UF2_PAYLOAD_SIZE - 1) // UF2_PAYLOAD_SIZE
    blocks = []
    for block_number in range(block_count):
        offset = block_number * UF2_PAYLOAD_SIZE
        payload = image[offset:offset + UF2_PAYLOAD_SIZE]
        payload = payload.ljust(UF2_PAYLOAD_SIZE, b"\0")
        header = struct.pack(
            "<8I",
            UF2_MAGIC_START0,
            UF2_MAGIC_START1,
            UF2_FLAG_FAMILY_ID_PRESENT,
            SLOT_ADDRESS + offset,
            UF2_PAYLOAD_SIZE,
            block_number,
            block_count,
            FAMILY_ID,
        )
        padding = bytes(UF2_DATA_CAPACITY - UF2_PAYLOAD_SIZE)
        blocks.append(header + payload + padding + struct.pack("<I", UF2_MAGIC_END))
    return b"".join(blocks)


def verify_uf2(uf2: bytes, image: bytes) -> int:
    if len(uf2) % UF2_BLOCK_SIZE:
        raise ValueError("UF2 length is not a multiple of 512 bytes")
    block_count = len(uf2) // UF2_BLOCK_SIZE
    expected_count = (len(image) + UF2_PAYLOAD_SIZE - 1) // UF2_PAYLOAD_SIZE
    if block_count != expected_count:
        raise ValueError("UF2 block count does not match the MCUboot image")

    reconstructed = bytearray()
    for block_number in range(block_count):
        start = block_number * UF2_BLOCK_SIZE
        block = uf2[start:start + UF2_BLOCK_SIZE]
        fields = struct.unpack("<8I", block[:32])
        magic0, magic1, flags, address, payload_size, index, total, family = fields
        magic_end = struct.unpack("<I", block[-4:])[0]
        expected_address = SLOT_ADDRESS + block_number * UF2_PAYLOAD_SIZE
        if (magic0, magic1, magic_end) != (
            UF2_MAGIC_START0,
            UF2_MAGIC_START1,
            UF2_MAGIC_END,
        ):
            raise ValueError(f"bad UF2 magic in block {block_number}")
        if flags != UF2_FLAG_FAMILY_ID_PRESENT or family != FAMILY_ID:
            raise ValueError(f"bad family metadata in block {block_number}")
        if address != expected_address or payload_size != UF2_PAYLOAD_SIZE:
            raise ValueError(f"bad address or payload size in block {block_number}")
        if index != block_number or total != block_count:
            raise ValueError(f"bad sequence metadata in block {block_number}")
        reconstructed.extend(block[32:32 + payload_size])

    if reconstructed[:len(image)] != image:
        raise ValueError("UF2 payload does not reproduce the MCUboot image")
    if any(reconstructed[len(image):]):
        raise ValueError("UF2 tail padding is not zero")
    return block_count


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("application", type=Path)
    parser.add_argument("mcuboot_image", type=Path)
    parser.add_argument("uf2", type=Path)
    args = parser.parse_args()

    application = args.application.read_bytes()
    image = build_mcuboot_image(application)
    verify_mcuboot_image(image, application)
    uf2 = encode_uf2(image)
    block_count = verify_uf2(uf2, image)

    args.mcuboot_image.parent.mkdir(parents=True, exist_ok=True)
    args.uf2.parent.mkdir(parents=True, exist_ok=True)
    args.mcuboot_image.write_bytes(image)
    args.uf2.write_bytes(uf2)
    verify_mcuboot_image(args.mcuboot_image.read_bytes(), application)
    verify_uf2(args.uf2.read_bytes(), image)

    print("qmk_port_ch582 imk64 update verified: "
          f"slot=0x{SLOT_ADDRESS:08x} app=0x{APP_ADDRESS:08x} "
          f"family=0x{FAMILY_ID:08x} blocks={block_count} "
          f"app_bytes={len(application)} image_bytes={len(image)} "
          f"uf2_bytes={len(uf2)}")


if __name__ == "__main__":
    main()
