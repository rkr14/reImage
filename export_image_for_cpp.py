#!/usr/bin/env python3
"""
export_image_for_cpp.py

Usage examples:
  python export_image_for_cpp.py --img cat.jpg --out_prefix data/cat --rect 50 30 250 220
  python export_image_for_cpp.py --img cat.jpg --out_prefix data/cat --mask mymask.png

Output:
  data/cat.image.bin    (uint8 RGB, row-major)
  data/cat.seed.bin     (int8 per-pixel: -1,0,1)  (only if --mask or --rect used)
  data/cat.meta.json    (JSON with width,height,channels,seed_mode,...)
"""
import argparse
import json
import numpy as np
import cv2
import os
import subprocess
import sys

def make_rect_mask(h, w, rect):
    x0, y0, x1, y1 = rect
    mask = np.full((h, w), -1, dtype=np.int8)   # default unknown
    # outside rectangle -> background (0)
    mask[:,:] = -1
    mask[0:y0, :] = 0
    mask[y1+1:, :] = 0
    mask[:, 0:x0] = 0
    mask[:, x1+1:] = 0
    # note: inside rect remains -1 (unknown)
    return mask

def make_mask_from_image(mask_path, h, w):
    """Load mask image (any grayscale or color). Map nonzero->foreground(1), zero->background(0)."""
    m = cv2.imread(mask_path, cv2.IMREAD_UNCHANGED)
    if m is None:
        raise RuntimeError(f"Could not open mask file {mask_path}")
    if len(m.shape) == 3:
        # color -> convert to grayscale
        m = cv2.cvtColor(m, cv2.COLOR_BGR2GRAY)
    m = cv2.resize(m, (w, h), interpolation=cv2.INTER_NEAREST)
    # threshold: non-zero => fg
    out = np.where(m > 0, 1, 0).astype(np.int8)
    return out

def main():
    p = argparse.ArgumentParser()
    p.add_argument("--img", required=True, help="input image path")
    p.add_argument("--out_prefix", required=True, help="prefix for output files (e.g. data/cat)")
    p.add_argument("--rect", nargs=4, type=int, help="rectangle x0 y0 x1 y1 (inclusive)")
    p.add_argument("--mask", help="path to mask image (nonzero -> foreground)")
    p.add_argument("--call_cpp", help="optional path to C++ binary to call after writing files", default=None)
    args = p.parse_args()

    img = cv2.imread(args.img, cv2.IMREAD_COLOR)
    if img is None:
        print("Failed to load image", args.img, file=sys.stderr)
        sys.exit(1)

    h, w = img.shape[:2]

    # convert BGR -> RGB
    rgb = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
    rgb = rgb.astype(np.uint8)

    # write raw rgb bytes (row-major, R,G,B interleaved)
    image_bin_path = args.out_prefix + ".image.bin"
    # ensure output directory exists when out_prefix includes directories
    out_dir = os.path.dirname(image_bin_path)
    if out_dir:
        os.makedirs(out_dir, exist_ok=True)
    rgb.tofile(image_bin_path)   # raw bytes (uint8)

    # prepare metadata
    meta = {
        "width": int(w),
        "height": int(h),
        "channels": 3,
        "dtype": "uint8",
        "image_bin": os.path.basename(image_bin_path)
    }

    seed_bin_path = args.out_prefix + ".seed.bin"
    seed_mode = "none"
    if args.rect:
        x0,y0,x1,y1 = args.rect
        # clamp
        x0 = max(0, min(x0, w-1))
        x1 = max(0, min(x1, w-1))
        y0 = max(0, min(y0, h-1))
        y1 = max(0, min(y1, h-1))
        if x1 < x0: x0,x1 = x1,x0
        if y1 < y0: y0,y1 = y1,y0
        mask = make_rect_mask(h, w, (x0,y0,x1,y1))
        mask.tofile(seed_bin_path)
        seed_mode = "rect"
        meta["seed_mode"] = "rect"
        meta["rect"] = [int(x0), int(y0), int(x1), int(y1)]
        meta["seed_bin"] = os.path.basename(seed_bin_path)
    elif args.mask:
        mask_img = make_mask_from_image(args.mask, h, w)
        # map 0->0 (bg), 1->1 (fg); since mask provides definite info for all pixels,
        # we keep them as 0/1
        mask = mask_img.astype(np.int8)
        mask.tofile(seed_bin_path)
        seed_mode = "mask"
        meta["seed_mode"] = "mask"
        meta["seed_bin"] = os.path.basename(seed_bin_path)
    else:
        # no seed provided -> write none; segmentation code may still accept default behavior
        seed_mode = "none"

    meta_path = args.out_prefix + ".meta.json"
    with open(meta_path, "w") as f:
        json.dump(meta, f, indent=2)

    print("Wrote:", image_bin_path, meta_path, ("and " + seed_bin_path) if seed_mode != "none" else "")
    if args.call_cpp:
        # call the C++ binary with args: image_bin meta_path seed_bin (if any) or rect args
        # we pass an extra arg for the C++ to write a mask file we can read back
        mask_path = args.out_prefix + ".mask.bin"
        cmd = [args.call_cpp, image_bin_path, meta_path]
        if seed_mode != "none":
            cmd += [seed_bin_path]
        cmd += [mask_path]
        print("Calling C++:", " ".join(cmd))
        subprocess.run(cmd, check=True)

        # if C++ produced a mask file, compose an RGBA PNG with background transparent
        if os.path.exists(mask_path):
            # read raw image
            img = np.fromfile(image_bin_path, dtype=np.uint8)
            img = img.reshape((h, w, 3))
            mask = np.fromfile(mask_path, dtype=np.uint8).reshape((h, w))
            # create BGRA for OpenCV (B,G,R,A)
            bgra = np.zeros((h, w, 4), dtype=np.uint8)
            bgra[:, :, 0] = img[:, :, 2]
            bgra[:, :, 1] = img[:, :, 1]
            bgra[:, :, 2] = img[:, :, 0]
            bgra[:, :, 3] = (mask > 0).astype(np.uint8) * 255
            out_png = args.out_prefix + ".segmented.png"
            cv2.imwrite(out_png, bgra)
            print("Wrote segmented PNG:", out_png)

if __name__ == "__main__":
    main()
