#include <iostream>
#include <string>
#include <fstream>
#include <unistd.h>
#include <pthread.h>
#include "globals.h"
#include "FLIRcamServerFuncs.h"
#include "runFLIRCam.h"
#include <fmt/core.h>
#include <opencv2/opencv.hpp>
#include <commander/commander.h>
#include <functional>

using namespace std;
using json = nlohmann::json;

/*
Callback function to do nothing!
Inputs:
    data - array of the raw camera data
Output:
    return 1 if error
*/
int SimpleCallback (unsigned short* data){
    cout << "I could be working here!" << endl;
    return 0;
}

/*
Constructor with no callback
*/
FLIRCameraServer::FLIRCameraServer(){
        fmt::print("FLIRCameraServer\n");
        GLOB_CALLBACK = SimpleCallback;
    }

/*
Constructor with callback
*/
FLIRCameraServer::FLIRCameraServer(std::function<int(unsigned short*)> AnalysisFunc){
        fmt::print("FLIRCameraServer\n");
        GLOB_CALLBACK = AnalysisFunc;
    }

/*
Destructor
*/
FLIRCameraServer::~FLIRCameraServer(){
        fmt::print("~FLIRCameraServer\n");
    }

//Get status of camera
string FLIRCameraServer::status(){
	string ret_msg;
	if(GLOB_CAM_STATUS == 0){
		ret_msg = "Camera Not Connected!";
	}else if(GLOB_CAM_STATUS == 1){
		ret_msg = "Camera Connecting";
	}else if(GLOB_RECONFIGURE == 1){
		ret_msg = "Camera Reconfiguring";
	}else if(GLOB_RUNNING == 1){
		ret_msg = "Camera Running! Saving " + std::to_string(GLOB_NUMFRAMES) + " frames per file";
	}else if(GLOB_STOPPING == 1){
		ret_msg = "Camera Stopping";
	}else{
		ret_msg = "Camera Waiting";
	}

	return ret_msg;
}

// Get parameters from the global variable
configuration FLIRCameraServer::getparams(){
	configuration ret_c;
    pthread_mutex_lock(&GLOB_FLAG_LOCK);
    ret_c = GLOB_CONFIG_PARAMS;
    pthread_mutex_unlock(&GLOB_FLAG_LOCK);
    return ret_c;
	}

/*
Reconfigure all parameters on the camera;
The complexity here is due to checking for bounds
Inputs:
    c - configuration struct
*/
string FLIRCameraServer::reconfigure_all(configuration c){
    string ret_msg;
    pthread_mutex_lock(&GLOB_FLAG_LOCK);
    if ((unsigned)(c.gain-GLOB_GAIN_MIN) > (GLOB_GAIN_MAX-GLOB_GAIN_MIN)){
        ret_msg = "Gain out of bounds! Gain should be between "+ std::to_string(GLOB_GAIN_MIN) + " and " + std::to_string(GLOB_GAIN_MAX);
    } else if ((unsigned)(c.exptime-GLOB_EXPTIME_MIN) > (GLOB_EXPTIME_MAX-GLOB_EXPTIME_MIN)){
        ret_msg = "Exposure Time out of bounds! Exposure Time should be between "+ std::to_string(GLOB_EXPTIME_MIN) + " and " + std::to_string(GLOB_EXPTIME_MAX);
    } else if (((unsigned)(c.width-GLOB_WIDTH_MIN) > (GLOB_WIDTH_MAX-GLOB_WIDTH_MIN)) || (c.width % 4 > 0)){
        ret_msg = "Width out of bounds! Width should be a multiple of 4 and between "+ std::to_string(GLOB_WIDTH_MIN) + " and " + std::to_string(GLOB_WIDTH_MAX);
    } else if (((unsigned)(c.height-GLOB_HEIGHT_MIN) > (GLOB_HEIGHT_MAX-GLOB_HEIGHT_MIN)) || (c.height % 4 > 0)){
        ret_msg = "Height out of bounds! Height should be a multiple of 4 and between "+ std::to_string(GLOB_HEIGHT_MIN) + " and " + std::to_string(GLOB_HEIGHT_MAX);
    } else if (((unsigned)(c.offsetX) > (GLOB_WIDTH_MAX-c.width)) || (c.offsetX % 4 > 0)){
        ret_msg = "X offset out of bounds! Offset X should be a multiple of 4 and between 0 and " + std::to_string(GLOB_WIDTH_MAX-c.width);
    } else if (((unsigned)(c.offsetY) > (GLOB_HEIGHT_MAX-c.height)) || (c.offsetY % 4 > 0)){
        ret_msg = "Y offset out of bounds! Offset Y should be a multiple of 4 and between 0 and " + std::to_string(GLOB_HEIGHT_MAX-c.height);
    } else if ((unsigned)(c.blacklevel-GLOB_BLACKLEVEL_MIN) > (GLOB_BLACKLEVEL_MAX-GLOB_BLACKLEVEL_MIN)){
        ret_msg = "Black Level out of bounds! Black level should be between "+ std::to_string(GLOB_BLACKLEVEL_MIN) + " and " + std::to_string(GLOB_BLACKLEVEL_MAX);
    } else {
        GLOB_CONFIG_PARAMS = c; // Set global variable from the input configuration (JSON) struct
        GLOB_RECONFIGURE = 1;
        ret_msg = "Camera Reconfigured";
    }

    pthread_mutex_unlock(&GLOB_FLAG_LOCK);
    while (GLOB_RECONFIGURE == 1){
        usleep(100);
    }
    return ret_msg;
}


