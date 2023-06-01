// Based on the work done by https://github.com/miks/spinnaker-fps-test
#ifndef _FLIRCAMERA_
#define _FLIRCAMERA_

#include "Spinnaker.h"
#include "toml.hpp"

/* FLIR CAMERA CLASS
   Contains necessary methods and attributes for running
   a FLIR Camera
*/
class FLIRCamera {
    public:

        // Pointer to camera
        Spinnaker::CameraPtr pCam;

        // TOML configuration table
        toml::table config;

        // Dimensions of image
        int width;
        int height;

        int width_min;
        int width_max;
        
        int height_min;
        int height_max;

		// Offset of ROI from top left corner
        int offset_x;
        int offset_y;

        // Exposure time of images
        int exposure_time;
        
        int exposure_time_min;
        int exposure_time_max;

        // Software gain
        int gain;
        
        int gain_min;
        int gain_max;
        
        // Pixel format of images (Mono16)
        std::string pixel_format;

        // Acquisition mode of camera (continuous)
        std::string acquisition_mode;

        // Set bit depth of ADC
        std::string adc_bit_depth;

        // Set black level in percent
        double black_level;
        double black_level_min;
        double black_level_max;


        // Buffer size for image data
        unsigned int buffer_size;

        // Number of pixels in image
        unsigned int imsize;

        // Total exposure time over all images
        double total_exposure;

        // Timestamp of first image
        std::string timestamp;
        
        //Saving directory; prefix is without frame number and extension
        std::string savefilename_prefix;
        std::string savefilename;


		/* Constructor: Takes the camera pointer and config table
           and saves them (and config values) as object attributes
           INPUTS:
              pCam_init - Spinnaker camera pointer
              config_init - Parsed TOML table   */
        FLIRCamera(Spinnaker::CameraPtr pCam_init, toml::table config_init);

		/* Function to setup and start the camera. MUST CALL BEFORE USING!!! */
        void InitCamera();
        
        /* Function to reconfigure all parameters. Inputs are explanatory */
        void ReconfigureAll(int new_gain, int new_exptime, int new_width, int new_height, int new_offsetX, 
                            int new_offsetY, float new_blacklevel, int new_buffersize, std::string new_savedir);


		/* Function to De-initialise camera. MUST CALL AFTER USING!!! */
        void DeinitCamera();

        /* Function to take a number of images with a camera and optionally work on them.
           INPUTS:
              num_frames - number of images to take
              start_index - frame number index of where in the circular buffer to start taking images
              f - a callback function that will be applied to each image in real time.
                  If f returns 1, it will end acquisition regardless of how long it has to go.
                  Give NULL for no callback function.
           OUTPUTS:
                0 on regular exit
                1 on callback exit
        */
        int GrabFrames(unsigned long num_frames, unsigned long start_index, int (*f)(unsigned short*));

        /* Write a given array of image data as a FITS file
           INPUTS:
              num_images - number of images in the array to write
              start_index - frame number index of where in the circular buffer to start saving images
        */
        int SaveFITS(unsigned long num_images, unsigned long start_index);

	private:
	    /* Helper functions for "ReconfigureAll" */
        void ReconfigureInt(std::string parameter, int value);
        
        void ReconfigureFloat(std::string parameter, float value);
};

#endif // _FLIRCAMERA_
