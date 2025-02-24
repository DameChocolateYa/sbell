#ifndef TRANSLATOR_HPP
#define TRANSLATOR_HPP

#include "json.hpp"

using json = nlohmann::json;

class Translator {
    std::unordered_map<std::string, std::string> messages;

public:
    bool loadLanguage(const std::string& lang);
    std::string get(const std::string& key);
}

#endif
