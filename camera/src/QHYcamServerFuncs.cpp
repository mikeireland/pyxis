#include <iostream>
#include <string>
#include <fstream>
#include <unistd.h>
#include <pthread.h>
#include "globals.h"
#include "runQHYCam.h"
#include <fmt/core.h>
#include <opencv2/opencv.hpp>
#include <commander/commander.h>

using namespace std;
using json = nlohmann::json;

struct QHYCameraServer {

    QHYCameraServer()
    {
        fmt::print("QHYCameraServer\n");
    }

    ~QHYCameraServer()
    {
        fmt::print("~QHYCameraServer\n");
    }


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

configuration getparams(){
	configuration ret_c;
    pthread_mutex_lock(&GLOB_FLAG_LOCK);
    ret_c = GLOB_CONFIG_PARAMS;
    pthread_mutex_unlock(&GLOB_FLAG_LOCK);
    return ret_c;
	}


string reconfigure_all(configuration c){
    string ret_msg;
    pthread_mutex_lock(&GLOB_FLAG_LOCK);
    GLOB_CONFIG_PARAMS = c;
    GLOB_RECONFIGURE = 1;
    pthread_mutex_unlock(&GLOB_FLAG_LOCK);
	ret_msg = "Camera Reconfiguring";
    return ret_msg;
}

string reconfigure_gain(float gain){
    string ret_msg;
    pthread_mutex_lock(&GLOB_FLAG_LOCK);
    GLOB_CONFIG_PARAMS.gain = gain;
    GLOB_RECONFIGURE = 1;
    pthread_mutex_unlock(&GLOB_FLAG_LOCK);
	ret_msg = "Camera Reconfiguring Gain";
    return ret_msg;
}

string reconfigure_exptime(float exptime){
    string ret_msg;
    pthread_mutex_lock(&GLOB_FLAG_LOCK);
    GLOB_CONFIG_PARAMS.exptime = exptime;
    GLOB_RECONFIGURE = 1;
    pthread_mutex_unlock(&GLOB_FLAG_LOCK);
	ret_msg = "Camera Reconfiguring Exposure Time";
    return ret_msg;
}

string reconfigure_width(int width){
    string ret_msg;
    pthread_mutex_lock(&GLOB_FLAG_LOCK);
    GLOB_CONFIG_PARAMS.width = width;
    GLOB_RECONFIGURE = 1;
    pthread_mutex_unlock(&GLOB_FLAG_LOCK);
	ret_msg = "Camera Reconfiguring Width";
    return ret_msg;
}

string reconfigure_height(int height){
    string ret_msg;
    pthread_mutex_lock(&GLOB_FLAG_LOCK);
    GLOB_CONFIG_PARAMS.height = height;
    GLOB_RECONFIGURE = 1;
    pthread_mutex_unlock(&GLOB_FLAG_LOCK);
	ret_msg = "Camera Reconfiguring Height";
    return ret_msg;
}

string reconfigure_offsetX(int offsetX){
    string ret_msg;
    pthread_mutex_lock(&GLOB_FLAG_LOCK);
    GLOB_CONFIG_PARAMS.offsetX = offsetX;
    GLOB_RECONFIGURE = 1;
    pthread_mutex_unlock(&GLOB_FLAG_LOCK);
	ret_msg = "Camera Reconfiguring X offset";
    return ret_msg;
}

string reconfigure_offsetY(int offsetY){
    string ret_msg;
    pthread_mutex_lock(&GLOB_FLAG_LOCK);
    GLOB_CONFIG_PARAMS.offsetY = offsetY;
    GLOB_RECONFIGURE = 1;
    pthread_mutex_unlock(&GLOB_FLAG_LOCK);
	ret_msg = "Camera Reconfiguring Y offset";
    return ret_msg;
}

string reconfigure_blacklevel(float blacklevel){
    string ret_msg;
    pthread_mutex_lock(&GLOB_FLAG_LOCK);
    GLOB_CONFIG_PARAMS.blacklevel = blacklevel;
    GLOB_RECONFIGURE = 1;
    pthread_mutex_unlock(&GLOB_FLAG_LOCK);
	ret_msg = "Camera Reconfiguring Black Level";
    return ret_msg;
}

string reconfigure_buffersize(float buffersize){
    string ret_msg;
    pthread_mutex_lock(&GLOB_FLAG_LOCK);
    GLOB_CONFIG_PARAMS.buffersize = buffersize;
    GLOB_RECONFIGURE = 1;
    pthread_mutex_unlock(&GLOB_FLAG_LOCK);
	ret_msg = "Camera Reconfiguring Buffer Size";
    return ret_msg;
}

string reconfigure_savedir(float savedir){
    string ret_msg;
    pthread_mutex_lock(&GLOB_FLAG_LOCK);
    GLOB_CONFIG_PARAMS.savedir = savedir;
    GLOB_RECONFIGURE = 1;
    pthread_mutex_unlock(&GLOB_FLAG_LOCK);
	ret_msg = "Camera Reconfiguring Save Directory";
    return ret_msg;
}

string connectcam(){
	string ret_msg;
	if(GLOB_CAM_STATUS == 0){
		pthread_create(&GLOB_CAMTHREAD, NULL, runCam, NULL);

		ret_msg = "Connecting Camera";
	}else{
		ret_msg = "Camera Already Connecting/Connected!";
	}

	return ret_msg;
}

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

//struct image_return{}

string getlatestimage(){
	string ret_msg;
	if(GLOB_CAM_STATUS == 2){
		if(GLOB_RUNNING == 1){
			if(GLOB_RECONFIGURE == 0 and GLOB_STOPPING == 0){
				pthread_mutex_lock(&GLOB_LATEST_IMG_INDEX_LOCK);
				int img_index = GLOB_LATEST_IMG_INDEX;
				pthread_mutex_unlock(&GLOB_LATEST_IMG_INDEX_LOCK);

				unsigned short *ret_image_array;
				ret_image_array = (unsigned short*)malloc(sizeof(unsigned short)*GLOB_IMSIZE);



				pthread_mutex_lock(&GLOB_IMG_MUTEX_ARRAY[img_index]);
				memcpy(ret_image_array,GLOB_IMG_ARRAY+GLOB_IMSIZE*img_index,GLOB_IMSIZE*2);
				pthread_mutex_unlock(&GLOB_IMG_MUTEX_ARRAY[img_index]);

                int height = GLOB_IMSIZE/GLOB_WIDTH;


				cv::Mat mat (height,GLOB_WIDTH,CV_16U,ret_image_array);

                //cv::Mat compressed_mat = mat / 256;
                
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


                json j;
                j["Image"]["rows"] = mat.rows;
                j["Image"]["cols"] = mat.cols;
                j["Image"]["channels"] = mat.channels();
                j["Image"]["data"] = array;

                std::string s = j.dump();


				ret_msg = s;
				
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


COMMANDER_REGISTER(m)
{

    // You can register a class or a struct using the instance method.
    // Using `instance<Gyro>` will instantiate a new instance of Gyro
    // only when a gyro command is called.
    m.instance<QHYCameraServer>("QHYCam")
        // To insterface a class method, you can use the `def` method.
        .def("status", &QHYCameraServer::status, "Camera Status")
        .def("connect", &QHYCameraServer::connectcam, "Connect the camera")
        .def("disconnect", &QHYCameraServer::disconnectcam, "Disconnect the camera")
        .def("start", &QHYCameraServer::startcam, "Start exposures")
        .def("stop", &QHYCameraServer::stopcam, "Stop exposures")
        .def("getlatestfilename", &QHYCameraServer::getlatestfilename, "Get the latest image filename")
        .def("getlatestimage", &QHYCameraServer::getlatestimage, "Get the latest image data")
        .def("reconfigure_all", &QHYCameraServer::reconfigure_all, "Reconfigure all parameters")
        .def("reconfigure_gain", &QHYCameraServer::reconfigure_gain, "Reconfigure the gain")
        .def("reconfigure_exptime", &QHYCameraServer::reconfigure_exptime, "Reconfigure the exposure time")
        .def("reconfigure_width", &QHYCameraServer::reconfigure_width, "Reconfigure the width")
        .def("reconfigure_height", &QHYCameraServer::reconfigure_height, "Reconfigure the height")
        .def("reconfigure_offsetX", &QHYCameraServer::reconfigure_offsetX, "Reconfigure the X offset")
        .def("reconfigure_offsetY", &QHYCameraServer::reconfigure_offsetY, "Reconfigure the Y offset")
        .def("reconfigure_blacklevel", &QHYCameraServer::reconfigure_blacklevel, "Reconfigure the black level")
        .def("reconfigure_buffersize", &QHYCameraServer::reconfigure_buffersize, "Reconfigure the buffer size")
        .def("reconfigure_savedir", &QHYCameraServer::reconfigure_savedir, "Reconfigure the save directory")
        .def("getparams", &QHYCameraServer::getparams, "Get all parameters");

}
