// Based on the work done by https://github.com/miks/spinnaker-fps-test

#include <iostream>
#include <string>
#include <ctime>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <chrono>
#include <algorithm>
#include "QHYCamera.h"
#include "qhyccd.h"
#include "toml.hpp"
#include "fitsio.h"
#include "globals.h"

using namespace std;

// Get QHY SDK version
void SDKVersion(){
    unsigned int  YMDS[4];
    unsigned char sVersion[80];

    memset ((char *)sVersion,0x00,sizeof(sVersion));
    GetQHYCCDSDKVersion(&YMDS[0],&YMDS[1],&YMDS[2],&YMDS[3]);

    if ((YMDS[1] < 10)&&(YMDS[2] < 10)){
        sprintf((char *)sVersion,"V20%d0%d0%d_%d\n",YMDS[0],YMDS[1],YMDS[2],YMDS[3]	);
    } else if ((YMDS[1] < 10)&&(YMDS[2] > 10)){
        sprintf((char *)sVersion,"V20%d0%d%d_%d\n",YMDS[0],YMDS[1],YMDS[2],YMDS[3]	);
    } else if ((YMDS[1] > 10)&&(YMDS[2] < 10)){
        sprintf((char *)sVersion,"V20%d%d0%d_%d\n",YMDS[0],YMDS[1],YMDS[2],YMDS[3]	);
    } else{
        sprintf((char *)sVersion,"V20%d%d%d_%d\n",YMDS[0],YMDS[1],YMDS[2],YMDS[3]	);
    }

    fprintf(stderr,"QHYCCD SDK Version: %s\n", sVersion);
}

// Get firmware version. Takes the QHY camera handle.
void FirmWareVersion(qhyccd_handle *h){
    unsigned char fwv[32],FWInfo[256];
    unsigned int ret;
    memset (FWInfo,0x00,sizeof(FWInfo));
    ret = GetQHYCCDFWVersion(h,fwv);
    if(ret == QHYCCD_SUCCESS){
        if((fwv[0] >> 4) <= 9){
            sprintf((char *)FWInfo,"Firmware version:20%d_%d_%d\n",((fwv[0] >> 4) + 0x10),
                (fwv[0]&~0xf0),fwv[1]);
        } else{
            sprintf((char *)FWInfo,"Firmware version:20%d_%d_%d\n",(fwv[0] >> 4),
                    (fwv[0]&~0xf0),fwv[1]);
        }
    } else{
        sprintf((char *)FWInfo,"Firmware version:Not Found!\n");
    }
        fprintf(stderr,"%s\n", FWInfo);
}




/* Constructor: Takes the camera pointer and config table
   and saves them (and config values) as object attributes
   INPUTS:
      pCam_init - QHY camera pointer
      config_init - Parsed TOML table
*/
QHYCamera::QHYCamera(qhyccd_handle *pCam_init, toml::table config_init){

    pCamHandle = pCam_init;
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

    gamma = config["camera"]["gamma"].value_or(1.0);
    black_level = config["camera"]["black_level"].value_or(0);
    black_level_min = config["bounds"]["black_level"][0].value_or(0);
    black_level_max = config["bounds"]["black_level"][1].value_or(0);
    
    pixel_format = 16;
    acquisition_mode = 1;
    readout_mode = config["camera"]["readout_mode"].value_or(2);

    imsize = width*height;
    buffer_size = config["camera"]["buffer_size"].value_or(0);

    savefilename_prefix = config["fits"]["filename_prefix"].value_or("");
    savefilename = savefilename_prefix + ".fits";
}



