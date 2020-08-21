// Based on the work done by https://github.com/miks/spinnaker-fps-test

#include <iostream>
#include <string>
#include <ctime>
#include <chrono>
#include <algorithm>
#include "acquisition.h"
#include "Spinnaker.h"
#include "SpinGenApi/SpinnakerGenApi.h"
#include "cpptoml/cpptoml.h"

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


/* Function to take a number of images with a camera and optionally work on them.
   INPUTS:
      pCam - Spinnaker pointer to a FLIR camera
      config - cpptoml pointer to a configuration table
      num_frames - number of images to take
      fits_array - allocated vector to store image data in
      times_struct - a structure that allows the storage of a timestamp and duration of the exposures
      use_func - a flag as to whether to apply a real time function to the image data
      *f - a function that will be applied to each image in real time if use_func is set

*/
void GrabFrames(Spinnaker::CameraPtr pCam, std::shared_ptr<cpptoml::table> config, unsigned long num_frames, unsigned short* fits_array, Times& times_struct, int use_func, void (*f)(unsigned short*)) {
    try {

        // Load in the configuration file
        cout << "Loading Config" << endl;
        int width = config->get_qualified_as<int>("camera.width").value_or(0);
        int height = config->get_qualified_as<int>("camera.height").value_or(0);
        int offset_x = config->get_qualified_as<int>("camera.offset_x").value_or(0);
        int offset_y = config->get_qualified_as<int>("camera.offset_y").value_or(0);
        int exposure_time = config->get_qualified_as<int>("camera.exposure_time").value_or(0);
        int gain = config->get_qualified_as<int>("camera.gain").value_or(0);
        string pixel_format = config->get_qualified_as<string>("camera.pixel_format").value_or("");
        string acquisition_mode = config->get_qualified_as<string>("camera.acquisition_mode").value_or("");
        string auto_exposure = config->get_qualified_as<string>("camera.auto_exposure").value_or("");
        string auto_gain = config->get_qualified_as<string>("camera.auto_gain").value_or("");
        string adc_bit_depth = config->get_qualified_as<string>("camera.adc_bit_depth").value_or("");
        int buffer_size = config->get_qualified_as<int>("fits.buffer_size").value_or(0);
        int imsize = width*height;

        cout << "Initialising Camera" << endl;

        // Initialize camera
        pCam->Init();

        // Retrieve GenICam node_map
        INodeMap & node_map = pCam->GetNodeMap();


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
        // Set auto exposure mode
        CEnumerationPtr ptr_auto_exposure = node_map.GetNode("ExposureAuto");
        CEnumEntryPtr ptr_auto_exposure_node = ptr_auto_exposure->GetEntryByName(auto_exposure.c_str());
        ptr_auto_exposure->SetIntValue(ptr_auto_exposure_node->GetValue());

        // Set exposure time
        CFloatPtr   ptr_exposure_time = node_map.GetNode("ExposureTime");
        ptr_exposure_time->SetValue(exposure_time); // pass value in microseconds

        // Set gain
        CFloatPtr   ptr_gain = node_map.GetNode("Gain");
        ptr_gain->SetValue(gain); // pass value in microseconds

        // Set pixel format
        CEnumerationPtr ptr_pixel_format = node_map.GetNode("PixelFormat");
        CEnumEntryPtr ptr_pixel_format_node = ptr_pixel_format->GetEntryByName(pixel_format.c_str());
        ptr_pixel_format->SetIntValue(ptr_pixel_format_node->GetValue());

        // Set aqcuisition mode
        CEnumerationPtr ptr_acquisition_mode = node_map.GetNode("AcquisitionMode");
        CEnumEntryPtr ptr_acquisition_mode_node = ptr_acquisition_mode->GetEntryByName(acquisition_mode.c_str());
        ptr_acquisition_mode->SetIntValue(ptr_acquisition_mode_node->GetValue());

        // Set auto gain mode
        CEnumerationPtr ptr_auto_gain = node_map.GetNode("GainAuto");
        CEnumEntryPtr ptr_auto_gain_node = ptr_auto_gain->GetEntryByName(auto_gain.c_str());
        ptr_auto_gain->SetIntValue(ptr_auto_gain_node->GetValue());

        // Set adc bit depth
        CEnumerationPtr ptr_adc_bit_depth = node_map.GetNode("AdcBitDepth");
        CEnumEntryPtr ptr_adc_bit_depth_node = ptr_adc_bit_depth->GetEntryByName(adc_bit_depth.c_str());
        ptr_adc_bit_depth->SetIntValue(ptr_adc_bit_depth_node->GetValue());


        // Get camera device information.
        cout << "Camera device information" << endl
             << "=========================" << endl;
        cout << Label("Model") << CStringPtr( node_map.GetNode( "DeviceModelName") )->GetValue() << endl;
        cout << Label("Firmware version") << CStringPtr( node_map.GetNode( "DeviceFirmwareVersion") )->GetValue() << endl;
        cout << Label("Serial number") << CStringPtr( node_map.GetNode( "DeviceSerialNumber") )->GetValue() << endl;
        cout << Label("Max resolution") << ptr_width->GetMax() << " x " << ptr_height->GetMax() << endl;
        cout << Label("Min exposure time") << ptr_exposure_time->GetMin() << endl;
        cout << endl;


        // Camera settings
        cout << "Camera device settings" << endl << "======================" << endl;
        cout << Label("Acquisition mode") << ptr_acquisition_mode->GetCurrentEntry()->GetSymbolic() << endl;
        cout << Label("Pixel format") << ptr_pixel_format->GetCurrentEntry()->GetSymbolic() << endl;
        cout << Label("ADC bit depth") << ptr_adc_bit_depth->GetCurrentEntry()->GetSymbolic() << endl;
        cout << Label("Auto gain") << ptr_auto_gain->GetCurrentEntry()->GetSymbolic() << endl;
        cout << Label("Auto exposure") << ptr_auto_exposure->GetCurrentEntry()->GetSymbolic() << endl;
        cout << Label("Exposure time") << ptr_exposure_time->GetValue() << endl;
        cout << Label("Gain") << ptr_gain->GetValue() << endl;
        cout << Label("Width") << ptr_width->GetValue() << endl;
        cout << Label("Height") << ptr_height->GetValue() << endl;
        cout << Label("Offset X") << ptr_offset_x->GetValue() << endl;
        cout << Label("Offset Y") << ptr_offset_y->GetValue() << endl;
        cout << endl;

        cout << "Start Acquisition" << endl;


        std::chrono::time_point<std::chrono::steady_clock> start, end;

        // Start aqcuisition
        pCam->BeginAcquisition();

        //Set timestamp
        time_t start_time = time(0);

        //Begin timing
        start = std::chrono::steady_clock::now();

        for (unsigned int image_cnt = 0; image_cnt < num_frames; image_cnt++){

            // Retrive image
            ImagePtr ptr_result_image = pCam->GetNextImage();


            // Load current raw image data into an array
            unsigned short* data = (unsigned short*)ptr_result_image->GetData();

            // Append data to an allocated memory array
            memcpy(fits_array+imsize*(image_cnt%buffer_size), data, imsize*2);

            // Do something with the data in real time if required (set use_func for this)
            if (use_func == 1){
                (*f)(data);
            }

            // Release image
            ptr_result_image->Release();
        }

        cout << endl;

        // End Timiong
        end = std::chrono::steady_clock::now();

        // End Acquisition
        pCam->EndAcquisition();

        cout << "Finished Acquisition" << endl;

        //Set timestamp
        tm *gmtm = gmtime(&start_time);
        char* dt = asctime(gmtm);

        times_struct.timestamp = dt;

        //Calculate duration
        double duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

        times_struct.total_exposure = duration;

        // Deinitialise camera
        pCam->DeInit();
    }
    catch (Spinnaker::Exception &e) {
        cout << "Error: " << e.what() << endl;
    }
}
