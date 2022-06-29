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
#include "helperFunc.h"

using namespace std;

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
    gamma = config["camera"]["gamma"].value_or(1.0);
    black_level = config["camera"]["black_level"].value_or(0);

    pixel_format = config["camera"]["pixel_format"].value_or(16);
    acquisition_mode = config["camera"]["acquisition_mode"].value_or(1);
    readout_mode = config["camera"]["readout_mode"].value_or(0);

    imsize = width*height;
    buffer_size = config["camera"]["buffer_size"].value_or(0);

}


/* Function to setup and start the camera. MUST CALL BEFORE USING!!! */
int QHYCamera::InitCamera(){

    cout << "Camera device information" << endl
         << "=========================" << endl;

    FirmWareVersion(pCamHandle);

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
        printf("SetQHYCCDStreamMode: %d failure, error: %d\n", acquisition_mode, retVal);
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

	//retVal = SetQHYCCDDebayerOnOff(pCamHandle, false);
	//retVal = SetQHYCCDBinMode(pCamHandle, 1, 1);

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


/* Function to take a number of images with a camera and optionally work on them. Uses the single frame API.
   INPUTS:
      num_frames - number of images to take
      fits_array - allocated array to store image data in
      f - a callback function that will be applied to each image in real time.
          Give NULL for no callback function.
*/
int QHYCamera::GrabFramesSingle(unsigned long num_frames, unsigned char* image_array, int (*f)(unsigned char*)) {

    if(acquisition_mode != 0){
        printf("Wrong acquisition mode! Exiting\n");
        return 1;
    }
	unsigned int retVal;
    cout << "Start Acquisition" << endl;

    // get requested memory length
    //unsigned long length = GetQHYCCDMemLength(pCamHandle);
    unsigned long length = imsize*sizeof(unsigned char)*pixel_format/8;

    unsigned char *ImgData;
    unsigned int channels;

    if(length > 0){
        ImgData = (unsigned char *)malloc(length);
        memset(ImgData,0,length);
        printf("Allocated memory for image: %lu [uchar].\n", length);
    } else{
        printf("Frame memory space length failure \n");
        return 1;
    }

    std::chrono::time_point<std::chrono::steady_clock> start, end;

    //Set timestamp
    time_t start_time = time(0);

    //Begin timing
    start = std::chrono::steady_clock::now();

    for (unsigned int image_cnt = 0; image_cnt < num_frames; image_cnt++){
		cout << "Taking Image" << endl;
		// single frame
		printf("ExpQHYCCDSingleFrame(pCamHandle) - start...\n");
		retVal = ExpQHYCCDSingleFrame(pCamHandle);
		printf("ExpQHYCCDSingleFrame(pCamHandle) - end...\n");
		if (QHYCCD_ERROR != retVal){
			printf("ExpQHYCCDSingleFrame success.\n");
		} else{
			printf("ExpQHYCCDSingleFrame failure, error: %d\n", retVal);
			return 1;
		}

		retVal = GetQHYCCDSingleFrame(pCamHandle, &width, &height, &bpp, &channels, ImgData);
  		if (QHYCCD_SUCCESS == retVal){
    		printf("GetQHYCCDSingleFrame: %d x %d, bpp: %d, channels: %d, success.\n", width, height, bpp, channels);
    	} else{
    		printf("GetQHYCCDSingleFrame failure, error: %d\n", retVal);
    		return 1;
    	}
		cout << static_cast<unsigned>(ImgData[0]) << endl;
        // Append data to an allocated memory array
        memcpy(image_array+imsize*pixel_format/8*(image_cnt%buffer_size), ImgData, imsize*pixel_format/8);

        // Do something with the data in real time if required
        // If 0 is returned by the callback function, end acquisition (regardless of
        // number of frames to go).
        if (*f != NULL){
            int result;

            result = (*f)(ImgData);

            if (result == 0){

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

                cout << "Callback Request: Finished Acquisition" << endl;

                //Set timestamp
                tm *gmtm = gmtime(&start_time);
                char* dt = asctime(gmtm);

                timestamp = dt;

                //Calculate duration
                double duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

                total_exposure = duration;

                return 0;
            }
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
    
    return 0;

}

/* Function to take a number of images with a camera and optionally work on them. Uses the live frame API. USE THIS ONE!!!
   INPUTS:
      num_frames - number of images to take
      fits_array - allocated array to store image data in
      f - a callback function that will be applied to each image in real time.
          Give NULL for no callback function.
*/
int QHYCamera::GrabFrames(unsigned long num_frames, unsigned char* image_array, int (*f)(unsigned char*)) {

    if(acquisition_mode != 1){
        printf("Wrong acquisition mode! Exiting\n");
        return 1;
    }

    cout << "Start Acquisition" << endl;


    int retVal = BeginQHYCCDLive(pCamHandle);
    if(retVal == QHYCCD_SUCCESS){
        printf("BeginQHYCCDLive success!\n");
    } else{
        printf("BeginQHYCCDLive failed\n");
        return 1;
    }

    // get requested memory length
    //unsigned long length = GetQHYCCDMemLength(pCamHandle);
    unsigned long length = imsize*sizeof(unsigned char)*pixel_format/8;

    unsigned char *ImgData = (unsigned char *)malloc(length);
    memset(ImgData,0,length);
    
    unsigned int channels;

    std::chrono::time_point<std::chrono::steady_clock> start, end;

    //Set timestamp
    time_t start_time = time(0);

    //Begin timing
    start = std::chrono::steady_clock::now();
	cout << "Taking Images" << endl;
    for (unsigned int image_cnt = 0; image_cnt < num_frames;){
		
        // Retrive image
        retVal = GetQHYCCDLiveFrame(pCamHandle,&width,&height,&bpp,&channels,ImgData);
        if(retVal == QHYCCD_SUCCESS){
         	image_cnt++;
         	//printf("pix=%x\n",ImgData[100]);
         	//printf("pix=%x\n",ImgData[101]);
            printf("GetQHYCCDLiveFrame: %d x %d, bpp: %d, channels: %d, success.\n", width, height, bpp, channels);

            // Append data to an allocated memory array
            memcpy(image_array+imsize*pixel_format/8*(image_cnt%buffer_size), ImgData, imsize*pixel_format/8);

            // Do something with the data in real time if required
            // If 0 is returned by the callback function, end acquisition (regardless of
            // number of frames to go).
            if (*f != NULL){
                int result;

                result = (*f)(ImgData);

                if (result == 0){

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

                    cout << "Callback Request: Finished Acquisition" << endl;

                    //Set timestamp
                    tm *gmtm = gmtime(&start_time);
                    char* dt = asctime(gmtm);

                    timestamp = dt;

                    //Calculate duration
                    double duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

                    total_exposure = duration;

                    return 0;

                }
            }

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
    
    return 0;

}




/* Write a given array of image data as a FITS file
   INPUTS:
      image_array - array of image data to write
      num_images - number of images in the array to writes
*/
int QHYCamera::SaveFITS(unsigned short* image_array, int num_images)
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

    // Write starting time in UTC
    if ( fits_write_key(fptr, TSTRING, "STARTTIME", &timestamp,
         "Timestamp of beginning of exposure UTC", &status) )
         return( status );

    // Write starting time in UTC
    if ( fits_write_key(fptr, TINT, "PIXEL FORMAT", &pixel_format,
         "Pixel Format", &status) )
         return( status );

    // Write starting time in UTC
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
