//
// Created by airat on 06.02.2021.
//

#include "WebServ.hpp"
#include "sys/time.h"

#define PREFIX_DEFAULT_PATH "/default_folder/" // webserv root

std::vector<Server*> WebServ::_servers;
std::list<ServerContext*> WebServ::servers_list;
std::string WebServ::_webserv_root_path;
std::list<std::string> WebServ::_lang_code_list;
std::map<std::string, std::list<int> > WebServ::_already_listening_host_plus_port;

void WebServ::start() {
	initLanguageCodesList();

	char *absolute_path = getcwd(NULL, 0);
	if (!absolute_path) {
		utils::exitWithLog();
	}
	std::string abs_path = std::string(absolute_path);
	free(absolute_path);

	setWebServRootPath(std::string(abs_path) + PREFIX_DEFAULT_PATH);

	std::list<ServerContext*>::const_iterator it = servers_list.begin();
	std::list<ServerContext*>::const_iterator ite = servers_list.end();

	while (it != ite) {

		Server *temp = new Server((*it));
		_servers.push_back(temp);
		++it;
	}

	serveConnections();

	std::cout << "Server(s) are started" << std::endl;
}

void WebServ::stop() {
	std::vector<Server*>::iterator _servers_it = _servers.begin();

	while (_servers_it != _servers.end()) {
		delete *_servers_it;
		++_servers_it;
	}
	std::cout << "server stopped" << std::endl;
}


bool WebServ::isAlreadyListeningHostPlusPort(const std::string& host_str, int port) {
    std::string all_hosts = "*";
    if (_already_listening_host_plus_port.count(all_hosts))
    {
        if ((std::find(_already_listening_host_plus_port[all_hosts].begin(),
                       _already_listening_host_plus_port[all_hosts].end(),
                       port) != _already_listening_host_plus_port[all_hosts].end()))
        {
            return true;
        }
    }

    if (!_already_listening_host_plus_port.count(host_str))
    {
        return false;
    }
    else
    {
        if ((std::find(_already_listening_host_plus_port[host_str].begin(),
					   _already_listening_host_plus_port[host_str].end(),
                       port) == _already_listening_host_plus_port[host_str].end()))
        {
            return false;
        }
        return true;
    }
}

int WebServ::getMaxFD(void) {
    int max_fd = 0;
    for(std::vector<Server*>::iterator it = _servers.begin(); it != _servers.end(); it++) {
        std::list<Listener*> listeners = (*it)->getListeners();
        std::list<Listener*>::iterator it_l = listeners.begin();
        while (it_l != listeners.end()) {
            int tmp = (*it_l)->getMaxFD();
            if (tmp > max_fd)
				max_fd = tmp;
            ++it_l;
        }
    }
    return max_fd;
}

void WebServ::serveConnections() {
	fd_set temp_read_set;
	fd_set temp_write_set;
	struct   timeval tv;
	tv.tv_sec = 30;
	tv.tv_usec = 0;
	while(true) {

		FD_ZERO(&temp_read_set);
		FD_ZERO(&temp_write_set);

		for (int i = 0; i < getServersCount(); i++) {
			Server *server = getServerByPosition(i);

			std::list<Listener*> listeners = server->getListeners();
            std::list<Listener*>::iterator it_l = listeners.begin();
            while (it_l != listeners.end()) {
                FD_SET((*it_l)->getListener(), &temp_read_set);
				for(std::list<int>::const_iterator it = (*it_l)->getReadClients().begin(); it != (*it_l)->getReadClients().end(); it++)
					FD_SET(*it, &temp_read_set);
				for(std::list<int>::const_iterator it = (*it_l)->getWriteClients().begin(); it != (*it_l)->getWriteClients().end(); it++)
					FD_SET(*it, &temp_write_set);
                (*it_l)->updateMaxFD();
                ++it_l;
            }
		}

		// Ждём события в одном из сокетов
		if (select(getMaxFD() + 1,
				   &temp_read_set,
				   &temp_write_set,
				   NULL,
				   &tv) < 0) {
			utils::exitWithLog();
		}

		// Определяем тип события и выполняем соответствующие действия
		for (int i = 0; i < getServersCount(); i++) {
			Server *server = getServerByPosition(i);

            std::list<Listener*> listeners = server->getListeners();
            std::list<Listener*>::iterator it_l = listeners.begin();
            while (it_l != listeners.end()) {
                if (FD_ISSET((*it_l)->getListener(), &temp_read_set)) {
                    // Поступил новый запрос на соединение, используем accept
                    (*it_l)->acceptConnection();
                }
                (*it_l)->processConnections(&temp_read_set, &temp_write_set);
                ++it_l;
            }
		}
	}
}

int WebServ::getServersCount(void) { return _servers.size(); }
Server* WebServ::getServerByPosition(int i) { return _servers[i]; }
const std::string& WebServ::getWebServRootPath(void) { return _webserv_root_path;}
std::list<std::string>& WebServ::getLanguageCodesList(void) { return _lang_code_list;}

void WebServ::setWebServRootPath(const std::string& path) { _webserv_root_path = path;}
void WebServ::setInAlreadyListeningHostPlusPort(const std::string& host_str, int port) {_already_listening_host_plus_port[host_str].push_back(port);}

void WebServ::initLanguageCodesList(void)
{
	_lang_code_list.push_back("en");
	_lang_code_list.push_back("uk");
}
