# reImage

Use this to build using CMake:
cd cpp
mkdir build
cd build
cmake .. -G "MinGW Makefiles"
cmake --build .

Use this to run interactive_run.py:
python interactive_run.py --exe ./cpp/build/segment.exe --out_prefix data/run1 --timeout 600