#!/usr/bin/env python3
"""
interactive_run.py

Usage:
    python interactive_run.py --exe ./segment

What it does:
- Ask user (via small text prompt) for image path (or pass via CLI)
- Show the image and let user draw a rectangle with mouse (click-drag-release)
- After rectangle is committed, create binary files:
    - <prefix>.image.bin  (uint8 RGB, row-major)
    - <prefix>.seed.bin   (int8, -1 inside rect, 0 outside)
    - <prefix>.meta.json  (width,height,channels,rect)
- Call the C++ binary (segment) with rect mode:
    ./segment image.bin W H rect x0 y0 x1 y1 out_mask.bin
- Wait for c++ to finish, read out_mask.bin (uint8 0/1), make overlay and show it.
- Saves overlay as <prefix>_overlay.png and mask as out_mask.bin (as produced by C++).
"""

import argparse
import json
import os
import subprocess
import sys
from dataclasses import dataclass
from typing import Tuple

import cv2
import numpy as np

# -------------------------
# Helper dataclass
# -------------------------
@dataclass
class Rect:
    x0: int
    y0: int
    x1: int
    y1: int

    def normalized(self):
        x0 = min(self.x0, self.x1)
        x1 = max(self.x0, self.x1)
        y0 = min(self.y0, self.y1)
        y1 = max(self.y0, self.y1)
        return Rect(x0, y0, x1, y1)

# -------------------------
# Interactive rectangle drawer
# -------------------------
class RectDrawer:
    """
    Interactive rectangle drawer that automatically scales the image
    to fit the current screen while preserving coordinate mapping.
    """

    def __init__(self, winname: str, image: np.ndarray):
        self.winname = winname
        self.original_image = image.copy()
        self.orig_h, self.orig_w = image.shape[:2]
        self.display = None
        self.scale = 1.0

        # Determine appropriate scaling factor
        screen_w = 1366  # safe default if not available
        screen_h = 768
        try:
            import tkinter as tk
            root = tk.Tk()
            screen_w = root.winfo_screenwidth()
            screen_h = root.winfo_screenheight()
            root.destroy()
        except Exception:
            pass  # fallback if tkinter not available

        max_w = int(screen_w * 0.9)
        max_h = int(screen_h * 0.9)
        scale_w = max_w / self.orig_w
        scale_h = max_h / self.orig_h
        self.scale = min(1.0, scale_w, scale_h)
        if self.scale < 1.0:
            new_w = int(self.orig_w * self.scale)
            new_h = int(self.orig_h * self.scale)
            self.display = cv2.resize(self.original_image, (new_w, new_h), interpolation=cv2.INTER_AREA)
        else:
            self.display = self.original_image.copy()

        self.base_image = self.display.copy()
        self.start = None
        self.end = None
        self.rect_scaled = None
        self.drawing = False

        cv2.namedWindow(self.winname, cv2.WINDOW_NORMAL)
        cv2.resizeWindow(self.winname, self.display.shape[1], self.display.shape[0])
        cv2.setMouseCallback(self.winname, self._mouse_cb)

    def _mouse_cb(self, event, x, y, flags, param):
        if event == cv2.EVENT_LBUTTONDOWN:
            self.drawing = True
            self.start = (x, y)
            self.end = (x, y)
        elif event == cv2.EVENT_MOUSEMOVE and self.drawing:
            self.end = (x, y)
            self._redraw()
        elif event == cv2.EVENT_LBUTTONUP:
            self.drawing = False
            self.end = (x, y)
            self.rect_scaled = (self.start, self.end)
            self._redraw()

    def _redraw(self):
        self.display = self.base_image.copy()
        if self.start and self.end:
            cv2.rectangle(self.display, self.start, self.end, (0, 255, 0), 2)
        h, w = self.display.shape[:2]
        info = "Drag rectangle. 'r' reset, Enter accept, Esc quit"
        cv2.putText(self.display, info, (10, h - 10),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 255, 255), 1, cv2.LINE_AA)

    def run(self) -> Rect:
        self._redraw()
        while True:
            cv2.imshow(self.winname, self.display)
            key = cv2.waitKey(20) & 0xFF
            if key == ord('r'):
                self.start = None
                self.end = None
                self.rect_scaled = None
                self._redraw()
            elif key in (13, 10):  # Enter
                if self.rect_scaled is None:
                    print("No rectangle defined.")
                    continue
                # Convert scaled coords back to original image space
                (x0, y0), (x1, y1) = self.rect_scaled
                x0, y0, x1, y1 = [int(v / self.scale) for v in (x0, y0, x1, y1)]
                cv2.destroyWindow(self.winname)
                return Rect(x0, y0, x1, y1).normalized()
            elif key == 27:  # Esc
                cv2.destroyWindow(self.winname)
                raise SystemExit("User cancelled rectangle selection.")


