//
// Created by jnannie on 2/13/21.
//

#include <string>
#include <set>
#include "Request.hpp"

const std::set<std::string> Request::implemented_headers = Request::initRequestHeaders();

std::set<std::string> Request::initRequestHeaders() {
	std::set<std::string> implemented_headers;
	implemented_headers.insert("accept-charset"); // Accept-Charset: utf-8  Accept-Charset: iso-8859-5, unicode-1-1;q=0.8
	implemented_headers.insert("accept-language"); // Accept-Language: ru
	implemented_headers.insert("authorization"); // Authorization: Basic QWxhZGRpbjpvcGVuIHNlc2FtZQ==
	implemented_headers.insert("content-language"); // Content-Language: en, ase, ru
	implemented_headers.insert("content-length"); // Content-Length: 1348
	implemented_headers.insert("content-location");
	implemented_headers.insert("content-type"); // Content-Type: text/html;charset=utf-8
	implemented_headers.insert("date"); // Date: Tue, 15 Nov 1994 08:12:31 GMT
	implemented_headers.insert("host"); // Host: ru.wikipedia.org
	implemented_headers.insert("last-modified");
	implemented_headers.insert("referer"); // Referer: http://en.wikipedia.org/wiki/Main_Page
	implemented_headers.insert("transfer-encoding"); // Transfer-Encoding: gzip, chunked
	implemented_headers.insert("user-agent"); // User-Agent: Mozilla/5.0 (X11; Linux i686; rv:2.0.1) Gecko/20100101 Firefox/4.0.1
	// for PUT method
	implemented_headers.insert("expect"); // Expect: 100-continue
	implemented_headers.insert("content-range"); // https://efim360.ru/rfc-7231-protokol-peredachi-giperteksta-http-1-1-semantika-i-kontent/#4-3-4-PUT
	return implemented_headers;
}

Request::Request()
		: _status_code(200),
		  _remote_addr(),
		  _server_port(0),
		  _close_connection(false),
		  _handling_server(NULL),
		  _handling_location(NULL),
		  _is_alias_path(false),
		  _body_bytes_read(0),
		  _header_has_been_read(false),
		  _response_content_lang(DEFAULT_RESPONSE_LANGUAGE),
		  _is_chunked(false),
		  _port(0),
		  _lang_file_pos(0) {}

Request::Request(struct sockaddr_in & remote_addr, int server_port)
		:  _status_code(200),
		   _remote_addr(remote_addr),
		   _server_port(server_port),
		   _close_connection(false),
		   _handling_server(NULL),
		   _handling_location(NULL),
		   _is_alias_path(false),
		   _body_bytes_read(0),
		   _header_has_been_read(false),
		   _response_content_lang(DEFAULT_RESPONSE_LANGUAGE),
		   _is_chunked(false),
		   _port(0),
		   _lang_file_pos(0) {}

Request::~Request() {}

bool Request::isMethodAllowed(const LocationContext& handling_location) {
	const std::list<std::string> limit_except = (handling_location).getLimitExceptMethods();
	if (limit_except.empty())
		return false;

	std::list<std::string>::const_iterator it = limit_except.begin();
	while (it != limit_except.end()) {
		if (_method == (*it)) {
			return false;
		}
		++it;
	}
	return true;
}

void Request::setStatusCode(int status_code) {
	_status_code = status_code;
	if (!isStatusCodeOk()) {
        throw WebServ::NotOKStatusCodeException();
	}
}

void Request::setAbsoluteRootPathForRequest(void) {
    std::string globalRootPath = WebServ::getWebServRootPath();
    std::string cont_root_path;

    if (_handling_location) {
        cont_root_path = _handling_location->getAliasPath();
        if (!cont_root_path.empty()) {
            _absolute_root_path_for_request = cont_root_path;
            _is_alias_path = true;
            return ;
        } else {
            cont_root_path = _handling_location->getRootPath() + _handling_location->getLocationPath();
        }
    } else {
        cont_root_path = _handling_server->getRootPath();
    }

    if (cont_root_path[0] == '/') { // if context root path is absolute by itself
        _absolute_root_path_for_request = cont_root_path;
    } else {
        _absolute_root_path_for_request = globalRootPath + cont_root_path;
    }
}

