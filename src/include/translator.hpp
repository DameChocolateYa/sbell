#ifndef TRANSLATOR_HPP
#define TRANSLATOR_HPP

#include <iostream>
#include <unordered_map>

class Translator {
    std::unordered_map<std::string, std::string> messages;

public:
    Translator();
    ~Translator();

    std::string get(const std::string& key);
private:
    bool loadLanguage(const std::string& lang);
};

#endif