/* Function to setup and start the camera. MUST CALL BEFORE USING!!! */
int QHYCamera::InitCamera(){

    cout << "Camera device information" << endl
         << "=========================" << endl;

    FirmWareVersion(pCamHandle); //Get firmware version

	unsigned int retVal;

    // Set Capture Mode
    retVal = SetQHYCCDStreamMode(pCamHandle, acquisition_mode);
    if (QHYCCD_SUCCESS == retVal){
        printf("SetQHYCCDStreamMode set to: %d, success.\n", acquisition_mode);
    } else{
        printf("SetQHYCCDStreamMode: %d failure, error: %d\n", acquisition_mode, retVal);
        return 1;
    }


    // Set Read mode
    retVal = SetQHYCCDReadMode(pCamHandle, readout_mode);
    if (QHYCCD_SUCCESS == retVal){
        printf("SetQHYCCDReadMode set to: %d, success.\n", readout_mode);
        char name [32];
        retVal = GetQHYCCDReadModeName(pCamHandle,readout_mode,name);
        if (QHYCCD_SUCCESS == retVal){
            printf("Read mode set to: %s\n", name);
        } else{
            printf("Read mode name failure, error: %d\n", retVal);
            return 1;
        }
    } else{
        printf("SetQHYCCDReadMode: %d failure, error: %d\n", readout_mode, retVal);
        return 1;
    }

    // Initialize camera
    retVal = InitQHYCCD(pCamHandle);
    if (QHYCCD_SUCCESS == retVal){
        printf("InitQHYCCD success.\n");
    } else{
        printf("InitQHYCCD faililure, error: %d\n", retVal);
        return 1;
    }

    double chipWidthMM;
    double chipHeightMM;
    double pixelWidthUM;
    double pixelHeightUM;
    unsigned int maxImageSizeX;
    unsigned int maxImageSizeY;

    // get chip info
    retVal = GetQHYCCDChipInfo(pCamHandle, &chipWidthMM, &chipHeightMM, &maxImageSizeX, &maxImageSizeY, &pixelWidthUM, &pixelHeightUM, &bpp);
    if (QHYCCD_SUCCESS == retVal){
        printf("GetQHYCCDChipInfo:\n");
        printf("Chip  size width x height     : %.3f x %.3f [mm]\n", chipWidthMM, chipHeightMM);
        printf("Pixel size width x height     : %.3f x %.3f [um]\n", pixelWidthUM, pixelHeightUM);
        printf("Image size width x height     : %d x %d\n", maxImageSizeX, maxImageSizeY);
        printf("Bit depth                     : %d \n", bpp);
    } else{
        printf("GetQHYCCDChipInfo failure, error: %d\n", retVal);
        return 1;
    }

    //Configure Camera

    // Set pixel format
    retVal = IsQHYCCDControlAvailable(pCamHandle, CONTROL_TRANSFERBIT);
    if (QHYCCD_SUCCESS == retVal){
        retVal = SetQHYCCDBitsMode(pCamHandle, pixel_format);
        if (retVal == QHYCCD_SUCCESS){
            printf("Bit Resolution set to: %d, success\n", pixel_format);
        } else{
            printf("Bit Resolution failure, error: %d\n", retVal);
            getchar();
            return 1;
        }
    }


    // set image resolution
    retVal = SetQHYCCDResolution(pCamHandle, offset_x, offset_y, width, height);
        
    if (QHYCCD_SUCCESS == retVal){
        printf("SetQHYCCDResolution roiStartX x roiStartY: %d x %d\n", offset_x, offset_y);
        printf("SetQHYCCDResolution roiSizeX  x roiSizeY : %d x %d\n", width, height);
    } else{
        printf("SetQHYCCDResolution failure, error: %d\n", retVal);
        return 1;
    }


    // Set GAMMA (Offset)
    retVal = IsQHYCCDControlAvailable(pCamHandle, CONTROL_GAMMA);
    if (QHYCCD_SUCCESS == retVal){
        retVal = SetQHYCCDParam(pCamHandle, CONTROL_GAMMA, gamma);
        if (retVal == QHYCCD_SUCCESS){
            printf("SetQHYCCDParam CONTROL_GAMMA set to: %f, success\n", gamma);
        } else{
            printf("SetQHYCCDParam CONTROL_GAMMA failure, error: %d\n", retVal);
            getchar();
            return 1;
        }
    }
    
    // Set Black Level (Offset)
    retVal = IsQHYCCDControlAvailable(pCamHandle, CONTROL_OFFSET);
    if (QHYCCD_SUCCESS == retVal){
        retVal = SetQHYCCDParam(pCamHandle, CONTROL_OFFSET, black_level);
        if (retVal == QHYCCD_SUCCESS){
            printf("SetQHYCCDParam CONTROL_OFFSET set to: %d, success\n", black_level);
        } else{
            printf("SetQHYCCDParam CONTROL_OFFSET failure, error: %d\n", retVal);
            getchar();
            return 1;
        }
    }
   
    // Set exposure time
    retVal = IsQHYCCDControlAvailable(pCamHandle, CONTROL_EXPOSURE);
    if (QHYCCD_SUCCESS == retVal){
        retVal = SetQHYCCDParam(pCamHandle, CONTROL_EXPOSURE, exposure_time);
        if (retVal == QHYCCD_SUCCESS){
            printf("SetQHYCCDParam CONTROL_EXPOSURE set to: %d, success\n", exposure_time);
        } else{
            printf("SetQHYCCDParam CONTROL_EXPOSURE failure, error: %d\n", retVal);
            getchar();
            return 1;
        }
    }

    // Set gain
    retVal = IsQHYCCDControlAvailable(pCamHandle, CONTROL_GAIN);
    if (QHYCCD_SUCCESS == retVal){
        retVal = SetQHYCCDParam(pCamHandle, CONTROL_GAIN, gain);
        if (retVal == QHYCCD_SUCCESS){
            printf("SetQHYCCDParam CONTROL_GAIN set to: %d, success\n", gain);
        } else{
            printf("SetQHYCCDParam CONTROL_GAIN failure, error: %d\n", retVal);
            getchar();
            return 1;
        }
    } 

	// A few more values??
    retVal = SetQHYCCDParam(pCamHandle, CONTROL_USBTRAFFIC, 255);
    retVal = SetQHYCCDParam(pCamHandle, CONTROL_DDR, 1.0);
    
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
    GLOB_CONFIG_PARAMS.blacklevel = static_cast<float>(black_level);
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
    GLOB_BLACKLEVEL_MAX = static_cast<double>(black_level_max);
    GLOB_BLACKLEVEL_MIN = static_cast<double>(black_level_min);    
    
    pthread_mutex_unlock(&GLOB_FLAG_LOCK);
    
    return 0;
}

