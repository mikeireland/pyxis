#ifndef _QHYCamServer_
#define _QHYCamServer_

#include <string>
#include "globals.h"
#include <commander/commander.h>
#include <functional>

using namespace std;
using json = nlohmann::json;

int SimpleCallback (unsigned short* data);

// QHY Camera Server
struct QHYCameraServer {

    QHYCameraServer();
    
    QHYCameraServer(std::function<int(unsigned short*)> AnalysisFunc);

    ~QHYCameraServer();


//Get status of camera
string status();

// Get parameters from the global variable
configuration getparams();

// Reconfigure parameters from an input configuration struct
string reconfigure_all(configuration c);


/* ##################################################### */

/* Set each parameter separately: */
string reconfigure_gain(float gain);

string reconfigure_exptime(float exptime);

string reconfigure_width(int width);

string reconfigure_height(int height);

string reconfigure_offsetX(int offsetX);

string reconfigure_offsetY(int offsetY);

string reconfigure_blacklevel(int blacklevel);

string reconfigure_buffersize(float buffersize);

string reconfigure_savedir(float savedir);

/* ##################################################### */

// Connect to a camera, by starting up the runCam pThread
string connectcam();

// Disconnect to a camera, by signalling and joining the runCam pThread
string disconnectcam();

// Start acquisition of the camera. Takes in the number of frames to save
// per FITS file (or 0 for continuous, no saving)
string startcam(int num_frames);

// Stop acquisition of the camera
string stopcam();

// Get the filename of the latest saved FITS image
string getlatestfilename();

// Get the latest image data from the camera thread
string getlatestimage(int compression, int binning);

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

#endif
