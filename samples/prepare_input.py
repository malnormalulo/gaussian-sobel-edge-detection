#!/usr/bin/env python3
"""
Convert a BMP/PNG sample into the four C headers used by the MCU project:

  input.h, out_monochrome.h, out_gaussian_blur.h, out_sobel_edge.h

By default the image is used at its native resolution; pass --width/--height
to resize. The reference outputs (out_*.h) are produced by re-running the
baseline naive pipeline (matching 'baseline SW implementation/main.c': same
weights, same edge handling, same THRESHOLD, same uint8 truncation) so
print_summary on the MCU keeps producing meaningful accuracy numbers.

REMEMBER: after running this with non-128 dimensions, update HEIGHT/WIDTH in
gaussian-sobel-edge-detection/proj_cm55/core_shared.h to match.

Usage:
    python3 prepare_input.py samples/sample1.bmp                # use 320x240 as-is
    python3 prepare_input.py samples/sample1.bmp --width 128 --height 128
    python3 prepare_input.py samples/sample1.bmp --no-refs

Requires Pillow + numpy.
"""
import argparse
import math
import os
import sys

import numpy as np
from PIL import Image

SIGMA = 1.0
GBLUR_KSIZE = 3
SED_KSIZE = 3
THRESHOLD = 25.0

DEFAULT_OUT_DIR = os.path.join(
    os.path.dirname(os.path.abspath(__file__)),
    "..", "gaussian-sobel-edge-detection", "proj_cm55",
)

GX = np.array([[-1, 0, 1], [-2, 0, 2], [-1, 0, 1]], dtype=np.float64)
GY = np.array([[-1, -2, -1], [0, 0, 0], [1, 2, 1]], dtype=np.float64)


def emit_header(path, name, body, height, width, h_macro, w_macro):
    per_line = 16
    lines = []
    for i in range(0, len(body), per_line):
        chunk = body[i:i + per_line]
        lines.append("    " + ", ".join(f"0x{b:02x}" for b in chunk) + ",")
    with open(path, "w") as f:
        f.write(f"#define {h_macro} {height}\n")
        f.write(f"#define {w_macro} {width}\n\n")
        f.write(f"// array size is {len(body)}\n")
        f.write(f"static const uint8_t {name}[] = {{\n")
        f.write("\n".join(lines))
        f.write("\n};\n")


def load_image(path, width, height, crop):
    img = Image.open(path).convert("RGB")
    src_w, src_h = img.size
    target_w = width if width else src_w
    target_h = height if height else src_h
    if (target_w, target_h) != (src_w, src_h):
        if crop:
            target_aspect = target_w / target_h
            src_aspect = src_w / src_h
            if src_aspect > target_aspect:
                new_w = int(round(src_h * target_aspect))
                left = (src_w - new_w) // 2
                img = img.crop((left, 0, left + new_w, src_h))
            else:
                new_h = int(round(src_w / target_aspect))
                top = (src_h - new_h) // 2
                img = img.crop((0, top, src_w, top + new_h))
        img = img.resize((target_w, target_h), Image.LANCZOS)
    return np.array(img, dtype=np.uint8), target_w, target_h


F32 = np.float32
C_03  = F32(0.3)
C_059 = F32(0.59)
C_011 = F32(0.11)
F32_255 = F32(255.0)
F32_THR = F32(THRESHOLD)


def to_monochrome(rgb):
    r = rgb[..., 0].astype(F32)
    g = rgb[..., 1].astype(F32)
    b = rgb[..., 2].astype(F32)
    y = C_03 * r + C_059 * g + C_011 * b
    return y.astype(np.uint8)


def gaussian_kernel(size, sigma):
    r = size // 2
    k = np.zeros((size, size), dtype=F32)
    for i in range(size):
        for j in range(size):
            x, y = i - r, j - r
            k[i, j] = F32(math.exp(-(x * x + y * y) / (2.0 * sigma * sigma))
                          / (2.0 * math.pi * sigma * sigma))
    return k / k.sum()


