// Based on the work done by https://github.com/miks/spinnaker-fps-test
#ifndef _FLIRCAMERA_
#define _FLIRCAMERA_

#include "Spinnaker.h"
#include "toml.hpp"

/* Function to pad out strings to print them nicely.
   INPUTS:
      str - string to pad
      num - total size of string to print
      padding_char - character to pad the end of the string
                     until it is of size num

   OUTPUT:
      Padded string
*/
std::string Label(std::string str, const size_t num = 20, const char padding_char = ' ');


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

		// Offset of ROI from top left corner
        int offset_x;
        int offset_y;

        // Exposure time of images
        int exposure_time;

        // Software gain
        int gain;

        // Pixel format of images (Mono16 is good default)
        std::string pixel_format;

        // Acquisition mode of camera (continuous) 
        std::string acquisition_mode;

        // Set bit depth of ADC
        std::string adc_bit_depth;

        // Set black level in percent
        double black_level;
    
        // Buffer size for image data
        int buffer_size;

        // Number of pixels in image
        int imsize;

        // Total exposure time over all images
        double total_exposure;

        // Timestamp of first image
        char* timestamp;

        // TRIGGER VALUES

        // Whether trigger is on or off
        std::string trigger_mode;

        // Trigger functionality
        std::string trigger_selector;

        // Trigger source
        std::string trigger_source;


		/* Constructor: Takes the camera pointer and config table
           and saves them (and config values) as object attributes  
           INPUTS:
              pCam_init - Spinnaker camera pointer
              config_init - Parsed TOML table   */
        FLIRCamera(Spinnaker::CameraPtr pCam_init, toml::table config_init);

		/* Function to setup and start the camera. MUST CALL BEFORE USING!!! */
        void InitCamera();

		/* Function to De-initialise camera. MUST CALL AFTER USING!!! */
        void DeinitCamera();

        /* Function to take a number of images with a camera and optionally work on them.
           INPUTS:
              num_frames - number of images to take
              fits_array - allocated array to store image data in
              f - a callback function that will be applied to each image in real time. 
                  Give NULL for no callback function.   
        */
        void GrabFrames(unsigned long num_frames, unsigned short* image_array, void (*f)(unsigned short*));

        /* Write a given array of image data as a FITS file
           INPUTS:
              image_array - array of image data to write
              num_images - number of images in the array to write
        */
        int SaveFITS(unsigned short* image_array, int num_images);


    private:
        // Configure the trigger
        int ConfigTrigger(Spinnaker::GenApi::INodeMap& node_map);
        // Reset the camera by turning off trigger        
        int ResetTrigger(Spinnaker::GenApi::INodeMap& node_map);
        // Function to grab an image through the trigger        
        int GrabImageByTrigger(Spinnaker::GenApi::INodeMap& node_map);

};

#endif // _FLIRCAMERA_
