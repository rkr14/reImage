#pragma once
#include <vector>
#include <string>
#include <cstdint>

/*
Simple abstracted class to maintain seed information
This supports both rectangular coordinates based input (for rectangular user input)
and general mask, though in this project we are not using rectangular mode
*/
class SeedMask {
public:
    // Construct from full mask file (int8 raw)
    SeedMask(const std::string& seed_bin_path, int width, int height);

    // Construct from rectangle: outside rect => 0 (bg), inside => -1 (unknown)
    SeedMask(int width, int height, int x0, int y0, int x1, int y1);
    //Not being used here

    // Get label at pixel: -1 unknown, 0 background, 1 foreground
    int getLabel(int x, int y) const;
    /*
    Abstarction being used by other files, 
    this will finally return the initial seed status of the pixel
    */

    int width() const { return W; }
    int height() const { return H; }

private:
    int W, H;
    std::vector<int8_t> data; // row-major
};
