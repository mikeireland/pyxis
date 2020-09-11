// Based on the work done by https://github.com/miks/spinnaker-fps-test

#include <iostream>
#include <string>
#include <ctime>
#include <chrono>
#include <algorithm>
#include "Spinnaker.h"
#include "FLIRCamera.h"
#include "toml.hpp"
#include "fitsio.h"

using namespace Spinnaker;
using namespace Spinnaker::GenApi;
using namespace Spinnaker::GenICam;
using namespace std;


/* Function to pad out strings to print them nicely.
   INPUTS:
      str - string to pad
      num - total size of string to print
      padding_char - character to pad the end of the string
                     until it is of size num

   OUTPUT:
      Padded string
*/
std::string Label(std::string str, const size_t num, const char padding_char) {
    if(num > str.size()){
        str.insert(str.end(), num - str.size(), padding_char);
        }
        return str + ": ";
    }


/* Constructor: Takes the camera pointer and config table
   and saves them (and config values) as object attributes
   INPUTS:
      pCam_init - Spinnaker camera pointer
      config_init - Parsed TOML table
*/
FLIRCamera::FLIRCamera(Spinnaker::CameraPtr pCam_init, toml::table config_init){

    pCam = pCam_init;
    config = config_init;

    cout << "Loading Config" << endl;
    width = config["camera"]["width"].value_or(0);
    height = config["camera"]["height"].value_or(0);
    offset_x = config["camera"]["offset_x"].value_or(0);
    offset_y = config["camera"]["offset_y"].value_or(0);
    exposure_time = config["camera"]["exposure_time"].value_or(0);
    gain = config["camera"]["gain"].value_or(0);
    pixel_format = config["camera"]["pixel_format"].value_or("");
    acquisition_mode = config["camera"]["acquisition_mode"].value_or("");
    adc_bit_depth = config["camera"]["adc_bit_depth"].value_or("");
    black_level = config["camera"]["black_level"].value_or(0);
    buffer_size = config["camera"]["buffer_size"].value_or(0);
    imsize = width*height;

    trigger_mode = config["camera"]["trigger"]["trigger_mode"].value_or("");
    trigger_selector = config["camera"]["trigger"]["trigger_selector"].value_or("");
    trigger_source = config["camera"]["trigger"]["trigger_source"].value_or("");
}