/* ##################################################### */

/* Set each parameter separately: */
string FLIRCameraServer::reconfigure_gain(float gain){
    string ret_msg;
    pthread_mutex_lock(&GLOB_FLAG_LOCK);
    if ((unsigned)(gain-GLOB_GAIN_MIN) <= (GLOB_GAIN_MAX-GLOB_GAIN_MIN)){
        GLOB_CONFIG_PARAMS.gain = gain;
        GLOB_RECONFIGURE = 1;
        ret_msg = "Camera Reconfigured Gain";
    } else{
        ret_msg = "Gain out of bounds! Gain should be between "+ std::to_string(GLOB_GAIN_MIN) + " and " + std::to_string(GLOB_GAIN_MAX);
    }
    pthread_mutex_unlock(&GLOB_FLAG_LOCK);
	
    while (GLOB_RECONFIGURE == 1){
        usleep(100);
    }
    return ret_msg;
}

string FLIRCameraServer::reconfigure_exptime(float exptime){
    string ret_msg;
    pthread_mutex_lock(&GLOB_FLAG_LOCK);
    if ((unsigned)(exptime-GLOB_EXPTIME_MIN) <= (GLOB_EXPTIME_MAX-GLOB_EXPTIME_MIN)){
        GLOB_CONFIG_PARAMS.exptime = exptime;
        GLOB_RECONFIGURE = 1;
        ret_msg = "Camera Reconfigured Exposure Time";
    } else{
        ret_msg = "Exposure Time out of bounds! Exposure Time should be between "+ std::to_string(GLOB_EXPTIME_MIN) + " and " + std::to_string(GLOB_EXPTIME_MAX);
    }
    pthread_mutex_unlock(&GLOB_FLAG_LOCK);
    while (GLOB_RECONFIGURE == 1){
        usleep(100);
    }
    return ret_msg;
}

string FLIRCameraServer::reconfigure_width(int width){
    string ret_msg;
    pthread_mutex_lock(&GLOB_FLAG_LOCK);
    if (((unsigned)(width-GLOB_WIDTH_MIN) <= (GLOB_WIDTH_MAX-GLOB_WIDTH_MIN)) || (width % 4 > 0)){
        GLOB_CONFIG_PARAMS.width = width;
        GLOB_RECONFIGURE = 1;
        ret_msg = "Camera Reconfigured Width";
    } else{
        ret_msg = "Width out of bounds! Width should be a multiple of 4 and between "+ std::to_string(GLOB_WIDTH_MIN) + " and " + std::to_string(GLOB_WIDTH_MAX);
    }
    pthread_mutex_unlock(&GLOB_FLAG_LOCK);
    while (GLOB_RECONFIGURE == 1){
        usleep(100);
    }
    return ret_msg;
}

