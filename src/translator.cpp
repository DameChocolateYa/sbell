/*
 *  This file is part of Sbell Interpreter.
 *
 *  Sbell is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.

 *   Sbell is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.

 *   You should have received a copy of the GNU General Public License
 *   along with Sbell.  If not, see <http://www.gnu.org/licenses/>.
 */

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