# -------------------------
# Scribble drawer (FG/BG)
# -------------------------
class ScribbleDrawer:
    """
    Draw foreground/background scribbles on the image.
    Left mouse button => foreground (label = 1, red)
    Right mouse button => background (label = -1, blue)
    Keys:
      'r' -> reset
      Enter -> accept and return mask (HxW int8: 1 fg, -1 bg, 0 unknown)
      Esc -> cancel
    """

    def __init__(self, winname: str, image: np.ndarray, brush_radius: int = 6):
        self.winname = winname
        # display in BGR so colors look correct when shown with OpenCV
        self.image = image.copy()
        self.h, self.w = self.image.shape[:2]
        self.display = self.image.copy()
        self.br = int(brush_radius)
        self.mask = np.zeros((self.h, self.w), dtype=np.int8)
        self.drawing = False
        self.button = None  # 1 for left (fg), 2 for right (bg)

        cv2.namedWindow(self.winname, cv2.WINDOW_NORMAL)
        cv2.resizeWindow(self.winname, min(self.w, 1600), min(self.h, 900))
        cv2.setMouseCallback(self.winname, self._mouse_cb)
        self._redraw()

    def _mouse_cb(self, event, x, y, flags, param):
        if event == cv2.EVENT_LBUTTONDOWN:
            self.drawing = True
            self.button = 1
            cv2.circle(self.mask, (x, y), self.br, 1, -1)
            self._draw_circle_on_display((x, y), 1)
        elif event == cv2.EVENT_RBUTTONDOWN:
            self.drawing = True
            self.button = 2
            cv2.circle(self.mask, (x, y), self.br, -1, -1)
            self._draw_circle_on_display((x, y), -1)
        elif event == cv2.EVENT_MOUSEMOVE and self.drawing:
            if self.button == 1:
                cv2.circle(self.mask, (x, y), self.br, 1, -1)
                self._draw_circle_on_display((x, y), 1)
            elif self.button == 2:
                cv2.circle(self.mask, (x, y), self.br, -1, -1)
                self._draw_circle_on_display((x, y), -1)
        elif event in (cv2.EVENT_LBUTTONUP, cv2.EVENT_RBUTTONUP):
            self.drawing = False
            self.button = None

    def _draw_circle_on_display(self, center, label):
        color = (0, 0, 255) if label == 1 else (255, 0, 0)  # BGR: red fg, blue bg
        cv2.circle(self.display, center, self.br, color, -1)
        self._draw_instructions()

    def _draw_instructions(self):
        info = "Left=FG(red), Right=BG(blue), r=reset, Enter=accept, Esc=quit"
        cv2.putText(self.display, info, (10, self.h - 10),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.45, (255, 255, 255), 1, cv2.LINE_AA)

    def _redraw(self):
        # rebuild display from original + mask overlays
        self.display = self.image.copy()
        fg = (self.mask == 1).astype(np.uint8) * 255
        bg = (self.mask == -1).astype(np.uint8) * 255
        if fg.any():
            fg3 = cv2.merge([fg, np.zeros_like(fg), np.zeros_like(fg)])  # BGR red
            self.display = cv2.addWeighted(self.display, 1.0, fg3, 0.5, 0)
        if bg.any():
            bg3 = cv2.merge([np.zeros_like(bg), np.zeros_like(bg), bg])
            # swap to blue channel for visibility (BGR)
            bg3 = cv2.merge([bg, np.zeros_like(bg), np.zeros_like(bg)])
            self.display = cv2.addWeighted(self.display, 1.0, bg3, 0.5, 0)
        self._draw_instructions()

    def run(self) -> np.ndarray:
        self._redraw()
        while True:
            cv2.imshow(self.winname, self.display)
            key = cv2.waitKey(20) & 0xFF
            if key == ord('r'):
                self.mask.fill(0)
                self._redraw()
            elif key in (13, 10):  # Enter
                cv2.destroyWindow(self.winname)
                return self.mask.copy()
            elif key == 27:  # Esc
                cv2.destroyWindow(self.winname)
                raise SystemExit("User cancelled scribble selection.")


# -------------------------
# Export functions
# -------------------------
def export_image_bin(rgb_image: np.ndarray, out_path: str):
    """
    rgb_image: H x W x 3, dtype=uint8, RGB order.
    Writes raw bytes to out_path.
    """
    assert rgb_image.dtype == np.uint8 and rgb_image.ndim == 3 and rgb_image.shape[2] == 3
    rgb_image.tofile(out_path)
    print(f"Wrote image binary: {out_path}")

