#pragma once
#include "Image.h"
#include "SeedMask.h"
#include <vector>
#include <cmath>

class DataModel {
public:
    DataModel(int binsPerChannel, double alpha, double epsilon);

    void buildHistograms(const Image& img, const SeedMask& seeds);
    void computeDataCosts(const Image& img, const SeedMask& seeds);

    double getDpFG(int x, int y) const;
    double getDpBG(int x, int y) const;
    int width() const { return W; }
    int height() const { return H; }

private:
    int bins, totalBins;
    double alpha, epsilon;
    int W, H;

    std::vector<double> histFG, histBG;
    std::vector<double> DpFG, DpBG;

    int getBinIndex(const Vec3& c) const;
    void normalize(std::vector<double>& hist);
};
