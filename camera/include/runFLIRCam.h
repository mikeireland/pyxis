#ifndef _RUNFLIRCAM_
#define _RUNFLIRCAM_

#include "FLIRCamera.h"
#include "globals.h"

int CallbackFunc (unsigned short* data);

int reconfigure(configuration c, FLIRCamera& Fcam);

void *runCam(void*);



#endif // _RUNFLIRCAM_
