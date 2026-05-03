#!/usr/bin/env python3
"""
Convert a captured MCU serial log (containing dump_buffer hex blocks emitted
when running with core_naive.h) into the three reference headers:

    out_monochrome.h, out_gaussian_blur.h, out_sobel_edge.h

After running this, rebuild & flash with core.h (optimized). print_summary
will then compare optimized output against the MCU's own naive output --
no more host-vs-MCU float-ordering noise.

Usage:
    # 1. flash naive build, capture serial output:
    picocom --logfile naive.log /dev/ttyACM0 -b 115200    # or any terminal
    # 2. extract headers from naive run:
    python3 dump_to_headers.py naive.log --width 320 --height 240
    # 3. rebuild with core.h, flash, observe accuracy

The block names emitted by dump_buffer ("monochrome", "gaussian_blur",
"sobel_edge") map to the matching header / array / macro names.
"""
import argparse
import os
import re
import sys

BLOCK_RE = re.compile(
    r"===BEGIN\s+(?P<name>\S+)\s+(?P<size>\d+)===\s*"
    r"(?P<body>[0-9a-fA-F\s]+?)"
    r"===END\s+(?P=name)===",
    re.DOTALL,
)

DEFAULT_OUT_DIR = os.path.join(
    os.path.dirname(os.path.abspath(__file__)),
    "..","gaussian-sobel-edge-detection", "proj_cm55",
)

MAPPING = {
    "monochrome":    ("out_monochrome.h",    "out_monochrome",
                      "OUT_MONOCHROME_HEIGHT", "OUT_MONOCHROME_WIDTH"),
    "gaussian_blur": ("out_gaussian_blur.h", "out_gaussian_blur",
                      "OUT_GAUSSIAN_BLUR_HEIGHT", "OUT_GAUSSIAN_BLUR_WIDTH"),
    "sobel_edge":    ("out_sobel_edge.h",    "out_sobel_edge",
                      "OUT_SOBEL_EDGE_HEIGHT", "OUT_SOBEL_EDGE_WIDTH"),
}


def emit_header(path, name, gray_bytes, height, width, h_macro, w_macro):
    # replicate each gray byte 3x to match the existing RGB-replicated format
    # that print_summary's `expected[i*3]` indexing assumes.
    rgb = bytearray(len(gray_bytes) * 3)
    for i, b in enumerate(gray_bytes):
        rgb[3*i] = b
        rgb[3*i + 1] = b
        rgb[3*i + 2] = b

    per_line = 16
    lines = []
    for i in range(0, len(rgb), per_line):
        chunk = rgb[i:i + per_line]
        lines.append("    " + ", ".join(f"0x{b:02x}" for b in chunk) + ",")
    with open(path, "w") as f:
        f.write(f"#define {h_macro} {height}\n")
        f.write(f"#define {w_macro} {width}\n\n")
        f.write(f"// array size is {len(rgb)}\n")
        f.write(f"static const uint8_t {name}[] = {{\n")
        f.write("\n".join(lines))
        f.write("\n};\n")


def main():
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    ap.add_argument("log", help="captured serial log from a naive-build run")
    ap.add_argument("--width",  type=int, required=True)
    ap.add_argument("--height", type=int, required=True)
    ap.add_argument("--out-dir", default=DEFAULT_OUT_DIR)
    args = ap.parse_args()

    if not os.path.isdir(args.out_dir):
        sys.exit(f"output dir does not exist: {args.out_dir}")

    expected = args.width * args.height
    with open(args.log, "r", errors="replace") as f:
        text = f.read()

    blocks = list(BLOCK_RE.finditer(text))
    if not blocks:
        sys.exit("No ===BEGIN/END=== blocks found.")

    found = {}
    for m in blocks:
        name = m.group("name")
        size = int(m.group("size"))
        if name not in MAPPING:
            print(f"skipping unknown block '{name}'", file=sys.stderr)
            continue
        if size != expected:
            sys.exit(f"{name}: size {size} does not match {args.width}x{args.height}={expected}")
        hex_body = re.sub(r"\s+", "", m.group("body"))
        if len(hex_body) != 2 * size:
            sys.exit(f"{name}: declared {size} bytes, got {len(hex_body)//2}")
        found[name] = bytes.fromhex(hex_body)

    missing = set(MAPPING) - set(found)
    if missing:
        sys.exit(f"missing block(s) in log: {sorted(missing)}")

    for name, gray in found.items():
        fname, arr_name, h_macro, w_macro = MAPPING[name]
        out_path = os.path.join(args.out_dir, fname)
        emit_header(out_path, arr_name, gray, args.height, args.width, h_macro, w_macro)
        print(f"wrote {out_path}")


if __name__ == "__main__":
    main()
