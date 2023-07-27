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

struct CentroidInterp {
    int interp_size = 3;
    cv::Point2d operator()(const cv::Mat &image, const cv::Rect &sub_rect);
    cv::Point2d operator()(const cv::Mat &image, const cv::Point &center) {
        return operator()(image, cv::Rect(center.x - (interp_size-1)/2, center.y - (interp_size-1)/2, interp_size, interp_size));
    }
    void meshgrid(const cv::Mat &x, const cv::Mat &y, cv::Mat &X, cv::Mat &Y) {
        cv::repeat(x.reshape(1, 1), y.total(), 1, X);
        cv::repeat(y.reshape(1, 1).t(), 1, x.total(), Y);
    }
};

struct ImageProcessBasic {
    bool do_gauss = true;
    int gauss_radius = 21;

    cv::Mat diff_image;

    Point2D<int> operator()(const cv::Mat &image_off, const cv::Mat &image_on);
};

struct ImageProcessSubMat {
    bool do_gauss = false;
    int gauss_radius = 21;
    std::size_t margin = 20;
    int threshold = 10;

    cv::Mat diff_image;

    cv::Rect_<int> sub_rect{cv::Point_<int>{0, 0}, cv::Point_<int>{1440, 1080}};

    Point2D<int> operator()(const cv::Mat &image_off, const cv::Mat &image_on);
    Point2D<int> get_location(const cv::Mat &image_off,
                              const cv::Mat &image_on);
};

// too lazy to do inheritance
struct ImageProcessSubMatInterp {
    bool do_gauss = false;
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

// too lazy to do inheritance
struct ImageProcessSubMatInterpSingle {

    bool do_gauss = true;
    int gauss_radius = 21;
    std::size_t margin = 500;

    int centroid_interp_size = 3;


    using InterpFunc =
        std::function<cv::Point2d(const cv::Mat &, const cv::Point &)>;

    explicit ImageProcessSubMatInterpSingle(const int& height, const int& width, InterpFunc func = CentroidInterp())
        : imheight(height), imwidth(width), interp(func){};

    int imheight;
    int imwidth;

    cv::Rect_<int> sub_rect{cv::Point_<int>{0, 0}, cv::Point_<int>{imwidth, imheight}};



    cv::Point2d operator()(const cv::Mat &image);
    cv::Point2d get_location(const cv::Mat &image);

private:
    InterpFunc interp;

};

} // namespace image
