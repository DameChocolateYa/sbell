/*
 *  conffile.hpp
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

#ifndef CONFFILE_HPP
#define CONFFILE_HPP

#include <iostream>

const std::string CONFFILE = std::string(getenv("HOME")) + "/.sbellrc";

std::string replaceVariableSymbol(std::string& text);

void setLine(const std::string& line, const std::string& identifier);

void setInterpreterVariable(const std::string& name, const std::string& value);

#endif
