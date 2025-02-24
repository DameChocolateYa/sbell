#include "include/json.hpp"

#include <iostream>
#include <fstream>
#include <unordered_map>

using json = nlohmann::json;

bool Translator::loadLanguage(const std::string& lang) {
    std::ifstream file("lang" + lang + ".json");
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