def gaussian_blur(img, kernel):
    h, w = img.shape
    r = kernel.shape[0] // 2
    k = kernel.astype(F32)
    padded = np.zeros((h + 2 * r, w + 2 * r), dtype=F32)
    padded[r:r + h, r:r + w] = img
    mask = np.zeros_like(padded)
    mask[r:r + h, r:r + w] = F32(1.0)

    cell = np.zeros((h, w), dtype=F32)
    wsum = np.zeros((h, w), dtype=F32)
    for ki in range(-r, r + 1):
        for kj in range(-r, r + 1):
            wt = k[ki + r, kj + r]
            cell += wt * padded[r + ki:r + ki + h, r + kj:r + kj + w]
            wsum += wt * mask[r + ki:r + ki + h, r + kj:r + kj + w]
    return (cell / wsum).astype(np.uint8)


def sobel(img):
    h, w = img.shape
    r = SED_KSIZE // 2
    img_f = img.astype(F32)
    gx = GX.astype(F32)
    gy = GY.astype(F32)
    sx = np.zeros((h, w), dtype=F32)
    sy = np.zeros((h, w), dtype=F32)
    for ki in range(-r, r + 1):
        rows = np.clip(np.arange(h) + ki, 0, h - 1)
        for kj in range(-r, r + 1):
            cols = np.clip(np.arange(w) + kj, 0, w - 1)
            shifted = img_f[rows[:, None], cols[None, :]]
            sx += shifted * gx[ki + r, kj + r]
            sy += shifted * gy[ki + r, kj + r]
    mag = np.sqrt(sx * sx + sy * sy)
    mag = np.where(mag > F32_255, F32_255, mag)
    mag = np.where(mag < F32_THR, F32(0.0), mag)
    return mag.astype(np.uint8)


def gray_to_rgb_bytes(gray):
    return np.repeat(gray.reshape(-1)[:, None], 3, axis=1).reshape(-1).tolist()


def main():
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    ap.add_argument("input", help="path to a .bmp or .png sample")
    ap.add_argument("--width", type=int, default=None,
                    help="output width (default: source image width)")
    ap.add_argument("--height", type=int, default=None,
                    help="output height (default: source image height)")
    ap.add_argument("--crop", action="store_true",
                    help="center-crop to target aspect ratio before resizing")
    ap.add_argument("--out-dir", default=DEFAULT_OUT_DIR,
                    help="proj_cm55 directory where headers are written")
    ap.add_argument("--no-refs", action="store_true",
                    help="only emit input.h, skip the three reference output headers")
    args = ap.parse_args()

    if not os.path.isdir(args.out_dir):
        sys.exit(f"output dir does not exist: {args.out_dir}")

    rgb, w, h = load_image(args.input, args.width, args.height, args.crop)
    print(f"loaded {args.input} -> {w}x{h} RGB")
    print(f"REMINDER: set HEIGHT={h}, WIDTH={w} in core_shared.h "
          f"if it does not already match.")

    flat = rgb.reshape(-1).tolist()
    emit_header(os.path.join(args.out_dir, "input.h"),
                "input_image", flat, h, w, "IN_HEIGHT", "IN_WIDTH")
    print(f"  wrote input.h ({len(flat)} bytes)")

    if args.no_refs:
        return

    print("running naive pipeline...")
    mono = to_monochrome(rgb)
    emit_header(os.path.join(args.out_dir, "out_monochrome.h"),
                "out_monochrome", gray_to_rgb_bytes(mono), h, w,
                "OUT_MONOCHROME_HEIGHT", "OUT_MONOCHROME_WIDTH")
    print("  wrote out_monochrome.h")

    k = gaussian_kernel(GBLUR_KSIZE, SIGMA)
    blur = gaussian_blur(mono, k)
    emit_header(os.path.join(args.out_dir, "out_gaussian_blur.h"),
                "out_gaussian_blur", gray_to_rgb_bytes(blur), h, w,
                "OUT_GAUSSIAN_BLUR_HEIGHT", "OUT_GAUSSIAN_BLUR_WIDTH")
    print("  wrote out_gaussian_blur.h")

    edge = sobel(blur)
    emit_header(os.path.join(args.out_dir, "out_sobel_edge.h"),
                "out_sobel_edge", gray_to_rgb_bytes(edge), h, w,
                "OUT_SOBEL_EDGE_HEIGHT", "OUT_SOBEL_EDGE_WIDTH")
    print("  wrote out_sobel_edge.h")


if __name__ == "__main__":
    main()
