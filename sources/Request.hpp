#ifndef REQUEST_HPP
# define REQUEST_HPP

#include <map>
#include <set>
#include <string>
#include "WebServ.hpp"
#include "Response.hpp"
#include "config_parsing/Config.hpp"

#define MAX_HEADER_LINE_LENGTH 8192 //http://nginx.org/en/docs/http/ngx_http_core_module.html#large_client_header_buffers

class Request {
	friend class Response;
	friend class Listener;
	friend class WebServ;

public:
	static const std::set<std::string> implemented_headers;

	static std::set<std::string> initRequestHeaders();

private:
	int _status_code;
	std::map<std::string, std::string> _headers;
	std::string _raw_request;
	struct sockaddr_in _remote_addr;
	std::string _method;
	std::string _request_target;
	std::string _query_string;
	std::string _http_version;
	std::string _content;
	int _server_port;
	bool _close_connection;
	ServerContext* _handling_server;
	LocationContext* _handling_location;
	std::string _absolute_root_path_for_request;
	bool _is_alias_path;
	std::size_t _body_bytes_read;
	std::string _put_filename;

	bool _header_has_been_read;
	std::string _cgi_script_path;
	std::string _response_content_lang;

	bool _is_chunked;
	std::string _host;
	int _port;
	std::size_t _lang_file_pos;

public:
	Request();
	Request(struct sockaddr_in & remote_addr, int server_port);
	~Request(void);

	void setStatusCode(int status_code);
	void setStatusCodeNoExept(int status_code);
	void setHandlingServer(ServerContext* handling_server);
	void setHandlingLocation(LocationContext * location);
	void setAbsoluteRootPathForRequest();
	void setCgiScriptPathForRequest(const std::string& path);
	void setHostAndPort(const std::string& host, int port);
	void setReponseContentLang(const std::string& lang);

	const std::string& getAbsoluteRootPathForRequest() const;
	int getStatusCode() const;
	const std::string& getCgiScriptPath() const;

	void parseRequestLine();
	void parseUri();
	void parseHeaders();
	void handleExpectHeader();
	void handleAcceptCharsetHeader();
	void handleAcceptLanguageHeader();
	bool isStatusCodeOk() const;
	bool isStatusCodeError() const;
	void checkForMaxBodySize(long body_size);
	void writeBodyInFile();
	bool targetIsFile();
	void appendRequestTarget(std::string & filename, std::string &request_target);
	bool isMethodAllowed(const LocationContext& handling_location);

private:
	std::string _getAbsolutePathForPutRequests(void) const;
	bool _isRegFileExists(const std::string& full_filename);
	std::list<std::string> _parseAndSortAcceptByQuality(std::string value);
	static bool _qualityPred(const Pair<std::string, float>& first, const Pair<std::string, float>& second);
	std::list<std::string> _sortValuesByQuality(std::list<Pair<std::string, float> >& values_list);
	Pair<int, float> _parseSpecificFloatValueForHeader(std::string str);
	Pair<std::string, float> _parseValueAndQuality(std::string str);
};

#endif
