// Based on the work done by https://github.com/miks/spinnaker-fps-test

#include <iostream>
#include <string>
#include <ctime>
#include <vector>
#include <algorithm>
#include "acquisition.h"
#include "Spinnaker.h"
#include "SpinGenApi/SpinnakerGenApi.h"
#include "cpptoml/cpptoml.h"

using namespace Spinnaker;
using namespace Spinnaker::GenApi;
using namespace Spinnaker::GenICam;
using namespace std;

std::string Label(std::string str, const size_t num, const char paddingChar) {
    if(num > str.size()){
        str.insert(str.end(), num - str.size(), paddingChar);
        }
        return str + ": ";
    }



void grabFrames(Spinnaker::CameraPtr pCam, std::shared_ptr<cpptoml::table> config, unsigned long numFrames, vector<int> fitsArray, struct times timesStruct, int useFunc, void (*f)(unsigned char*)) {
    try {
        cout << "Loading Config" << endl;
        int width = config->get_qualified_as<int>("camera.width").value_or(0);
        int height = config->get_qualified_as<int>("camera.height").value_or(0);
        int offset_x = config->get_qualified_as<int>("camera.offset_x").value_or(0);
        int offset_y = config->get_qualified_as<int>("camera.offset_y").value_or(0);
        int exposure_time = config->get_qualified_as<int>("camera.exposure_time").value_or(0);
        string pixel_format = config->get_qualified_as<string>("camera.pixel_format").value_or("");
        string acquisition_mode = config->get_qualified_as<string>("camera.acquisition_mode").value_or("");
        string auto_exposure = config->get_qualified_as<string>("camera.auto_exposure").value_or("");
        string auto_gain = config->get_qualified_as<string>("camera.auto_gain").value_or("");
        string adc_bit_depth = config->get_qualified_as<string>("camera.adc_bit_depth").value_or("");

        long bufferSize = config->get_qualified_as<long>("fits.bufferSize").value_or(0);
        int imsize = width*height;


        cout << "Initialising Camera" << endl;

        // Initialize camera
        pCam->Init();

        // Retrieve GenICam node_map
        INodeMap & node_map = pCam->GetNodeMap();

        //Configure Camera

        // set width
        CIntegerPtr ptr_width = node_map.GetNode("Width");
        ptr_width->SetValue(width);

        // set height
        CIntegerPtr ptr_height = node_map.GetNode("Height");
        ptr_height->SetValue(height);

        // set x offset
        CIntegerPtr ptr_offset_x = node_map.GetNode("OffsetX");
        ptr_offset_x->SetValue(offset_x);
        // set y offset
        CIntegerPtr ptr_offset_y = node_map.GetNode("OffsetY");

        ptr_offset_y->SetValue(offset_y);
        // set auto exposure mode
        CEnumerationPtr ptr_auto_exposure = node_map.GetNode("ExposureAuto");
        CEnumEntryPtr ptr_auto_exposure_node = ptr_auto_exposure->GetEntryByName(auto_exposure.c_str());
        ptr_auto_exposure->SetIntValue(ptr_auto_exposure_node->GetValue());

        // set exposure time
        CFloatPtr   ptr_exposure_time = node_map.GetNode("ExposureTime");
        ptr_exposure_time->SetValue(exposure_time); // pass value in microseconds

        // set pixel format
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
        cout << Label("Width") << ptr_width->GetValue() << endl;
        cout << Label("Height") << ptr_height->GetValue() << endl;
        cout << Label("Offset X") << ptr_offset_x->GetValue() << endl;
        cout << Label("Offset Y") << ptr_offset_y->GetValue() << endl;
        cout << endl;

        /*
        // Configure chunk data
        err = ConfigureChunkData(nodeMap);
        if (err < 0){
            return err;
        }
        */

        cout << "Start Acquisition" << endl;
        time_t time_begin = time(0);

        tm *gmtm = gmtime(&time_begin);
        char* dt = asctime(gmtm);

        timesStruct.timestamp = dt;

        // Start aqcuisition
        pCam->BeginAcquisition();

        for (unsigned int imageCnt = 0; imageCnt < numFrames; imageCnt++){

            ImagePtr pResultImage = pCam->GetNextImage();

            unsigned char* data = (unsigned char*)pResultImage->GetData();

            copy(data,data+imsize,fitsArray.begin()+imsize*(imageCnt%bufferSize));

            //GetChunkData(pCam,chunkArray[imageCnt%bufferSize]);

            if (useFunc == 1){
                (*f)(data);
            }

            pResultImage->Release(); 
        }

        cout << endl;

        // Deinitialize camera
        pCam->EndAcquisition();

        time_t time_end = time(0);

        time_t duration = time_end - time_begin;

        timesStruct.totalexposure = (int)duration;
        cout << "Finished Acquisition" << endl;
        /*
        // Disable chunk data
        err = DisableChunkData(nodeMap);
        if (err < 0){
            return err;
        }
        */
        pCam->DeInit();
    }
    catch (Spinnaker::Exception &e) {
        cout << "Error: " << e.what() << endl;
    }
}
