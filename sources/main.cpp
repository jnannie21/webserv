#include <signal.h>
#include <iostream>

#include "Server.hpp"
#include "WebServ.hpp"

#include "cpp_libft/libft.hpp"
#include "Base64.hpp"

#define CONFIG_FILE_DEFAULT_PATH "./WEBSERV.CONF"


WebServ webserv;

void StopSignalHandler(int signal) {
	(void)signal;
	webserv.stop();

	exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[])
{
    std::string path_to_config;
    if (argc > 2) {
        std::cout << "too many arguments" << std::endl;
        exit(EXIT_FAILURE);
    }

    if  (argc == 2) {
        path_to_config = argv[1];
    } else  {
        path_to_config = CONFIG_FILE_DEFAULT_PATH;
    }

	signal(SIGINT, StopSignalHandler);

    try
    {
        Config _config = Config(path_to_config);
        WebServ::servers_list = _config.getServersList();

		WebServ::start();
		WebServ::stop();
    }
    catch (Config::BadConfigException & e)
    {}
    return 0;
}
