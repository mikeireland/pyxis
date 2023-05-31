#pragma once

#include <opencv2/opencv.hpp>
struct centroid {
    double x;
    double y;
};
namespace centroid_funcs {

cv::Point2d getCentroidCOG(const cv::Mat &image, const cv::Point &center, int interp_size);

cv::Point2d getCentroidWCOG(const cv::Mat &image, const cv::Point &center, const cv::Mat &weights, int interp_size, double gain);

cv::Mat weightFunction(int interp_size, double sigma);

cv::Point2d windowCentroidCOG(const cv::Mat &image, int interp_size, int gauss_radius, const cv::Rect &window );

cv::Point2d windowCentroidCOG(const cv::Mat &image, int interp_size, int gauss_radius, const cv::Point &center, int window_size);

cv::Point2d windowCentroidWCOG(const cv::Mat &image, int interp_size, int gauss_radius, const cv::Rect &window,
                               const cv::Mat &weights, double gain );

cv::Point2d windowCentroidWCOG(const cv::Mat &image, int interp_size, int gauss_radius, const cv::Point &center, int window_size,
                               const cv::Mat &weights, double gain);        
}
