#ifndef REQUEST_HPP
# define REQUEST_HPP

#include <string>
#include <map>
#include <set>

#include "../config_parsing/Config.hpp"
class Request;
#include "../WebServ.hpp"
#include <climits>
#include "../response/Response.hpp"

#define MAX_HEADER_LINE_LENGTH 8192 //http://nginx.org/en/docs/http/ngx_http_core_module.html#large_client_header_buffers
#define DEFAULT_REQUEST_STATUS_CODE 200

#define BUFFER_LENGHT 1024
#define TIME_OUT 20000


class Request {
	private:
		Request();

	public:

		Request(struct sockaddr_in & remote_addr, int server_port);
//		Request(const std::string& request);
		~Request(void);

		std::string & getRawRequest(void);

		void setStatusCode(int status_code);
		int getStatusCode();
		void parseRequestLine(void);
		void stringToLower(std::string & str);
		bool isStatusCodeOk();
		void parseChunkedContent();
		void getContentByLength();
		void parseHeaders(void);
		void parseBody();
//		void checkMethod();
//		void checkRequestTarget();
//		void checkHttpVersion();
//		void checkHeaders();

	private:
		static std::set<std::string> initRequestHeaders();
		static std::list<int> initOkStatusCodes(void);

	public:
		std::string _raw_request;

	public:
		static const std::set<std::string> implemented_headers;
        static const std::list<int> OK_STATUS_CODES;

	public:
		int _status_code;
		std::string _method;
		std::string _request_target;
		std::string _query_string;
		std::string _http_version;

		std::map<std::string, std::string> _headers;

		std::string _content;

		struct sockaddr_in _remote_addr;
		int _server_port;

		bool _close_connection;

//		std::string _remote_addr;

//		size_t _client_max_body_size; // need to use client_max_body_size from server config



    void setHandlingServer(ServerContext* handling_server); // Airat
    void setHandlingLocation(LocationContext* location_to_route); // Airat
    void setAbsoluteRootPathForRequest(void); // Airat

    const std::string& getAbsoluteRootPathForRequest(void) const { // Airat
        return _absolute_root_path_for_request;
    }

    const std::list<std::string>& getIndexPagesListForRequest(void) const; // Airat


	void    parsURL();

    public:
        ServerContext* _handling_server; // Airat
        LocationContext* _handling_location; // Airat
        std::string _absolute_root_path_for_request; // Airat
        bool _is_alias_path; // Airat
//		bool _is_chunked;
//		bool _is_content_lenght;
//		size_t _content_lenght;


    bool isHeaderWasRead(void) const { return _header_was_read; }
    void setHeaderWasRead(void) { _header_was_read = true; }


//    long long getHeaderContentLength(void) const {
//        std::map<std::string, std::string>::const_iterator it = _headers.find("content-length");
//        if (it == _headers.end())
//            return -1;
//        return libft::stoll_base((*it).second, 10);
//    }

    void setHeaderEndPos(std::size_t val) { _header_end_pos = val;}


    bool isMethodLimited(const std::string& method) const;
    void handleExpectHeader(void);

    void increaseReadBodySize(int bytes_read) {
        _read_body_size += bytes_read;
    }

    long long getReadBodySize(void) {
        return _read_body_size;
    }

    bool checkToClientMaxBodySize(void);

    int shift_from_buf_start;
    char _buf[BUFFER_LENGHT];
    int _bytes_read;

    std::string _full_filename;

    bool writeBodyReadBytesIntoFile() {
        int file = open(_full_filename.c_str(), O_RDWR | O_APPEND, 0666);
        if (file <= 0) {
            _status_code = 500;
            return false;
        }

        if (is_chunked)
        {
            write(file, _content.c_str(), _content.size());
            _content.clear();
        }
        else
        {
            if (shift_from_buf_start > 0)
            {
                int len = _bytes_read - shift_from_buf_start;
                write(file, _buf + shift_from_buf_start, len);
                shift_from_buf_start = 0;
            }
            else
            {
                write(file, _buf, _bytes_read);
            }
        }

        close(file);
        return true;
    }

    bool checkIsMayFileBeOpenedOrCreated(void) {
        int flags;
        if (_is_file_exists) {
            flags = O_RDWR | O_TRUNC | O_APPEND;
        }
        else {
            flags = O_RDWR | O_CREAT | O_APPEND;
        }

        int file = open(_full_filename.c_str(), flags, 0666);
        if (file <= 0) {
            _status_code = 500;
            return false;
        }
        close(file);
        return true;
    }

    bool isFileExists(void) {
        struct stat buffer;
        return (stat (_full_filename.c_str(), &buffer) == 0);
    }

    void setFileExistenceStatus(bool value) {
        _is_file_exists = value;
    }

    bool isConcreteHeaderExists(const std::string& header_name) {
        if (_headers.find(header_name) == _headers.end()) {
            return false;
        }
        return true;
    }

    bool targetIsFile(void) {
        struct stat info_buf;

        if (stat(_full_filename.c_str(), &info_buf) == -1) {
            std::cout << strerror(errno) << std::endl;
            _status_code = 500;
            return false;
        }

        int file_type = info_buf.st_mode & S_IFMT;

        if (file_type == S_IFREG) // return true if it's file
            return true;
        return false;
    }

    bool getFileExistenceStatus(void) const {
        return _is_file_exists;
    }

    void setNeedWritingBodyToFile(bool value) {
        _is_need_writing_body_to_file = value;
    }

    bool getNeedWritingBodyToFile(void) const {
        return _is_need_writing_body_to_file;
    }

    std::string getAbsolutePathForPUTRequests(void) const;


private:
	bool _isValidValue(std::string & hex_str);




private:
        std::size_t _header_end_pos;
        bool _header_was_read;
        bool _is_wokrs_with_files;
        bool _is_file_exists;
        long long _read_body_size;

        bool _is_need_writing_body_to_file;

public:
        bool is_chunked;
        bool checkToClientMaxBodySize(long long int value_to_check);


};


#endif