std::string Request::_getAbsolutePathForPutRequests() const {
    std::string globalRootPath = WebServ::getWebServRootPath();
    std::string cont_root_path;

    if (_handling_location) {
        cont_root_path = _handling_location->getAliasPath();
        if (!cont_root_path.empty()) {
            return cont_root_path;
        } else {
            cont_root_path = _handling_location->getRootPath();
        }
    } else {
        cont_root_path = _handling_server->getRootPath();
    }

    if (cont_root_path[0] == '/') { // if context root path is absolute by itself
        return cont_root_path;
    } else {
        return (globalRootPath + cont_root_path);
    }
}

void Request::parseRequestLine() {
	std::string request_line;

	size_t request_line_length = _raw_request.find("\r\n");
	if (request_line_length == std::string::npos)
		setStatusCode(400);
	request_line = _raw_request.substr(0, request_line_length);
	size_t word_end = request_line.find(' ');
	if (word_end == std::string::npos)
		setStatusCode(400);
	_method = request_line.substr(0, word_end);
	request_line.erase(0, word_end + 1);

	word_end = request_line.find('?'); // TODO: take word first, to ' ', and find '?' in it, just in case, but not mandatory
	bool there_is_query = word_end != std::string::npos;
	if (there_is_query) {
		_request_target = request_line.substr(0, word_end);
		request_line.erase(0, word_end + 1);
	}

	word_end = request_line.find(' ');
	if (word_end == std::string::npos)
		setStatusCode(400);
	if (there_is_query)
		_query_string = request_line.substr(0, word_end);
	else
		_request_target = request_line.substr(0, word_end);
	request_line.erase(0, word_end + 1);

	if (request_line.find(' ') != std::string::npos)
		setStatusCode(400);

	_http_version = request_line;
	_raw_request.erase(0, request_line_length + 2);

	if (_method.length() + _request_target.length() + _http_version.length() + 4 > MAX_HEADER_LINE_LENGTH)
		setStatusCode(414); // http://nginx.org/en/docs/http/ngx_http_core_module.html#large_client_header_buffers
}

void Request::parseHeaders() {
	std::string field_name;
	std::string field_value;
	size_t field_name_length;
	size_t field_value_length;

	size_t line_length = _raw_request.find("\r\n");
	while (line_length != 0) {
		if (line_length > MAX_HEADER_LINE_LENGTH
			|| line_length == std::string::npos) {
			setStatusCode(400); // http://nginx.org/en/docs/http/ngx_http_core_module.html#large_client_header_buffers
		}

		field_name_length = _raw_request.find(':'); // field-name
		if (field_name_length == std::string::npos) {
			setStatusCode(400);
		}
		field_name = _raw_request.substr(0, field_name_length);

		libft::string_to_lower(field_name); // field_name is case-insensitive so we make it lowercase to make life easy

		if (field_name.find(' ') != std::string::npos) { // no spaces inside field-name, rfc 3.2.4
			setStatusCode(400);
		}
		_raw_request.erase(0, field_name_length + 1);


		field_value_length = line_length - field_name_length - 1; // field-value
		if (_raw_request[0] == ' ') {
			_raw_request.erase(0, 1); // remove optional whitespace in the beginning of field-value
			field_value_length--;
		}
		if (_raw_request[field_value_length - 1] == ' ') {
			_raw_request.erase(field_value_length - 1, 1); // remove optional whitespace in the end of field-value
			field_value_length--;
		}
		field_value = _raw_request.substr(0, field_value_length);
		_raw_request.erase(0, field_value_length + 2);
			if (_headers.count(field_name)) {
				if (field_name == "host")
					setStatusCode(400);
				if (field_name == "content-length") {
					_close_connection = true; // rfc 7230 3.3.3
					setStatusCode(400);
				}
				_headers[field_name].append(",");
			}
			_headers[field_name].append(field_value); // add field_name-field_value to map

		line_length = _raw_request.find("\r\n");
	}
	_raw_request.erase(0, 2);
	if (_headers.empty())
		setStatusCode(400);
}

