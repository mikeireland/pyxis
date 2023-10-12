#include <opencv2/opencv.hpp>
#include <cassert>
#include "centroid.hpp"


namespace centroid_funcs {

/*
Implementation of Numpy mesh grid
Inputs:
    x - x coordinates
    y - y coordinates
    X - output X array
    Y - output Y array
*/
void meshgrid(const cv::Mat &x, const cv::Mat &y, cv::Mat &X, cv::Mat &Y) {
        cv::repeat(x.reshape(1, 1), y.total(), 1, X);
        cv::repeat(y.reshape(1, 1).t(), 1, x.total(), Y);
    }
    
 
/*
Centroiding function to take an image (or subset thereof), with the brightest pixel identified, and interpolate using the simple centre of gravity method.
Called on by the windowed functions, which are higher level and should be preferred
Inputs
    image - image (or subset thereof) to extract centroid from
    center - position of brightest pixel
    interp_size - radius of nearest pixels to interpolate centroid
Output
    2D coordinates of centroid
*/
cv::Point2d getCentroidCOG(const cv::Mat &image, const cv::Point &center, int interp_size) {

    assert(image.channels() == 1 &&
           "CentroidInterp only defined for grey scale image");
    
    // Define interpolation region
    auto sub_rect = cv::Rect(center.x - (interp_size-1)/2, center.y - (interp_size-1)/2, 
                        interp_size, interp_size);
    cv::Mat img = image(sub_rect);

    // Set up meshgrid
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
    
    // Get centre of gravity centroid position
    x = double(cv::sum(XX.mul(img))[0]) / cv::sum(img)[0];
    y = double(cv::sum(YY.mul(img))[0]) / cv::sum(img)[0];
    return cv::Point2d(x, y) + static_cast<cv::Point2d>(sub_rect.tl());
}

/*
Function to calculate the WCOG weights (super Gaussian form)
Inputs
    interp_size - radius of nearest pixels to interpolate centroid
Output
    Matrix of weights, of size interp_size x interp_size
*/
cv::Mat weightFunction(int interp_size, double sigma){

    int radius = (interp_size-1)/2;
    
    // Meshgrid
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
    
    // Calculate super-Gaussian weights
    cv::pow(XX2,2,XX2);
    cv::pow(YY2,2,YY2);
    ZZ = XX2 + YY2;
    cv::pow(ZZ,2,ZZ);
    ZZ *= -1.0/(2.0*sigma*sigma);

    cv::exp(ZZ,ZZ);

    return ZZ;
}

/*
Centroiding function to take an image (or subset thereof), with the brightest pixel identified, and interpolate using the weighted centre of gravity method.
Called on by the windowed functions, which are higher level and should be preferred
Inputs
    image - image (or subset thereof) to extract centroid from
    center - position of brightest pixel
    weights - weights for the WCOG method
    gain - gain for the WCOG method
    interp_size - radius of nearest pixels to interpolate centroid
Output
    2D coordinates of centroid
*/
cv::Point2d getCentroidWCOG(const cv::Mat &image, const cv::Point &center, const cv::Mat &weights, int interp_size, double gain) {

    assert(image.channels() == 1 &&
           "CentroidInterp only defined for grey scale image");
    
    // Define interpolation region
    int radius = (interp_size-1)/2;
    auto sub_rect = cv::Rect(center.x - radius, center.y - radius, interp_size, interp_size);

    cv::Mat img = image(sub_rect);
    
    // Set up meshgrid
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

    // Weight the image
    img.convertTo(weighted_image, CV_32F);
    weighted_image = weighted_image.mul(weights);
    
    // Calculate centroid based on WCOG
    x = double(cv::sum(XX.mul(weighted_image))[0]) / cv::sum(weighted_image)[0];
    y = double(cv::sum(YY.mul(weighted_image))[0]) / cv::sum(weighted_image)[0];
 
    return (cv::Point2d(x, y) + static_cast<cv::Point2d>(sub_rect.tl()))*gain;
}

/*
Windowed centroiding function to take an image and find the brightest centroid using the simple centre of gravity method.
Undergoes gaussian smoothing beforehand to remove hot pixels. Accepts an arbitrary window.
Inputs
    image - image to extract centroid from
    interp_size - radius of nearest pixels to interpolate centroid
    gauss_radius - radius of gaussian smoothing kernel
    window - window from which to extract the centroid
    weights - weights for the WCOG method
    gain - gain for the WCOG method
Output
    2D coordinates of centroid
*/
cv::Point2d windowCentroidCOG(const cv::Mat &image, int interp_size, int gauss_radius, const cv::Rect &window ) {

    assert(image.channels() == 1 &&
           "CentroidInterp only defined for grey scale image");
    
    // Extract sub region
    cv::Mat window_img = image(window);

    // Clone for gaussian blur
    cv::Mat gauss_img = window_img.clone();
    // Perform a gaussian blur for noise purposes
    cv::GaussianBlur(gauss_img, gauss_img, cv::Size(gauss_radius, gauss_radius), 0, 0, cv::BORDER_DEFAULT);

    cv::Point2i p_est;
    cv::Point2d p_ret;

    // Bounds to avoid extending beyond the edges
    auto safe_bounds = cv::Rect((interp_size-1)/2, (interp_size-1)/2, window.width-(interp_size-1), window.height-(interp_size-1));

    cv::Mat safe_img = gauss_img(safe_bounds);

    // Locate brightest pixel from gaussian blurred image
    cv::minMaxLoc(safe_img, nullptr, nullptr, nullptr, &p_est);
    // Correct for bounds
    p_est += static_cast<cv::Point2i>(window.tl()) + static_cast<cv::Point2i>(safe_bounds.tl());

    // Extract centroid from NON-gaussian image, but centred on extracted brightest pixel
    p_ret = getCentroidCOG(image, p_est, interp_size);

    return p_ret;
}

/*
Windowed centroiding function to take an image and find the brightest centroid using the simple centre of gravity method.
Undergoes gaussian smoothing beforehand to remove hot pixels. Window is square, centred on a point with a given side length
Inputs
    image - image to extract centroid from
    interp_size - radius of nearest pixels to interpolate centroid
    gauss_radius - radius of gaussian smoothing kernel
    center - centre of window
    window_size - side length of window
Output
    2D coordinates of centroid
*/
cv::Point2d windowCentroidCOG(const cv::Mat &image, int interp_size, int gauss_radius, const cv::Point &center, int window_size) {

    assert(image.channels() == 1 &&
           "CentroidInterp only defined for grey scale image");

    // Define and extract sub region
    auto window = cv::Rect(center.x - (window_size)/2, center.y - (window_size)/2, window_size, window_size);
    cv::Mat window_img = image(window);

    // Clone for gaussian blur
    cv::Mat gauss_img = window_img.clone();
    // Perform a gaussian blur for noise purposes
    cv::GaussianBlur(gauss_img, gauss_img, cv::Size(gauss_radius, gauss_radius), 0, 0, cv::BORDER_DEFAULT);

    cv::Point2i p_est;
    cv::Point2d p_ret;

    // Locate brightest pixel from gaussian blurred image
    cv::minMaxLoc(gauss_img, nullptr, nullptr, nullptr, &p_est);
    
    // Correct for bounds
    p_est += static_cast<cv::Point2i>(window.tl());

    // Extract centroid from NON-gaussian image, but centred on extracted brightest pixel
    p_ret = getCentroidCOG(image, p_est, interp_size);

    return p_ret;
}

/*
Windowed centroiding function to take an image and find the brightest centroid using the weighted centre of gravity method.
Undergoes gaussian smoothing beforehand to remove hot pixels. Accepts an arbitrary window.
Inputs
    image - image to extract centroid from
    interp_size - radius of nearest pixels to interpolate centroid
    gauss_radius - radius of gaussian smoothing kernel
    window - window from which to extract the centroid
    weights - weights for the WCOG method
    gain - gain for the WCOG method
Output
    2D coordinates of centroid
*/
cv::Point2d windowCentroidWCOG(const cv::Mat &image, int interp_size, int gauss_radius, const cv::Rect &window,
                               const cv::Mat &weights, double gain ) {

    assert(image.channels() == 1 &&
           "CentroidInterp only defined for grey scale image");
    
    // Define and extract sub region
    cv::Mat window_img = image(window);

    // Clone for gaussian blur
    cv::Mat gauss_img = window_img.clone();
    // Perform a gaussian blur for noise purposes
    cv::GaussianBlur(gauss_img, gauss_img, cv::Size(gauss_radius, gauss_radius), 0, 0, cv::BORDER_DEFAULT);

    cv::Point2i p_est;
    cv::Point2d p_ret;

    // Locate brightest pixel from gaussian blurred image
    cv::minMaxLoc(gauss_img, nullptr, nullptr, nullptr, &p_est);

    // Correct for bounds
    p_est += static_cast<cv::Point2i>(window.tl());

    // Extract centroid from NON-gaussian image, but centred on extracted brightest pixel
    p_ret = getCentroidWCOG(image, p_est, weights, interp_size, gain);

    return p_ret;
}

/*
Windowed centroiding function to take an image and find the brightest centroid using the weighted centre of gravity method.
Undergoes gaussian smoothing beforehand to remove hot pixels. Window is square, centred on a point with a given side length
Inputs
    image - image to extract centroid from
    interp_size - radius of nearest pixels to interpolate centroid
    gauss_radius - radius of gaussian smoothing kernel
    center - centre of window
    window_size - side length of window
    weights - weights for the WCOG method
    gain - gain for the WCOG method
Output
    2D coordinates of centroid
*/
cv::Point2d windowCentroidWCOG(const cv::Mat &image, int interp_size, int gauss_radius, const cv::Point &center, int window_size,
                               const cv::Mat &weights, double gain) {

    assert(image.channels() == 1 &&
           "CentroidInterp only defined for grey scale image");

    std::cout << center << std::endl;    

    // Define and extract sub region
    auto window = cv::Rect(center.x - (window_size)/2, center.y - (window_size)/2, window_size, window_size);
    cv::Mat window_img = image(window);

    // Clone for gaussian blur
    cv::Mat gauss_img = window_img.clone();
    // Perform a gaussian blur for noise purposes
    cv::GaussianBlur(gauss_img, gauss_img, cv::Size(gauss_radius, gauss_radius), 0, 0, cv::BORDER_DEFAULT);

    cv::Point2i p_est;
    cv::Point2d p_ret;
    
    // Bounds to avoid extending beyond the edges
    auto safe_bounds = cv::Rect((interp_size-1)/2, (interp_size-1)/2, window_size-(interp_size-1), window_size-(interp_size-1));

    cv::Mat safe_img = gauss_img(safe_bounds);

    // Locate brightest pixel from gaussian blurred image
    cv::minMaxLoc(safe_img, nullptr, nullptr, nullptr, &p_est);

    // Correct for bounds
    p_est += static_cast<cv::Point2i>(window.tl()) + static_cast<cv::Point2i>(safe_bounds.tl());

    // Extract centroid from NON-gaussian image, but centred on extracted brightest pixel
    p_ret = getCentroidWCOG(image, p_est, weights, interp_size, gain);

    return p_ret;
}
}