/* Function to setup and start the camera. MUST CALL BEFORE USING!!! */
void FLIRCamera::InitCamera(){
    // Initialize camera
    pCam->Init();

    INodeMap& node_map = pCam->GetNodeMap();

    //Configure Camera
    // Set width
    CIntegerPtr ptr_width = node_map.GetNode("Width");
    ptr_width->SetValue(width);

    // Set height
    CIntegerPtr ptr_height = node_map.GetNode("Height");
    ptr_height->SetValue(height);

    // Set x offset
    CIntegerPtr ptr_offset_x = node_map.GetNode("OffsetX");
    ptr_offset_x->SetValue(offset_x);

    // Set y offset
    CIntegerPtr ptr_offset_y = node_map.GetNode("OffsetY");
    ptr_offset_y->SetValue(offset_y);

    // Set auto exposure mode to OFF
    char auto_exposure[] = "Off";
    CEnumerationPtr ptr_auto_exposure = node_map.GetNode("ExposureAuto");
    CEnumEntryPtr ptr_auto_exposure_node = ptr_auto_exposure->GetEntryByName(auto_exposure);
    ptr_auto_exposure->SetIntValue(ptr_auto_exposure_node->GetValue());

    // Set auto gain mode to OFF
    char auto_gain[] = "Off";
    CEnumerationPtr ptr_auto_gain = node_map.GetNode("GainAuto");
    CEnumEntryPtr ptr_auto_gain_node = ptr_auto_gain->GetEntryByName(auto_gain);
    ptr_auto_gain->SetIntValue(ptr_auto_gain_node->GetValue());

    // Set Gamma Correction to OFF
    bool gamma = false;
    CBooleanPtr ptr_gamma = node_map.GetNode("GammaEnable");
    ptr_gamma->SetValue(gamma);

    // Set Black Level
    CFloatPtr   ptr_black = node_map.GetNode("BlackLevel");
    ptr_black->SetValue(black_level); // pass value in percent

    // Set exposure time
    CFloatPtr   ptr_exposure_time = node_map.GetNode("ExposureTime");
    ptr_exposure_time->SetValue(exposure_time); // pass value in microseconds

    // Set gain
    CFloatPtr   ptr_gain = node_map.GetNode("Gain");
    ptr_gain->SetValue(gain); // pass value in dB

    // Set pixel format
    CEnumerationPtr ptr_pixel_format = node_map.GetNode("PixelFormat");
    CEnumEntryPtr ptr_pixel_format_node = ptr_pixel_format->GetEntryByName(pixel_format.c_str());
    ptr_pixel_format->SetIntValue(ptr_pixel_format_node->GetValue());

    // Set aqcuisition mode
    CEnumerationPtr ptr_acquisition_mode = node_map.GetNode("AcquisitionMode");
    CEnumEntryPtr ptr_acquisition_mode_node = ptr_acquisition_mode->GetEntryByName(acquisition_mode.c_str());
    ptr_acquisition_mode->SetIntValue(ptr_acquisition_mode_node->GetValue());

    // Set adc bit depth
    CEnumerationPtr ptr_adc_bit_depth = node_map.GetNode("AdcBitDepth");
    CEnumEntryPtr ptr_adc_bit_depth_node = ptr_adc_bit_depth->GetEntryByName(adc_bit_depth.c_str());
    ptr_adc_bit_depth->SetIntValue(ptr_adc_bit_depth_node->GetValue());

    // Print Camera device information.
    cout << "Camera device information" << endl
         << "=========================" << endl;
    cout << Label("Model") << CStringPtr( node_map.GetNode( "DeviceModelName") )->GetValue() << endl;
    cout << Label("Firmware version") << CStringPtr( node_map.GetNode( "DeviceFirmwareVersion") )->GetValue() << endl;
    cout << Label("Serial number") << CStringPtr( node_map.GetNode( "DeviceSerialNumber") )->GetValue() << endl;
    cout << Label("Max resolution") << ptr_width->GetMax() << " x " << ptr_height->GetMax() << endl;
    cout << Label("Min exposure time") << ptr_exposure_time->GetMin() << endl;
    cout << endl;

    // Print Camera settings
    cout << "Camera device settings" << endl << "======================" << endl;
    cout << Label("Acquisition mode") << ptr_acquisition_mode->GetCurrentEntry()->GetSymbolic() << endl;
    cout << Label("Pixel format") << ptr_pixel_format->GetCurrentEntry()->GetSymbolic() << endl;
    cout << Label("ADC bit depth") << ptr_adc_bit_depth->GetCurrentEntry()->GetSymbolic() << endl;
    cout << Label("Exposure time") << ptr_exposure_time->GetValue() << endl;
    cout << Label("Gain") << ptr_gain->GetValue() << endl;
    cout << Label("Width") << ptr_width->GetValue() << endl;
    cout << Label("Height") << ptr_height->GetValue() << endl;
    cout << Label("Offset X") << ptr_offset_x->GetValue() << endl;
    cout << Label("Offset Y") << ptr_offset_y->GetValue() << endl;
    cout << endl;

    // Configure trigger if requested
    if (trigger_mode=="On"){
        ConfigTrigger(node_map);
    }

}

int FLIRCamera::ConfigTrigger(INodeMap& node_map){
    int result = 0;
    cout << endl << endl << "*** CONFIGURING TRIGGER ***" << endl << endl;

    try
    {
        // Ensure trigger mode off
        // *** NOTES ***
        // The trigger must be disabled in order to configure whether the source
        // is software or hardware.
        CEnumerationPtr ptr_trigger_mode = node_map.GetNode("TriggerMode");
        CEnumEntryPtr ptr_trigger_mode_off = ptr_trigger_mode->GetEntryByName("Off");
        ptr_trigger_mode->SetIntValue(ptr_trigger_mode_off->GetValue());
        cout << "Trigger mode disabled..." << endl;

        // Select trigger source
        CEnumerationPtr ptr_trigger_source = node_map.GetNode("TriggerSource");
        CEnumEntryPtr ptr_trigger_source_node = ptr_trigger_source->GetEntryByName(trigger_source.c_str());
        ptr_trigger_source->SetIntValue(ptr_trigger_source_node->GetValue());
        cout << "Trigger source set to " << trigger_source << endl;

        // Select trigger selector
        CEnumerationPtr ptr_trigger_selector = node_map.GetNode("TriggerSelector");
        CEnumEntryPtr ptr_trigger_selector_node = ptr_trigger_selector->GetEntryByName(trigger_selector.c_str());
        ptr_trigger_selector->SetIntValue(ptr_trigger_selector_node->GetValue());
        cout << "Trigger selector set to " << trigger_selector << endl;

        // Turn trigger mode on
        CEnumEntryPtr ptr_trigger_mode_on = ptr_trigger_mode->GetEntryByName("On");
        ptr_trigger_mode->SetIntValue(ptr_trigger_mode_on->GetValue());
        cout << "Trigger mode turned back on..." << endl << endl;
    }
    catch (Spinnaker::Exception& e)
    {
        cout << "Error: " << e.what() << endl;
        result = -1;
    }
    return result;
}


