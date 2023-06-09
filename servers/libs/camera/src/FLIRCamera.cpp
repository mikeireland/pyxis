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
    
    width_min = config["bounds"]["width"][0].value_or(0);    
    width_max = config["bounds"]["width"][1].value_or(0);
    
    height_min = config["bounds"]["height"][0].value_or(0); 
    height_max = config["bounds"]["height"][1].value_or(0); 
    
    exposure_time_min = config["bounds"]["exposure_time"][0].value_or(0);     
    exposure_time_max = config["bounds"]["exposure_time"][1].value_or(0); 
    
    gain_min = config["bounds"]["gain"][0].value_or(0); 
    gain_max = config["bounds"]["gain"][1].value_or(0); 

    pixel_format = "Mono16";
    acquisition_mode = "Continuous";
    adc_bit_depth = config["camera"]["adc_bit_depth"].value_or("");

    black_level = config["camera"]["black_level"].value_or(0.0);
    black_level_min = config["bounds"]["black_level"][0].value_or(0.0);
    black_level_max = config["bounds"]["black_level"][1].value_or(0.0);
    
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
    
    // Configure Buffer Mode to only return the newest frame ("NewestOnly"). Can set to "NewestFirst" if things are breaking.
    // Default is "OldestFirst" for some reason, hence a few bugs!
    // Retrieve Stream Parameters device nodemap
    Spinnaker::GenApi::INodeMap& sNodeMap = pCam->GetTLStreamNodeMap();
    // Retrieve Buffer Handling Mode Information
    CEnumerationPtr ptrHandlingMode = sNodeMap.GetNode("StreamBufferHandlingMode");
    if (!IsReadable(ptrHandlingMode) || !IsWritable(ptrHandlingMode)){
        cout << "Unable to set Buffer Handling mode (node retrieval). Aborting..." << endl << endl;
    }
    CEnumEntryPtr ptrHandlingModeEntry = ptrHandlingMode->GetCurrentEntry();
    if (!IsReadable(ptrHandlingModeEntry)){
        cout << "Unable to get Buffer Handling mode (Entry retrieval). Aborting..." << endl << endl;
    }
    ptrHandlingModeEntry = ptrHandlingMode->GetEntryByName("NewestOnly");
    ptrHandlingMode->SetIntValue(ptrHandlingModeEntry->GetValue());
    cout << "Buffer Handling Mode has been set to " << ptrHandlingModeEntry->GetDisplayName() << endl;

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
    
    // Set global configuration struct
    pthread_mutex_lock(&GLOB_FLAG_LOCK);
    GLOB_IMSIZE = imsize;
    GLOB_WIDTH = width;
    GLOB_CONFIG_PARAMS.gain = gain;
    GLOB_CONFIG_PARAMS.exptime = exposure_time;
    GLOB_CONFIG_PARAMS.width = width;
    GLOB_CONFIG_PARAMS.height = height;
    GLOB_CONFIG_PARAMS.offsetX = offset_x;
    GLOB_CONFIG_PARAMS.offsetY = offset_y;
    GLOB_CONFIG_PARAMS.blacklevel = black_level;
    GLOB_CONFIG_PARAMS.buffersize = buffer_size;
    GLOB_CONFIG_PARAMS.savedir = savefilename_prefix;
    
    GLOB_WIDTH_MAX = width_max;
    GLOB_WIDTH_MIN = width_min;
    GLOB_HEIGHT_MAX = height_max;
    GLOB_HEIGHT_MIN = height_min;
    GLOB_GAIN_MAX = gain_max;
    GLOB_GAIN_MIN = gain_min;
    GLOB_EXPTIME_MAX = exposure_time_max;
    GLOB_EXPTIME_MIN = exposure_time_min;
    GLOB_BLACKLEVEL_MAX = black_level_max;
    GLOB_BLACKLEVEL_MIN = black_level_min;
    
    pthread_mutex_unlock(&GLOB_FLAG_LOCK);

}

/* Reconfigure a float parameter for a FLIR Camera*/
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

/* Reconfigure a int parameter for a FLIR Camera*/
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

