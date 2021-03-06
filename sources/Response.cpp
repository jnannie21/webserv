//
// Created by jnannie on 2/13/21.
//

#include <sstream>
#include <string>
#include <set>
#include <dirent.h>
#include "Response.hpp"
#include "Base64.hpp"
#include <sys/time.h>

std::set<std::string> Response::implemented_headers = Response::_initResponseHeaders();

std::map<int, std::string> Response::status_codes = Response::_initStatusCodes();

std::set<std::string> Response::_initResponseHeaders() {
	std::set<std::string> implemented_headers;
	implemented_headers.insert("allow"); // Allow: OPTIONS, GET, HEAD
	implemented_headers.insert("content-language"); // Content-Language: en, ase, ru
	implemented_headers.insert("content-length"); // Content-Length: 1348
	implemented_headers.insert("content-location");
	implemented_headers.insert("content-type"); // Content-Type: text/html;charset=utf-8
	implemented_headers.insert("date"); // Date: Tue, 15 Nov 1994 08:12:31 GMT
	implemented_headers.insert("last-modified");
	implemented_headers.insert("location"); // Location: http://example.com/about.html#contacts
	implemented_headers.insert("retry-after");
	implemented_headers.insert("server"); // Server: Apache/2.2.17 (Win32) PHP/5.3.5
	implemented_headers.insert("transfer-encoding"); // Transfer-Encoding: gzip, chunked
	implemented_headers.insert("www-authenticate"); //WWW-Authenticate: Newauth realm="apps", type=1, title="Login to \"apps\"", Basic realm="simple"
	return implemented_headers;
}

std::map<int,std::string> Response::_initStatusCodes() {
	std::map<int,std::string> status_codes;
	status_codes[100] = "CONTINUE";
	status_codes[101] = "SWITCHING PROTOCOLS";
	status_codes[200] = "OK";
	status_codes[201] = "CREATED";
	status_codes[202] = "ACCEPTED";
	status_codes[203] = "NON-AUTHORITATIVE INFORMATION";
	status_codes[204] = "NO CONTENT";
	status_codes[205] = "RESET CONTENT";
	status_codes[206] = "PARTIAL CONTENT";
	status_codes[300] = "MULTIPLE CHOICES";
	status_codes[301] = "MOVED PERMANENTLY";
	status_codes[302] = "FOUND";
	status_codes[303] = "SEE OTHER";
	status_codes[304] = "NOT MODIFIED";
	status_codes[305] = "USE PROXY";
	status_codes[307] = "TEMPORARY REDIRECT";
	status_codes[400] = "BAD REQUEST";
	status_codes[401] = "UNAUTHORIZED";
	status_codes[402] = "PAYMENT REQUIRED";
	status_codes[403] = "FORBIDDEN";
	status_codes[404] = "NOT FOUND";
	status_codes[405] = "METHOD NOT ALLOWED";
	status_codes[406] = "NOT ACCEPTABLE";
	status_codes[407] = "PROXY AUTHENTICATION REQUIRED";
	status_codes[408] = "REQUEST TIMEOUT";
	status_codes[409] = "CONFLICT";
	status_codes[410] = "GONE";
	status_codes[411] = "LENGTH REQUIRED";
	status_codes[412] = "PRECONDITION FAILED";
	status_codes[413] = "PAYLOAD TOO LARGE";
	status_codes[414] = "URI TOO LONG";
	status_codes[415] = "UNSUPPORTED MEDIA TYPE";
	status_codes[416] = "RANGE NOT SATISFIABLE";
	status_codes[417] = "EXPECTATION FAILED";
	status_codes[426] = "UPGRADE REQUIRED";
	status_codes[500] = "INTERNAL SERVER ERROR";
	status_codes[501] = "NOT IMPLEMENTED";
	status_codes[502] = "BAD GATEWAY";
	status_codes[503] = "SERVICE UNAVAILABLE";
	status_codes[504] = "GATEWAY TIMEOUT";
	status_codes[505] = "HTTP VERSION NOT SUPPORTED";
	return status_codes;
}

Response::Response() :
		_request(NULL),
		_socket(0),
		_in_progress(false),
		_remains(0),
		_sent_len(0) { }

Response::Response(Request * request, int socket) :
				_request(request),
				_socket(socket),
				_in_progress(false),
				_remains(0),
				_sent_len(0) { }

