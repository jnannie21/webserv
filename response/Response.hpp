//
// Created by jnannie on 2/12/21.
//

#ifndef RESPONSE_HPP
#define RESPONSE_HPP

class Response;

#include "../request_handlers/Request.hpp"
#include <sys/socket.h>
#include <string>
#include <set>
//#include "../Server.hpp"

#include <list>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

#include <sys/types.h>
#include <sys/wait.h>

class Response {
public:
	Response(Request* request, int socket);

	~Response(void);

	void sendResponse();

	void generateGetResponse();
	void generateHeadResponse();
	void generatePutResponse();
	void generatePostResponse();
	void generateResponse();
	void generateResponseByStatusCode();
	void generateStatusLine();
	void generateHeaders();
	void generateContent();
	void readFileToContent(std::string & filename);
//	bool isStatusCodeOk();
	void generateAutoindex(std::string & filename);
	void setContentTypeByFileExt(std::string & ext);
	bool isMethodAllowed();
//	void setStatusCode(int status_code);

private:
	std::string getDateHeader();
	std::string getLastModifiedHeader(time_t tv_sec);
	std::string getLocationHeader();
	std::string getAllowHeader();
	std::string getWwwAuthenticateHeader();

	std::string _getUserFromCredentials();

private:
	struct tm _getCalendarTime(time_t tv_sec);
	struct tm _gmtime(time_t tv_sec);
	std::string _getExt(std::string filename);
	bool _isCgiExt(std::string & ext);
	void _runCgi(std::string & filename);
	void _setEnv(char* env[], std::string & filename, std::map<std::string, std::string> & cgiVariables);
	void _parseStatusLineFromCgiResponse();
	void _parseHeadersFromCgiResponse();
	std::string _inet_ntoa(struct in_addr sin_addr);



private:
	static std::map<int,std::string> initStatusCodes();
	static std::set<std::string> initResponseHeaders();

public:
	static std::set<std::string> implemented_headers;
	static std::map<int, std::string> status_codes;

private:
	Request* _request;
	int _socket;
	std::string _http_version;
//	int _status_code;
//	std::string _reason_phrase;
	std::map<std::string, std::string> _headers;
	std::string _raw_response;
	std::string _content;

	std::string _file_ext;
	std::string _cgi_response;
	std::string _cgi_status_line;
	std::map<std::string, std::string> _cgi_headers;

//	Server* _server;

//	std::string _root; // TODO:temporary

private: // headers
	std::string _content_type;
	std::string _allow;
	std::string _last_modified;
	std::string _location;
	std::string _www_authenticate;

    bool setIndexFileContentToResponseContent(void); // Airat


};


#endif //RESPONSE_HPP
