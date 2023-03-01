/*
 * This is an attempt for a minimalist server class.
 * Mike Ireland, 23 Aug 2020
 *
 */

#include <string>
#include <toml.hpp>

#ifndef SERVER_H // include guard
#define SERVER_H

// The command buffer should be up to MAX_ARG * (MAX_COMMAND_SIZE + 1) in size.
#define MAX_ARGS 10
#define MAX_COMMAND_SIZE 80
#define COMMAND_BUFFERSIZE (MAX_ARGS * (MAX_COMMAND_SIZE + 1))

/*
 * Prototypes
 */

class Server {
  public:
    Server(int);
    ~Server();
    int Cmd_exit(int argc, char **argv);
    int Cmd_help(int argc, char **argv);
    int Cmd_get_config(int argc, char **argv);
    int Cmd_get_led(int argc, char **argv);
    int Cmd_get_orientation(int argc, char **argv);
    void RunBackgroundTasks();
    int GetNextCommand();
    int RunNextCommand(void);
    void NewLine();
    void SendRawMessage(char *message, int len);
    int Message(const char *the_string, ...);
    int OpenServerSocket(void);
    void CloseServerSocket(void);
    bool CommandAvailable();

  private:
    int funct_argc;
    char funct_strs[MAX_ARGS][MAX_COMMAND_SIZE];
    char *funct_argv[MAX_ARGS];
    int port;
    int client_socket;
    void *context;
    void *responder;
    int rc;
};

namespace serverfunc {
int load_config(std::string path);

int save_config(std::string path);

toml::table &get_config();
std::string get_led_location_string();
std::string get_self_orientation();
} // namespace serverfunc

#endif /* SERVER_H */