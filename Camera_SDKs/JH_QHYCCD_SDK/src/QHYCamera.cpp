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
    int i = 0;
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
      pCam_init - Spinnaker camera pointer
      config_init - Parsed TOML table
*/
QHYCamera::QHYCamera(qhyccd_handle pCam_init, toml::table config_init){

    pCamHandle = pCam_init;
    config = config_init;

    cout << "Loading Config" << endl;
    width = config["camera"]["width"].value_or(0);
    height = config["camera"]["height"].value_or(0);
    offset_x = config["camera"]["offset_x"].value_or(0);
    offset_y = config["camera"]["offset_y"].value_or(0);

    exposure_time = config["camera"]["exposure_time"].value_or(0);
    gain = config["camera"]["gain"].value_or(0);
    black_level = config["camera"]["black_level"].value_or(0);

    pixel_format = config["camera"]["pixel_format"].value_or(16);
    acquisition_mode = config["camera"]["acquisition_mode"].value_or(1);
    readout_mode = config["camera"]["readout_mode"].value_or(0);

    imsize = width*height;
    buffer_size = config["camera"]["buffer_size"].value_or(0);

}


/* Function to setup and start the camera. MUST CALL BEFORE USING!!! */
void QHYCamera::InitCamera(){

    cout << "Camera device information" << endl
         << "=========================" << endl;

    FirmWareVersion(pCamHandle);

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
        printf("SetQHYCCDReadMode set to: %d, success.\n", acquisition_mode);
        char* name;
        retVal = GetQHYCCDReadModeName(pCamHandle,readout_mode,name);
        if (QHYCCD_SUCCESS == retVal){
            printf("Read mode set to: %d\n", name)
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

    // Set Gamma to 0
    retVal = IsQHYCCDControlAvailable(pCamHandle, CONTROL_GAMMA);
    if (QHYCCD_SUCCESS == retVal){
        retVal = SetQHYCCDParam(pCamHandle, CONTROL_GAMMA, 0);
        if (retVal == QHYCCD_SUCCESS){
            printf("SetQHYCCDParam CONTROL_GAMMA set to: %d, success\n", 0);
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

}


/* Function to De-initialise camera. MUST CALL AFTER USING!!! */
void FLIRCamera::DeinitCamera(){

    // close camera handle
    retVal = CloseQHYCCD(pCamHandle);
    if (QHYCCD_SUCCESS == retVal){
        printf("Close QHYCCD success.\n");
    } else{
        printf("Close QHYCCD failure, error: %d\n", retVal);
    }
}


/* Function to take a number of images with a camera and optionally work on them.
   INPUTS:
      num_frames - number of images to take
      fits_array - allocated array to store image data in
      f - a callback function that will be applied to each image in real time.
          Give NULL for no callback function.
*/
void QHYCamera::GrabFrames(unsigned long num_frames, unsigned short* image_array, int (*f)(unsigned short*)) {

    if(readout_mode != 1){
        printf("Wrong Readout mode! Exiting");
        return 1;
    }

    cout << "Start Acquisition" << endl;

    ret = BeginQHYCCDLive(pCamHandle);
    if(ret == QHYCCD_SUCCESS){
        printf("BeginQHYCCDLive success!\n");
    } else{
        printf("BeginQHYCCDLive failed\n");
        return 1;
    }

    // get requested memory length
    uint32_t length = GetQHYCCDMemLength(pCamHandle);

    unsigned short *ImgData;
    unsigned int channels;

    if(length > 0){
        ImgData = (unsigned short *)malloc(length);
        memset(ImgData,0,length);
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

        // Retrive image
        ret = GetQHYCCDLiveFrame(pCamHandle,&width,&height,&bpp,&channels,ImgData);
        if(ret == QHYCCD_SUCCESS){
            printf("GetQHYCCDLiveFrame: %d x %d, bpp: %d, channels: %d, success.\n", width, height, bpp, channels);

            // Append data to an allocated memory array
            memcpy(image_array+imsize*(image_cnt%buffer_size), ImgData, imsize*2);

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
                    ret = StopQHYCCDLive(pCamHandle);
                    if(ret == QHYCCD_SUCCESS){
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

                    return;

                }
            }

            // Release image
            delete(ImgData);

          } else{
            printf("GetQHYCCDSingleFrame failure, error: %d\n", retVal);
          }
    }

    cout << endl;

    // End Timing
    end = std::chrono::steady_clock::now();

    // End Acquisition
    ret = StopQHYCCDLive(pCamHandle);
    if(ret == QHYCCD_SUCCESS){
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