def export_seed_bin_scribbles(mask: np.ndarray, out_path: str):
    """
    mask: H x W int8 with values -1 (background), 0 (unknown), 1 (foreground)
    Writes raw int8 bytes to out_path.
    """
    assert mask.dtype == np.int8 and mask.ndim == 2
    # Convert to C++ SeedMask convention: -1 = unknown, 0 = background, 1 = foreground
    out = np.empty_like(mask, dtype=np.int8)
    out[mask == 1] = 1
    out[mask == -1] = 0
    out[mask == 0] = -1
    out.tofile(out_path)
    print(f"Wrote scribble seed binary: {out_path}")
    return mask

def write_scribble_json(out_prefix: str, mask: np.ndarray):
    """
    Write a small JSON that indicates whether foreground/background scribbles exist.
    This can be consumed by the C++ graph builder to decide whether to apply
    infinite (hard) terminal weights to FG/BG.
    """
    fg_present = bool((mask == 1).any())
    bg_present = bool((mask == -1).any())
    data = {
        "fg_confirm": fg_present,
        "bg_confirm": bg_present,
        "fg_value": 1,
        "bg_value": -1
    }
    path = out_prefix + ".scribbles.json"
    with open(path, "w") as f:
        json.dump(data, f, indent=2)
    print(f"Wrote scribble json: {path}")
    return path

def export_seed_bin_rect(w: int, h: int, rect: Rect, out_path: str):
    """
    Create seed file with int8 per pixel:
      - outside rect => 0 (background)
      - inside rect => -1 (unknown)
    Row-major order, shape (h,w)
    """
    mask = np.full((h, w), -1, dtype=np.int8)
    r = rect.normalized()
    # clamp
    x0 = max(0, min(w-1, r.x0)); x1 = max(0, min(w-1, r.x1))
    y0 = max(0, min(h-1, r.y0)); y1 = max(0, min(h-1, r.y1))
    # outside rect -> 0
    mask[:,:] = 0
    mask[y0:y1+1, x0:x1+1] = -1
    mask.tofile(out_path)
    print(f"Wrote seed binary: {out_path}")
    return mask

def write_meta_json(out_prefix: str, width: int, height: int, rect: Rect, image_bin_name: str, seed_bin_name: str):
    meta = {
        "width": int(width),
        "height": int(height),
        "channels": 3,
        "image_bin": os.path.basename(image_bin_name),
        "seed_bin": os.path.basename(seed_bin_name),
        "seed_mode": "rect",
        "rect": [int(rect.x0), int(rect.y0), int(rect.x1), int(rect.y1)]
    }
    meta_path = out_prefix + ".meta.json"
    with open(meta_path, "w") as f:
        json.dump(meta, f, indent=2)
    print(f"Wrote meta: {meta_path}")
    return meta_path

# -------------------------
# Call C++ binary
# -------------------------
def call_segment_binary(exe_path: str, image_bin: str, W: int, H: int,
                        mode: str, out_mask_path: str,
                        seed_bin: str = None, scribble_json: str = None,
                        cwd: str = None, timeout: int = None):
    """
    Calls C++ 'segment' exe.

    Two supported modes:
      - 'rect'      : argv: ./segment image.bin W H rect x0 y0 x1 y1 out_mask.bin
      - 'scribbles' : argv: ./segment image.bin W H scribbles seed.bin scribbles.json out_mask.bin

    Returns CompletedProcess instance.
    """
    if mode == "rect":
        # seed_bin expected to be None; for compatibility caller should pass Rect in place of seed_bin
        raise RuntimeError("rect mode not supported by this signature; use existing call site for rect")
    elif mode == "scribbles":
        if seed_bin is None or scribble_json is None:
            raise RuntimeError("scribbles mode requires seed_bin and scribble_json")
        cmd = [exe_path, image_bin, str(W), str(H), "scribbles", seed_bin, scribble_json, out_mask_path]
    else:
        raise RuntimeError(f"Unknown mode for segmentation: {mode}")

    print("Calling C++:", " ".join(cmd))
    try:
        result = subprocess.run(cmd, cwd=cwd, capture_output=True, text=True, timeout=timeout)
    except subprocess.TimeoutExpired:
        raise RuntimeError("C++ segmentation timed out.")
    if result.returncode != 0:
        print("C++ stderr:\n", result.stderr)
        raise RuntimeError(f"C++ segmentation failed with exit code {result.returncode}")
    print("C++ stdout:\n", result.stdout)
    return result

# -------------------------
# Read and visualize output
# -------------------------
def read_mask_bin(mask_path: str, W: int, H: int) -> np.ndarray:
    size = W * H
    arr = np.fromfile(mask_path, dtype=np.uint8, count=size)
    if arr.size != size:
        raise RuntimeError(f"Mask size mismatch: expected {size}, got {arr.size}")
    arr = arr.reshape((H, W))
    return arr

