// Based on the work done by https://github.com/miks/spinnaker-fps-test
#ifndef _QHYCAMERA_
#define _QHYCAMERA_

#include "qhyccd.h"
#include "toml.hpp"

void SDKVersion();
void FirmWareVersion(qhyccd_handle *h);

/* QHYCCD CAMERA CLASS
   Contains necessary methods and attributes for running
   a QHYCCD Camera
*/
class QHYCamera {
    public:

        // Pointer to camera
        qhyccd_handle *pCamHandle;

        // TOML configuration table
        toml::table config;

		    // Dimensions of image
        unsigned int width;
        unsigned int height;

		    // Offset of ROI from top left corner
        unsigned int offset_x;
       	unsigned int offset_y;

        // Exposure time of images
        int exposure_time;

        // Software gain
        int gain;
        
        // Software gain
        double gamma;

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

		// Bits per pixel
        unsigned int bpp;

        // Total exposure time over all images
        double total_exposure;

        // Timestamp of first image
        char* timestamp;
        
        //Saving directory
        std::string savefilename_prefix;
        std::string savefilename;


	    /* Constructor: Takes the camera pointer and config table
       and saves them (and config values) as object attributes
       INPUTS:
          pCam_init - Spinnaker camera pointer
          config_init - Parsed TOML table   */
        QHYCamera(qhyccd_handle *pCam_init, toml::table config_init);

	    /* Function to setup and start the camera. MUST CALL BEFORE USING!!! */
        int InitCamera();

	    /* Function to De-initialise camera. MUST CALL AFTER USING!!! */
        void DeinitCamera();
        
        int ReconfigureAll(int new_gain, int new_exptime, int new_width, int new_height, int new_offsetX, int new_offsetY, int new_blacklevel, int new_buffersize, std::string new_savedir);

        /* Function to take a number of images with a camera and optionally work on them.
           Default version uses live frame mode, the "Single" version uses the single frame mode.
           The latter should only really be used for debugging.
           INPUTS:
              num_frames - number of images to take
              fits_array - allocated array to store image data in
              f - a callback function that will be applied to each image in real time.
                  If f returns 0, it will end acquisition regardless of how long it has to go.
                  Give NULL for no callback function.
        */
        int GrabFrames(unsigned long num_frames, unsigned long start_index, int (*f)(unsigned short*));


        /* Write a given array of image data as a FITS file
           INPUTS:
              image_array - array of image data to write
              num_images - number of images in the array to write
        */
        int SaveFITS(unsigned long num_images, unsigned long start_index);

};

#endif // _FLIRCAMERA_
