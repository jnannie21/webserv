//
// Created by jnannie on 2/22/21.
//

#include "Listener.hpp"
#include "WebServ.hpp"
#include "Response.hpp"
#include "Base64.hpp"
#include <sys/time.h>

#define TIME_OUT 300000
#define BUFFER_SIZE 1024

Listener::~Listener(void) {
    std::list<int>::iterator read = _clients_read.begin();
    std::list<int>::iterator write = _clients_write.begin();
    while (read != _clients_read.end()) {
        close(*read);
        ++read;
    }
    while (write != _clients_write.end()) {
        close(*write);
        ++write;
    }
}

Listener::Listener(const std::string &host, in_addr_t host_addr, int port)
		: _host(host), _host_addr(host_addr), _port(port)
{
	_listener = socket(AF_INET, SOCK_STREAM, 0);
	if(_listener < 0)
		utils::exitWithLog();

	if (fcntl(_listener, F_SETFL, O_NONBLOCK) < 0) //превращает сокет в неблокирующий
		utils::exitWithLog();

	int optval = 1;
	if (setsockopt(_listener, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1)
		utils::exitWithLog();

	_addr.sin_family = AF_INET;
	_addr.sin_port = htons(port); // TODO: is htons it allowed?
	_addr.sin_addr.s_addr = _host_addr;

	if (bind(_listener, (struct sockaddr *)&_addr, sizeof(_addr)) < 0)
		utils::exitWithLog();

	if (listen(_listener, SOMAXCONN) < 0)
		utils::exitWithLog();
}

int                     Listener::getListener(void) const { return _listener; }
const std::list<int>&   Listener::getReadClients(void) const { return _clients_read; }
const std::list<int>&   Listener::getWriteClients(void) const { return _clients_write; }
const int &             Listener::getMaxFD() const { return _max_fd; }


void Listener::updateMaxFD(void) {
	int max_tmp = _listener;
	if (!_clients_read.empty()) {
		max_tmp = std::max(max_tmp, *std::max_element(_clients_read.begin(), _clients_read.end()));
	}
	if (!_clients_write.empty()) {
		max_tmp = std::max(max_tmp, *std::max_element(_clients_write.begin(), _clients_write.end()));
	}
	_max_fd = max_tmp;
}

long Listener::_get_time(void)
{
	struct timeval current;

	gettimeofday(&current, NULL);
	return ((long)(((current.tv_sec) * 1000) + ((current.tv_usec) / 1000)));
}

void Listener::acceptConnection(void) {
	socklen_t len = sizeof(_remote_addr);
	int sock = accept(_listener, (struct sockaddr *)&_remote_addr, &len);
	if(sock < 0)
		utils::exitWithLog();

	if (fcntl(sock, F_SETFL, O_NONBLOCK) < 0)
		utils::exitWithLog();

	int optval = 1;
	if (setsockopt(_listener, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1)
		utils::exitWithLog();
	_clients_read.push_back(sock);
	_client_requests.erase(sock);
	_client_response.erase(sock);
	_client_requests[sock] = Request(_remote_addr, _port);
	_last_time[sock] = _get_time();
	_client_response[sock] = Response(&_client_requests[sock], sock);
}

void Listener::processConnections(fd_set* globalReadSetPtr, fd_set* globalWriteSetPtr) {
    handleRequests(globalReadSetPtr);
	handleResponses(globalWriteSetPtr); // Routing is made inside handling Responses
}

void Listener::closeSocket(std::list<int>::iterator & it) {
	close(*it);
	_client_requests.erase(*it);
	_client_response.erase(*it);
}

// return TRUE if header was read else FALSE
bool Listener::checkIfHeaderHasRead(Request* request) {
    const std::string& raw_request = request->getRawRequest();

    std::size_t headers_end = raw_request.find("\r\n\r\n");
    if (std::string::npos != headers_end) {
        if (request->_bytes_read > (headers_end + 4)) {
			request->_body_bytes_read += (request->_bytes_read - (headers_end + 4));
        }
        return true;
    }
    return false;
}

// return TRUE if length of body is reached else FALSE
bool Listener::continueReadBody(Request* request) {
    const std::map<std::string, std::string>& headers = request->_headers;
//    const std::string& body = request_obj->getRawBody(); // TODO: body is wrong, headers are removed during parsing so the whole _raw_request in the body
    std::string& body = request->getRawRequest();

    // TODO: NEED CHECKS !!!! SEEMS LIKE SHOULDNT WORK
    std::map<std::string, std::string>::const_iterator it = headers.find("transfer-encoding");
    if ((it != headers.end()) && ((*it).second.find("chunked") != std::string::npos)) {
		request->_is_chunked = true;

		size_t start_line_length = body.find("\r\n");
		if (start_line_length == std::string::npos)
			return false;

		std::string start_line;
		std::string chunk_length_field;
		size_t chunk_length;
		size_t sum_content_length = 0;

        while (!body.empty()) { // jnannie: remade like Request::parseChunkedContent()

			if (start_line_length == std::string::npos)
				return false;
			if (start_line_length > MAX_HEADER_LINE_LENGTH) {
				request->setStatusCode(400);
				return true;
			}
			start_line = body.substr(0, start_line_length);

			chunk_length_field = start_line.substr(0, body.find(';')); // to ';' or full line

			libft::string_to_lower(chunk_length_field);
			if (chunk_length_field.find_first_not_of("0123456789abcdef") != std::string::npos) {
				request->setStatusCode(400);
				return true;
			}
			chunk_length = libft::strtoul_base(chunk_length_field, 16);
			if (chunk_length == ULONG_MAX || chunk_length > ULONG_MAX - sum_content_length) {
				request->setStatusCode(413);// 413 (Request Entity Too Large)
				return true;
			}
			sum_content_length += chunk_length;
			if (body.size() < start_line_length + 2 + chunk_length + 2)
				return false;

			body.erase(0, start_line_length + 2); // remove start line
			request->_content.append(request->_raw_request.substr(0, chunk_length));
			body.erase(0, chunk_length + 2); // remove rest of chunk
			if (chunk_length == 0) // why? body is now empty so loop is over
			    return true;

			start_line_length = body.find("\r\n");
        }
        return false;
    }
    else if ((request->_headers.count("content-length"))) {
		std::size_t content_length;
		content_length = libft::strtoul_base(request->_headers["content-length"], 10);

        if (!request->checkClientMaxBodySize(content_length)) {
            return true;
        }

		if (request->_body_bytes_read == content_length) {
			request->_content.append(body, 0, content_length);
			body.clear();
			return true;
        }

        return false;
    }
    return true;
}


/*
 * authorization functions to move
 */
std::vector<std::string> Listener::parser_log_pass(std::string file, Request*  request) {
	std::vector<std::string> log_pass;

	char *str;
	int  read;
	int fd_file = open(file.c_str(), O_RDONLY);
	if (fd_file == -1)
        request->setStatusCode(500);

	while ((read = get_next_line(fd_file, &str)) == 1) {
		log_pass.push_back(str);
		delete str;
	}
    if (read == -1) {
        request->setStatusCode(500);
    }
	log_pass.push_back(str);
	delete str;
	close(fd_file);
	return log_pass;
}

bool    find_log_pass(std::vector<std::string> log_pass, std::string const& credentials) {
	std::vector<std::string>::iterator it = log_pass.begin();

	while (it != log_pass.end()) {
		if (Base64::base64_encode(*it) == credentials)
			return true;
		++it;
	}
	return false;
}


// returns TRUE if we are ready and need body and its length check, etc.
void Listener::processHeaders(int client_socket) {
    Request * request = &_client_requests[client_socket];

    request->parseRequestLine();
    request->parseHeaders();
    request->parsURL();

    WebServ::routeRequest(_host, _port, request, request->_request_target);

    std::size_t lang_start_pos;
    if ((lang_start_pos = request->_request_target.find("_lang_")) != std::string::npos) {
        lang_start_pos += 6; // pass "_lang_"
        request->_lang_file_pos = lang_start_pos;
    }

    if (request->_handling_location) {
		if (request->_handling_location->getAuthEnable()) { // TODO: add to config file
			if (request->_headers.count("authorization")) {
				std::vector<std::string> log_pass = parser_log_pass(std::string("passwd"), request);
				std::string auth_scheme = request->_headers["authorization"].substr(0, 5);
				libft::string_to_lower(auth_scheme);
				std::string credentials = request->_headers["authorization"].substr(6);
				credentials = credentials.substr(credentials.find_first_not_of(" ")); // remove whitespaces
				if (auth_scheme != "basic" || !find_log_pass(log_pass, credentials)) {
					request->setStatusCode(401);
//					return false;
				}
			} else {
				request->setStatusCode(401);
//				return false;
			}
		}
    }
    request->handleExpectHeader();

//    if (!request->isStatusCodeOk()) {
//        return false;
//    }

    request->checkClientMaxBodySize();
//    if (!request->isStatusCodeOk()) {
//        return false;
//    }

    if (request->_method == "PUT") {
        // https://efim360.ru/rfc-7231-protokol-peredachi-giperteksta-http-1-1-semantika-i-kontent/#4-3-4-PUT
        if (request->_headers.count("content-range")) {
            request->setStatusCode(400);
//            return false;
        }

//        std::string target = request->_request_target.substr(1);
        request->_put_filename = request->getAbsolutePathForPutRequests() + request->_request_target;

//        bool status =
//        request->setFileExistenceStatus(status);
		request->_file_exists = request->checkIfFileExists();
        request->setNeedWritingBodyToFile(true);

        if (request->_file_exists)
        {
            if (!request->targetIsFile()) {
//                if (request->isStatusCodeOk()) {
                    request->setStatusCode(409);
//                }
//                return false;
            }
        }
//        request->checkFile(request->_put_filename);
    }
//    return true;
}

bool Listener::_readBody(Request * request, int socket) {
	bool body_was_read = continueReadBody(&_client_requests[socket]);
	bool writing_to_file_result = true;

	if (request->getNeedWritingBodyToFile() && body_was_read) {
//		writing_to_file_result = request->writeBodyReadBytesIntoFile();
		request->writeBodyReadBytesIntoFile();

//		if (!writing_to_file_result) {
//			request->setStatusCode(500);
//		}
	}

	if (body_was_read || !writing_to_file_result) {
		return true;
//		it = _clients_read.erase(it);
	} else {
		return false;
//		++it;
	}
}

void Listener::handleRequests(fd_set* globalReadSetPtr) {
	std::list<int>::iterator it = _clients_read.begin();
	char _buf[BUFFER_SIZE];

	while (it != _clients_read.end() ) {
        try {
            try {
                if (FD_ISSET(*it, globalReadSetPtr)) { // Поступили данные от клиента, читаем их
                    // Check client header info was read or not
                    Request *request = &_client_requests[*it];
                    request->setHostAndPort(_host, _port);
//                    bool header_was_read_client = request->isHeaderWasRead();

                    request->_bytes_read = recv(*it, _buf, BUFFER_SIZE, 0);

                    if (request->_bytes_read <= 0) { // if 0 then connection is closed, else if < 0 according to subject we must close connection
						closeSocket(it);
						it = _clients_read.erase(it);
						continue;
                    } else
						_last_time[*it] = _get_time();
//					_buf[request->_bytes_read] = '\0';

					request->getRawRequest().append(_buf, request->_bytes_read);

                    // Behavior based on was or not read header
                    if (!request->_header_was_read) {
                        request->_header_was_read = checkIfHeaderHasRead(request);
                        if (request->_header_was_read) {

//                            if (processHeaders(*it)) {
							processHeaders(*it);
							if (_readBody(request, *it)) {
								_clients_write.push_back(*it);
								it = _clients_read.erase(it);
							}
							else
								++it;
//                            } else {
//                                _clients_write.push_back(*it);
//                                it = _clients_read.erase(it);
//                            }
                        } else // jnannie: we can read and write only once according to checklist
                            ++it;
                    } else {
						request->_body_bytes_read += request->_bytes_read;
						if (_readBody(request, *it)) {
							_clients_write.push_back(*it);
							it = _clients_read.erase(it);
						}
						else
							++it;
                    }
                }
                else {
                    if ((_get_time() - _last_time[*it]) > TIME_OUT) {
                        std::cout << "socket " << *it << " closed due to timeout" << std::endl;
						closeSocket(it);
						it = _clients_read.erase(it);
                        continue;
                    }
                    ++it;
                }
            }
            catch (std::bad_alloc const &e)
            {
                _client_requests[*it]._close_connection = true;
                _client_requests[*it].setStatusCode(500);
            }
        }
        catch (WebServ::NotOKStatusCodeException &e)
        {
            _clients_write.push_back(*it);
            it = _clients_read.erase(it);
        }
	}
}

void Listener::handleResponses(fd_set* globalWriteSetPtr) {
	std::list<int>::iterator it = _clients_write.begin();

	int fd;
	while (it != _clients_write.end()) {
		fd = *it;
		if (FD_ISSET(fd, globalWriteSetPtr)) {
			Request* request = &_client_requests[fd];
			Response* response = &_client_response[fd];
                if (!response->getInProgress()) {
                    response->generateResponse();
                    response->setRemains();
                    request->_content.clear(); // just to free memory // but it doesnt free
                    response->getContent().clear();
                }

			response->sendResponse();

			if (request->_close_connection || (request->_headers.count("connection") && request->_headers["connection"] == "close")) {
				_client_requests.erase(fd);
				_client_response.erase(fd);
				close(fd);
				it = _clients_write.erase(it);
				std::cout << "connection closed, socket " << fd << std::endl;
			} else if (!response->getInProgress()) {
					_client_requests.erase(fd);
					_client_response.erase(fd);
					_client_requests[fd] = Request(_remote_addr, _port);
					_client_response[fd] = Response(&_client_requests[fd], fd);
					_clients_read.push_back(fd);
					it = _clients_write.erase(it);
				} else
					++it;
		} else {
			++it;
		}
	}
}
