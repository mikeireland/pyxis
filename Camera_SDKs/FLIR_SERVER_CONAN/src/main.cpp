
#include <commander/commander.h>
#include "globals.h"
#include <string>
#include <iostream>
namespace co = commander;
using namespace std;

/*int main(int argc, char **argv) {

	int port = 2543;
	msg_connectcam();
	string msg;
	
	int i = 0;
	while(i<5){
		msg = msg_getstatus();
		cout << msg << endl;
		msg = msg_startcam(5);
		cout << msg << endl;
		sleep(1);
		msg = msg_getlatestfilename();
		cout << msg << endl;
		msg = msg_getlatestimage();
		cout << msg << endl;
		i++;
	}
	i = 0;
	while(i<3){
		msg = msg_getstatus();
		cout << msg << endl;
		msg = msg_stopcam();
		cout << msg << endl;
		sleep(1);
		i++;
	}
	msg = msg_disconnectcam();
	cout << msg << endl;
	sleep(1);

} */

int main(int argc, char* argv[]) {
    
    GLOB_CONFIGFILE = (char*)"config/defaultConfig.toml";
    
    string IP = "192.168.1.4";
    string port = "4001";
    
    string TCPString = "tcp://" + IP + ":" + port;
    char TCPCharArr[TCPString.length() + 1];
    strcpy(TCPCharArr, TCPString.c_str());
    
    argc = 3;
    char* argv_new[3];
    argv_new[1] = (char*)"--socket";
    argv_new[2] = TCPCharArr;

    co::Server s(argc, argv_new);
    
    s.run();
}