/* Function to De-initialise camera. MUST CALL AFTER USING!!! */
void FLIRCamera::DeinitCamera(){

    INodeMap& node_map = pCam->GetNodeMap();

    if (trigger_mode=="On"){
        ResetTrigger(node_map);
    }

    pCam->DeInit();
}


// This function returns the camera to a normal state by turning off trigger
// mode.
int FLIRCamera::ResetTrigger(INodeMap& node_map){
    int result = 0;
    try
    {
        // Turn trigger mode back off
        CEnumerationPtr ptr_trigger_mode = node_map.GetNode("TriggerMode");
        CEnumEntryPtr ptr_trigger_mode_off = ptr_trigger_mode->GetEntryByName("Off");
        ptr_trigger_mode->SetIntValue(ptr_trigger_mode_off->GetValue());

        cout << "Trigger mode disabled..." << endl << endl;
    }
    catch (Spinnaker::Exception& e)
    {
        cout << "Error: " << e.what() << endl;
        result = -1;
    }
    return result;
}


// This function retrieves a single image using the trigger. In this example,
// only a single image is captured and made available for acquisition - as such,
// attempting to acquire two images for a single trigger execution would cause
// the example to hang.
int FLIRCamera::GrabImageByTrigger(INodeMap& node_map){
    int result = 0;
    try
    {
        //
        // Use trigger to capture image
        //
        // *** NOTES ***
        // The software trigger only feigns being executed by the Enter key;
        // what might not be immediately apparent is that there is not a
        // continuous stream of images being captured; in other examples that
        // acquire images, the camera captures a continuous stream of images.
        // When an image is retrieved, it is plucked from the stream.
        //
        if (trigger_source == "Software")
        {
            // Get user input
            cout << "Press the Enter key to initiate software trigger." << endl;
            getchar();
            // Execute software trigger
            CCommandPtr ptr_software_trigger_command = node_map.GetNode("TriggerSoftware");
            if (!IsAvailable(ptr_software_trigger_command) || !IsWritable(ptr_software_trigger_command))
            {
                cout << "Unable to execute trigger. Aborting..." << endl;
                return -1;
            }
            ptr_software_trigger_command->Execute();

        }
        else
        {
            // Execute hardware trigger
            cout << "Use the hardware to trigger image acquisition." << endl;
        }
    }
    catch (Spinnaker::Exception& e)
    {
        cout << "Error: " << e.what() << endl;
        result = -1;
    }
    return result;
}


/* Function to take a number of images with a camera and optionally work on them.
   INPUTS:
      num_frames - number of images to take
      fits_array - allocated array to store image data in
      f - a callback function that will be applied to each image in real time.
          Give NULL for no callback function.
*/
void FLIRCamera::GrabFrames(unsigned long num_frames, unsigned short* image_array, int (*f)(unsigned short*)) {
    try {
        cout << "Start Acquisition" << endl;

        std::chrono::time_point<std::chrono::steady_clock> start, end;

        INodeMap& node_map = pCam->GetNodeMap();

        //Set timestamp
        time_t start_time = time(0);

        // Start aqcuisition
        pCam->BeginAcquisition();

        //Begin timing
        start = std::chrono::steady_clock::now();

        for (unsigned int image_cnt = 0; image_cnt < num_frames; image_cnt++){

            // If trigger mode is on, wait for trigger to take image
            if (trigger_mode=="On"){
                GrabImageByTrigger(node_map);
            }

            // Retrive image
            ImagePtr ptr_result_image = pCam->GetNextImage();

            // Load current raw image data into an array
            unsigned short* data = (unsigned short*)ptr_result_image->GetData();

            // Append data to an allocated memory array
            memcpy(image_array+imsize*(image_cnt%buffer_size), data, imsize*2);

            // Do something with the data in real time if required
            // If 0 is returned by the callback function, end acquisition (regardless of 
            // number of frames to go).
            if (*f != NULL){
                int result;

                result = (*f)(data);

                if (result == 0){

                    cout << endl;

                    // End Timing
                    end = std::chrono::steady_clock::now();

                    // End Acquisition
                    pCam->EndAcquisition();

                    cout << "Callback Request: Finished Acquisition" << endl;

                    //Set timestamp
                    tm *gmtm = gmtime(&start_time);
                    char* dt = asctime(gmtm);

                    timestamp = dt;

                    //Calculate duration
                    double duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

                    total_exposure = duration;

                    return;

                }
            }

            // Release image
            ptr_result_image->Release();

        }

        cout << endl;

        // End Timing
        end = std::chrono::steady_clock::now();

        // End Acquisition
        pCam->EndAcquisition();

        cout << "Finished Acquisition" << endl;

        //Set timestamp
        tm *gmtm = gmtime(&start_time);
        char* dt = asctime(gmtm);

        timestamp = dt;

        //Calculate duration
        double duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

        total_exposure = duration;

    }
    catch (Spinnaker::Exception &e) {
        cout << "Error: " << e.what() << endl;
    }
}


