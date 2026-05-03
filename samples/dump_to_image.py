#!/usr/bin/env python3
"""
Parse hex dumps emitted by the MCU over UART and save each as a grayscale BMP.

The MCU emits blocks like:

    ===BEGIN <name> <size>===
    <hex bytes, no separators, line-wrapped>
    ===END <name>===

Capture the serial output to a text file (any terminal that logs to file
works: minicom -C, screen -L, picocom --logfile, pyserial-miniterm, etc.),
then run:

    python3 dump_to_image.py serial.log --prefix optimized_
    python3 dump_to_image.py serial.log --prefix naive_     --out-dir naive

Stdlib only (writes uncompressed 8-bit grayscale BMP via a 256-entry palette).
"""
import argparse
import os
import re
import struct
import sys

BLOCK_RE = re.compile(
    r"===BEGIN\s+(?P<name>\S+)\s+(?P<size>\d+)===\s*"
    r"(?P<body>[0-9a-fA-F\s]+?)"
    r"===END\s+(?P=name)===",
    re.DOTALL,
)


def write_bmp_gray(path, pixels, width, height):
    row_size = (width + 3) & ~3
    pad = row_size - width
    pixel_data_size = row_size * height
    pixel_offset = 14 + 40 + 256 * 4
    file_size = pixel_offset + pixel_data_size

    with open(path, "wb") as f:
        f.write(b"BM")
        f.write(struct.pack("<I", file_size))
        f.write(struct.pack("<HH", 0, 0))
        f.write(struct.pack("<I", pixel_offset))
        f.write(struct.pack("<I", 40))
        f.write(struct.pack("<i", width))
        f.write(struct.pack("<i", height))
        f.write(struct.pack("<HH", 1, 8))
        f.write(struct.pack("<I", 0))
        f.write(struct.pack("<I", pixel_data_size))
        f.write(struct.pack("<ii", 2835, 2835))
        f.write(struct.pack("<I", 256))
        f.write(struct.pack("<I", 256))
        for i in range(256):
            f.write(bytes((i, i, i, 0)))
        zero_pad = b"\x00" * pad
        for y in range(height - 1, -1, -1):
            f.write(pixels[y * width:(y + 1) * width])
            if pad:
                f.write(zero_pad)


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("log", help="captured serial log file")
    ap.add_argument("--width", type=int, default=128,
                    help="image width in pixels (default: 128)")
    ap.add_argument("--out-dir", default=".", help="output directory")
    ap.add_argument("--prefix", default="",
                    help="filename prefix, e.g. optimized_ or naive_")
    args = ap.parse_args()

    with open(args.log, "r", errors="replace") as f:
        text = f.read()

    blocks = list(BLOCK_RE.finditer(text))
    if not blocks:
        sys.exit("No ===BEGIN/END=== blocks found in " + args.log)

    os.makedirs(args.out_dir, exist_ok=True)

    for m in blocks:
        name = m.group("name")
        size = int(m.group("size"))
        hex_body = re.sub(r"\s+", "", m.group("body"))
        if len(hex_body) != 2 * size:
            print(f"WARN: {name}: declared {size} bytes, got {len(hex_body)//2} -- skipped",
                  file=sys.stderr)
            continue
        try:
            data = bytes.fromhex(hex_body)
        except ValueError as e:
            print(f"WARN: {name}: bad hex ({e}) -- skipped", file=sys.stderr)
            continue
        if size % args.width != 0:
            print(f"WARN: {name}: size {size} not divisible by width {args.width} -- skipped",
                  file=sys.stderr)
            continue
        height = size // args.width
        out = os.path.join(args.out_dir, f"{args.prefix}{name}.bmp")
        write_bmp_gray(out, data, args.width, height)
        print(f"wrote {out} ({args.width}x{height})")


if __name__ == "__main__":
    main()
