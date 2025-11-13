#include "DataModel.h"
#include <cmath>
#include <algorithm>
#include <iostream>

DataModel::DataModel(int binsPerChannel, double alpha_, double epsilon_)
    : bins(binsPerChannel), alpha(alpha_), eps(epsilon_)
{
    totalBins = bins * bins * bins;
    histFG.assign(totalBins, 0.0);
    histBG.assign(totalBins, 0.0);
}

int DataModel::getBinIndex(const Vec3& c) const {
    // assume color channels 0..255
    int rBin = std::min(static_cast<int>(c.r / (256.0 / bins)), bins - 1);
    int gBin = std::min(static_cast<int>(c.g / (256.0 / bins)), bins - 1);
    int bBin = std::min(static_cast<int>(c.b / (256.0 / bins)), bins - 1);
    return rBin * bins * bins + gBin * bins + bBin;
}

void DataModel::normalize(std::vector<double>& hist) {
    double total = 0.0;
    for (double v : hist) total += v;
    total += alpha * totalBins;
    for (double &v : hist) v = (v + alpha) / total;
}

void DataModel::buildHistograms(const Image& img, const SeedMask& seeds) {
    W = img.width();
    H = img.height();

    std::fill(histFG.begin(), histFG.end(), 0.0);
    std::fill(histBG.begin(), histBG.end(), 0.0);

    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            int label = seeds.getLabel(x, y);
            if (label == 1) {
                int idx = getBinIndex(img.getColor(x, y));
                histFG[idx] += 1.0;
            } else if (label == 0) {
                int idx = getBinIndex(img.getColor(x, y));
                histBG[idx] += 1.0;
            }
        }
    }

    // If histFG or histBG is all zeros (no seeds), smoothing will give uniform distribution
    normalize(histFG);
    normalize(histBG);
}

void DataModel::computeDataCosts(const Image& img, const SeedMask& seeds) {
    // Ensure buildHistograms ran
    W = img.width();
    H = img.height();
    DpFG.assign(static_cast<size_t>(W) * H, 0.0);
    DpBG.assign(static_cast<size_t>(W) * H, 0.0);

    const double K = 1e9;

    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            int idx = y * W + x;
            Vec3 c = img.getColor(x, y);
            int b = getBinIndex(c);
            double Pfg = histFG[b];
            double Pbg = histBG[b];
            double Dfg = -std::log(Pfg + eps);
            double Dbg = -std::log(Pbg + eps);
            int label = seeds.getLabel(x, y);
            if (label == 1) { Dfg = 0.0; Dbg = K; }
            else if (label == 0) { Dfg = K; Dbg = 0.0; }
            DpFG[idx] = Dfg;
            DpBG[idx] = Dbg;
        }
    }
}

double DataModel::getDpFG(int x, int y) const {
    return DpFG[y * W + x];
}
double DataModel::getDpBG(int x, int y) const {
    return DpBG[y * W + x];
}
