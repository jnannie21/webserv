//
// Created by airat on 09.02.2021.
//

#include "LocationContext.hpp"

LocationContext::LocationContext(const std::string& uri) : _uri(uri) {

}

void LocationContext::setErrorPageDirectiveInfo(const std::map<int, std::map<std::string, std::string> >& error_page_info) {
    if (!_is_error_pages_info_was_updated) {
        _is_error_pages_info_was_updated = true;
        _error_pages_info.clear();
    }

    AContext::setErrorPageDirectiveInfo(error_page_info);
}

