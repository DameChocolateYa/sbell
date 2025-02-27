#include <iostream>
#include <fstream>
#include <unordered_map>
#include <nlohmann/json.hpp>
#include <sstream>

#include "include/translator.hpp"

using json = nlohmann::json;

Translator::Translator() {
    std::string lang(getenv("LANG"));
    std::string token = lang.substr(0, lang.find("_"));
    loadLanguage(token);
}

Translator::~Translator() {}

bool Translator::loadLanguage(const std::string& lang) {
    std::ifstream file("/etc/sbell/lang/" + lang + ".json");
    if (!file) {
        std::cerr << "Error: cannot load lang\n";
	return false;
    }
    json j;
    file >> j;
    for (auto& [key, value] : j.items()) {
        messages[key] = value;
    }
    return true;
}

std::string Translator::get(const std::string& key) {
    return messages.count(key) ? messages[key] : "??";
}