/* Function to De-initialise camera. MUST CALL AFTER USING!!! */
void QHYCamera::DeinitCamera(){

    // close camera handle
    unsigned int retVal = CloseQHYCCD(pCamHandle);
    if (QHYCCD_SUCCESS == retVal){
        printf("Close QHYCCD success.\n");
    } else{
        printf("Close QHYCCD failure, error: %d\n", retVal);
    }
}

/* Function to reconfigure all parameters, both in the camera and in the class parameters. Inputs are explanatory. Outputs 0 on success */
int QHYCamera::ReconfigureAll(int new_gain, int new_exptime, int new_width, int new_height, int new_offsetX, 
                              int new_offsetY, int new_blacklevel, int new_buffersize, string new_savedir){

    unsigned int retVal;
    // Set exposure time
    retVal = IsQHYCCDControlAvailable(pCamHandle, CONTROL_EXPOSURE);
    if (QHYCCD_SUCCESS == retVal){
        retVal = SetQHYCCDParam(pCamHandle, CONTROL_EXPOSURE, new_exptime);
        if (retVal == QHYCCD_SUCCESS){
            printf("SetQHYCCDParam CONTROL_EXPOSURE set to: %d, success\n", new_exptime);
        } else{
            printf("SetQHYCCDParam CONTROL_EXPOSURE failure, error: %d\n", retVal);
            getchar();
            return 1;
        }
    }
    
    exposure_time = new_exptime;

    // Set gain
    retVal = IsQHYCCDControlAvailable(pCamHandle, CONTROL_GAIN);
    if (QHYCCD_SUCCESS == retVal){
        retVal = SetQHYCCDParam(pCamHandle, CONTROL_GAIN, new_gain);
        if (retVal == QHYCCD_SUCCESS){
            printf("SetQHYCCDParam CONTROL_GAIN set to: %d, success\n", new_gain);
        } else{
            printf("SetQHYCCDParam CONTROL_GAIN failure, error: %d\n", retVal);
            getchar();
            return 1;
        }
    } 
    
    gain = new_gain;

    // Set new CCD resolution (offsets, size)
    retVal = SetQHYCCDResolution(pCamHandle, new_offsetX, new_offsetY, new_width, new_height);
        
    if (QHYCCD_SUCCESS == retVal){
        printf("SetQHYCCDResolution roiStartX x roiStartY: %d x %d\n", new_offsetX, new_offsetY);
        printf("SetQHYCCDResolution roiSizeX  x roiSizeY : %d x %d\n", new_width, new_height);
    } else{
        printf("SetQHYCCDResolution failure, error: %d\n", retVal);
        return 1;
    }

    width = new_width;
    height = new_height;
    offset_x = new_offsetX;
    offset_y = new_offsetY;
    
    // Set black level
    retVal = IsQHYCCDControlAvailable(pCamHandle, CONTROL_OFFSET);
    if (QHYCCD_SUCCESS == retVal){
        retVal = SetQHYCCDParam(pCamHandle, CONTROL_OFFSET, new_blacklevel);
        if (retVal == QHYCCD_SUCCESS){
            printf("SetQHYCCDParam CONTROL_OFFSET set to: %d, success\n", new_blacklevel);
        } else{
            printf("SetQHYCCDParam CONTROL_OFFSET failure, error: %d\n", retVal);
            getchar();
            return 1;
        }
    }
   
    black_level = new_blacklevel;
    
    buffer_size = new_buffersize;
    this->imsize = new_width*new_height;
    savefilename_prefix = new_savedir;
    
    GLOB_IMSIZE = imsize;
    GLOB_WIDTH = new_width;

    return 0;
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
      1 on error
      2 on callback exit
*/
int QHYCamera::GrabFrames(unsigned long num_frames, unsigned long start_index, int (*f)(unsigned short*)) {

    int main_result = 0;

    if(acquisition_mode != 1){
        printf("Wrong acquisition mode! Exiting\n");
        return 1;
    }

    cout << "Start Acquisition" << endl;

    // Begin Camera Acquisition
    int retVal = BeginQHYCCDLive(pCamHandle);
    if(retVal == QHYCCD_SUCCESS){
        printf("BeginQHYCCDLive success!\n");
    } else{
        printf("BeginQHYCCDLive failed\n");
        return 1;
    }

    // Get requested memory length for one image
    unsigned long length = imsize*sizeof(unsigned char)*2;

    // Memory for one image
    unsigned char *ImgData = (unsigned char *)malloc(length);
    memset(ImgData,0,length);
    
    unsigned int channels;

    std::chrono::time_point<std::chrono::steady_clock> start, end;

    //Set timestamp
    time_t start_time = time(0);

    //Begin timing
    start = std::chrono::steady_clock::now();
    
    int current_index = 0;
    for (unsigned int image_cnt = 0; image_cnt < num_frames;){
		
        // Retrieve image
        retVal = GetQHYCCDLiveFrame(pCamHandle,&width,&height,&bpp,&channels,ImgData);
        if(retVal == QHYCCD_SUCCESS){
         	image_cnt++;

            // Append data to an allocated memory array
            current_index = (start_index + image_cnt)%buffer_size;
            
            // Convert to 16 bit
            unsigned short *converted_data;
	        converted_data = (unsigned short *) ImgData;
            
            // Append data to an circular buffer array
            pthread_mutex_lock(&GLOB_IMG_MUTEX_ARRAY[current_index]);
			memcpy(GLOB_IMG_ARRAY+imsize*current_index, converted_data, imsize*2);
			pthread_mutex_unlock(&GLOB_IMG_MUTEX_ARRAY[current_index]);
			
          
            // Do something with the data in real time if required
            // If 1 is returned by the callback function, end acquisition (regardless of
            // number of frames to go).
            if (*f != NULL){
                int result;

                result = (*f)(converted_data);

                if (result == 1){

                	main_result = 2;
                    break;
                }
            }
            
            // Release image
            pthread_mutex_lock(&GLOB_LATEST_IMG_INDEX_LOCK);
            GLOB_LATEST_IMG_INDEX = current_index;
            pthread_mutex_unlock(&GLOB_LATEST_IMG_INDEX_LOCK);
            

    	} else if(retVal != -1){
            printf("GetQHYCCDLiveFrame failure, error: %d\n", retVal);
        }
    }
    
    // Release image
    delete(ImgData);
    
    cout << endl;

    // End Timing
    end = std::chrono::steady_clock::now();

    // End Acquisition
    retVal = StopQHYCCDLive(pCamHandle);
    if(retVal == QHYCCD_SUCCESS){
        printf("StopQHYCCDLive success!\n");
    } else{
        printf("StopQHYCCDLive failed\n");
        return 1;
    }

    cout << "Finished Acquisition" << endl;

    //Set timestamp
    tm *gmtm = gmtime(&start_time);
    char* dt = asctime(gmtm);

    timestamp = dt;

    //Calculate duration
    double duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    total_exposure = duration;
    
    return main_result;
}


/* Write a given array of image data as a FITS file. Outputs 0 on success.
   INPUTS:
      num_images - number of images in the array to write
      start_index - frame number index of where in the circular buffer to start saving images
*/
int QHYCamera::SaveFITS(unsigned long num_images, unsigned long start_index)
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

    // Write Target Name
    if ( fits_write_key(fptr, TSTRING, "TARGETNAME", &GLOB_TARGET_NAME[0],
         "Name of Target", &status) )
         return( status );

    // Write RA
    if ( fits_write_key(fptr, TDOUBLE, "RA", &GLOB_RA,
         "Right Ascension (deg)", &status) )
         return( status );

    // Write DEC
    if ( fits_write_key(fptr, TDOUBLE, "DEC", &GLOB_DEC,
         "Declination (deg)", &status) )
         return( status );

    // Write Baseline
    if ( fits_write_key(fptr, TDOUBLE, "BASELINE", &GLOB_BASELINE,
         "Baseline (m)", &status) )
         return( status );

    // Write Datatype
    if ( fits_write_key(fptr, TSTRING, "DATATYPE", &GLOB_DATATYPE[0],
         "Baseline (m)", &status) )
         return( status );

    // Write Pixel Format
    if ( fits_write_key(fptr, TINT, "PIXEL FORMAT", &pixel_format,
         "Pixel Format", &status) )
         return( status );

    // Write Readout Mode
    if ( fits_write_key(fptr, TINT, "READOUTMODE", &readout_mode,
         "Readout mode", &status) )
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
         "Software Gain (-)", &status) )
         return( status );
         
    // Write black level
    if ( fits_write_key(fptr, TINT, "BLACK LEVEL", &black_level,
         "Black Level (ADU)", &status) )
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
    return( status );
}
