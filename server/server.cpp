/*
 * This is an attempt for a minimalist server class.
 * To add a new command called xxx, just add a Server::Cmd_xxx method
 * both here and in the header file. The makefile takes care of the
 * help and case statement converting strings to a method in an
 * old-fashioned and arguably messy way.
 *
 * Mike Ireland, 23 Aug 2020
 *
 */
#include "server.h"
#include <string.h>
#include <stdarg.h>
#include <sys/select.h>
// It is certainly more conventional to use a C++ version of ZMQ, but this works...
#include <zmq.h>

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

Server::Server(int port){
    // Initialise key variables.
    this->port = port;

    //Initialise the arrays for commands
    for (int i=0;i<MAX_ARGS;i++){
		this->funct_argv[i] = (char *)this->funct_strs[i];
	}

}

Server::~Server(){
    // Add destructors here
}

// Default Commands.
int Server::Cmd_exit(int argc, char **argv){
    //Exit cleanly from the server.
    //----------------------------------------
	return FATAL;
}

int Server::Cmd_help(int argc, char **argv){
    //List the commands if no argument.
    //If called with an argument, the argument is a command
    //for which help is too be given.
    //----------------------------------------

    // This is a hack to enable the makefile to easily create all help
    // strings.
    // Insert output of "sed '{:q;N;s/\n/\\n/g;t q}' cmds"
    #include "helpstring.include"
    if (argc==1) {
        this->Message(kHelpString);
        return NOERROR;
    } else {
        this->Message("Help for individual commands not implemented yet. Awaiting TOML library choice.");
        return WARNING;
    }
}

// Add additional commands here with the same Cmd_ syntax as above.


void Server::RunBackgroundTasks(){
    //Run background tasks relevant to this server.
}

// Return command pointers based on strings.
int Server::RunNextCommand(){
    int argc = this->funct_argc;
    if (argc==0){
        this->Message("Error: No function to run");
        return ERROR;
    }
    char **argv = this->funct_argv;
    #include "cmd_elseif.include"
    } else {
        this->Message("Unknown command: %s (strlen %d) \n", argv[0], strlen(argv[0]));
        return WARNING;
    }
}

// Acknowledge a successful command by returning a newline or prompt
void Server::NewLine(){
	if (this->client_socket == -1){
		printf("> ");
		fflush(stdout);
	} else {
        this->Message("\n");
		this->client_socket =-1;
	}
}

// Send a raw (e.g. binary) message back to the client.
void Server::SendRawMessage(char *message,int len)
{
    if (this->client_socket != -1){
        zmq_send(this->responder,message,len,0);
    }
}

// Send a string message back to the client.
int Server::Message(const char *fmt, ...)
{
	char err_string[4097];
	int sprintf_len;
	va_list args;
	va_start(args, fmt);
	sprintf_len = vsprintf(err_string, fmt, args);
	if (this->client_socket==-1){
		fprintf(stderr, "%s\n", err_string);
	} else {
        zmq_send(this->responder,err_string,strlen(err_string),0);
        /* Error checking??? */
	}
	return NOERROR;
}

//Get the next command and parse it.
int Server::GetNextCommand()
{
    char command[COMMAND_BUFFERSIZE];
	char *astr;

	struct timeval tv = { 0L, 0L };
	fd_set fds;
	int nr;

	/* Start off with no command */
	command[0]=0;
    this->client_socket=-1;

	/* Set up our file descriptor set. Stdin is file descriptor number 0 */
	FD_ZERO(&fds);
    FD_SET(0, &fds);
	if (select(1, &fds, NULL, NULL, &tv) > 0){
		fgets(command, COMMAND_BUFFERSIZE, stdin);
        nr = strlen(command);
	} else {
        /* Receive from ZMQ */
        nr = zmq_recv (this->responder, command, COMMAND_BUFFERSIZE-1, ZMQ_DONTWAIT);
        if (nr>0)
            this->client_socket=0;
    }
    /* Null terminate and tidy up */
    command[nr]=0;
    if (command[nr-1] == '\n' || command[nr-1] == '\r') command[nr-1]=0;
	if (command[nr-2] == '\r') command[nr-2]=0;

	// Now parse the command
	this->funct_argc=0;
	this->funct_strs[0][0]=0;
	astr=strtok(command, " \n\t");
	while (astr != NULL){
	    strcpy((char *)this->funct_strs[funct_argc], astr);
		this->funct_argc++;
		astr=strtok(NULL," \n\t");
	}
	return NOERROR;
}

// Opens a message socket that can be used for accepting and sending
// outside messages.
int Server::OpenServerSocket(void)
{
    char tcp_string[40];
	/* Reset current socket pointer. */
    this->client_socket=-1;

    /* Straight out of ZMQ tutorial*/
    context = zmq_ctx_new ();
    this->responder = zmq_socket (context, ZMQ_REP);
    sprintf(tcp_string, "tcp://*:%d", this->port);
    rc = zmq_bind (this->responder, tcp_string);
    if (rc != 0){
        this->Message("ZMQ socket failure.");
        return FATAL;
    }
	return NOERROR;
}

// Close the server socket if it's open. Remember this could cause
// trouble if you do this while the channel is server. If the message
// socket is not open this function will do nothing.
void Server::CloseServerSocket(void)
{
	/* Do nothing if it isn't open */
	if (this->responder == NULL) return;

	zmq_close(this->responder);

}

// Is there a command available?
bool Server::CommandAvailable(void)
{
    if (this->funct_argc>0) return true;
    else return false;
}
