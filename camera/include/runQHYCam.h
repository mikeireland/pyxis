#ifndef _RUNQHYCAM_
#define _RUNQHYCAM_

#include "QHYCamera.h"
#include "globals.h"

/* A function that can be used to perform real time data analysis on a frame (eg Fringe Tracking)
   INPUTS:
      data - raw image data of a single frame
   Returns 0 on regular exit, 1 if the stopping flag is set
*/
int CallbackFunc (unsigned short* data);

/* Function to reconfigure all parameters
    INPUTS:
       c - configuration (json) structure with all the values
       Qcam - QHYCamera class 
    Returns 0 on success, 1 on error
*/
int reconfigure(configuration c, QHYCamera& Qcam);

/* Main pThread function to run the camera with a separate thread */ 
void *runCam(void*);



#endif // _RUNQHYCAM_
