
/*
 *  installer.cpp
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

#include "ftxui/dom/elements.hpp"
#include <cstdlib>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>

#include <ftxui/dom/elements.hpp>
#include <iostream>

int main() {
    using namespace ftxui;

    std::string msg = "Do you want to install sbell?";
    bool exit_program = false;

    auto screen = ScreenInteractive::TerminalOutput();


    auto bt_continue_to_install_yes = Button(
        "Install",
        [&] {
            msg = "Installation in progres (compiling)...\n";
            screen.ExitLoopClosure()();
        }
    );
    auto bt_continue_to_install_no = Button(
        "Abort",
        [&] {
            std::cout << "Aborting...\n";
            exit_program = true;
            screen.ExitLoopClosure()();
        }
    );
    auto continue_buttons = Container::Horizontal({bt_continue_to_install_yes, bt_continue_to_install_no});

    auto component_continue_installation = Renderer(continue_buttons, [&] {
        return vbox({
            text("Sbell Installer"),
            separator(),
            continue_buttons->Render(),
            separator(),
            text(msg),
        }) | border;
    });

    screen.Loop(component_continue_installation);

    if (exit_program) {
        return -1;
    }

    if (system("which git > /dev/null 2>&1")) {
        std::cout << "Missing git\n";
        return -1;
    }

    if (system("git clone https://github.com/DameChocolateYa/sbell.git") != 0) {
        std::cout << "Failed to clone sbell repository\n";
        return -1;
    }

    if (system("which cmake > /dev/null 2>&1")) {
        std::cout << "Missing cmake\n";
        return -1;
    }
    if (system("which make > /dev/null 2>&1")) {
        std::cout << "Missing make\n";
    }
    if (system("mkdir -p sbell/build") != 0) {
        std::cerr << "Error creating building dir\n";
        return -1;
    }
    if (system("pkg-config --exists nlohmann_json") != 0) {
        std::cerr << "Missing nlohmann-json\n";
        return -1;
    }
    if (system("cd sbell/build && cmake ..") != 0) {
        std::cerr << "Error with cmake\n";
        return -1;
    }
    if (system("cd sbell/build && make") != 0) {
        std::cerr << "Error with make\n";
        return -1;
    }

    auto bt_continue_moving_binary_yes = Button(
        "Yes (recommended)",
        [&] {
            msg = "Proceding moving binary file...";
            screen.ExitLoopClosure()();
        }
    );
    auto bt_continue_moving_binary_no = Button(
        "No",
        [&] {
            exit_program = true;
            msg = "Stopping here...";
            screen.ExitLoopClosure()();
        }
    );
    auto continue_moving_binary_file_buttons = Container::Horizontal({bt_continue_moving_binary_yes, bt_continue_moving_binary_no});

    auto component_continue_moving_binary_file = Renderer(
        continue_moving_binary_file_buttons,
        [&] {
            return vbox({
                text("Recommended, it will move binary file to a global place"),
                separator(),
                continue_moving_binary_file_buttons->Render(),
                separator(),
                text(msg),
            }) | border;
        }
    );
    screen.Loop(component_continue_moving_binary_file);

    if (exit_program) {
        return 0;
    }

    if (system("mv sbell/build/sbell /usr/bin/") != 0) {
        std::cerr << "Error moving binary file\n";
        return -1;
    }
    if (system("rm -fr sbell/build") != 0) {
        std::cerr << "\033[1;33mWARNING: cannot remove build directory, the installation will continue...\033[0m\n";
    }

    auto bt_move_lang_files_yes = Button(
        "Yes (recommended)",
        [&] {
            msg = "Proceding moving lang files...\n";
            screen.ExitLoopClosure()();
        }
    );
    auto bt_move_lang_files_no = Button(
        "No",
        [&] {
            msg = "Stopping here...\n";
            exit_program = true;
            screen.ExitLoopClosure()();
        }
    );
    auto move_lang_files_button = Container::Horizontal({bt_move_lang_files_yes, bt_move_lang_files_no});

    auto component_move_lang_files = Renderer(
            move_lang_files_button,
            [&] {
                return vbox({
                    text("Recommended, it will place the lang files for sbell traductions"),
                    separator(),
                    move_lang_files_button->Render(),
                    separator(),
                    text(msg),
                }) | border;
            }
    );

    screen.Loop(component_move_lang_files);

    if (exit_program) {
        return 0;
    }

    if (system("mkdir -p /etc/sbell/") != 0) {
        std::cerr << "Error creating sbell config directory\n";
        return -1;
    }
    if (system("cp -r sbell/lang/ /etc/sbell/") != 0) {
        std::cerr << "Error copying lang files\n";
        return -1;
    }

    if (system("rm -fr sbell") != 0) {
        std::cerr << "\033[1;33mWARNING: cannot remove local folder of sbell repository, installation will continue, but you should remove manually the firectory\n";
    }

    std::cout << "Installation finished\n";
    return 0;
}