/* Write a given array of image data as a FITS file
   INPUTS:
      image_array - array of image data to write
      num_images - number of images in the array to writes
*/
int FLIRCamera::SaveFITS(unsigned short* image_array, int num_images)
{
    // Pointer to the FITS file; defined in fitsio.h
    fitsfile *fptr;

    // Define filepath and name for the FITS file
    std::string file_dir = config["fits"]["file_dir"].value_or("");
    std::string filename = config["fits"]["filename"].value_or("");
    string file_path = "!" + file_dir + filename;

    // Configure FITS file
    int bitpix = config["fits"]["bitpix"].value_or(16);
    long naxis = 3; // 2D image over time
    long naxes[3] = {width, height, num_images};

    // Initialize status before calling fitsio routines
    int status = 0;

    // Create new FITS file. Will overwrite file with the same name!!
    if (fits_create_file(&fptr, file_path.c_str(), &status)){
       cout << "ERROR: Could not create FITS file" << endl;
       return( status );
    }
    //bitpix = 32;
    // Write the required keywords for the primary array image
    if ( fits_create_img(fptr,  bitpix, naxis, naxes, &status) )
         return( status );

    long fpixel = 1;
    long nelements = naxes[0] * naxes[1] * naxes[2];

    // Write the image (assuming input of unsigned integers)
    if ( fits_write_img(fptr, TUSHORT, fpixel, nelements, image_array, &status) ){
        cout << "ERROR: Could not write FITS file" << endl;
        cout << status << endl;
        return( status );
    }
    // Configure FITS header keywords

    char *pix_format = &pixel_format[0];
    char *adc = &adc_bit_depth[0];

    // Write starting time in UTC
    if ( fits_write_key(fptr, TSTRING, "STARTTIME", &timestamp,
         "Timestamp of beginning of exposure UTC", &status) )
         return( status );

    // Write starting time in UTC
    if ( fits_write_key(fptr, TSTRING, "PIXEL FORMAT", pix_format,
         "Pixel Format", &status) )
         return( status );

    // Write starting time in UTC
    if ( fits_write_key(fptr, TSTRING, "ADC BIT DEPTH", adc,
         "ADC Bit Depth", &status) )
         return( status );

    // Write individual exposure time
    if ( fits_write_key(fptr, TINT, "FRAMEEXPOSURE", &exposure_time,
         "Individual Exposure Time (us)", &status) )
         return( status );

    // Write total exposure time
    if ( fits_write_key(fptr, TDOUBLE, "TOTALEXPOSURE", &total_exposure,
         "Total Exposure Time (ms)", &status) )
         return( status );

    // Write gain
    if ( fits_write_key(fptr, TINT, "GAIN", &gain,
         "Software Gain (dB)", &status) )
         return( status );

    // Write height
    if ( fits_write_key(fptr, TINT, "HEIGHT", &height,
         "Image Height (px)", &status) )
         return( status );

    // Write width
    if ( fits_write_key(fptr, TINT, "WIDTH", &width,
         "Image Width (px)", &status) )
          return( status );

    // Write offset x
    if ( fits_write_key(fptr, TINT, "XOFFSET", &offset_x,
         "Image X Offset (px)", &status) )
         return( status );

    // Write offset y
    if ( fits_write_key(fptr, TINT, "YOFFSET", &offset_y,
         "Image Y Offset (px)", &status) )
         return( status );

    // Close file
    fits_close_file(fptr, &status);
    return( status );
}
