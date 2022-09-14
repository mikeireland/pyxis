#include <iostream>
#include <string>
#include <fstream>
#include <unistd.h>
#include <pthread.h>
#include "globals.h"
#include "runFLIRCam.h"
#include <fmt/core.h>
#include <opencv2/opencv.hpp>
#include <commander/commander.h>

using namespace std;
using json = nlohmann::json;

// FLIR Camera Server
struct FLIRCameraServer {

    FLIRCameraServer()
    {
        fmt::print("FLIRCameraServer\n");
    }

    ~FLIRCameraServer()
    {
        fmt::print("~FLIRCameraServer\n");
    }


//Get status of camera
string status(){
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
configuration getparams(){
	configuration ret_c;
    pthread_mutex_lock(&GLOB_FLAG_LOCK);
    ret_c = GLOB_CONFIG_PARAMS;
    pthread_mutex_unlock(&GLOB_FLAG_LOCK);
    return ret_c;
	}

// Reconfigure parameters from an input configuration struct
string reconfigure_all(configuration c){
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
    } else if ((unsigned)(c.offsetX) > (GLOB_WIDTH_MAX-c.width)){
        ret_msg = "X offset out of bounds! Offset X should be between 0 and " + std::to_string(GLOB_WIDTH_MAX-GLOB_CONFIG_PARAMS.width);
    } else if ((unsigned)(c.offsetY) > (GLOB_HEIGHT_MAX-c.height)){
        ret_msg = "Y offset out of bounds! Offset Y should be between 0 and " + std::to_string(GLOB_HEIGHT_MAX-GLOB_CONFIG_PARAMS.height);
    } else if ((unsigned)(c.blacklevel-GLOB_BLACKLEVEL_MIN) > (GLOB_BLACKLEVEL_MAX-GLOB_BLACKLEVEL_MIN)){
        ret_msg = "Black Level out of bounds! Black level should be between "+ std::to_string(GLOB_BLACKLEVEL_MIN) + " and " + std::to_string(GLOB_BLACKLEVEL_MAX);
    } else {
        GLOB_CONFIG_PARAMS = c; // Set global variable from the input configuration (JSON) struct
        GLOB_RECONFIGURE = 1;
        ret_msg = "Camera Reconfigured";
    }

    pthread_mutex_unlock(&GLOB_FLAG_LOCK);
    return ret_msg;
}


/* ##################################################### */

/* Set each parameter separately: */
string reconfigure_gain(float gain){
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
	
    return ret_msg;
}

string reconfigure_exptime(float exptime){
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
    return ret_msg;
}

string reconfigure_width(int width){
    string ret_msg;
    pthread_mutex_lock(&GLOB_FLAG_LOCK);
    if (((unsigned)(width-GLOB_WIDTH_MIN) > (GLOB_WIDTH_MAX-GLOB_WIDTH_MIN)) || (width % 4 > 0)){
        GLOB_CONFIG_PARAMS.width = width;
        GLOB_RECONFIGURE = 1;
        ret_msg = "Camera Reconfigured Width";
    } else{
        ret_msg = "Width out of bounds! Width should be a multiple of 4 and between "+ std::to_string(GLOB_WIDTH_MIN) + " and " + std::to_string(GLOB_WIDTH_MAX);
    }
    pthread_mutex_unlock(&GLOB_FLAG_LOCK);
    return ret_msg;
}

string reconfigure_height(int height){
    string ret_msg;
    pthread_mutex_lock(&GLOB_FLAG_LOCK);
    if (((unsigned)(height-GLOB_HEIGHT_MIN) > (GLOB_HEIGHT_MAX-GLOB_HEIGHT_MIN)) || (height % 4 > 0)){
        GLOB_CONFIG_PARAMS.height = height;
        GLOB_RECONFIGURE = 1;
        ret_msg = "Camera Reconfigured Height";
    } else{
        ret_msg = "Height out of bounds! Height should be a multiple of 4 and between "+ std::to_string(GLOB_HEIGHT_MIN) + " and " + std::to_string(GLOB_HEIGHT_MAX);
    }
    pthread_mutex_unlock(&GLOB_FLAG_LOCK);
    return ret_msg;
}

string reconfigure_offsetX(int offsetX){
    string ret_msg;
    pthread_mutex_lock(&GLOB_FLAG_LOCK);
    if ((unsigned)(offsetX) <= (GLOB_WIDTH_MAX-GLOB_CONFIG_PARAMS.width)){
        GLOB_CONFIG_PARAMS.offsetX = offsetX;
        GLOB_RECONFIGURE = 1;
        ret_msg = "Camera Reconfigured X offset";
    } else{
        ret_msg = "X offset out of bounds! Offset X should be between 0 and " + std::to_string(GLOB_WIDTH_MAX-GLOB_CONFIG_PARAMS.width);
    }
    pthread_mutex_unlock(&GLOB_FLAG_LOCK);
    return ret_msg;
}

string reconfigure_offsetY(int offsetY){
    string ret_msg;
    pthread_mutex_lock(&GLOB_FLAG_LOCK);
    if ((unsigned)(offsetY) <= (GLOB_HEIGHT_MAX-GLOB_CONFIG_PARAMS.height)){
        GLOB_CONFIG_PARAMS.offsetY = offsetY;
        GLOB_RECONFIGURE = 1;
        ret_msg = "Camera Reconfigured Y offset";
    } else{
        ret_msg = "Y offset out of bounds! Offset Y should be between 0 and " + std::to_string(GLOB_HEIGHT_MAX-GLOB_CONFIG_PARAMS.height);
    }
    pthread_mutex_unlock(&GLOB_FLAG_LOCK);
    return ret_msg;
}

string reconfigure_blacklevel(float blacklevel){
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
    return ret_msg;
}

string reconfigure_buffersize(float buffersize){
    string ret_msg;
    pthread_mutex_lock(&GLOB_FLAG_LOCK);
    GLOB_CONFIG_PARAMS.buffersize = buffersize;
    GLOB_RECONFIGURE = 1;
    pthread_mutex_unlock(&GLOB_FLAG_LOCK);
	ret_msg = "Camera Reconfigured Buffer Size";
    return ret_msg;
}

string reconfigure_savedir(float savedir){
    string ret_msg;
    pthread_mutex_lock(&GLOB_FLAG_LOCK);
    GLOB_CONFIG_PARAMS.savedir = savedir;
    GLOB_RECONFIGURE = 1;
    pthread_mutex_unlock(&GLOB_FLAG_LOCK);
	ret_msg = "Camera Reconfigured Save Directory";
    return ret_msg;
}

/* ##################################################### */

// Connect to a camera, by starting up the runCam pThread
string connectcam(){
	string ret_msg;
	if(GLOB_CAM_STATUS == 0){
		pthread_create(&GLOB_CAMTHREAD, NULL, runCam, NULL);

		ret_msg = "Connected Camera";
	}else{
		ret_msg = "Camera Already Connecting/Connected!";
	}

	return ret_msg;
}

// Disconnect to a camera, by signalling and joining the runCam pThread
string disconnectcam(){
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

// Start acquisition of the camera. Takes in the number of frames to save
// per FITS file (or 0 for continuous, no saving)
string startcam(int num_frames){
	string ret_msg;
	if(GLOB_CAM_STATUS == 2){
		if(GLOB_RUNNING == 0){
			if(GLOB_RECONFIGURE == 0 and GLOB_STOPPING == 0){
				pthread_mutex_lock(&GLOB_FLAG_LOCK);
				GLOB_NUMFRAMES = num_frames;
				GLOB_RUNNING = 1;
				pthread_mutex_unlock(&GLOB_FLAG_LOCK);
				ret_msg = "Starting Camera Exposures";
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
string stopcam(){
	string ret_msg;
	if(GLOB_CAM_STATUS == 2){
		if(GLOB_RUNNING == 1){
			if(GLOB_RECONFIGURE == 0 and GLOB_STOPPING == 0){
				pthread_mutex_lock(&GLOB_FLAG_LOCK);
				GLOB_STOPPING = 1;
				pthread_mutex_unlock(&GLOB_FLAG_LOCK);
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
string getlatestfilename(){
	string ret_msg;
	if(GLOB_CAM_STATUS == 2){
		if(GLOB_RUNNING == 1){
			if(GLOB_RECONFIGURE == 0 and GLOB_STOPPING == 0){
				pthread_mutex_lock(&GLOB_LATEST_FILE_LOCK);
				ret_msg = GLOB_LATEST_FILE;
				pthread_mutex_unlock(&GLOB_LATEST_FILE_LOCK);
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

// Get the latest image data from the camera thread
string getlatestimage(){
	string ret_msg;
	if(GLOB_CAM_STATUS == 2){
		if(GLOB_RUNNING == 1){
			if(GLOB_RECONFIGURE == 0 and GLOB_STOPPING == 0){
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

                //cv::Mat compressed_mat = mat / 256;
                
                // Convert data to vector
                std::vector<unsigned short> array;
                if (mat.isContinuous()) {
                  // array.assign((float*)mat.datastart, (float*)mat.dataend); // <- has problems for sub-matrix like mat = big_mat.row(i)
                  array.assign((unsigned short*)mat.data, (unsigned short*)mat.data + mat.total()*mat.channels());
                } else {
                  for (int i = 0; i < mat.rows; ++i) {
                    array.insert(array.end(), mat.ptr<unsigned short>(i), mat.ptr<unsigned short>(i)+mat.cols*mat.channels());
                  }
                }
                /*
                std::vector<uchar> array;
                if (compressed_mat.isContinuous()) {
                  // array.assign(mat.datastart, mat.dataend); // <- has problems for sub-matrix like mat = big_mat.row(i)
                  array.assign(compressed_mat.data, compressed_mat.data + compressed_mat.total()*compressed_mat.channels());
                } else {
                  for (int i = 0; i < mat.rows; ++i) {
                    array.insert(array.end(), compressed_mat.ptr<uchar>(i), compressed_mat.ptr<uchar>(i)+compressed_mat.cols*compressed_mat.channels());
                  }
                }*/
                  
                cout << array[0] << endl;

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

};

// Serialiser to convert configuration struct to/from JSON
namespace nlohmann {
    template <>
    struct adl_serializer<configuration> {
        static void to_json(json& j, const configuration& p) {
            j = json{{"gain", p.gain}, {"exptime", p.exptime},
                     {"width", p.width}, {"height", p.height},
                     {"offsetX", p.offsetX}, {"offsetY", p.offsetY},
                     {"blacklevel", p.blacklevel}, {"buffersize", p.buffersize},
                     {"savedir", p.savedir}};
        }

        static void from_json(const json& j, configuration& p) {
            j.at("gain").get_to(p.gain);
            j.at("exptime").get_to(p.exptime);
            j.at("width").get_to(p.width);
            j.at("height").get_to(p.height);
            j.at("offsetX").get_to(p.offsetX);
            j.at("offsetY").get_to(p.offsetY);
            j.at("blacklevel").get_to(p.blacklevel);
            j.at("buffersize").get_to(p.buffersize);
            j.at("savedir").get_to(p.savedir);
        }
    };
}


// Register as commander server
COMMANDER_REGISTER(m)
{
    m.instance<FLIRCameraServer>("FLIRCam")
        // To insterface a class method, you can use the `def` method.
        .def("status", &FLIRCameraServer::status, "Camera Status")
        .def("connect", &FLIRCameraServer::connectcam, "Connect the camera")
        .def("disconnect", &FLIRCameraServer::disconnectcam, "Disconnect the camera")
        .def("start", &FLIRCameraServer::startcam, "Start exposures")
        .def("stop", &FLIRCameraServer::stopcam, "Stop exposures")
        .def("getlatestfilename", &FLIRCameraServer::getlatestfilename, "Get the latest image filename")
        .def("getlatestimage", &FLIRCameraServer::getlatestimage, "Get the latest image data")
        .def("reconfigure_all", &FLIRCameraServer::reconfigure_all, "Reconfigure all parameters")
        .def("reconfigure_gain", &FLIRCameraServer::reconfigure_gain, "Reconfigure the gain")
        .def("reconfigure_exptime", &FLIRCameraServer::reconfigure_exptime, "Reconfigure the exposure time")
        .def("reconfigure_width", &FLIRCameraServer::reconfigure_width, "Reconfigure the width")
        .def("reconfigure_height", &FLIRCameraServer::reconfigure_height, "Reconfigure the height")
        .def("reconfigure_offsetX", &FLIRCameraServer::reconfigure_offsetX, "Reconfigure the X offset")
        .def("reconfigure_offsetY", &FLIRCameraServer::reconfigure_offsetY, "Reconfigure the Y offset")
        .def("reconfigure_blacklevel", &FLIRCameraServer::reconfigure_blacklevel, "Reconfigure the black level")
        .def("reconfigure_buffersize", &FLIRCameraServer::reconfigure_buffersize, "Reconfigure the buffer size")
        .def("reconfigure_savedir", &FLIRCameraServer::reconfigure_savedir, "Reconfigure the save directory")
        .def("getparams", &FLIRCameraServer::getparams, "Get all parameters");

}