def overlay_and_show(rgb_image: np.ndarray, mask: np.ndarray, out_overlay_path: str = None):
    """
    mask is binary 0/1 array HxW, rgb_image is uint8 HxW3 (RGB)
    Show overlay and save optional file.
    """
    assert rgb_image.dtype == np.uint8
    colored = rgb_image.copy()
    alpha = 0.55
    # red overlay for foreground
    red = np.zeros_like(colored)
    red[:, :, 0] = 255  # R channel
    # overlay where mask==1
    if mask.dtype != np.uint8:
        mask_u8 = (mask.astype(np.uint8) * 255).astype(np.uint8)
    else:
        mask_u8 = (mask * 255).astype(np.uint8)
    mask3 = np.stack([mask_u8, mask_u8, mask_u8], axis=2) > 0
    colored[mask3] = (colored[mask3] * (1.0 - alpha) + red[mask3] * alpha).astype(np.uint8)
    # show
    win = "Segmentation Overlay - press any key to continue"
    cv2.imshow(win, cv2.cvtColor(colored, cv2.COLOR_RGB2BGR))
    cv2.waitKey(0)
    cv2.destroyWindow(win)
    if out_overlay_path:
        # save BGR with cv2
        bgr = cv2.cvtColor(colored, cv2.COLOR_RGB2BGR)
        cv2.imwrite(out_overlay_path, bgr)
        print("Saved overlay:", out_overlay_path)

# -------------------------
# Main interactive flow
# -------------------------
def main():
    parser = argparse.ArgumentParser(description="Interactive segmenter frontend")
    parser.add_argument("--exe", required=True, help="Path to segment executable")
    parser.add_argument("--out_prefix", default="data/run", help="Prefix for output files (no extension)")
    parser.add_argument("--timeout", type=int, default=120, help="Timeout for C++ call in seconds")
    args = parser.parse_args()

    exe_path = args.exe
    out_prefix = args.out_prefix

    # 1) Ask for image path
    image_path = input("Enter path to image file (or drag and drop path here): ").strip().strip('"')
    if not os.path.isfile(image_path):
        print("File not found:", image_path)
        return

    # 2) Load image (OpenCV loads BGR, convert to RGB)
    img_bgr = cv2.imread(image_path, cv2.IMREAD_COLOR)
    if img_bgr is None:
        print("Failed to load image with OpenCV:", image_path)
        return
    img_rgb = cv2.cvtColor(img_bgr, cv2.COLOR_BGR2RGB)
    H, W = img_rgb.shape[:2]
    print(f"Loaded image {image_path} ({W}x{H})")

    # 3) Interactive scribble drawing
    print("Draw scribbles: Left=foreground, Right=background. Press Enter when done, Esc to cancel, 'r' to reset.")
    drawer = ScribbleDrawer("Scribble FG/BG", img_bgr)
    try:
        scribble_mask = drawer.run()
    except SystemExit as e:
        print("Cancelled by user.")
        return
    print("Scribbles collected.")

    # 4) Export files
    os.makedirs(os.path.dirname(out_prefix) or ".", exist_ok=True)
    image_bin_path = out_prefix + ".image.bin"
    seed_bin_path = out_prefix + ".seed.bin"
    meta_path = out_prefix + ".meta.json"
    export_image_bin(img_rgb, image_bin_path)
    _mask = export_seed_bin_scribbles(scribble_mask, seed_bin_path)
    scribble_json = write_scribble_json(out_prefix, scribble_mask)
    # write meta noting scribbles mode
    meta = {
        "width": int(W),
        "height": int(H),
        "channels": 3,
        "image_bin": os.path.basename(image_bin_path),
        "seed_bin": os.path.basename(seed_bin_path),
        "seed_mode": "scribbles",
        "scribbles_json": os.path.basename(scribble_json)
    }
    with open(meta_path, "w") as f:
        json.dump(meta, f, indent=2)
    print(f"Wrote meta: {meta_path}")

    # 5) Call C++ executable
    out_mask_path = out_prefix + ".out_mask.bin"
    try:
        cp = call_segment_binary(exe_path, image_bin_path, W, H, mode="scribbles",
                                 out_mask_path=out_mask_path, seed_bin=seed_bin_path,
                                 scribble_json=scribble_json, timeout=args.timeout)
    except Exception as e:
        print("Error running C++ segmenter:", e)
        return

    # 6) Read C++ output mask and show overlay
    try:
        mask = read_mask_bin(out_mask_path, W, H)
    except Exception as e:
        print("Failed to read output mask:", e)
        # optionally, read from python-constructed mask for debugging
        return

    overlay_path = out_prefix + "_overlay.png"
    overlay_and_show(img_rgb, mask, overlay_path)
    print("Done. Overlay saved to", overlay_path)


if __name__ == "__main__":
    main()

