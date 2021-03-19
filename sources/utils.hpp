//
// Created by airat on 04.02.2021.
//

#ifndef WEBSERV_UTILS_H
#define WEBSERV_UTILS_H

#include <string>


namespace utils {
    void exitWithLog(void);
    void exitWithLog(const std::string& errorLog);
    bool isHttpMethod(const std::string& value);

    template <class Container, class Value>
    bool is_exists_in_container(const Container& container_to_search_in, const Value& value_to_search) {
        if (container_to_search_in.find(value_to_search) == container_to_search_in.end()) {
            return false;
        }
        return true;
    }
}

#endif //WEBSERV_UTILS_H
