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
#include "globals.h"

using namespace Spinnaker;
using namespace Spinnaker::GenApi;
using namespace Spinnaker::GenICam;
using namespace std;


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

    savefilename_prefix = config["fits"]["filename_prefix"].value_or("");
    savefilename = savefilename_prefix + ".fits";
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
    
    GLOB_IMSIZE = imsize;
    pthread_mutex_lock(&GLOB_FLAG_LOCK);
    GLOB_CONFIG_PARAMS.gain = gain;
    GLOB_CONFIG_PARAMS.exptime = exposure_time;
    GLOB_CONFIG_PARAMS.width = width;
    GLOB_CONFIG_PARAMS.height = height;
    GLOB_CONFIG_PARAMS.offsetX = offset_x;
    GLOB_CONFIG_PARAMS.offsetY = offset_y;
    GLOB_CONFIG_PARAMS.blacklevel = black_level;
    GLOB_CONFIG_PARAMS.buffersize = buffer_size;
    GLOB_CONFIG_PARAMS.savedir = savefilename;
    pthread_mutex_unlock(&GLOB_FLAG_LOCK);

}

void FLIRCamera::ReconfigureFloat(std::string parameter, float value){

    INodeMap& node_map = pCam->GetNodeMap();

	char* temp_param;
	temp_param = &parameter[0];

    // Set parameter
    CFloatPtr ptr_parameter = node_map.GetNode(temp_param);
    ptr_parameter->SetValue(value);

    // Print Camera setting
    cout << "Setting " << parameter << " to: " << ptr_parameter->GetValue() << endl ;
}

void FLIRCamera::ReconfigureInt(std::string parameter, int value){

    INodeMap& node_map = pCam->GetNodeMap();

	char* temp_param;
	temp_param = &parameter[0];

    // Set parameter
    CIntegerPtr ptr_parameter = node_map.GetNode(temp_param);
    ptr_parameter->SetValue(value);

    // Print Camera setting
    cout << "Setting " << parameter << " to: " << ptr_parameter->GetValue() << endl ;
}


void FLIRCamera::ReconfigureAll(int new_gain, int new_exptime, int new_width, int new_height, int new_offsetX, int new_offsetY, int new_blacklevel, int new_buffersize, string new_savedir){

    ReconfigureFloat("Gain",new_gain);
    ReconfigureFloat("ExposureTime",new_exptime);
    ReconfigureInt("Width",new_width);
    ReconfigureInt("Height",new_height);
    ReconfigureInt("OffsetX",new_offsetX);
    ReconfigureInt("OffsetY",new_offsetY);
    ReconfigureFloat("BlackLevel",new_blacklevel);
    buffer_size = new_buffersize;
    imsize = new_width*new_height;
    savefilename_prefix = new_savedir;
    
    GLOB_IMSIZE = imsize;

}


/* Function to De-initialise camera. MUST CALL AFTER USING!!! */
void FLIRCamera::DeinitCamera(){
	INodeMap& node_map = pCam->GetNodeMap();
    pCam->DeInit();
}



/* Function to take a number of images with a camera and optionally work on them.
   INPUTS:
      num_frames - number of images to take
      fits_array - allocated array to store image data in
      f - a callback function that will be applied to each image in real time.
          Give NULL for no callback function.
*/
int FLIRCamera::GrabFrames(unsigned long num_frames, unsigned long start_index, int (*f)(unsigned short*)) {
    int main_result = 0;
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

		int current_index = 0;
        for (unsigned int image_cnt = 0; image_cnt < num_frames; image_cnt++){

            // Retrive image
            ImagePtr ptr_result_image = pCam->GetNextImage();

            // Load current raw image data into an array
            unsigned short* data = (unsigned short*)ptr_result_image->GetData();

			current_index = (start_index + image_cnt)%buffer_size;
            // Append data to an allocated memory array
            pthread_mutex_lock(&GLOB_IMG_MUTEX_ARRAY[current_index]);
			memcpy(GLOB_IMG_ARRAY+imsize*current_index, data, imsize*2);
			pthread_mutex_unlock(&GLOB_IMG_MUTEX_ARRAY[current_index]);

            // Do something with the data in real time if required
            // If 0 is returned by the callback function, end acquisition (regardless of
            // number of frames to go).
            if (*f != NULL){
                int result;

                result = (*f)(data);

                if (result == 0){

                	main_result = 1;
                    ptr_result_image->Release();
                    break;

                }
            }

            // Release image
            pthread_mutex_lock(&GLOB_LATEST_IMG_INDEX_LOCK);
            GLOB_LATEST_IMG_INDEX = current_index;
            pthread_mutex_unlock(&GLOB_LATEST_IMG_INDEX_LOCK);
            
            ptr_result_image->Release();

        }


        // End Timing
        end = std::chrono::steady_clock::now();

        // End Acquisition
        pCam->EndAcquisition();

        cout << "Finished Acquisition" << endl << endl;

        //Set timestamp
        tm *gmtm = gmtime(&start_time);
        char* dt = asctime(gmtm);

        timestamp = dt;

        //Calculate duration
        double duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

        total_exposure = duration;

        return main_result;

    }
    catch (Spinnaker::Exception &e) {
        cout << "Error: " << e.what() << endl;
    }
    return main_result;
}


/* Write a given array of image data as a FITS file
   INPUTS:
      image_array - array of image data to write
      num_images - number of images in the array to writes
*/
int FLIRCamera::SaveFITS(unsigned long num_images, unsigned long start_index)
{
    // Pointer to the FITS file; defined in fitsio.h
    fitsfile *fptr;

	unsigned short *linear_image_array;
	linear_image_array = (unsigned short*)malloc(sizeof(unsigned short)*imsize*num_images);
	
	unsigned long current_index;
	for(unsigned long i=0;i<num_images;i++){
		current_index = (start_index + i)%buffer_size;
		pthread_mutex_lock(&GLOB_IMG_MUTEX_ARRAY[current_index]);
		memcpy(linear_image_array,GLOB_IMG_ARRAY+imsize*current_index,imsize*2);
		pthread_mutex_unlock(&GLOB_IMG_MUTEX_ARRAY[current_index]);
	}

    // Define filepath and name for the FITS file
    string file_path = "!" + savefilename + ".fits";

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
    if ( fits_write_img(fptr, TUSHORT, fpixel, nelements, linear_image_array, &status) ){
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
	pthread_mutex_lock(&GLOB_LATEST_FILE_LOCK);
    GLOB_LATEST_FILE = savefilename + ".fits";
   	pthread_mutex_unlock(&GLOB_LATEST_FILE_LOCK);
   	
    return( status );
}
