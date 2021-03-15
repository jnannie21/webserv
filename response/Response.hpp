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
#include <list>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

#include <sys/types.h>
#include <sys/wait.h>

class Response {
public:
	Response(Request * request, int socket);
	Response();
	~Response(void);

	void sendResponse();
	void generateGetResponse();
    void generateHeadResponseCore();
    void generateHeadResponse();
	void generatePutResponse();
	void generatePostResponse();
	void generateResponse();
	void generateResponseByStatusCode();
	void generateStatusLine();
	void generateHeaders();
	void generateContent();
	void readFileToContent(std::string & filename);
	void generateAutoindex(std::string & filename);
	void setContentTypeByFileExt(std::string & ext);
	bool isMethodAllowed();
	static std::set<std::string> implemented_headers;
	static std::map<int, std::string> status_codes;
	bool setIndexFileContentToResponseContent(void); // Airat
	void updateRequestForErrorPage(const std::string& error_page_link);
	void generateDefaultResponseByStatusCode();
	void generateResponseForErrorPage(void);
	const std::string searchForErrorPageLinkAndSetChangeError() const;
	void checkForAcceptPrefixHeaders(void);
	void setRemains();

private:
	std::string getDateHeader();
	std::string getLastModifiedHeader(time_t tv_sec);
	std::string getLocationHeader(bool is_file);
	std::string getAllowHeader();
	std::string getWwwAuthenticateHeader();
	std::string getRetryAfterHeader();
	std::string _getUserFromCredentials();

private:
	struct tm _getCalendarTime(time_t tv_sec);
	struct tm _gmtime(time_t tv_sec);
	std::string _getExt(std::string filename);
	bool _isCgiExt();
	void _runCgi(std::string & filename);
	void _setEnv(std::vector<char *> & env, std::string & filename, std::map<std::string, std::string> & cgiVariables);
	void _parseHeadersFromCgiResponse();
	std::string _inet_ntoa(struct in_addr sin_addr);
	static std::map<int,std::string> initStatusCodes();
	static std::set<std::string> initResponseHeaders();

private:
	Request * _request;
	int _socket;
	std::string _http_version;
	std::map<std::string, std::string> _headers;
	std::string _raw_response;
// headers
	std::string _content_type;
	std::string _allow;
	std::string _last_modified;
	std::string _location;
	std::string _content_location;
	std::string _www_authenticate;
	std::string _retry_after;

public:
	std::string _content;
	std::string _file_ext;
	std::string _cgi_response;
	std::string _cgi_status_line;
	std::map<std::string, std::string> _cgi_headers;
	bool in_progress;
	long remains;
	long sent_len;
	int error_code_for_generaion;

};

void _appendRequestTarget(std::string & filename, Request* _request);
void _appendRequestTarget(std::string & filename, Request* _request, const std::string& tmp_request_target);



#endif //RESPONSE_HPP
