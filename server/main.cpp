/*
 * This server borrows heavily from CHARA and SUSI interferometer code, which was
 * written in C. Therefore, even though this is a prototype, it doesn't conform to
 * our standards yet! Most importantly, zmqcoms is still in C and needs a
 significant re-write. Mike Ireland, 23 Aug 2020
 *
 */
#include "server.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>

/* 
 * Error messages
 */

#ifndef _ERRDEFS_
#define _ERRDEFS_

#define MESSAGE_LATER 2 /* Signal from a cmd_ function that a message will be returned later, via a background task. */
#define MESSAGE 1 /* Use error system for putting up a message */
#define NOERROR	0 /* No error has occured */
#define WARNING (-1) /* A warning, nothing too dangerous has happened */
#define ERROR	(-2) /* Some kind of major error.Prog does not have to reboot */
#define FATAL	(-3) /* A fatal error has occured. The programme must exit(1)*/ 
#endif

/* The main program */
int main(int argc, char **argv)
{
    // Import and initialise the server. This is ideally the only line to be changed
    // when upgrading to a real server!!! All initialisations should occur within the 
    // Server constructor. 
    Server main_server(3001);
	int retval=NOERROR;

	/* Get ready for accepting connections... */
	if (main_server.OpenServerSocket() == FATAL){
	    fprintf(stderr, "Fatal error: Could not open server socket!\n");
	    return -1;
	} 
	main_server.NewLine(); 

	/* Our infinite loop. */
	while (retval != FATAL) 
	{
        /* Get the next command*/
		main_server.GetNextCommand();
		if (main_server.CommandAvailable()){
		    retval = main_server.RunNextCommand();
            main_server.NewLine(); 
		} else usleep(1000);
		/* Now do the background tasks. NB, these could be an array of tasks only done
		sometimes...  details up to the server implementation. */
		main_server.RunBackgroundTasks();
	}
	// Cleanup functions should automatically be invoked here when the main_server goes
	// out of scope. 
	return 0;
}
