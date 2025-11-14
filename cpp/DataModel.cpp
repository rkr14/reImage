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
    fgHard = true;
    bgHard = true;
}

int DataModel::getBinIndex(const Vec3& c) const {
    // We assume color channels to be in between 0 to 255
    int rBin = std::min(static_cast<int>(c.r / (256.0 / bins)), bins - 1);
    int gBin = std::min(static_cast<int>(c.g / (256.0 / bins)), bins - 1);
    int bBin = std::min(static_cast<int>(c.b / (256.0 / bins)), bins - 1);
    return rBin * bins * bins + gBin * bins + bBin;
}

/*
Normalize histogram
We pass into the color counts for each bin
Dividing by the total number of elements turns it into probabilities
We use laplacian smoothing to prevent 0 inside logarithms
*/
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
                int idx = getBinIndex(img.getColor(x, y));      //which bin does this colour belong to
                histFG[idx] += 1.0;                             //increase the count at that index
            }   //foreground                      
            else if (label == 0) {
                int idx = getBinIndex(img.getColor(x, y));
                histBG[idx] += 1.0;
            }   //backfround
        }
    }

    // If histFG or histBG is all zeros (no seeds), smoothing will give uniform distribution
    normalize(histFG);
    normalize(histBG);
}

/*
Sets the values for DpFG and DpBG for each pixel
DpFG = -log(p(this pixel belongs to foreground))
the probability is calculated on basis of the histogram
Further improvement area: use gausian mixture models instead of histograms to build probabilities

Now model data from the histogram as probabilities to use them as edge weights
These edge weights are terms of an energy expression
The function of the graph cuts is to minimize the energy
*/
void DataModel::computeDataCosts(const Image& img, const SeedMask& seeds) {

    // Ensure buildHistograms ran
    W = img.width();
    H = img.height();
    DpFG.assign(static_cast<size_t>(W) * H, 0.0);
    DpBG.assign(static_cast<size_t>(W) * H, 0.0);

    const double K = 1e9;       //used in creating hard connections for known
                                //foreground or background pixels

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

            // label: -1 unknown, 0 background, 1 foreground
            if (label == 1) {
                if (fgHard) { Dfg = 0.0; Dbg = K; }
                // else keep Dfg/Dbg computed from histograms (soft)
                //We will be using hard mappings
            }
            else if (label == 0) {
                if (bgHard) { Dfg = K; Dbg = 0.0; }
                // else keep Dfg/Dbg computed from histograms (soft)
                //We will be using hard connections in our case
            }
            DpFG[idx] = Dfg;
            DpBG[idx] = Dbg;
        }
    }
}

void DataModel::setHardSeeds(bool fg_hard, bool bg_hard) {
    fgHard = fg_hard;
    bgHard = bg_hard;
}

double DataModel::getDpFG(int x, int y) const {
    return DpFG[y * W + x];
}
double DataModel::getDpBG(int x, int y) const {
    return DpBG[y * W + x];
}