Response::~Response(void) { }


void Response::_generateStatusLine() {
	_raw_response += "HTTP/1.1 ";
	_raw_response += libft::ultostr_base(_request->getStatusCode(), 10);
	_raw_response += " ";
	_raw_response += Response::status_codes[_request->getStatusCode()];
	_raw_response += "\r\n";
}

/*
 * https://stackoverflow.com/questions/7960318/math-to-convert-seconds-since-1970-into-date-and-vice-versa
 * explanations http://howardhinnant.github.io/date_algorithms.html
 */
struct tm Response::_getCalendarTime(time_t tv_sec) const { // TODO: maybe should make it simplier. !!! [Airat Comment] Dima, kak hochesh )
	struct tm calendar_time;
	int days = tv_sec / 86400;
	days += 719468;
	int era = (days >= 0 ? days : days - 146096) / 146097;
	int doe = days - era * 146097;          // [0, 146096]
	int yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;  // [0, 399]
	int y = yoe + era * 400;
	int doy = doe - (365 * yoe + yoe / 4 - yoe / 100);                // [0, 365]
	int mp = (5 * doy + 2) / 153;                                   // [0, 11]
	int d = doy - (153 * mp + 2) / 5 + 1;                             // [1, 31]
	int m = mp + (mp < 10 ? 3 : -9);                            // [1, 12]

	calendar_time.tm_sec = tv_sec % 60;
	calendar_time.tm_min = tv_sec % 3600 / 60;
	calendar_time.tm_hour = tv_sec % 86400 / 3600;

	calendar_time.tm_mday = d;
	calendar_time.tm_mon = m - 1;
	calendar_time.tm_year = y + (m <= 2) - 1900;

	days = tv_sec / 86400;
	calendar_time.tm_wday = (days >= -4 ? (days + 4) % 7 : (days + 5) % 7 + 6);
	return calendar_time;
}

struct tm Response::_gmtime(time_t tv_sec) const {
	struct tm calendar_time;
	calendar_time = _getCalendarTime(tv_sec);
	return calendar_time;
}

std::string Response::_getDateHeader() const {
	char s[30]; // Wed, 24 Feb 2021 12:10:04 GMT + '\0'
	struct tm calendar_time;
	struct timeval tv;

	gettimeofday(&tv, NULL);
	calendar_time = _gmtime(tv.tv_sec);
	strftime(s, sizeof(s), "%a, %d %b %Y %H:%M:%S GMT", &calendar_time);

	std::string date_header;
	date_header.append("date: ").append(s).append("\r\n");
	return date_header;
}

std::string Response::_getLastModifiedHeader(time_t tv_sec) const {
	char s[30]; // Wed, 24 Feb 2021 12:10:04 GMT + '\0'
	struct tm calendar_time;

	calendar_time = _gmtime(tv_sec);
	strftime(s, sizeof(s), "%a, %d %b %Y %H:%M:%S GMT", &calendar_time);

	std::string date_header;
	date_header.append("last-modified: ").append(s).append("\r\n");
	return date_header;
}

std::string Response::_getLocationHeader(bool is_file) const {
	std::string location;

	location += "location: ";
	location += "http://";
	location += _request->_headers["host"];
	location += _request->_request_target;
	if (!is_file) {
        location += '/';
	}
	location += "\r\n";

	return location;
}

std::string Response::_getAllowHeader() const {
	std::list<std::string> allowed_methods = _request->_handling_location->getLimitExceptMethods();
	std::string allow = "Allow: ";
	for (std::list<std::string>::iterator it = allowed_methods.begin(); it != allowed_methods.end(); ++it) {
		allow += *it;
		allow += ",";
	}
	allow.erase(allow.size() - 1, 1);
	allow += "\r\n";
	return allow;
}

std::string Response::_getWwwAuthenticateHeader() const {
	std::string www_authenticate = "WWW-Authenticate: Basic realm=\"restricted\", charset=\"UTF-8\"\r\n";
	return www_authenticate;
}

std::string Response::_getRetryAfterHeader() const {
    std::string retryAfter = ("Retry-After: " + std::string(RETRY_AFTER_SECOND_DELAY) + "\r\n");
    return retryAfter;
}

