#pragma once
#include "Image.h"
#include "SeedMask.h"
#include <vector>

class DataModel {
public:
    DataModel(int binsPerChannel = 8, double alpha = 1.0, double epsilon = 1e-9);

    // Build histograms from seeds
    void buildHistograms(const Image& img, const SeedMask& seeds);

    // Compute per-pixel data costs DpFG and DpBG and store internally
    void computeDataCosts(const Image& img, const SeedMask& seeds);

    double getDpFG(int x, int y) const;
    double getDpBG(int x, int y) const;

    int width() const { return W; }
    int height() const { return H; }

private:
    int bins;
    int totalBins;
    double alpha;
    double eps;

    int W, H;
    std::vector<double> histFG, histBG;
    std::vector<double> DpFG, DpBG;

    int getBinIndex(const Vec3& c) const;
    void normalize(std::vector<double>& hist);
};
