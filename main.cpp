#include "Server.hpp"
#include "WebServ.hpp"

void serveConnections(WebServ& webServ) {
    while(TRUE) {

        Server &server = webServ.getFrontServer();

        FD_ZERO(server.getReadSetPtr());
        FD_ZERO(server.getWriteSetPtr());
        FD_SET(server.getListener(), server.getReadSetPtr());

        server.setToReadFDSet();
        server.setToWriteFDSet();

        // Ждём события в одном из сокетов
        server.updateMaxFD();
        if (select(server.getMaxFD() + 1,
                   server.getReadSetPtr(),
                   server.getWriteSetPtr(),
                   NULL,
                   NULL) < 0) {
            utils::exitWithLog();
        }

        // Определяем тип события и выполняем соответствующие действия
        if (FD_ISSET(server.getListener(), server.getReadSetPtr())) {
            // Поступил новый запрос на соединение, используем accept
            server.acceptConnection();
        }

        server.processConnections();

        webServ.addServer(server); //push server to end
        webServ.popFrontServer(); //Remove next element
    }
}


int main(int argc, char *argv[])
{
    std::string path_to_config;
    if  (argc == 2) {
        path_to_config = argv[1];
    } else if (argc == 1) {
        path_to_config = CONFIG_FILE_DEFAULT_PATH;
    } else {
        std::cout << "Please provide only 1 arg -> path to webserv config" << std::endl\
            << "or dont provide nothing and will be used CONFIG_FILE_DEFAULT_PATH" << std::endl;
        exit(EXIT_FAILURE);
    }
    WebServ webServ = WebServ(path_to_config);
    serveConnections(webServ);
    return 0;
}