void Response::_generateHeaders() {
	_raw_response += "server: webserv\r\n";
	_raw_response += _www_authenticate;
	_raw_response += _getDateHeader();
	_raw_response += _content_type;
	_raw_response += _allow;
	_raw_response += _retry_after;
	_raw_response += "Content-Length: ";
	_raw_response += libft::ultostr_base(_content.length(), 10);
	_raw_response += "\r\n";
	_raw_response += _last_modified;
	_raw_response += _location;
	_raw_response += _content_location;

    if (_request->_lang_file_pos)
        _raw_response += "Content-Language: " "en-US, ru-RU" "\r\n";

    for (std::map<std::string, std::string>::iterator it = _cgi_headers.begin(); it != _cgi_headers.end(); ++it) {
		_raw_response += (*it).first + ": " + (*it).second + "\r\n";
	}
	_raw_response += "\r\n";
}

std::string Response::_getErrorPagePath() const {
    AContext * context = (_request->_handling_location != NULL) ?
                         static_cast<AContext*>(_request->_handling_location) :
                         static_cast<AContext*> (_request->_handling_server);

    if (context)
    {
        const std::map<int, std::map<std::string, std::string> > &error_page_info = context->getErrorPagesDirectiveInfo();

        std::map<int, std::map<std::string, std::string> >::const_iterator it_search = error_page_info.find(_request->getStatusCode());

        if (it_search != error_page_info.end()) {
            const std::map<std::string, std::string> &params_map = (it_search->second);

            std::string change_error_code = "";
            std::string redirect_uri = "";

            std::map<std::string, std::string>::const_iterator param_it;

            param_it = params_map.find(ERROR_PAGE_CHANGE_ERROR_CODE);
            if (param_it != params_map.end()) {
                change_error_code = param_it->second;
                if (change_error_code.size()) {
                    _request->setStatusCodeNoExept(libft::stoll_base(change_error_code, 10));
                }
            }

            param_it = params_map.find(ERROR_PAGE_REDIRECT_URI);
            if (param_it != params_map.end()) {
                redirect_uri = param_it->second;
            }
            return redirect_uri;
        }
    }
    return "";
}

void Response::_readErrorPage(const std::string & error_page) {
	int fd;
	std::string content;
	struct stat stat_buf;
	size_t ret;
	std::string filename = _request->getAbsoluteRootPathForRequest();
	_request->appendRequestTarget(filename, error_page);
	if (stat(filename.c_str(), &stat_buf) == 0 && S_ISREG(stat_buf.st_mode)) {
		if ((fd = open(filename.c_str(), O_RDONLY, S_IRWXU)) == -1)
			return ;
		std::vector<char> buf;
		buf.reserve(stat_buf.st_size);
		if ((ret = read(fd, &buf[0], stat_buf.st_size)) >= 0) {
			_content.append(&buf[0], ret);
		}
	}
}

void Response::_generateResponseByStatusCode() {
	_content_type = "Content-Type: text/html;charset=utf-8\r\n";
    if (_request->_method != "HEAD" && _request->isStatusCodeError()) {
		std::string error_page = _getErrorPagePath();
    	if (error_page.size())
    		_readErrorPage(error_page);
		if (_content.size() == 0)
			_content.append(libft::ultostr_base(_request->getStatusCode(), 10)).append(" ").append(Response::status_codes[_request->getStatusCode()]).append("\r\n");
    }

    _generateStatusLine();

    if (_request->getStatusCode() == 401) {
        _www_authenticate = _getWwwAuthenticateHeader();
    }

    if (_request->getStatusCode() != 100) { // cURL dont recognize 100 status code response with headers
        _generateHeaders();
        _raw_response.append(_content);
    }
}

void Response::_readFileToContent(std::string & filename) {
	char buf[1024 + 1];
	int ret;
	int fd;

	fd = open(filename.c_str(), O_RDONLY);
    if (fd <= 0) {
        _request->setStatusCode(500);
    }

	while ((ret = read(fd, buf, 1024))) {
		if (ret < 0)
			_request->setStatusCode(500);
		_content.append(buf, ret);
	}
	close(fd);
}

void Response::_setContentTypeByFileExt(const std::string & ext) {
	if (ext == "")
		_content_type = "Content-Type: application/octet-stream\r\n";
	else {
		if (ext == "txt")
			_content_type = "Content-Type: text/plain;charset=utf-8\r\n";
		else if (ext == "html")
			_content_type = "Content-Type: text/html;charset=utf-8\r\n";
		else if (ext == "jpg")
			_content_type = "Content-Type: image/jpeg;\r\n";
		else
			_content_type = "Content-Type: application/octet-stream\r\n";
	}
}

