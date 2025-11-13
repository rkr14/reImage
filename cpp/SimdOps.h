#pragma once
#include "Image.h"
#include <immintrin.h> 
#include <cmath>


namespace simd {

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
#endif

    // fallback
    [[nodiscard]] inline double colorDistSq(const Vec3& a, const Vec3& b) noexcept {
        const double dr = a.r - b.r;
        const double dg = a.g - b.g;
        const double db = a.b - b.b;
        return dr*dr + dg*dg + db*db;
    }

} // namespace simd