void Request::parseUri() {
	std::string url = _request_target;
	std::string res;
	std::string tmp;
	std::list<std::string>   path;
	std::list<std::string>::iterator it = path.begin();
	int count = 0;
	int lenght = url.length();

	size_t hashtag_pos = url.find('#');
	if (hashtag_pos != std::string::npos)
		url.resize(hashtag_pos); // nginx just ignore all that after '#', even %00
	while (count < lenght) {
		while (url[count] == '/') // if "//////"
			++count;
		if (url[count] == '.' && // if "/./" of "/."
			(count + 1 == lenght || (count + 1 < lenght && url[count + 1] == '/')))
			count += 1;
		if (url[count] == '.' && // if "/../" or "/.."
			((count + 2 < lenght && url[count + 1] == '.' && url[count + 2] == '/') ||
			 (count + 2 == lenght && url[count + 1] == '.'))) {
			if (!path.empty()) {
				path.pop_back();
			}
			count += 2;
		}
		while (count < lenght && url[count] != '/') {
			if (url[count] == '%') {
				char ch;
				std::string hex_str = url.substr(count + 1, 2);
				libft::string_to_lower(hex_str);
				if (hex_str.find_first_not_of("0123456789abcdef") == std::string::npos && hex_str != "00") { // nginx returns 400 when %00, but when %01-%17 it returns 404, the same with other Excluded US-ASCII Characters
					ch  = libft::strtoul_base(hex_str, 16);
					tmp += ch;
					count += 3;
					continue;
				} else {
					setStatusCode(400);
				}
			}
			tmp += url[count];
			++count;
		}
		if (!tmp.empty()) {
			path.push_back(tmp);
			tmp.clear();
		}
	}
	it = path.begin();
	while (it != path.end()) {
		res += '/';
		res += *it;
		++it;
	}

	if (res.empty())
		_request_target = '/';
	else {
		_request_target = res;
		if (url.size() && url[url.size() - 1] == '/')
			_request_target += '/';
	}
}

std::list<std::string> Request::_parseAndSortAcceptByQuality(std::string value) {

    std::list<Pair<std::string, float> > with_quality;
    std::size_t pos;

    while(!value.empty()) {
        pos = value.find_first_of(',');
        with_quality.push_back(_parseValueAndQuality(value.substr(0, pos)));

        if (pos != std::string::npos)
            pos++;
        value.erase(0, pos);

        while(libft::isspace(value[0])) {
            value.erase(0, 1);
        }
    }

    std::list<Pair<std::string, float> >::iterator it = with_quality.begin();
    const std::list<std::string>& lang_codes = WebServ::getLanguageCodesList();
    while (it != with_quality.end())
    {
        if (it->first.size() < 2)
        {
            it = with_quality.erase(it);
        }
        else
        {
            if (std::find(lang_codes.begin(), lang_codes.end(), it->first.substr(0, 2)) ==  lang_codes.end()) {
                it = with_quality.erase(it);
            } else {
                ++it;
            }
        }
    }
    return _sortValuesByQuality(with_quality);
}

/*
 * RFC-7231 5.1.1 Expect
 * Author: Airat (GDrake)
 */
void Request::handleExpectHeader() {
    const std::map<std::string, std::string>& headers = _headers;

    std::map<std::string, std::string>::const_iterator it = headers.find("expect");
    if ( it != headers.end()) {
        std::string value = (*it).second;
        libft::string_to_lower(value);

        if (value != "100-continue") {
            setStatusCode(417);
        } else {
			setStatusCode(100);
        }
    }
}