bool Response::_isMethodAllowed() const {
	if (!_request->_handling_location)
		return true;
	std::list<std::string> allowed_methods = _request->_handling_location->getLimitExceptMethods();

	if (allowed_methods.empty())
		return true;
	for (std::list<std::string>::iterator it = allowed_methods.begin(); it != allowed_methods.end(); ++it) {
		if (*it == _request->_method)
			return true;
	}
	return false;
}

std::string Response::_getExt(const std::string& filename) const {
	std::string ext;
	size_t dot_pos;

	dot_pos = filename.rfind('.');
	if (dot_pos != std::string::npos)
		ext = filename.substr(dot_pos + 1);
	return ext;
}

bool Response::_isCgiExt() const {
	if (_request->getCgiScriptPath().empty())
		return false;
	return true;
}

/*
 * https://topic.alibabacloud.com/a/implement-inet_aton-and-inet_ntoa-function-functions-in-c-language_1_31_20001161.html
 */
std::string Response::_inet_ntoa(struct in_addr sin_addr) {
	std::string buf;
	unsigned char *str = (unsigned char *)&sin_addr;

	buf += libft::ultostr_base(static_cast<unsigned long>(str[0]), 10) + ".";
	buf += libft::ultostr_base(static_cast<unsigned long>(str[1]), 10) + ".";
	buf += libft::ultostr_base(static_cast<unsigned long>(str[2]), 10) + ".";
	buf += libft::ultostr_base(static_cast<unsigned long>(str[3]), 10);

	return buf;
}

std::string Response::_getUserFromCredentials() const {
	std::string user;
	size_t credential_pos = _request->_headers["authorization"].find(" ");
	credential_pos++;
	if (credential_pos != std::string::npos && _request->_headers["authorization"].size() > credential_pos) {
		std::string credentials = _request->_headers["authorization"].substr(credential_pos);
		credentials = credentials.substr(credentials.find_first_not_of(" "));
		user = Base64::base64_decode(credentials).substr(0, credentials.find(':'));
	}
	return user;
}

void Response::_setEnv(std::vector<char *> & env, const std::string & filename, std::map<std::string, std::string> & cgiVariables) {
	cgiVariables["AUTH_TYPE"] = "AUTH_TYPE=" + _request->_headers["authorization"].substr(0, _request->_headers["authorization"].find(' '));
	cgiVariables["CONTENT_LENGTH"] = "CONTENT_LENGTH=" + _request->_headers["content-length"];
	cgiVariables["CONTENT_TYPE"] = "CONTENT_TYPE=" + _request->_headers["content-type"];
	cgiVariables["GATEWAY_INTERFACE"].assign("GATEWAY_INTERFACE=").append("CGI/1.1");

	cgiVariables["PATH_INFO"].assign("PATH_INFO=").append(_request->_request_target);

	cgiVariables["PATH_TRANSLATED"].assign("PATH_TRANSLATED=").append(filename);
	cgiVariables["QUERY_STRING"] = "QUERY_STRING=" + _request->_query_string;
	cgiVariables["REMOTE_ADDR"].assign("REMOTE_ADDR=").append(_inet_ntoa(_request->_remote_addr.sin_addr).c_str());
	cgiVariables["REMOTE_IDENT"] = "REMOTE_IDENT=";

	cgiVariables["REMOTE_USER"].assign("REMOTE_USER=").append(_getUserFromCredentials());
	cgiVariables["REQUEST_METHOD"] = "REQUEST_METHOD=" + _request->_method;
	cgiVariables["REQUEST_URI"].assign("REQUEST_URI=").append(_request->_request_target); // there is no such Variable in rfc
	cgiVariables["SCRIPT_NAME"].assign("SCRIPT_NAME=").append(_request->_handling_location->getCgiScript().c_str());
	cgiVariables["SERVER_NAME"] = "SERVER_NAME=" + _request->_headers["host"].substr(0, _request->_headers["host"].find(':'));
	cgiVariables["SERVER_PORT"] = "SERVER_PORT=" + libft::ultostr_base(_request->_server_port, 10);
	cgiVariables["SERVER_PROTOCOL"].assign("SERVER_PROTOCOL=").append("HTTP/1.1");
	cgiVariables["SERVER_SOFTWARE"].assign("SERVER_SOFTWARE=").append("webserv");

	for (std::map<std::string, std::string>::iterator it = _request->_headers.begin(); it != _request->_headers.end(); ++it) {
		if (it->first != "content-length" && it->first != "content-Type" && it->first != "authorization") {
			std::string first = it->first;
			std::replace(first.begin(), first.end(), '-', '_');
			libft::string_to_upper(first);
			cgiVariables[first] = "HTTP_" + first + "=" + it->second;
		}
	}

	if (_file_ext == "php") {
		cgiVariables["REDIRECT_STATUS"] = "REDIRECT_STATUS=true"; // php doesnt work without it, dont know why yet
	}
	int i = 0;
	for (std::map<std::string, std::string>::iterator it = cgiVariables.begin(); it != cgiVariables.end(); ++it) {
		env.push_back(const_cast<char *>(it->second.c_str()));
		i++;
	}
	env.push_back(NULL);
}

