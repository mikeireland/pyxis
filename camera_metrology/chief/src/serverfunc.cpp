#include <fmt/core.h>
#include <fstream>
#include <image.hpp>
#include <memory>
#include <opencv2/opencv.hpp>
#include <string>
#include <toml.hpp>

using namespace std;

namespace {
std::shared_ptr<toml::table> config;
}

namespace serverfunc {

int load_config(string path) {
    config = make_shared<toml::table>(toml::parse_file(path));
    return 0;
}

int save_config(string path) {
    ofstream out(path);
    out << *config;
    return 0;
}

toml::table &get_config() { return *config; }

image::Point2D<double> get_led_location() {
    auto camera = (*config)["camera"];
    string &base_folder = camera["base_folder"].ref<string>();
    string file_off = base_folder + '/' + camera["file"]["off"].ref<string>();
    string file_on = base_folder + '/' + camera["file"]["on"].ref<string>();
    cv::Mat image_off = cv::imread(file_off, cv::IMREAD_GRAYSCALE);
    cv::Mat image_on = cv::imread(file_on, cv::IMREAD_GRAYSCALE);
    // should make this a class, but good enough for now
    static image::ImageProcessSubMatInterp ipb;
    auto p = ipb(image_off, image_on);
    return p;
}

string get_led_location_string() {
    auto p = get_led_location();
    // too lazy to introduce a json serialiser dependency
    auto tbl = toml::table{};
    auto arry = toml::array{};
    arry.push_back(toml::array{p.p1.x, p.p1.y});
    arry.push_back(toml::array{p.p2.x, p.p2.y});
    tbl.emplace<toml::array>("led"sv, arry);
    auto sstream = std::stringstream{};
    sstream << toml::json_formatter{tbl};
    return sstream.str();
}

/* get the orientation of the camera */
string get_self_orientation() {
    return fmt::format("[{},{},{},{}]", CV_PI / 4, 0, 1, 1);
}
} // namespace serverfunc