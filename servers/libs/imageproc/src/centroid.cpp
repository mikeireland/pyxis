#include <opencv2/opencv.hpp>
#include <cassert>
#include "centroid.hpp"


namespace centroid_funcs {


void meshgrid(const cv::Mat &x, const cv::Mat &y, cv::Mat &X, cv::Mat &Y) {
        cv::repeat(x.reshape(1, 1), y.total(), 1, X);
        cv::repeat(y.reshape(1, 1).t(), 1, x.total(), Y);
    }
    
 

cv::Point2d getCentroidCOG(const cv::Mat &image, const cv::Point &center, int interp_size) {

    assert(image.channels() == 1 &&
           "CentroidInterp only defined for grey scale image");
    
    auto sub_rect = cv::Rect(center.x - (interp_size-1)/2, center.y - (interp_size-1)/2, 
                        interp_size, interp_size);
    cv::Mat img = image(sub_rect);
    cv::Mat gridx, gridy;
    cv::Mat XX, YY;
    double x, y;
    for (int i = 0; i != sub_rect.width; i++){
        gridx.push_back(i);
        }
    for (int i = 0; i != sub_rect.height; i++){
        gridy.push_back(i);
        }
    meshgrid(gridx, gridy, XX, YY);
    XX.convertTo(XX, img.type());
    YY.convertTo(YY, img.type());
    
    std::cout << XX << std::endl;
    x = double(cv::sum(XX.mul(img))[0]) / cv::sum(img)[0];
    y = double(cv::sum(YY.mul(img))[0]) / cv::sum(img)[0];
    return cv::Point2d(x, y) + static_cast<cv::Point2d>(sub_rect.tl());
}

cv::Mat weightFunction(int interp_size, double sigma){

    int radius = (interp_size-1)/2;
    
    cv::Mat gridx, gridy;
    cv::Mat XX, YY;
    cv::Mat XX2, YY2, ZZ;
    double x, y;
    for (int i = 0; i != interp_size; i++){
        gridx.push_back(i);
        gridy.push_back(i);
    }
    meshgrid(gridx, gridy, XX, YY);
    XX.convertTo(XX, CV_32F);
    YY.convertTo(YY, CV_32F);

    XX2 = XX.clone() - radius;
    YY2 = YY.clone() - radius;
    
    cv::pow(XX2,2,XX2);
    cv::pow(YY2,2,YY2);
    ZZ = XX2 + YY2;
    cv::pow(ZZ,2,ZZ);
    ZZ *= -1.0/(2.0*sigma*sigma);

    cv::exp(ZZ,ZZ);

    return ZZ;
}


cv::Point2d getCentroidWCOG(const cv::Mat &image, const cv::Point &center, const cv::Mat &weights, int interp_size, double gain) {

    assert(image.channels() == 1 &&
           "CentroidInterp only defined for grey scale image");
    
    int radius = (interp_size-1)/2;
    auto sub_rect = cv::Rect(center.x - radius, center.y - radius, interp_size, interp_size);

    cv::Mat img = image(sub_rect);
    
    cv::Mat gridx, gridy;
    cv::Mat XX, YY, weighted_image;
    double x, y;
    for (int i = 0; i != interp_size; i++){
        gridx.push_back(i);
        gridy.push_back(i);
    }
    meshgrid(gridx, gridy, XX, YY);
    XX.convertTo(XX, CV_32F);
    YY.convertTo(YY, CV_32F);

    img.convertTo(weighted_image, CV_32F);
    weighted_image = weighted_image.mul(weights);
    
    x = double(cv::sum(XX.mul(weighted_image))[0]) / cv::sum(weighted_image)[0];
    y = double(cv::sum(YY.mul(weighted_image))[0]) / cv::sum(weighted_image)[0];
    
    x*=gain;
    y*=gain;
    return cv::Point2d(x, y) + static_cast<cv::Point2d>(sub_rect.tl());
}


cv::Point2d windowCentroidCOG(const cv::Mat &image, int interp_size, int gauss_radius, const cv::Rect &window ) {

    assert(image.channels() == 1 &&
           "CentroidInterp only defined for grey scale image");
    
    cv::Mat window_img = image(window);

    cv::Mat gauss_img = window_img.clone();
    // Perform a gaussian blur for noise purposes
    cv::GaussianBlur(gauss_img, gauss_img, cv::Size(gauss_radius, gauss_radius), 0, 0, cv::BORDER_DEFAULT);

    cv::Point2i p_est;
    
    cv::Point2d p_ret;

    cv::minMaxLoc(gauss_img, nullptr, nullptr, nullptr, &p_est);

    p_est += static_cast<cv::Point2i>(window.tl());

    p_ret = getCentroidCOG(image, p_est, interp_size);

    return p_ret;
}

cv::Point2d windowCentroidCOG(const cv::Mat &image, int interp_size, int gauss_radius, const cv::Point &center, int window_size) {

    assert(image.channels() == 1 &&
           "CentroidInterp only defined for grey scale image");

    auto window = cv::Rect(center.x - (window_size-1)/2, center.y - (window_size-1)/2, window_size, window_size);
    cv::Mat window_img = image(window);

    cv::Mat gauss_img = window_img.clone();
    // Perform a gaussian blur for noise purposes
    cv::GaussianBlur(gauss_img, gauss_img, cv::Size(gauss_radius, gauss_radius), 0, 0, cv::BORDER_DEFAULT);

    cv::Point2i p_est;
    cv::Point2d p_ret;

    cv::minMaxLoc(gauss_img, nullptr, nullptr, nullptr, &p_est);
    
    p_est += static_cast<cv::Point2i>(window.tl());

    p_ret = getCentroidCOG(image, p_est, interp_size);

    return p_ret;
}

cv::Point2d windowCentroidWCOG(const cv::Mat &image, int interp_size, int gauss_radius, const cv::Rect &window,
                               const cv::Mat &weights, double gain ) {

    assert(image.channels() == 1 &&
           "CentroidInterp only defined for grey scale image");
    
    cv::Mat window_img = image(window);

    cv::Mat gauss_img = window_img.clone();
    // Perform a gaussian blur for noise purposes
    cv::GaussianBlur(gauss_img, gauss_img, cv::Size(gauss_radius, gauss_radius), 0, 0, cv::BORDER_DEFAULT);

    cv::Point2i p_est;
    cv::Point2d p_ret;

    cv::minMaxLoc(gauss_img, nullptr, nullptr, nullptr, &p_est);

    p_est += static_cast<cv::Point2i>(window.tl());

    p_ret = getCentroidWCOG(image, p_est, weights, interp_size, gain);

    return p_ret;
}

cv::Point2d windowCentroidWCOG(const cv::Mat &image, int interp_size, int gauss_radius, const cv::Point &center, int window_size,
                               const cv::Mat &weights, double gain) {

    assert(image.channels() == 1 &&
           "CentroidInterp only defined for grey scale image");

    auto window = cv::Rect(center.x - (window_size-1)/2, center.y - (window_size-1)/2, window_size, window_size);
    cv::Mat window_img = image(window);

    cv::Mat gauss_img = window_img.clone();
    // Perform a gaussian blur for noise purposes
    cv::GaussianBlur(gauss_img, gauss_img, cv::Size(gauss_radius, gauss_radius), 0, 0, cv::BORDER_DEFAULT);

    cv::Point2i p_est;
    cv::Point2d p_ret;
    
    auto safe_bounds = cv::Rect((interp_size-1)/2, (interp_size-1)/2, window_size-(interp_size-1), window_size-(interp_size-1));

    cv::minMaxLoc(gauss_img(safe_bounds), nullptr, nullptr, nullptr, &p_est);

    p_est += static_cast<cv::Point2i>(window.tl()) + static_cast<cv::Point2i>(safe_bounds.tl());

    p_ret = getCentroidWCOG(image, p_est, weights, interp_size, gain);

    return p_ret;
}
}
