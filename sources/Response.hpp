//
// Created by jnannie on 2/12/21.
//

#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include "Request.hpp"
#include <sys/socket.h>
#include <string>
#include <set>
#include <list>

class Request;

class Response {
public:
	static std::set<std::string> implemented_headers;
	static std::map<int, std::string> status_codes;

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
	std::string _content;
	std::string _file_ext;
	std::string _cgi_response;
	std::map<std::string, std::string> _cgi_headers;
	bool _in_progress;
	long _remains;
	long _sent_len;

public:
	Response(Request * request, int socket);
	Response();
	~Response(void);

	void generateResponse();
	void setRemains();
	bool getInProgress() const;
	std::string & getContent();
	void sendResponse();

private:
	static std::map<int,std::string> _initStatusCodes();
	static std::set<std::string> _initResponseHeaders();

	void _generateGetResponse();
	void _generateContentForGetRequest();
	void _generateHeadResponse();
	void _generatePutResponse();
	void _generatePostResponse();
	void _generateResponseByStatusCode();
	void _generateStatusLine();
	void _generateHeaders();
	void _readFileToContent(std::string & filename);
	void _setContentTypeByFileExt(const std::string & ext);
	bool _isMethodAllowed() const;
	std::string _getErrorPagePath() const;
	void _readErrorPage(const std::string & error_page);
	void _handleAcceptHeaders();
	std::string _getDateHeader() const;
	std::string _getLastModifiedHeader(time_t tv_sec) const;
	std::string _getLocationHeader(bool is_file) const;
	std::string _getAllowHeader() const;
	std::string _getWwwAuthenticateHeader() const;
	std::string _getRetryAfterHeader() const;
	std::string _getUserFromCredentials() const;
	struct tm _getCalendarTime(time_t tv_sec) const;
	struct tm _gmtime(time_t tv_sec) const;
	std::string _getExt(const std::string & filename) const;
	bool _isCgiExt() const;
	void _runCgi(const std::string & filename);
	void _setEnv(std::vector<char *> & env, const std::string & filename, std::map<std::string, std::string> & cgiVariables);
	void _parseHeadersFromCgiResponse();
	std::string _inet_ntoa(const struct in_addr sin_addr);

	std::string _generateAutoindex(const std::string & dir_name);
	std::list<std::map<std::string, std::list<std::string> > > _dir_opers(const std::string& dir_name);
	std::string _replaceQuoteToCode(const std::string& str);
	std::size_t _getCharsLen(const std::string& str);
	static bool _isUtf_8(char c);
};



#endif //RESPONSE_HPP