string FLIRCameraServer::reconfigure_height(int height){
    string ret_msg;
    pthread_mutex_lock(&GLOB_FLAG_LOCK);
    if (((unsigned)(height-GLOB_HEIGHT_MIN) <= (GLOB_HEIGHT_MAX-GLOB_HEIGHT_MIN)) || (height % 4 > 0)){
        GLOB_CONFIG_PARAMS.height = height;
        GLOB_RECONFIGURE = 1;
        ret_msg = "Camera Reconfigured Height";
    } else{
        ret_msg = "Height out of bounds! Height should be a multiple of 4 and between "+ std::to_string(GLOB_HEIGHT_MIN) + " and " + std::to_string(GLOB_HEIGHT_MAX);
    }
    pthread_mutex_unlock(&GLOB_FLAG_LOCK);
    while (GLOB_RECONFIGURE == 1){
        usleep(100);
    }
    return ret_msg;
}

string FLIRCameraServer::reconfigure_offsetX(int offsetX){
    string ret_msg;
    pthread_mutex_lock(&GLOB_FLAG_LOCK);
    if (((unsigned)(offsetX) <= (GLOB_WIDTH_MAX-GLOB_CONFIG_PARAMS.width)) || (offsetX % 4 > 0)){
        GLOB_CONFIG_PARAMS.offsetX = offsetX;
        GLOB_RECONFIGURE = 1;
        ret_msg = "Camera Reconfigured X offset";
    } else{
        ret_msg = "X offset out of bounds! Offset X should be a multiple of 4 and between 0 and " + std::to_string(GLOB_WIDTH_MAX-GLOB_CONFIG_PARAMS.width);
    }
    pthread_mutex_unlock(&GLOB_FLAG_LOCK);
    while (GLOB_RECONFIGURE == 1){
        usleep(100);
    }
    return ret_msg;
}

string FLIRCameraServer::reconfigure_offsetY(int offsetY){
    string ret_msg;
    pthread_mutex_lock(&GLOB_FLAG_LOCK);
    if (((unsigned)(offsetY) <= (GLOB_HEIGHT_MAX-GLOB_CONFIG_PARAMS.height)) || (offsetY % 4 > 0)){
        GLOB_CONFIG_PARAMS.offsetY = offsetY;
        GLOB_RECONFIGURE = 1;
        ret_msg = "Camera Reconfigured Y offset";
    } else{
        ret_msg = "Y offset out of bounds! Offset Y should be a multiple of 4 and between 0 and " + std::to_string(GLOB_HEIGHT_MAX-GLOB_CONFIG_PARAMS.height);
    }
    pthread_mutex_unlock(&GLOB_FLAG_LOCK);
    while (GLOB_RECONFIGURE == 1){
        usleep(100);
    }
    return ret_msg;
}

string FLIRCameraServer::reconfigure_blacklevel(float blacklevel){
    string ret_msg;
    pthread_mutex_lock(&GLOB_FLAG_LOCK);
    if ((unsigned)(blacklevel-GLOB_BLACKLEVEL_MIN) <= (GLOB_BLACKLEVEL_MAX-GLOB_BLACKLEVEL_MIN)){
        GLOB_CONFIG_PARAMS.blacklevel = blacklevel;
        GLOB_RECONFIGURE = 1;
        ret_msg = "Camera Reconfigured Black Level";
    } else{
        ret_msg = "Black Level out of bounds! Black level should be between "+ std::to_string(GLOB_BLACKLEVEL_MIN) + " and " + std::to_string(GLOB_BLACKLEVEL_MAX);
    }
    pthread_mutex_unlock(&GLOB_FLAG_LOCK);
    
    while (GLOB_RECONFIGURE == 1){
        usleep(100);
    }
    return ret_msg;
}

