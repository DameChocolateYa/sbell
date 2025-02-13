#include <iostream> 
#include <fstream>
#include <string>

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
            std::cerr << "Advertencia: enviroment var " << variableName << " is not defined\n";
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
        std::cerr << "export: error: cannot open conf file\n";
        return;
    }

    std::string text, line;
    bool variableFound = false;

    while (getline(readFile, line)) {
        if (line.find(identifier) != std::string::npos) {
            text += newLine + "\n";
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