/*
 * https://developer.mozilla.org/ru/docs/Web/HTTP/Headers/Accept-Charset
 * https://developer.mozilla.org/ru/docs/Web/HTTP/Headers/Accept-Language
 * https://developer.mozilla.org/en-US/docs/Glossary/Quality_values
 *
 * cURL TEST REQUESTS
 *
 * curl --header "Accept-Charset: utf-8, iso-8859-1;q=0.025, *;q=0.7" localhost:8080
 *
 * curl --header "Accept-Language: fr-CH, fr;q=0.9, en;q=0.8, de;q=0.7, *;q=0.5, ru-RU" localhost:8080
 *
 * Author: Airat (GDrake)
 */

void Request::handleAcceptCharsetHeader() {
    std::list<std::string> values = _parseAndSortAcceptByQuality("accept-language");

    bool is_found = (std::find(values.begin(), values.end(), DEFAULT_RESPONSE_CHARSET) != values.end());
    if (!is_found) {
        is_found = (std::find(values.begin(), values.end(), "*") != values.end());
    }
    if (!is_found && CHECK_ACCEPT_CHARSET_HEADER) {
        setStatusCode(406);
    }
}

void Request::handleAcceptLanguageHeader() {
    if (_lang_file_pos) {
        if (_headers.count("accept-language"))
        {
            std::list<std::string> values = _parseAndSortAcceptByQuality("accept-language");
            std::list<std::string>::const_iterator it = values.begin();

            bool is_found = false;
            while (it != values.end()) {
                std::string target = _request_target;
                target.insert(_lang_file_pos, *it);

                std::string full_filename = getAbsoluteRootPathForRequest();
				appendRequestTarget(full_filename, target);
                if (_isRegFileExists(full_filename)) {
                    _request_target = target;
                    is_found = true;
                    setReponseContentLang(*it);
                    break;
                }
                ++it;
            }
            if (!is_found) {
                is_found = (std::find(values.begin(), values.end(), "*") != values.end());
                if (is_found) {
                    std::string target = _request_target;
                    target.insert(_lang_file_pos, DEFAULT_RESPONSE_LANGUAGE);
                    is_found = true;
                    setReponseContentLang(*it);
                    _request_target = target;
                }
            }

            if (!is_found && CHECK_ACCEPT_LANGUAGE_HEADER) {
                setStatusCode(406);
            }
        }
        else if (_request_target[_lang_file_pos] == '.')
        {
            std::string target = _request_target;
            target.insert(_lang_file_pos, DEFAULT_RESPONSE_LANGUAGE);
            _request_target = target;
        }
    }
}

void Request::appendRequestTarget(std::string & filename, const std::string &request_target) {
	if (_handling_location) {
		std::string request_substr = request_target.substr(_handling_location->getLocationPath().length());
		if (filename[filename.size() - 1] != '/') {
			if (request_substr.size() && request_substr[0] != '/')
				filename += '/';
			filename += request_substr; // _request->_request_target always starts with '/'
		} else {
			if (request_substr[0] == '/')
				filename += request_substr.substr(1); // remove '/'
			else
				filename += request_substr;
		}
	} else {
		if (filename[filename.size() - 1] != '/')
			filename += request_target; // _request->_request_target always starts with '/'
		else
			filename += request_target.substr(1); // remove '/'
	}
}

void Request::setStatusCodeNoExept(int status_code) { _status_code = status_code;}
void Request::setHandlingServer(ServerContext* handling_server) { _handling_server = handling_server;}
void Request::setHandlingLocation(LocationContext * location) { _handling_location = location;}
void Request::setCgiScriptPathForRequest(const std::string& path) { _cgi_script_path = path;}
void Request::setHostAndPort(const std::string& host, const int port) { _host = host;  _port = port;}
void Request::setReponseContentLang(const std::string& lang) { _response_content_lang = lang;}

