// Based on the work done by https://github.com/miks/spinnaker-fps-test
#ifndef _QHYCAMERA_
#define _QHYCAMERA_

#include "qhyccd.h"
#include "toml.hpp"

/* QHYCCD CAMERA CLASS
   Contains necessary methods and attributes for running
   a QHYCCD Camera
*/
class QHYCamera {
    public:

        // Pointer to camera
        qhyccd_handle pCam;

        // TOML configuration table
        toml::table config;

		    // Dimensions of image
        int width;
        int height;

		    // Offset of ROI from top left corner
        int offset_x;
        int offset_y;

        // Exposure time of images
        int exposure_time;

        // Software gain
        int gain;

        // Set black level in percent
        int black_level;

        // Pixel format of images (8 or 16)
        int pixel_format;

        // Acquisition mode of camera (0 for single, 1 for live)
        int acquisition_mode;

        // Readout mode of camera (0,1 or 2)
        int readout_mode;

        // Buffer size for image data
        int buffer_size;

        // Number of pixels in image
        int imsize;

        unsigned int bpp;

        // Total exposure time over all images
        double total_exposure;

        // Timestamp of first image
        char* timestamp;


		    /* Constructor: Takes the camera pointer and config table
           and saves them (and config values) as object attributes
           INPUTS:
              pCam_init - Spinnaker camera pointer
              config_init - Parsed TOML table   */
        QHYCamera(qhyccd_handle pCam_init, toml::table config_init);

		    /* Function to setup and start the camera. MUST CALL BEFORE USING!!! */
        void InitCamera();

		    /* Function to De-initialise camera. MUST CALL AFTER USING!!! */
        void DeinitCamera();

        /* Function to take a number of images with a camera and optionally work on them.
           INPUTS:
              num_frames - number of images to take
              fits_array - allocated array to store image data in
              f - a callback function that will be applied to each image in real time.
                  If f returns 0, it will end acquisition regardless of how long it has to go.
                  Give NULL for no callback function.
        */
        void GrabFrames(unsigned long num_frames, unsigned short* image_array, int (*f)(unsigned short*));

        /* Write a given array of image data as a FITS file
           INPUTS:
              image_array - array of image data to write
              num_images - number of images in the array to write
        */
        int SaveFITS(unsigned short* image_array, int num_images);

};

#endif // _FLIRCAMERA_
