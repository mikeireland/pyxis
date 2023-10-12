#pragma once

#include <opencv2/opencv.hpp>

// Centroid struct
struct centroid {
    double x;
    double y;
};
namespace centroid_funcs {

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
cv::Point2d getCentroidCOG(const cv::Mat &image, const cv::Point &center, int interp_size);

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
cv::Point2d getCentroidWCOG(const cv::Mat &image, const cv::Point &center, const cv::Mat &weights, int interp_size, double gain);

/*
Function to calculate the WCOG weights (super Gaussian form)
Inputs
    interp_size - radius of nearest pixels to interpolate centroid
Output
    Matrix of weights, of size interp_size x interp_size
*/
cv::Mat weightFunction(int interp_size, double sigma);

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
cv::Point2d windowCentroidCOG(const cv::Mat &image, int interp_size, int gauss_radius, const cv::Rect &window );

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
cv::Point2d windowCentroidCOG(const cv::Mat &image, int interp_size, int gauss_radius, const cv::Point &center, int window_size);

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
                               const cv::Mat &weights, double gain );

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
                               const cv::Mat &weights, double gain);        
}
