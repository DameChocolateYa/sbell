/*
 *  defs.hpp
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

#ifndef DEFS_HPP
#define DEFS_HPP

#include <iostream>

namespace defs {
    namespace sbell {
        const std::string version = "v1.1.1";
	const std::string name = "SBELL";
	const std::string shell = name + " " + version;
	const std::string author = "DameChocolateYa";
        const std::string license = "The GNU GENERAL LICENSE v3.0 (GPL3)";
        const std::string yearOfCreation = "2025";
        const std::string yearOfLastUpdate = "2025";
	const std::string web = "https://github.com/DameChocolateYa/sbell"
    }
}

#endif
