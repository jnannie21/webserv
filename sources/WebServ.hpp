//
// Created by airat on 06.02.2021.
//

#ifndef WEBSERV_WEBSERV_HPP
#define WEBSERV_WEBSERV_HPP

#include <vector>
#include "Server.hpp"
#include "config_parsing/Config.hpp"

#define  RETRY_AFTER_SECOND_DELAY "1"  // for header Retry-After

#define DEFAULT_RESPONSE_CHARSET "utf-8"
#define DEFAULT_RESPONSE_LANGUAGE "en-US"

#define CHECK_ACCEPT_CHARSET_HEADER 1
#define CHECK_ACCEPT_LANGUAGE_HEADER 1

class Request;
class Server;

class WebServ {
private:
	static std::vector<Server*> _servers;

    static std::string _webserv_root_path;
    static std::map<std::string, std::list<int> > _already_listening_host_plus_port;
    static std::list<std::string> _lang_code_list;

public:
    class NotOKStatusCodeException: public std::exception { };

    class FatalErrorException: public std::exception { };

	static void start();
	static void stop();

    static std::list<ServerContext*> servers_list;

    static int getServersCount(void);
    static Server* getServerByPosition(int i);
    static std::list<ServerContext*> getAllAsteriskHostPortComboList(const int port);
    static const std::string& getWebServRootPath(void);
    static std::list<std::string>& getLanguageCodesList(void);
    static std::list<ServerContext*> getAllExactHostPortComboList(const std::string& host, const int port);

    static void setWebServRootPath(const std::string& path);
    static void setInAlreadyListeningHostPlusPort(const std::string& host_str, int port);

    static int getMaxFD();
	static void serveConnections();

    static ServerContext*   findServerForHandlingRequest(const std::string& host,
                                                       const int port,
                                                       const std::string& server_name);
    static void             routeRequest(const std::string& host, const int port,
                                                         Request* _client_request,
                                                         const std::string& request_target);
    static bool             isAlreadyListeningHostPlusPort(const std::string& host_str, int port);

	static LocationContext* searchForBestMatchLocation(ServerContext* handling_server,
														 Request* current_request,
														 const std::string& request_target);
	static bool isPartOfLocationPath(const std::list<std::string>& request_target, const std::list<std::string>& location_path);
	static int isServerNameMatch(const std::string& server_name, const std::list<std::string>& serv_name_list);

	static void initLanguageCodesList(void);

};

#endif //WEBSERV_WEBSERV_HPP
