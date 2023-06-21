#include <iostream>
#include <thread>
#include <string>
#include <commander/commander.h>
#include <sys/mman.h>

namespace co = commander;
using namespace std;

int main(int argc, char* argv[]) {
    mlockall(MCL_FUTURE);
    string IP = "192.168.1.4";
    string port = "4200";
    
    string TCPString = "tcp://" + IP + ":" + port;
    char TCPCharArr[TCPString.length() + 1];
    strcpy(TCPCharArr, TCPString.c_str());
    /*
    argc = 3;
    char* argv_new[3];
    argv_new[1] = (char*)"--socket";
    argv_new[2] = TCPCharArr;
    */
    argc = 1;
    char* argv_new[1];

    co::Server s(argc, argv_new);
    
    s.run();
}
