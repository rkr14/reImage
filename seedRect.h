#pragma once

struct Rect {
    int x0, y0;  // top-left corner
    int x1, y1;  // bottom-right corner
};

// SeedRect = defines initial user labeling based on a rectangle
class SeedRect {
public:
    SeedRect(int x0, int y0, int x1, int y1, int imgWidth, int imgHeight);

    // Returns label for pixel (x, y):
    //   0  -> sure background (outside rectangle)
    //  -1  -> unknown (inside rectangle)
    int getLabel(int x, int y) const;

    // For debugging
    Rect getRect() const;

private:
    Rect rect;
    int W, H;
};
