#include "Image.h"
#include <iostream>

Image::Image(const std::string& path) {
    data = cv::imread(path, cv::IMREAD_COLOR);
    if (data.empty()) {
        std::cerr << "Error: Could not load image " << path << std::endl;
        std::exit(1);
    }

    // Convert to double precision for consistent math
    data.convertTo(data, CV_64FC3);
}

int Image::width() const {
    return data.cols;
}

int Image::height() const {
    return data.rows;
}

Vec3 Image::getColor(int x, int y) const {
    cv::Vec3d pix = data.at<cv::Vec3d>(y, x);  // OpenCV stores BGR
    return { pix[2], pix[1], pix[0] };         // convert to RGB order
}

void Image::convertToLab() {
    cv::cvtColor(data, data, cv::COLOR_BGR2Lab);
}

const cv::Mat& Image::mat() const {
    return data;
}
