#ifndef _RUNQHYCAM_
#define _RUNQHYCAM_

#include "QHYCamera.h"
#include "globals.h"

int CallbackFunc (unsigned short* data);

int reconfigure(configuration c, QHYCamera& Qcam);

void *runCam(void*);



#endif // _RUNQHYCAM_