string FLIRCameraServer::reconfigure_buffersize(int buffersize){
    string ret_msg;
    pthread_mutex_lock(&GLOB_FLAG_LOCK);
    GLOB_CONFIG_PARAMS.buffersize = buffersize;
    GLOB_RECONFIGURE = 1;
    pthread_mutex_unlock(&GLOB_FLAG_LOCK);
	ret_msg = "Camera Reconfigured Buffer Size";
	while (GLOB_RECONFIGURE == 1){
        usleep(100);
    }
    return ret_msg;
}

string FLIRCameraServer::reconfigure_savedir(std::string savedir){
    string ret_msg;
    pthread_mutex_lock(&GLOB_FLAG_LOCK);
    GLOB_CONFIG_PARAMS.savedir = savedir;
    GLOB_RECONFIGURE = 1;
    pthread_mutex_unlock(&GLOB_FLAG_LOCK);
	ret_msg = "Camera Reconfigured Save Directory";
    while (GLOB_RECONFIGURE == 1){
        usleep(1000);
    }
    return ret_msg;
}

/* ##################################################### */

// Connect to a camera, by starting up the runCam pThread
string FLIRCameraServer::connectcam(){
	string ret_msg;
	if(GLOB_CAM_STATUS == 0){
		pthread_create(&GLOB_CAMTHREAD, NULL, runCam, NULL);
		ret_msg = "Connected Camera";
		while (GLOB_CAM_STATUS!=2){
            usleep(1000);
        }
	}else{
		ret_msg = "Camera Already Connecting/Connected!";
	}

	return ret_msg;
}

// Disconnect to a camera, by signalling and joining the runCam pThread
string FLIRCameraServer::disconnectcam(){
	string ret_msg;
	if(GLOB_CAM_STATUS == 2){
		pthread_mutex_lock(&GLOB_FLAG_LOCK);
		GLOB_CAM_STATUS = 0;
		pthread_mutex_unlock(&GLOB_FLAG_LOCK);

		pthread_join(GLOB_CAMTHREAD, NULL);
		ret_msg = "Disconnected Camera";

	}else{
		ret_msg = "Camera Not Connected or Currently Connecting!";
	}

	return ret_msg;
}

/* 
Start acquisition of the camera. 
Inputs:
    num_frames - number of frames to save. 0 is continuous, no saving
    coadd_flag - add frames together?
*/
string FLIRCameraServer::startcam(int num_frames, int coadd_flag){
	string ret_msg;
	if(GLOB_CAM_STATUS == 2){
		if(GLOB_RUNNING == 0){
			if(GLOB_RECONFIGURE == 0 and GLOB_STOPPING == 0){
			    if(coadd_flag == 1 and num_frames > 50){
				    ret_msg = "Too many frames to coadd! Maximum is 50";
				} else{
				    pthread_mutex_lock(&GLOB_FLAG_LOCK);
				    GLOB_NUMFRAMES = num_frames;
				    GLOB_COADD = coadd_flag;
				    GLOB_RUNNING = 1;
				    pthread_mutex_unlock(&GLOB_FLAG_LOCK);
				    ret_msg = "Starting Camera Exposures";
                }
			}else{
				ret_msg = "Camera Busy!";
			}
		}else{
			ret_msg = "Camera already running!";
		}
	}else{
		ret_msg = "Camera Not Connected or Currently Connecting!";
	}

	return ret_msg;

}

// Stop acquisition of the camera
string FLIRCameraServer::stopcam(){
	string ret_msg;
	if(GLOB_CAM_STATUS == 2){
		if(GLOB_RUNNING == 1){
			if(GLOB_RECONFIGURE == 0 and GLOB_STOPPING == 0){
				pthread_mutex_lock(&GLOB_FLAG_LOCK);
				GLOB_STOPPING = 1;
				pthread_mutex_unlock(&GLOB_FLAG_LOCK);
				while (GLOB_RUNNING == 1){
                    usleep(1000);
                }
				ret_msg = "Stopping Camera Exposures";
			}else{
				ret_msg = "Camera Busy!";
			}
		}else{
			ret_msg = "Camera not running!";
		}
	}else{
		ret_msg = "Camera Not Connected or Currently Connecting!";
	}

	return ret_msg;
}