/* Function to reconfigure all parameters, both in the camera and in the class parameters. Inputs are explanatory */
void FLIRCamera::ReconfigureAll(int new_gain, int new_exptime, int new_width, int new_height, int new_offsetX, 
                                int new_offsetY, float new_blacklevel, int new_buffersize, string new_savedir){

    ReconfigureFloat("Gain",new_gain);
    gain = new_gain;
    ReconfigureFloat("ExposureTime",new_exptime);
    exposure_time = new_exptime;
    ReconfigureInt("OffsetX",0); //Prevent Crashing
    ReconfigureInt("OffsetY",0); // Prevent Crashing
    ReconfigureInt("Width",new_width);
    width = new_width;
    ReconfigureInt("Height",new_height);
    height = new_height;
    ReconfigureInt("OffsetX",new_offsetX);
    offset_x = new_offsetX;
    ReconfigureInt("OffsetY",new_offsetY);
    offset_y = new_offsetY;
    ReconfigureFloat("BlackLevel",new_blacklevel);
    black_level = new_blacklevel;
    buffer_size = new_buffersize;
    this->imsize = new_width*new_height;
    savefilename_prefix = new_savedir;
    
    GLOB_IMSIZE = imsize;
    GLOB_WIDTH = new_width;
}


/* Function to De-initialise camera. MUST CALL AFTER USING!!! */
void FLIRCamera::DeinitCamera(){
	INodeMap& node_map = pCam->GetNodeMap();
    pCam->DeInit();
}



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
            // If 1 is returned by the callback function, end acquisition (regardless of
            // number of frames to go).
            if (*f != NULL){
                int result;

                result = (*f)(data);

                if (result == 1){

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
        std::string dt = asctime(gmtm);
        dt.pop_back();

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
      num_images - number of images in the array to write
      start_index - frame number index of where in the circular buffer to start saving images
*/
int FLIRCamera::SaveFITS(unsigned long num_images, unsigned long start_index)
{
    // Pointer to the FITS file; defined in fitsio.h
    fitsfile *fptr;

    // Take the circular buffer and put the relevant frames in a single, linear array
	unsigned short *linear_image_array;
	cout << imsize <<endl;
	linear_image_array = (unsigned short*)malloc(sizeof(unsigned short)*imsize*num_images);
	

	// Fill "saving" array with images
	unsigned long current_index;
	for(unsigned long i=0;i<num_images;i++){
		current_index = (start_index + i)%buffer_size;
		pthread_mutex_lock(&GLOB_IMG_MUTEX_ARRAY[current_index]);
		memcpy(linear_image_array+imsize*i,GLOB_IMG_ARRAY+imsize*current_index,imsize*2);
		pthread_mutex_unlock(&GLOB_IMG_MUTEX_ARRAY[current_index]);
	}

    

    // Define filepath and name for the FITS file
    string file_path = "!" + savefilename + ".fits";

    // Configure FITS file
    int bitpix = config["fits"]["bitpix"].value_or(20);
    long naxis = 3; // 2D image over time
    long naxes[3] = {width, height, num_images};
    
    
    //Coadd frames?
    if(GLOB_COADD){
        cout << "Start Coadd" << endl;
	    for(unsigned long i=0;i<imsize;i++){
	        for(unsigned long j=1;j<num_images;j++){
	            linear_image_array[i] += linear_image_array[i+j*imsize];
	        }
	    }
        naxes[2] = 1;
    }

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

    // Write starting time in UTC
    if ( fits_write_key(fptr, TSTRING, "STARTTIME", &timestamp[0],
         "Timestamp of beginning of exposure UTC", &status) )
         return( status );

    // Write pixel format
    if ( fits_write_key(fptr, TSTRING, "PIXEL FORMAT", &pixel_format[0],
         "Pixel Format", &status) )
         return( status );

    // Write ADC Bit Depth
    if ( fits_write_key(fptr, TSTRING, "ADC BIT DEPTH", &adc_bit_depth[0],
         "ADC Bit Depth", &status) )
         return( status );
         
    // Write Coadd Flag
    if ( fits_write_key(fptr, TINT, "COADD_FLAG", &GLOB_COADD,
         "Coadded Image Flag", &status) )
         return( status );
    
    if(GLOB_COADD){     
        // Number of frames coadded
        if ( fits_write_key(fptr, TINT, "COADD_NUM", &num_images,
             "Number of coadded images", &status) )
             return( status );
    }

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
         
    // Write black level
    if ( fits_write_key(fptr, TDOUBLE, "BLACK LEVEL", &black_level,
         "Black Level (ADU?)", &status) )
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
    free(linear_image_array);
    fits_close_file(fptr, &status);
	pthread_mutex_lock(&GLOB_LATEST_FILE_LOCK);
    GLOB_LATEST_FILE = savefilename + ".fits";
   	pthread_mutex_unlock(&GLOB_LATEST_FILE_LOCK);
   	
    return( status );
}
