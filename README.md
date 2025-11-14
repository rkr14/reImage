# reImage - Interactive Graph-Cut Image Segmentation

Professional image segmentation tool using graph-cut algorithm with AVX2-optimized C++ backend and PyQt6 GUI.

## Features

- 🎨 **Interactive Scribble Interface** - Draw foreground/background seeds with adjustable brush
- ⚡ **AVX2 Optimized** - 3-4x faster with SIMD vectorization
- 🖼️ **Auto-Display Results** - See segmentation overlay instantly
- 💾 **Export Results** - Save segmented images with transparency
- 📦 **Standalone Executable** - No installation required (Windows .exe provided)

## Quick Start (Windows)

Download `reImage.exe` from releases and double-click to run. No installation needed!

## Algorithm

Uses **graph-cut segmentation** with:
- **Dinic max-flow** algorithm for min-cut computation
- **8×8×8 RGB histograms** for color modeling
- **Adaptive β** for pairwise smoothness terms
- **4-neighborhood** graph structure

## Building from Source

### Prerequisites

**C++ Backend:**
- CMake 3.10+
- GCC/Clang (with AVX2 support) or MSVC
- C++17 compiler

**Python GUI:**
- Python 3.12+
- uv (package manager)

### Build C++ Backend

**Windows (MinGW):**
```bash
cd cpp/build
cmake .. -G "MinGW Makefiles"
cmake --build . -j
```

**Linux / macOS:**
```bash
cd cpp
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

### Build

**Install Python dependencies:**
```bash
uv sync
uv pip install pyinstaller pyqt6
```

**Create standalone executable:**
```bash
uv run --no-sync pyinstaller reImage.spec --clean
```

Output: `dist/reImage.exe` 

```bash
# Rectangle mode
./cpp/build/segment image.bin W H rect x0 y0 x1 y1 output.bin

# Mask mode (with scribbles)
./cpp/build/segment image.bin W H mask seed.bin output.bin
```

## Optimizations

### AVX2 SIMD
- Batch color distance computations (4 pixels at once)
- Vectorized operations in beta calculation
- ~3-4x speedup on graph construction

### Compiler Flags
- `-O3` - Maximum optimization
- `-march=native` - CPU-specific instructions
- `-mavx2` - Enable AVX2 explicitly
- `-ffast-math` - Aggressive floating-point
- LTO enabled for minimal binary size


- Inline `getColor()` (called millions of times)
- `alignas(32)` memory alignment for SIMD
- Cache-friendly loop ordering (horizontal/vertical separation)
- Const/constexpr where applicable

## Project Structure

```
reImage/
├── cpp/                    # C++ backend
│   ├── main.cpp           # CLI interface
│   ├── Image.{h,cpp}      # Image loading (AVX2-aligned)
│   ├── SeedMask.{h,cpp}   # Foreground/background seeds
│   ├── DataModel.{h,cpp}  # Histogram-based unary costs
│   ├── GraphBuilder.{h,cpp} # Graph construction (AVX2)
│   ├── Dinic.{h,cpp}      # Max-flow algorithm
│   ├── Segmenter.{h,cpp}  # Orchestration
│   ├── MinCut.h           # Min-cut extraction
│   ├── SimdOps.h          # AVX2 intrinsics
│   └── CMakeLists.txt
├── gui_app.py             # PyQt6 GUI application
├── reImage.spec           # PyInstaller build config
└── pyproject.toml         # Python dependencies

```

## Performance

- **Binary Size:** 109KB (C++), 91MB (standalone GUI with dependencies)
- **Typical Segmentation:** <1 second for 1920×1080 images
- **Memory:** ~50MB for graph construction

