#ifndef CONFFILE_HPP
#define CONFFILE_HPP

#include <iostream>

const std::string CONFFILE = std::string(getenv("HOME")) + "/.sbellrc";

std::string replaceVariableSymbol(std::string& text);

void setLine(const std::string& line, const std::string& identifier);

void setInterpreterVariable(const std::string& name, const std::string& value);

#endif
