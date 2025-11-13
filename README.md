# reImage

Use this to build using CMake:
cd cpp
mkdir build
cd build
cmake .. -G "MinGW Makefiles"
cmake --build .

Use this to run interactive_run.py:
python interactive_run.py --exe ./cpp/build/segment.exe --out_prefix data/run1 --timeout 600

Alternatively, here are explicit build commands that work on Linux/macOS and on Windows (MSYS/MinGW) respectively.

Linux / macOS (bash):
```bash
mkdir -p build
cd build
cmake ..        # or: cmake -S .. -B .
make -j$(nproc)
```

Windows (MinGW/MSYS shell):
```bash
mkdir build
cd build
cmake .. -G "MinGW Makefiles"
mingw32-make -j%NUMBER_OF_PROCESSORS%    # or use 'make -jN' in MSYS where nproc is available
```