void Response::_runCgi(const std::string & filename) { // filename is a *.php script
	int pid;
    	int exit_status = 0;
	std::string cgi_script;
	cgi_script = _request->getCgiScriptPath();
	char * argv[3] = {
			const_cast<char *>(cgi_script.c_str()),
			const_cast<char *>(filename.c_str()),
			NULL
	};
	std::vector<char *> env;

	std::map<std::string, std::string> cgiVariables;
	_setEnv(env, filename, cgiVariables);

	std::string out_file_path = WebServ::getWebServRootPath() + "temp_out";
	std::string in_file_path = WebServ::getWebServRootPath() + "temp_in";

	int fd_write;
	if ((fd_write = open(in_file_path.c_str(), O_CREAT | O_TRUNC | O_RDWR, S_IRWXU)) == -1)
		utils::exitWithLog();

	long ret;
	ret = write(fd_write, _request->_content.c_str(), _request->_content.size());
	if (ret < 0) {
		_request->setStatusCode(500);
	}

	close(fd_write);

	if ((pid = fork()) == -1) {
		 utils::exitWithLog();
	} else if (pid == 0) {
		if ((fd_write = open(in_file_path.c_str(), O_RDONLY, S_IRWXU)) == -1)
			utils::exitWithLog();
		dup2(fd_write, 0);
		close(fd_write);
		int fd_read;
		if ((fd_read = open(out_file_path.c_str(), O_CREAT | O_TRUNC | O_WRONLY, S_IRWXU)) == -1)
			utils::exitWithLog();
		dup2(fd_read, 1);
		close(fd_read);
		execve(argv[0], argv, &env[0]);
		exit(EXIT_FAILURE);
	}

	waitpid(pid, &exit_status, 0); // TODO: maybe we should use not blocking wait, then we need to save stdin and out backups and pass pipe descritpors to select
	if (WIFEXITED(exit_status))
		exit_status = WEXITSTATUS(exit_status);
	else if (WIFSIGNALED(exit_status))
		exit_status = exit_status | 128;

	if (exit_status == 0) {
		struct stat stat_buf;
		size_t size = 0;

		if (stat(out_file_path.c_str(), &stat_buf) == 0) { // getting file size of "out_file_path"
			size = stat_buf.st_size;
		} else {
			unlink(in_file_path.c_str());
			_request->setStatusCode(500);
		}

		int fd_read;
		if ((fd_read = open(out_file_path.c_str(), O_RDONLY, S_IRWXU)) == -1) {
			unlink(in_file_path.c_str());
			_request->setStatusCode(500);
		}

		std::vector<char> buf;
		buf.reserve(size);
		ret = 0;
		if ((ret = read(fd_read, &buf[0], size)) >= 0) {
			try {
				_cgi_response.append(&buf[0], ret);
			} catch (std::bad_alloc& ba) {
				_request->setStatusCodeNoExept(500);
			}
		}
		else {
			_request->setStatusCodeNoExept(500);
		}
		close(fd_read);
		std::cout << "cgi response received" << std::endl;
	}

	unlink(in_file_path.c_str());
	unlink(out_file_path.c_str());

	if (exit_status)
		_request->setStatusCode(500);
}

