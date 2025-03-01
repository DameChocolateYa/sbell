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
