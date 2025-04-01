/*
 *  conffile.cpp
 *
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
#include <string>
#include <vector>

#include "conffile.hpp"

std::string replace_variable_symbol(std::string& text) {
    size_t first_delimiter = text.find("$:");

    while (first_delimiter != std::string::npos) {
        size_t last_delimiter = text.find("$", first_delimiter + 2);
        if (last_delimiter == std::string::npos) {
            std::cerr << "Error: cannot find $ close var\n";
            break;
        }

        std::string variable_name = text.substr(first_delimiter + 2, last_delimiter - (first_delimiter + 2));

        const char* variableValue = getenv(variable_name.c_str());
        if (variableValue == nullptr) {
            std::cerr << "Warning: enviroment var " << variable_name << " is not defined\n";
            variableValue = "";
        }

        text.replace(first_delimiter, last_delimiter - first_delimiter + 1, variableValue);

        first_delimiter = text.find("$:", first_delimiter + 1);
    }

    return text;
}

void set_line(const std::string& new_line, const std::string& identifier) {
    std::ifstream read_file(CONF_FILE);
    if (!read_file.is_open()) {
        std::ofstream file(CONF_FILE);
        file << "";
        file.close();
    }

    std::string text, line;
    bool variable_found = false;

    while (getline(read_file, line)) {
        if (line.find(identifier) != std::string::npos) {
            text += new_line + "\n";
            variable_found = true;
            continue;
        }
        text += line + "\n";
    }

    if (!variable_found) text += new_line + "\n";

    read_file.close();

    std::ofstream writeFile(CONF_FILE, std::ios::trunc);
    if (!writeFile.is_open()) {
        std::cerr << "export: error: cannot open conf file\n";
        return;
    }

    writeFile << text;
    writeFile.close();
}

struct var {
   std::string name;
   std::string value;
};

std::vector<var> vars;

void set_interpreter_variable(const std::string& name, const std::string& value) {
    for(int i = 0; i < vars.size(); ++i) {
        if (vars[i].name == name) {
	    vars.erase(vars.begin()+i);
	}
    }
    vars.push_back(var{name, value});
    setenv(name.c_str(), value.c_str(), 1);
}