void Response::_parseHeadersFromCgiResponse() { // the same as in request headers parsing
	std::string field_name;
	std::string field_value;
	size_t field_name_length;
	size_t field_value_length;

	long headers_len = _cgi_response.find("\r\n\r\n") + 4;
	std::string headers = _cgi_response.substr(0, headers_len);

	size_t line_length = headers.find("\r\n");
	while (line_length != 0) {
		if (line_length > MAX_HEADER_LINE_LENGTH
			|| line_length == std::string::npos) {
			_request->setStatusCode(500); // http://nginx.org/en/docs/http/ngx_http_core_module.html#large_client_header_buffers
		}

		field_name_length = headers.find(':'); // field-name
		if (field_name_length == std::string::npos) {
			_request->setStatusCode(500);
		}
		field_name = headers.substr(0, field_name_length);

		libft::string_to_lower(field_name); // field_name is case-insensitive so we make it lowercase to make life easy

		if (field_name.find(' ') != std::string::npos) { // no spaces inside field-name, rfc 2.3.4
			_request->setStatusCode(500);
		}
		headers.erase(0, field_name_length + 1);

		field_value_length = line_length - field_name_length - 1; // field-value
		if (headers[0] == ' ') {
			headers.erase(0, 1); // remove optional whitespace in the beginning of field-value
			field_value_length--;
		}
		if (headers[field_value_length - 1] == ' ') {
			headers.erase(field_value_length - 1, 1); // remove optional whitespace in the end of field-value
			field_value_length--;
		}
		field_value = headers.substr(0, field_value_length);
		headers.erase(0, field_value_length + 2);

		_cgi_headers[field_name].append(field_value);

		line_length = headers.find("\r\n");
	}
	_cgi_response.erase(0, headers_len);
}

void Response::_generateContentForGetRequest() {
    if (!_isMethodAllowed() && (_request->getCgiScriptPath()).empty()) {
        _allow = _getAllowHeader();
        _request->setStatusCode(405);
    }

    std::string filename = _request->getAbsoluteRootPathForRequest();
	_request->appendRequestTarget(filename, _request->_request_target);

    struct stat stat_buf;
    std::string matching_index;

    if (stat(filename.c_str(), &stat_buf) == 0) { // file or directory exists
        if (S_ISDIR(stat_buf.st_mode)) { // filename is a directory
            if (filename[filename.size() - 1] != '/') {
                _location = _getLocationHeader(false);
				_retry_after = _getRetryAfterHeader();
                _request->setStatusCode(301); //Moved Permanently
            }
            std::list<std::string> index_list;
            if (_request->_handling_location)
                index_list = _request->_handling_location->getIndexPagesDirectiveInfo(); // try to search one of index file
            else
                index_list = _request->_handling_server->getIndexPagesDirectiveInfo(); // try to search one of index file

            if (!index_list.empty()) {
                for (std::list<std::string>::const_iterator it = index_list.begin(); it != index_list.end(); ++it) {
                    if (stat((filename + *it).c_str(), &stat_buf) == 0) {
						matching_index = *it;
                        filename += *it;
                        WebServ::routeRequest(_request->_host, _request->_port, _request, _request->_request_target + *it);
                        break ;
                    }
                }
                if (matching_index.empty() && !_request->_handling_server->isAutoindexEnabled() && (!_request->_handling_location || (_request->_handling_location && !_request->_handling_location->isAutoindexEnabled()))) { // test from subject wants 404 if there is index in config but file doesnt exist
                    _request->setStatusCode(404);
                }
            }
        }

		if (S_ISREG(stat_buf.st_mode)) {
			_file_ext = _getExt(filename);
			if (_isCgiExt()) {
				_runCgi(filename);
				_parseHeadersFromCgiResponse();
				if (_cgi_headers.count("content-length")) {
					_cgi_response.resize(libft::strtoul_base(_cgi_headers["content-length"], 10));
				}
				_content.swap(_cgi_response);
			} else {
				_setContentTypeByFileExt(_file_ext);
				_readFileToContent(filename);
				_last_modified = _getLastModifiedHeader(stat_buf.st_mtime);
			}
		} else if (S_ISDIR(stat_buf.st_mode)) {
			if (filename[filename.size() - 1] != '/') {
				if (_request->_request_target[_request->_request_target.size() - 1] != '/')
					_request->_request_target += '/';
				_request->_request_target += matching_index;

				_location = _getLocationHeader(false);

                _retry_after = _getRetryAfterHeader();
                _request->setStatusCode(301); //Moved Permanently
            }
            if ((_request->_handling_location && _request->_handling_location->isAutoindexEnabled()) || _request->_handling_server->isAutoindexEnabled()) {
                _content = _generateAutoindex(filename);
                _content_type = "Content-Type: text/html;charset=utf-8\r\n";
            } else {
                _request->setStatusCode(403);
            }
        } else {
            _request->setStatusCode(403);
        }
    } else {
        _request->setStatusCode(404);
    }
}

