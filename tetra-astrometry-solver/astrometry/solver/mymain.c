#include <stdio.h>
#include <iostream>
#include <string>
#include <fstream>

#include "engine-mainJH.h"
#include "wcsinfoJH.h"
#include "toml.hpp"


int main(int argc, char** args) {
    printf("I'm helpful!\n\n");

    // Print application build information
    cout << "Application build date: " << __DATE__ << " " << __TIME__ << endl << endl;

    string config_file;

    // Check if config file path is passed as argument
    if (argc > 3) {
        cout << "Too many arguments!" << endl;
        exit(1);
    } else if (argc < 2){
        // Load default config if nothing is passed
        cout << "No image file loaded" << endl;
        exit(1);
    } else if (argc == 2){
        // Load default config if nothing is passed
        cout << "No CONFIG file loaded" << endl;
        cout << "Will attempt to load default CONFIG" << endl;
        config_file = string("defaultConfig.toml");
    } else {
        // Assign config file value as string
        config_file = string(argv[2]);
    }

    // Check whether config file is readable/exists
    if (access(config_file.c_str(), R_OK) == -1) {
        cerr << "Config file is not readable" << endl;
        exit(0);
    }

    // Parse the configuration file
    toml::table config = toml::parse_file(config_file);

    string filename = string(argv[1]);
    size_t start = filename.find_last_of("/");
    size_t end = filename.find_last_of(".");
    string prefix;
    filename.copy(prefix,end-start-1,start+1);
    string output_fldr = config["output_folder"].value_or("");;
    string fldr_prefix = output_fldr + string("/") + prefix;
    string axy_filename = fldr_prefix + string(".axy");
    string wcs_filename = fldr_prefix + string(".wcs");

    if (mkdir_p(output_fldr)) {
        ERROR("Failed to create output directory %s", outdir);
        exit(-1);
    }

    // LOAD IMAGE

    // TETRA 3 FUNCTION

    // Write AXY File

    char argv0[] = "astrometry_engine";
    char argv1[] = axy_filename.c_str();
    char argv2[] = "-c";
    char argv3[] = "astrometry.cfg";
    char *argv[] = {argv0, argv1, argv2, argv3, NULL};

    if(!main_solver(4, argv)):{
        ERROR("Failed to solve");
        exit(-1);
    }

    double ra,dec,ori;

    if(!get_WCS_info(wcs_filename.c_str(), &ra, &dec, &ori)):{
        ERROR("Failed to retrieve WCS data");
        exit(-1);
    }

 // CONVERT TO ALT AZ

    return x;
}
