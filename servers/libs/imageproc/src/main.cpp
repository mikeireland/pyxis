#include <image.hpp>

#include <fmt/core.h>
#include <opencv2/opencv.hpp>

float get_mean(const cv::Mat &img) {
    cv::Scalar mean = cv::mean(img);
    return mean[0];
}

std::string type2str(int type) {
  std::string r;

  uchar depth = type & CV_MAT_DEPTH_MASK;
  uchar chans = 1 + (type >> CV_CN_SHIFT);

  switch ( depth ) {
    case CV_8U:  r = "CV_8U"; break;
    case CV_8S:  r = "CV_8S"; break;
    case CV_16U: r = "CV_16U"; break;
    case CV_16S: r = "CV_16S"; break;
    case CV_32S: r = "CV_32S"; break;
    case CV_32F: r = "CV_32F"; break;
    case CV_64F: r = "CV_64F"; break;
    default:     r = "User"; break;
  }

  r += "C";
  r += (chans+'0');

  return r;
}

int main(int argc, char **argv) {

    // auto file_off = "near_off.tiff";
    // auto file_on = "near_on.tiff";

    auto file_off = "far_off.tiff";
    auto file_on = "far_on.tiff";

    cv::Mat image_off = cv::imread(file_off, cv::IMREAD_GRAYSCALE);
    cv::Mat image_on  = cv::imread(file_on , cv::IMREAD_GRAYSCALE);

    auto image_type = image_off.type();

    fmt::print("image_off type: {}\n", type2str(image_type));


    // auto mean_on = get_mean(image_on);
    // auto mean_off = get_mean(image_off);

    // fmt::print("mean_on: {}\n", mean_on);
    // fmt::print("mean_off: {}\n", mean_off);

    auto ipb = image::ImageProcessSubMatInterp(image::CentroidInterp());
    ipb.do_gauss = true;

    // Warmup
    ipb(image_off, image_on);
    ipb(image_off, image_on);
    ipb(image_off, image_on);
    ipb(image_off, image_on);
    ipb(image_off, image_on);

    auto p = ipb(image_off, image_on);

    // print locations
    fmt::print("loc_1: ({}, {})\n", p.p1.x, p.p1.y);
    fmt::print("loc_2: ({}, {})\n", p.p2.x, p.p2.y);

    // draw circles around locations
    cv::circle(image_on, p.p1, 20, cv::Scalar(255), 2);
    cv::circle(image_on, p.p2, 20, cv::Scalar(255), 2);

    cv::namedWindow("Display Image diff", cv::WINDOW_AUTOSIZE);
    cv::imshow("Display Image diff", image_on);
    cv::waitKey(0);

    return 0;
}