void Response::_generateGetResponse() {
	_generateContentForGetRequest();

	_generateStatusLine();
	_generateHeaders();
	_raw_response += _content;
}

void Response::_generateHeadResponse() {
	_generateContentForGetRequest();

	_generateStatusLine();
	_generateHeaders();
}

void Response::_generatePutResponse() {
	_content_location = "Content-Location: " + _request->_request_target + "\r\n";
    _generateStatusLine();
    _generateHeaders();
}

void Response::_generatePostResponse() {

	if (!_isMethodAllowed() && (_request->getCgiScriptPath()).empty()) {
		_allow = _getAllowHeader();
		_request->setStatusCode(405);
	}

	std::string filename = _request->getAbsoluteRootPathForRequest();
	_request->appendRequestTarget(filename, _request->_request_target);

	_file_ext = _getExt(filename);
	if (_isCgiExt()) {
		_runCgi(filename);
		_parseHeadersFromCgiResponse();
		if (_cgi_headers.count("content-length")) {
			_cgi_response.resize(libft::strtoul_base(_cgi_headers["content-length"], 10));
		}
		_content.swap(_cgi_response);
	}

	_generateStatusLine();
	_generateHeaders();

	_raw_response += _content;
}

void Response::generateResponse() {
	if (_request->isStatusCodeOk()) {
	    try
        {
			_handleAcceptHeaders();

			if (_request->_method == "GET") {
				_generateGetResponse();
			} else if (_request->_method == "HEAD") {
				_generateHeadResponse();
			} else if (_request->_method == "PUT") {
				_generatePutResponse();
			} else if (_request->_method == "POST") {
				_generatePostResponse();
			} else {
				_request->setStatusCode(501); // 501 Not Implemented
			}
        }
	    catch (WebServ::NotOKStatusCodeException& e) {}
	}

	if (!_request->isStatusCodeOk()) {
		_generateResponseByStatusCode();
	}
}

void Response::sendResponse() {
	static int i = 0;

	long ret = 0;
		ret = send(_socket, _raw_response.c_str() + _sent_len, _remains, 0);

		if (ret >= 0) {
			_sent_len += ret;
			_remains -= ret;
			if (_remains == 0) {
				++i;
				std::cout << _raw_response.substr(0,200) << std::endl;
				std::cout << "response " << i << " is sent" << std::endl;
				_in_progress = false;
			} else {
				_in_progress =  true;
			}
		} else if (ret == -1) {
			_request->_close_connection = true;
			_in_progress = false;
		}
}

void Response::_handleAcceptHeaders(void) {
    if (_request->_headers.count("accept-charset")) {
        _request->handleAcceptCharsetHeader();
    }

	_request->handleAcceptLanguageHeader();
}

void Response::setRemains() {
	_remains = _raw_response.size();
}

bool Response::getInProgress() const {
	return _in_progress;
}

std::string & Response::getContent() {
	return _content;
}

std::string Response::_replaceQuoteToCode(const std::string& str) {
	std::string copy = str;
	std::string::iterator it = copy.begin();
	while (it != copy.end()) {
		if ((*it) == '"') {
			copy.replace(it, it+1, "%22");
		}
		++it;
	}
	return copy;
}

bool Response::_isUtf_8(char c) {
	return (c & 0xC0) == 0x80;
}

// https://stackoverflow.com/questions/31652407/how-to-get-the-accurate-length-of-a-stdstring
std::size_t Response::_getCharsLen(const std::string& str) {
	return (str.length() - count_if(str.begin(), str.end(), _isUtf_8));
}

