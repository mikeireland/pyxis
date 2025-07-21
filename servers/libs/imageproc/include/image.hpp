#pragma once

#include <opencv2/opencv.hpp>


namespace image {

// don't confuse with cv::Point2d ~ cv::Point_<double>
template <typename T> struct Point2D {
    cv::Point_<T> p1;
    cv::Point_<T> p2;
};

// simple subpixel interpolation with 3x3 subrect and grey scale image
struct LinearGradientInterp {
    cv::Point2d operator()(const cv::Mat &image, const cv::Rect &sub_rect);
    cv::Point2d operator()(const cv::Mat &image, const cv::Point &center) {
        return operator()(image, cv::Rect(center.x - 1, center.y - 1, 3, 3));
    }
    double inverse_linear_interp(cv::Point2d p1, cv::Point2d p2, double y);
};

struct ImageProcessSubMatInterp {
    bool do_gauss = true;
    bool do_median = true;
    double led_ratio_threshold = 0.6; // The second LED must be at least 96% of the first LED brightness to be considered valid
    int gauss_radius = 21;
    std::size_t margin = 20;
    int threshold = 10;

    cv::Mat diff_image;
    cv::Rect_<int> sub_rect{cv::Point_<int>{0, 0}, cv::Point_<int>{1440, 1080}};

    using InterpFunc =
        std::function<cv::Point2d(const cv::Mat &, const cv::Point &)>;

    explicit ImageProcessSubMatInterp(InterpFunc func = LinearGradientInterp())
        : interp(func){};

    Point2D<double> operator()(const cv::Mat &image_off,
                               const cv::Mat &image_on);
    Point2D<double> get_location(const cv::Mat &image_off,
                                 const cv::Mat &image_on);

private:
    InterpFunc interp;
};


cv::Point2d angle_to_center(const cv::Point2d& LEDposition, double img_width, double img_height, double pix_per_rad = 2.0);

double estimate_d(const cv::Point2d& alpha_1, const cv::Point2d& alpha_2);

cv::Point2d rotate90(const cv::Point2d& v);

cv::Point2d get_alpha_t(const cv::Point2d& alpha_1, const cv::Point2d& alpha_2, double beta, double gamma);

struct AlignmentError {
    cv::Point2d alpha_1;
    cv::Point2d alpha_2;
    cv::Point2d dlt_p;
};

AlignmentError compute_alignment_error(
    const cv::Point2d& LED1, const cv::Point2d& LED2,
    double beta, double gamma,
    const cv::Point2d& x0, const cv::Point2d& alpha_c,
    double width, double imsize, double pix_per_rad
);

} // namespace image