const std::string& Request::getAbsoluteRootPathForRequest(void) const { return _absolute_root_path_for_request;}
int Request::getStatusCode() const { return _status_code;}
const std::string& Request::getCgiScriptPath(void) const { return _cgi_script_path; }

bool Request::isStatusCodeOk() const { return _status_code == 200 || _status_code == 201 || _status_code == 204; }

bool Request::isStatusCodeError() const { return _status_code >= 400; }

void Request::checkForMaxBodySize(long body_size) {
	long client_max_body_size;
	if (_handling_location) {
		client_max_body_size = _handling_location->getClientMaxBodySizeInfo();
	} else {
		client_max_body_size = _handling_server->getClientMaxBodySizeInfo();
	}

	if (client_max_body_size && (body_size > client_max_body_size)) {
		setStatusCode(413);
	}
}

void Request::writeBodyInFile() {
	int file = open(_put_filename.c_str(), O_RDWR | O_TRUNC | O_CREAT, 0666);
	if (file <= 0) {
		setStatusCode(500);
	}

	write(file, _content.c_str(), _content.size());
	_content.clear();
	close(file);
}

bool Request::_isRegFileExists(const std::string& full_filename) {
	struct stat buffer;
	return ((stat (full_filename.c_str(), &buffer) == 0) && S_ISREG(buffer.st_mode));
}

bool Request::targetIsFile() {
	struct stat info_buf;

	if (stat(_put_filename.c_str(), &info_buf) == -1) {
		std::cout << strerror(errno) << std::endl;
		setStatusCode(500);
	}

	int file_type = info_buf.st_mode & S_IFMT;

	if (file_type == S_IFREG)
		return true;
	return false;
}


bool Request::_qualityPred(const Pair<std::string, float>& first, const Pair<std::string, float>& second) {
	if (first.second >= second.second)
		return true;
	return false;
}

std::list<std::string> Request::_sortValuesByQuality(std::list<Pair<std::string, float> >& values_list) {

	std::list<std::string> sorted_strs;

	values_list.sort(_qualityPred);

	std::list<Pair<std::string, float> >::const_iterator it = values_list.begin();
	while (it != values_list.end()) {
		sorted_strs.push_back(it->first);
		++it;
	}
	return sorted_strs;
}

Pair<int, float> Request::_parseSpecificFloatValueForHeader(std::string str) {
	int i = 0;
	float value = 0;

	Pair<int, float> err = Pair<int, float>(-1, -1);

	std::size_t size = str.size();
	if ((size > 5) || (size == 0)) {
		return err;
	}

	// ===
	if (str[i] == '0') {
		i++;
	} else if (str[i] == '1') {
		value += 1;
		i++;
	} else {
		return err;
	}
	size--;

	if (!size)
		return Pair<int, float>(i, value);

	// ===
	if (str[i] != '.')
		return err;

	i++;
	size--;
	if (!size)
		return Pair<int, float>(i, value);

	// ===
	float divider = 10.0f;
	while(size) {
		if (libft::isdigit(str[i])) {
			float tmp = static_cast<float>(str[i] - '0');
			value += ( tmp / divider);
			divider *= 10.0f;
		}
		else
			return err;

		i++;
		size--;
	}
	return Pair<int, float>(i, value);
}

Pair<std::string, float> Request::_parseValueAndQuality(std::string str) {
	float quality = 1.0f;
	std::size_t q_pos = str.find(";q=");

	if (q_pos == std::string::npos) {
		return Pair<std::string, float>(str, quality);
	}

	std::string q_str = str.substr(q_pos+3);
	Pair<int, float> quality_pair = _parseSpecificFloatValueForHeader(q_str);
	if (quality_pair.first != static_cast<int>(q_str.size())) {
		quality = -1.0f;
	}
	quality = quality_pair.second;

	return Pair<std::string, float>(str.substr(0, q_pos), quality);
}