std::string Response::_generateAutoindex(const std::string & dir_name)
{
	std::string host_n_port = "http://";
	host_n_port += _request->_headers["host"];
	std::string url = host_n_port + _request->_request_target;

	std::string response_body = "<html>";

	response_body += "<head>\n"
					 "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">\n"
					 "<title>Index of ";
	response_body += _request->_request_target;
	response_body +="</title>\n"
					"</head>\n";
	response_body += "<body>\n";
	response_body += "<h1>Index of ";
	response_body += _request->_request_target;
	response_body += "</h1><hr>\n";
	response_body += "<pre>\n";

	response_body += "<a href=\"";
	response_body += "../";
	response_body += "\">../</a>\n";

	std::list<std::map<std::string, std::list<std::string> > > info = _dir_opers(dir_name);
	std::list<std::map<std::string, std::list<std::string> > >::iterator it = info.begin();

	while (it != info.end()) {
		std::map<std::string, std::list<std::string> >::iterator it_map = (*it).begin();
		while (it_map != (*it).end()) {
			response_body += "<a href=\"";

			std::string correct = _replaceQuoteToCode((*it_map).first);
			response_body += (url + correct);
			response_body += "\">";

			std::string file_name = (*it_map).first;
			std::size_t fname_len = _getCharsLen(file_name);
			if (fname_len > 50) {
				std::size_t i = 47;
				std::string file_name_tmp = file_name.substr(0, i);
				while (_getCharsLen(file_name_tmp) != 47)
				{
					i++;
					file_name_tmp = file_name.substr(0, i);
				}
				if (i > 47)
					i++;
				file_name = file_name.substr(0,i) + "..>";
			}
			fname_len = _getCharsLen(file_name);
			response_body += file_name;
			response_body += "</a>";

			std::size_t count_of_spaces = (50 - fname_len) + 1;
			response_body += std::string(count_of_spaces, ' ');

			response_body += (*it_map).second.front(); // file modified datetime

			count_of_spaces = 20 - (*it_map).second.back().size();
			response_body += std::string(count_of_spaces, ' ');

			response_body += (*it_map).second.back(); // file size in bytes
			response_body += "\n";
			++it_map;
		}
		++it;
	}

	response_body += "</pre><hr>\n";
	response_body += "</body>\n";
	response_body += "</html>\n";

	return response_body;
}

std::list<std::map<std::string, std::list<std::string> > > Response::_dir_opers(const std::string& dir_name)
{
	DIR* dir_stream;
	dir_stream = opendir(dir_name.c_str());

	if (!dir_stream)
		_request->setStatusCode(500);
	struct dirent *info;

	std::map<std::string, std::list<std::string> > files;
	std::map<std::string, std::list<std::string> > dirs;

	std::map<std::string, std::list<std::string> >* tmp;
	while ((info = readdir(dir_stream))) {
		std::string file_obj_name = std::string(info->d_name);

		if (file_obj_name[0] == '.')
			continue;
		struct stat info_buf;
		std::string file_full_path = dir_name + file_obj_name;

		if (stat(file_full_path.c_str(), &info_buf) == -1) {
			_request->setStatusCode(500);
		}

		int file_type = info_buf.st_mode & S_IFMT;

		if (file_type == S_IFREG)
			tmp = &files;
		else if (file_type == S_IFDIR)
			tmp = &dirs;

		std::list<std::string> date_modified_and_size_in_bytes;

		std::string modified_seconds_to_str = libft::ultostr_base(info_buf.st_mtime, 10);
		struct tm modified_time;
		char *null_if_error = strptime(modified_seconds_to_str.c_str(), "%s", &modified_time);
		if (!null_if_error)
			_request->setStatusCode(500);
		char time_to_str[18]; // example: 31-Jan-2021 20:51
		strftime(time_to_str, 18, "%d-%b-%Y %H:%M", &modified_time);

		date_modified_and_size_in_bytes.push_back(std::string(time_to_str));

		// get size of file in bytes
		if (file_type == S_IFDIR)
			date_modified_and_size_in_bytes.push_back("-");
		else
			date_modified_and_size_in_bytes.push_back(libft::ultostr_base(info_buf.st_size, 10));

		if (file_type == S_IFDIR)
			file_obj_name += "/";
		(*tmp)[file_obj_name] = date_modified_and_size_in_bytes;

	}
	closedir(dir_stream);

	std::list<std::map<std::string, std::list<std::string> > > all_info;
	all_info.push_back(dirs);
	all_info.push_back(files);

	return all_info;
}

/*
PUT /nginx_meme.jpg HTTP/1.1

Host: localhost:8080

User-Agent: curl/7.75.0

Accept: *//*

Content-Length: 172172

Expect: 100-continue

 */