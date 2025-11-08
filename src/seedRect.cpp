#include "SeedRect.h"
#include <algorithm>

SeedRect::SeedRect(int x0, int y0, int x1, int y1, int imgWidth, int imgHeight)
    : W(imgWidth), H(imgHeight)
{
    rect.x0 = std::max(0, x0);
    rect.y0 = std::max(0, y0);
    rect.x1 = std::min(imgWidth - 1, x1);
    rect.y1 = std::min(imgHeight - 1, y1);
}

int SeedRect::getLabel(int x, int y) const {
    // Outside rectangle = sure background
    if (x < rect.x0 || x > rect.x1 || y < rect.y0 || y > rect.y1)
        return 0;
    else
        return -1; // inside rectangle = unknown
}

Rect SeedRect::getRect() const {
    return rect;
}
