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

    // Configure whether scribble-confirmed FG/BG should be treated as hard (infinite)
    // If false, scribbles are treated as soft evidence (use histogram-based costs).
    void setHardSeeds(bool fg_hard, bool bg_hard);

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
    bool fgHard;
    bool bgHard;

    int getBinIndex(const Vec3& c) const;
    void normalize(std::vector<double>& hist);
};
