#pragma once
#include "Image.h"
#include <vector>
#include <algorithm>
using namespace std;

class DataModel {
public:
    //Constructor
    DataModel(int binsPerChannel, double alpha, double epsilon);

    //function to build histogram
    void buildHistogram(const Image& img, const Image& seed);

    //function to compute data cost at each pixel using histogram
    void computeDataCost(const Image& img, const Image& seed);

    //Functions to acces the finally calculated D_p(FG) and D_p(BG) for each pixel
    double getDpFG(int x, int y);
    double getDpBG(int x, int y);

private:

    //standard global variables
    int bins;
    int totalBins;
    double alpha;                       //we will use this to perform laplace smoothing on the probabilities
    double epsilon;                     //we will use this as well to stabilize the log

    vector<double> histFG;              //histogram for FG
    vector<double> histBGl;             //histogram for BG

    vector<double> DpFG;                //per pixel Data term for FG
    vector<double> DpBG;                //per pixel Data term for BG

    void normalizeHistogram(vector<double>& hist);
};