// Get the filename of the latest saved FITS image
string FLIRCameraServer::getlatestfilename(){
	string ret_msg;
    pthread_mutex_lock(&GLOB_LATEST_FILE_LOCK);
    ret_msg = GLOB_LATEST_FILE;
    pthread_mutex_unlock(&GLOB_LATEST_FILE_LOCK);	
	return ret_msg;
}

/* 
Get latest image 
Inputs:
    compression - PNG compression parameter (see imencode, goes from 0-9)
    binning - flag to see whether to bin or not
*/
string FLIRCameraServer::getlatestimage(int compression, int binning){
	string ret_msg;
	if(GLOB_CAM_STATUS == 2){
		if(GLOB_RUNNING == 1){
			if(GLOB_RECONFIGURE == 0 and GLOB_STOPPING == 0){
			    cout << "starting image proc" << endl;
			    // Get latest index
				pthread_mutex_lock(&GLOB_LATEST_IMG_INDEX_LOCK);
				int img_index = GLOB_LATEST_IMG_INDEX;
				pthread_mutex_unlock(&GLOB_LATEST_IMG_INDEX_LOCK);

                // Make space for the image to send
				unsigned short *ret_image_array;
				ret_image_array = (unsigned short*)malloc(sizeof(unsigned short)*GLOB_IMSIZE);

                // Retrieve image
				pthread_mutex_lock(&GLOB_IMG_MUTEX_ARRAY[img_index]);
				memcpy(ret_image_array,GLOB_IMG_ARRAY+GLOB_IMSIZE*img_index,GLOB_IMSIZE*2);
				pthread_mutex_unlock(&GLOB_IMG_MUTEX_ARRAY[img_index]);

                int height = GLOB_IMSIZE/GLOB_WIDTH;

                // Make OpenCV matrix from image array
				cv::Mat mat (height,GLOB_WIDTH,CV_16U,ret_image_array);
                cout << "converting mat" << endl;
                mat.convertTo(mat, CV_8U, 1/256.0); // CONVERT TO 8 BIT
                
                // Binning (possibly some issues here?)
                if(binning){
                    cout << "binning" << endl;
                    cv::resize(mat,mat,cv::Size(), 0.5, 0.5,cv::INTER_AREA);
                }
                cout << "compressing" << endl;
                // Compress and Convert data to vector
                std::vector<uchar> array;
                std::vector<int> param(2);
                param[0] = cv::IMWRITE_PNG_COMPRESSION;
                param[1] = compression;// COMPRESSION 0-9
                cv::imencode(".png", mat, array, param);
                
                
                /* USE IF NO COMPRESSION!
                if (mat.isContinuous()) {
                  // array.assign((float*)mat.datastart, (float*)mat.dataend); // <- has problems for sub-matrix like mat = big_mat.row(i)
                  array.assign((unsigned short*)mat.data, (unsigned short*)mat.data + mat.total()*mat.channels());
                } else {
                  for (int i = 0; i < mat.rows; ++i) {
                    array.insert(array.end(), mat.ptr<unsigned short>(i), mat.ptr<unsigned short>(i)+mat.cols*mat.channels());
                  }
                }
                */

                //Turn into a JSON array for sending
                json j;
                j["Image"]["rows"] = mat.rows;
                j["Image"]["cols"] = mat.cols;
                j["Image"]["channels"] = mat.channels();
                j["Image"]["data"] = array;

                std::string s = j.dump();

                //Attach to message
				ret_msg = s;
				
			    //Free image
				free(ret_image_array);
			}else{
				ret_msg = "Camera Busy!";
			}
		}else{
			ret_msg = "Camera not running!";
		}
	}else{
		ret_msg = "Camera Not Connected or Currently Connecting!";
	}

	return ret_msg;
}
