/*
 *  This file is part of Sbell Interpreter.

 *  Sbell is free software: you can redistribute it and/or modify it under the terms of the
 *  GNU General Public License as published by the Free Software Foundation, either version 3 of the License,
 *  or (at your option) any later version.

 *  This software is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *  See the GNU General Public License for more details.

 *  You should have received a copy of the GNU General Public License along with this program.
 *  If not, see <https://www.gnu.org/licenses/>.
*/

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include "include/conffile.hpp"

std::string replaceVariableSymbol(std::string& text) {
    size_t firstDelimiter = text.find("$:");

    while (firstDelimiter != std::string::npos) {
        size_t lastDelimiter = text.find("$", firstDelimiter + 2);
        if (lastDelimiter == std::string::npos) {
            std::cerr << "Error: cannot find $ close var\n";
            break;
        }

        std::string variableName = text.substr(firstDelimiter + 2, lastDelimiter - (firstDelimiter + 2));

        const char* variableValue = getenv(variableName.c_str());
        if (variableValue == nullptr) {
            std::cerr << "Warning: enviroment var " << variableName << " is not defined\n";
            variableValue = "";
        }

        text.replace(firstDelimiter, lastDelimiter - firstDelimiter + 1, variableValue);

        firstDelimiter = text.find("$:", firstDelimiter + 1);
    }

    return text;
}

void setLine(const std::string& newLine, const std::string& identifier) {
    std::ifstream readFile(CONFFILE);
    if (!readFile.is_open()) {
        std::ofstream file(CONFFILE);
        file << "";
        file.close();
    }

    std::string text, line;
    bool variableFound = false;

    while (getline(readFile, line)) {
        if (line.find(identifier) != std::string::npos) {
            text += newLine + "\n";
            variableFound = true;
            continue;
        }
        text += line + "\n";
    }

    if (!variableFound) text += newLine + "\n";

    readFile.close();

    std::ofstream writeFile(CONFFILE, std::ios::trunc);
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

void setInterpreterVariable(const std::string& name, const std::string& value) {
    for(int i = 0; i < vars.size(); ++i) {
        if (vars[i].name == name) {
	    vars.erase(vars.begin()+i);
	}
    }
    vars.push_back(var{name, value});
    setenv(name.c_str(), value.c_str(), 1);
}
