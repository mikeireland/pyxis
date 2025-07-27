#include <cassert>
#include <fmt/core.h>
#include <image.hpp>
namespace image {

double LinearGradientInterp::inverse_linear_interp(cv::Point2d p1,
                                                   cv::Point2d p2, double y) {
    if (p1.y == p2.y)
        return (p1.x + p2.x) / 2.0;
    return (y - p1.y) / (p2.y - p1.y) * (p2.x - p1.x) + p1.x;
}

cv::Point2d LinearGradientInterp::operator()(const cv::Mat &image,
                                             const cv::Rect &sub_rect) {
                                             
    assert(sub_rect.width == 3 && sub_rect.height == 3 &&
           "LinearGradientInterp only defined for 3x3 subrect of image");
    assert(image.channels() == 1 &&
           "LinearGradientInterp only defined for grey scale image");

    const auto &img = image(sub_rect);

    cv::Mat sumx, sumy;
    cv::reduce(img, sumx, 0, cv::REDUCE_SUM, CV_32FC1);
    cv::reduce(img, sumy, 1, cv::REDUCE_SUM, CV_32FC1);
    // This "interp" function is a lambda that takes a cv::Mat and returns a double.
    // it is only available in this scope.
    auto interp = [this](const cv::Mat &v) {
        double diff[2];
        double ret;
        for (int i = 0; i != 2; i++) {
            diff[i] = (v.at<int>(i + 1) - v.at<int>(i)) / 2.0;
        }
        return inverse_linear_interp(cv::Point2d(-1 / 2.0, diff[0]),
                                     cv::Point2d(1 / 2.0, diff[1]), 0);
    };
    return cv::Point2d(interp(sumx), interp(sumy)) +
           static_cast<cv::Point2d>(sub_rect.tl());
}

Point2D<double>
ImageProcessSubMatInterp::get_location(const cv::Mat &image_on,
                                       const cv::Mat &image_off) {

    // cv::Mat image_on_float, image_off_float, diff_image;
    // auto sub_image_off = image_off(sub_rect);
    // auto sub_image_on = image_on(sub_rect);
    // diff the images the highlight LEDs
    cv::subtract(image_on, image_off, diff_image);
    //Edited by Qianhui: save the image for debugging
    // cv::imwrite("/home/pyxisuser/pyxis/servers/coarse_metrology/data/image_on.tiff", image_on);
    // cv::imwrite("/home/pyxisuser/pyxis/servers/coarse_metrology/data/image_off.tiff", image_off);
    
    if (do_gauss)
        cv::GaussianBlur(diff_image, diff_image,
                         cv::Size(gauss_radius, gauss_radius), 0, 0,
                         cv::BORDER_DEFAULT);
    if (do_median)
        cv::medianBlur(diff_image, diff_image, 5);//Edited by Qianhui: changed the median filter size from 3 to 5

    //diff_image.convertTo(diff_image, CV_32F);
    //Edited by Qianhui: save the image for debugging
    // cv::imwrite("/home/pyxisuser/pyxis/servers/coarse_metrology/data/diff_image_med.tiff", diff_image);
    
    Point2D<int> p;
    Point2D<double> p_ret;
    double maxVal1, maxVal2;

    // take maximum, find centroid in region, then set a circle around centroid to 0, and repeat
    cv::minMaxLoc(diff_image, nullptr, &maxVal1, nullptr, &p.p1);
    //p_ret.p1 = interp(diff_image, p.p1); // !!! Fails at the edge of the image
    p_ret.p1 = p.p1;
    //NB This next line looks like it will fail if the circle is too close to the edge of the image.
    cv::circle(diff_image, p.p1, 20, cv::Scalar(0, 0, 0), cv::FILLED);
    cv::minMaxLoc(diff_image, nullptr, &maxVal2, nullptr, &p.p2);
    //p_ret.p2 = interp(diff_image, p.p2); // !!! Fails at the edge of the image
    p_ret.p2 = p.p2;

    if (maxVal2 < led_ratio_threshold * maxVal1) {
        // Second peak is too weak, likely noise. led_ratio_threshold is defined in the image.hpp
        p_ret.p2 = cv::Point(-1, -1); // Obiously this is not found.
        std::cout << "Second maximal is too weak, likely noise. Not both LEDs are found" << std::endl;
    }
    else {
        // If both LEDs are found, we make the left-hand LED as LED1
        if (p_ret.p1.x > p_ret.p2.x) {
            std::swap(p_ret.p1, p_ret.p2);
        }
    }

    return p_ret;
}

Point2D<double> ImageProcessSubMatInterp::operator()(const cv::Mat &image_on,
                                                     const cv::Mat &image_off) {
    // auto p = get_location(image_off(sub_rect), image_on(sub_rect));
    // p.p1.x += sub_rect.x;
    // p.p1.y += sub_rect.y;
    // p.p2.x += sub_rect.x;
    // p.p2.y += sub_rect.y;

    // if (image_on.at<int>(p.p1) < threshold || image_on.at<int>(p.p2) <
    // threshold)
    // {
    //     // if the location is too dark, try the whole image
    //     p = get_location(image_off, image_on);
    // }
    // fmt::print("sub_rect: ({}, {}), ({}, {})\n", sub_rect.x, sub_rect.y,
    // sub_rect.width, sub_rect.height);

    auto p = get_location(image_on, image_off);// Upper lines are commented out by Qianhui, I don't get it, why the previous code needs to sub_rect???
    auto x_min = std::min(p.p1.x, p.p2.x);
    auto x_max = std::max(p.p1.x, p.p2.x);
    auto y_min = std::min(p.p1.y, p.p2.y);
    auto y_max = std::max(p.p1.y, p.p2.y);

    x_min = x_min < 20 ? 0 : x_min - margin;
    x_max = x_max > image_on.cols - 20 ? image_on.cols : x_max + margin;
    y_min = y_min < 20 ? 0 : y_min - margin;
    y_max = y_max > image_on.rows - 20 ? image_on.rows : y_max + margin;

    // sub_rect = cv::Rect(x_min, y_min, x_max - x_min, y_max - y_min);

    return p;
}


// Returns angular distance in radians from any LED to image center (corase met camera).

cv::Point2d angle_to_center(const cv::Point2d& LEDposition, double img_width, double img_height, double pix_per_rad) {
    double cx = (img_width - 1) / 2.0;
    double cy = (img_height - 1) / 2.0;
    double alpha_x = (LEDposition.x - cx) / pix_per_rad;
    double alpha_y = (LEDposition.y - cy) / pix_per_rad;
    return cv::Point2d(alpha_x, alpha_y);

}


// Estimate the distance (d) between chief and deputy based on the angular positions of the LEDs

double estimate_d(const cv::Point2d& alpha_1, const cv::Point2d& alpha_2) {
    //magnitude of vec(alpha1) - vec(alpha2):
    double diff = cv::norm(alpha_1 - alpha_2);

    // Avoid division by zero
    if (diff < 1e-6) return 0;

    double d = 90.0 / diff; //distance between chief and deputy is estimated as 90mm/ |alpha1 - alpha2 |
    return d;

}


//Define a rotation matrix for 90 degrees counter-clockwise rotation

cv::Point2d rotate90(const cv::Point2d& v) {
    // R90 * v = (-v.y, v.x)
    return cv::Point2d(-v.y, v.x);

}


// Compute alpha_t based on the coarse metrology camera LED positions
//alpha_t is the angular position where the light comes out from deputy.

cv::Point2d get_alpha_t(const cv::Point2d& alpha_1, const cv::Point2d& alpha_2, double beta, double gamma) {
    // Compute R90 * alpha2 and R90 * alpha1
    cv::Point2d R90_alpha2 = rotate90(alpha_2);
    cv::Point2d R90_alpha1 = rotate90(alpha_1);

    // (beta + gamma * R90) * alpha_2
    cv::Point2d term1 = beta * alpha_2 + gamma * R90_alpha2;

    // (1 - beta - gamma * R90) * alpha1 ==
    // (1 - beta) * alpha1 - gamma * R90 * alpha1
    cv::Point2d term2 = (1.0 - beta) * alpha_1 - gamma * R90_alpha1;

    cv::Point2d alpha_t = term1 + term2; //current alpha_t derived from LED position seen in coarse metrology camera
    return alpha_t;

}


// Calculate the current alpha_t on deputy and the error between alpha_t and expected alpha_t
// Compute alpha_t based on coarse met camera, defined in image.cpp and declared in image.hpp
AlignmentError compute_alignment_error(
    const cv::Point2d& LED1, //LED1 pixel coordinates
    const cv::Point2d& LED2, //LED2 pixel coordinates
    double beta,
    double gamma,
    const cv::Point2d& x0,
    const cv::Point2d& alpha_c,
    double width, // image width
    double imsize, // image size
    double pix_per_rad // pixels per radian (GLOB_PIX_PER_RAD)
) {
    try {
    // Check for invalid LED positions
        if ((LED1.x < 0 || LED1.y < 0) || (LED2.x < 0 || LED2.y < 0)) {
            std::cerr << "Error: Invalid LED position(s) detected." << std::endl;
            return AlignmentError{cv::Point2d(0,0), cv::Point2d(0,0), cv::Point2d(-1, -1)};
        }
        // Convert the pixel coordinates (p_ret) of LEDs to angular coordinates relative to the camera center
        cv::Point2d alpha_1 = angle_to_center(LED1, width, imsize/width, pix_per_rad);
        cv::Point2d alpha_2 = angle_to_center(LED2, width, imsize/width, pix_per_rad);

        // Estimate the distance (d) between chief and deputy
        double d = estimate_d(alpha_1, alpha_2);
        if (d <= 0) {
            std::cerr << "Error: Estimated distance d is zero or negative." << std::endl;
            return AlignmentError{alpha_1, alpha_2, cv::Point2d(0,0)};
        }

        // Compute alpha_t using your function
        cv::Point2d alpha_t = get_alpha_t(alpha_1, alpha_2, beta, gamma);

        // Compute expected alpha_t based on the chief design,
        // the light from deputy should aim into this expected alpha_t.
        // expected alpha_t = (x0 +alpha_c*d) / d
        cv::Point2d exp_alpha_t = (x0 + alpha_c * d) / d;

        // Calculate the difference vector
        cv::Point2d dlt_alpha_t = alpha_t - exp_alpha_t;
        cv::Point2d dlt_p = dlt_alpha_t * d;

        // Optionally print the results
        std::cout << "Distance d (mm): " << d << std::endl;
        std::cout << "Difference between alpha_t and expected_alpha_t: " << dlt_alpha_t << std::endl;
        std::cout << "Movement in Y and Z (mm): " << dlt_p << std::endl;

        // Return the difference vector
        return AlignmentError{alpha_1, alpha_2, dlt_p, alpha_t, exp_alpha_t};
    } catch (const std::exception& e) {
        std::cerr << "Exception in compute_alignment_error: " << e.what() << std::endl;
        return AlignmentError{cv::Point2d(0,0), cv::Point2d(0,0), cv::Point2d(-1, -1), cv::Point2d(0,0), cv::Point2d(0,0)};
    }
}

} // namespace image
