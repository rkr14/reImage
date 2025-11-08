#include "DataModel.h"
#include <iostream>
#include <algorithm>

DataModel::DataModel(int binsPerChannel, double alpha_, double epsilon_)
    : bins(binsPerChannel), alpha(alpha_), epsilon(epsilon_)
{
    totalBins = bins * bins * bins;
    histFG.assign(totalBins, 0.0);
    histBG.assign(totalBins, 0.0);
}

int DataModel::getBinIndex(const Vec3& c) const {
    int rBin = std::min(int(c.r / (256.0 / bins)), bins - 1);
    int gBin = std::min(int(c.g / (256.0 / bins)), bins - 1);
    int bBin = std::min(int(c.b / (256.0 / bins)), bins - 1);
    return rBin * bins * bins + gBin * bins + bBin;
}

void DataModel::normalize(std::vector<double>& hist) {
    double total = 0;
    for (auto v : hist) total += v;
    total += alpha * totalBins;
    for (auto& v : hist) v = (v + alpha) / total;
}

void DataModel::buildHistograms(const Image& img, const SeedMask& seeds) {
    W = img.width();
    H = img.height();
    std::fill(histFG.begin(), histFG.end(), 0.0);
    std::fill(histBG.begin(), histBG.end(), 0.0);

    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            int label = seeds.getLabel(x, y);
            if (label == 0) {  // background
                int bin = getBinIndex(img.getColor(x, y));
                histBG[bin]++;
            } else if (label == 1) {  // foreground
                int bin = getBinIndex(img.getColor(x, y));
                histFG[bin]++;
            }
        }
    }

    normalize(histFG);
    normalize(histBG);
}

void DataModel::computeDataCosts(const Image& img, const SeedMask& seeds) {
    const double K = 1e9;
    DpFG.resize(W * H);
    DpBG.resize(W * H);

    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            int idx = y * W + x;
            Vec3 c = img.getColor(x, y);
            int bin = getBinIndex(c);

            double Pfg = histFG[bin];
            double Pbg = histBG[bin];
            double Dfg = -std::log(Pfg + epsilon);
            double Dbg = -std::log(Pbg + epsilon);

            int label = seeds.getLabel(x, y);
            if (label == 1) { Dfg = 0; Dbg = K; }
            else if (label == 0) { Dfg = K; Dbg = 0; }

            DpFG[idx] = Dfg;
            DpBG[idx] = Dbg;
        }
    }
}

double DataModel::getDpFG(int x, int y) const { return DpFG[y * W + x]; }
double DataModel::getDpBG(int x, int y) const { return DpBG[y * W + x]; }
