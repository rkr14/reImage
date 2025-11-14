#pragma once
#include "Image.h"
#include <immintrin.h> 
#include <cmath>


namespace simd {

//if the computer has AVX2 hardware support
//We use this to parrelelize colour difference calculations to speed up 
//This process by around 3-4 times
#ifdef __AVX2__
    // Compute squared distance between 4 pairs of RGB colors using AVX2
    // Input: 4 colors in a (r0,g0,b0, r1,g1,b1, r2,g2,b2, r3,g3,b3)
    //        4 colors in b (r0,g0,b0, r1,g1,b1, r2,g2,b2, r3,g3,b3)
    // Output: 4 squared distances
    inline void colorDistSq4_AVX2(
        const double* a_rgb,  // 12 doubles: r0,g0,b0,r1,g1,b1,r2,g2,b2,r3,g3,b3
        const double* b_rgb,  // 12 doubles
        double* out_dist      // 4 output distances
    ) {
        // Load RGB triplets
        __m256d a0 = _mm256_loadu_pd(a_rgb);      // r0,g0,b0,r1
        __m256d a1 = _mm256_loadu_pd(a_rgb + 4);  // g1,b1,r2,g2
        __m256d a2 = _mm256_loadu_pd(a_rgb + 8);  // b2,r3,g3,b3
        
        __m256d b0 = _mm256_loadu_pd(b_rgb);
        __m256d b1 = _mm256_loadu_pd(b_rgb + 4);
        __m256d b2 = _mm256_loadu_pd(b_rgb + 8);
        
        // Compute differences
        __m256d diff0 = _mm256_sub_pd(a0, b0);
        __m256d diff1 = _mm256_sub_pd(a1, b1);
        __m256d diff2 = _mm256_sub_pd(a2, b2);
        
        // Square differences (FMA: diff * diff + 0)
        __m256d sq0 = _mm256_mul_pd(diff0, diff0);
        __m256d sq1 = _mm256_mul_pd(diff1, diff1);
        __m256d sq2 = _mm256_mul_pd(diff2, diff2);
        
        // Sum R²+G²+B² for each color pair
        // Layout: [r0²,g0²,b0²,r1²] [g1²,b1²,r2²,g2²] [b2²,r3²,g3²,b3²]
        // We need: [r0²+g0²+b0², r1²+g1²+b1², r2²+g2²+b2², r3²+g3²+b3²]
        
        // Extract components (scalar fallback for final reduction)
        alignas(32) double sq[12];
        _mm256_store_pd(sq, sq0);
        _mm256_store_pd(sq + 4, sq1);
        _mm256_store_pd(sq + 8, sq2);
        
        out_dist[0] = sq[0] + sq[1] + sq[2];    // r0²+g0²+b0²
        out_dist[1] = sq[3] + sq[4] + sq[5];    // r1²+g1²+b1²
        out_dist[2] = sq[6] + sq[7] + sq[8];    // r2²+g2²+b2²
        out_dist[3] = sq[9] + sq[10] + sq[11];  // r3²+g3²+b3²
    }
    
    // Batch exp computation using AVX2 
    inline __m256d exp_pd_avx2(__m256d x) {
        // Use polynomial approximation or call scalar exp
    
        alignas(32) double vals[4];
        _mm256_store_pd(vals, x);
        vals[0] = std::exp(vals[0]);
        vals[1] = std::exp(vals[1]);
        vals[2] = std::exp(vals[2]);
        vals[3] = std::exp(vals[3]);
        return _mm256_load_pd(vals);
    }

    // Batch logarithm computation using AVX2
    inline __m256d log_pd_avx2(__m256d x) {
        alignas(32) double vals[4];
        _mm256_store_pd(vals, x);
        vals[0] = std::log(vals[0]);
        vals[1] = std::log(vals[1]);
        vals[2] = std::log(vals[2]);
        vals[3] = std::log(vals[3]);
        return _mm256_load_pd(vals);
    }

    // Batch negation of logarithm: -log(x)
    inline void negLog4_AVX2(
        const double* inputs,  // 4 doubles: p0, p1, p2, p3
        double eps,            // epsilon for numerical stability
        double* out_neglog     // 4 output values: -log(p0+eps), etc.
    ) {
        __m256d eps_vec = _mm256_set1_pd(eps);
        __m256d in = _mm256_loadu_pd(inputs);
        __m256d safe_in = _mm256_add_pd(in, eps_vec);  // p + eps
        __m256d log_vals = log_pd_avx2(safe_in);
        __m256d neg_log = _mm256_mul_pd(log_vals, _mm256_set1_pd(-1.0));
        _mm256_storeu_pd(out_neglog, neg_log);
    }

    // Quantize 4 colors to histogram bins (RGB to bin indices)
    // Returns 4 bin indices
    inline void quantizeColors4_AVX2(
        const double* colors_rgb,  // 12 doubles: r0,g0,b0,r1,g1,b1,r2,g2,b2,r3,g3,b3
        int bins,                  // bins per channel
        int* out_bins              // 4 output bin indices
    ) {
        double scale = 256.0 / static_cast<double>(bins);
        alignas(32) double col[12];
        _mm256_storeu_pd(col, _mm256_loadu_pd(colors_rgb));
        _mm256_storeu_pd(col + 4, _mm256_loadu_pd(colors_rgb + 4));
        _mm256_storeu_pd(col + 8, _mm256_loadu_pd(colors_rgb + 8));
        
        // Quantize each color
        for (int i = 0; i < 4; ++i) {
            int rBin = static_cast<int>(col[i*3] / scale);
            int gBin = static_cast<int>(col[i*3 + 1] / scale);
            int bBin = static_cast<int>(col[i*3 + 2] / scale);
            
            // Clamp to valid range
            rBin = (rBin < bins) ? rBin : (bins - 1);
            gBin = (gBin < bins) ? gBin : (bins - 1);
            bBin = (bBin < bins) ? bBin : (bins - 1);
            
            out_bins[i] = rBin * bins * bins + gBin * bins + bBin;
        }
    }

    // Batch data cost computation: compute -log(p) for 4 colors
    // This combines quantization (i.e bin index lookup), histogram lookup, and log computation
    inline void computeDataCosts4_AVX2(
        const double* colors_rgb,  // 12 doubles: 4 RGB triplets
        int bins,
        int bins_cubed,
        const std::vector<double>& histFG,
        const std::vector<double>& histBG,
        double eps,
        double* out_Dfg,           // 4 output foreground costs
        double* out_Dbg            // 4 output background costs
    ) {
        alignas(32) int bin_indices[4];
        quantizeColors4_AVX2(colors_rgb, bins, bin_indices);
        
        alignas(32) double pfg[4], pbg[4];

        for (int i = 0; i < 4; ++i) {
            int idx = bin_indices[i];
            if (idx >= 0 && idx < bins_cubed) {
                pfg[i] = histFG[idx];
                pbg[i] = histBG[idx];
            } else {
                pfg[i] = eps;
                pbg[i] = eps;
            }
        }
        
        negLog4_AVX2(pfg, eps, out_Dfg);
        negLog4_AVX2(pbg, eps, out_Dbg);
    }

#endif

    // fallback
    [[nodiscard]] inline double colorDistSq(const Vec3& a, const Vec3& b) noexcept {
        const double dr = a.r - b.r;
        const double dg = a.g - b.g;
        const double db = a.b - b.b;
        return dr*dr + dg*dg + db*db;
    }

} // namespace simd
