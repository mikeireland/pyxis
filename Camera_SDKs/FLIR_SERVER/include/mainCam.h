#ifndef _MAINCAM_
#define _MAINCAM_

#include "FLIRCamera.h"
#include "helperFunc.h"

int CallbackFunc (unsigned short* data);

int reconfigure(configuration c, FLIRCamera Fcam);

void *runCam(void*);



#endif // _MAINCAM_
