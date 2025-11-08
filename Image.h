#pragma once
#include <string>
#include <opencv2/opencv.hpp>

// Simple color struct for internal use
struct Vec3 {
    double r, g, b;
};

class Image {
public:
    // Constructor: loads image from file
    explicit Image(const std::string& path);

    // Get dimensions
    int width() const;
    int height() const;

    // Access color at (x, y)
    Vec3 getColor(int x, int y) const;

    // Convert to Lab color space (optional, for later)
    void convertToLab();

    // For debugging or visualization
    const cv::Mat& mat() const;

private:
    cv::Mat data;  // Stores image as double precision BGR (OpenCV default)
};
