//
// Created by jnannie on 2/22/21.
//

#ifndef LISTENER_HPP
#define LISTENER_HPP

#include <string>
#include <list>
#include <sys/socket.h>
#include "Request.hpp"
#include "Response.hpp"

class Request;
class Response;

class Listener {
private:
	int _listener;
	int _max_fd;

	struct sockaddr_in _addr;
	const std::string _host;
	const in_addr_t _host_addr;
	const int _port;
	struct sockaddr_in _remote_addr;
	std::list<int> _clients_read; // second param for ready or not
	std::list<int> _clients_write; // second param for ready or not
	std::map<int, Request> _client_requests;
	std::map<int, Response> _client_response;
	std::map<int, long> _last_time;

public:
	Listener(const std::string& host, in_addr_t host_addr, int port);
	~Listener(void);

	int getListener(void) const;
	const std::list<int>& getReadClients(void) const;
	const std::list<int>& getWriteClients(void) const;
	const int & getMaxFD() const;
	void updateMaxFD(void);
	void acceptConnection(void);
	void processConnections(fd_set* globalReadSetPtr, fd_set* globalWriteSetPtr);

private:
	void _handleRequests(fd_set* globalReadSetPtr);
	void _handleResponses(fd_set* globalWriteSetPtr);
    void _closeSocket(std::list<int>::iterator & it);
	long _get_time();
	bool _checkIfHeadersHasBeenRead(Request* request);
    bool _continueReadBody(Request* request);
    void _processHeaders(int client_socket);
	bool _readBody(Request * request, int socket);

	std::vector<std::string> parser_log_pass(std::string file, Request* request);
};


#endif //LISTENER_HPP
