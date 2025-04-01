/*
 *  sbell.cpp
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

#include <fcntl.h>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <vector>
#include <cstdlib>
#include <sys/wait.h>
#include <termios.h>
#include <fstream>
#include <array>

#include "include/conffile.hpp"
#include "include/defs.hpp"
#include "include/translator.hpp"

std::string path_variable = getenv("PATH");

const std::string HIST_FILE = std::string(getenv("HOME")) + "/.sbell_hist";

std::vector<std::string> command_history;
int command_history_index = -1;

struct alias {
    std::string abreviated_name;
    std::string command;
};

std::vector<alias> alias_vector;

Translator t;

bool check_boolean_var(const char* variable, bool by_default=false) {
    if (getenv(variable) == nullptr) return by_default;

    return std::strcmp(getenv(variable), "true") == 0;
}

std::string replace_home_abreviation(std::string& text) {
    while (text.find("~") != std::string::npos) {
        size_t start_pos = text.find("~");
        std::string user_name = getenv("USER");
        std::string user_dir = "/home/" + user_name;
        text.replace(start_pos, 1, user_dir);
    }
    return text;
}

std::string get_unified_string(std::vector<std::string> vector, std::string separator = "") {
    std::string result;
    for (int i = 0; i < vector.size(); ++i) {
        result.append(vector[i] + separator);
    }
    return result;
}

void save_command_history(const std::string& command) {
    if (!command_history.empty() && command == command_history[command_history_index-1]) return;

    std::ofstream file(HIST_FILE, std::ios::app);
    file << command << "\n";
    file.close();
    command_history.push_back(command);
}

void load_command_history() {
    std::ifstream file(HIST_FILE);
    if (!file.is_open()) {
        save_command_history("echo 'Enjoy :)'");
        return;
    }
    std::string line;
    while (std::getline(file, line)) {
        command_history.push_back(line);
    }
    file.close();

    if (!command_history.empty()) {
    	command_history_index = command_history.size();
    }
    else {
    	command_history_index = 0;
    }
}

std::string get_current_path() {
    char cwd[1024];
    getcwd(cwd, sizeof(cwd));
    std::string pwd(cwd);

    std::string user_name = getenv("USER");
    user_name.append("");

    pwd.erase(0, pwd.find(user_name) + user_name.length());
    if (std::string(cwd).find(user_name) != std::string::npos) {
        std::string path = "(" + user_name + ")" + pwd;
        return path;
    }

    return std::string(cwd);
}


std::vector<std::string> split_command(std::string command) {
    std::vector<std::string> splited_command;
    std::string current_word;
    bool in_quotes = false;
    bool in_sub_command = false;  // To detect $()

    // Iterate every character of the stribg
    for (size_t i = 0; i < command.size(); ++i) {
        char ch = command[i];

        // Cout management
        if (ch == '"') {
            if (in_quotes) {
                splited_command.push_back(current_word); // Add word between couts
                current_word.clear();
                in_quotes = false;
            } else {
                in_quotes = true; // Init a string between couts
            }
        }
        else if (ch == ' ' && !in_quotes && !in_sub_command) {
            // If we are not inside of the coutes or sub-command $()
            if (!current_word.empty()) {
                splited_command.push_back(current_word);
                current_word.clear();
            }
        }
        else if (ch == '$' && i + 1 < command.size() && command[i + 1] == '(') {
            // We detect the sub-command init $()
            in_sub_command = true;
            current_word += ch;  // Add '$'
            i++; // Skip the '('
            current_word += command[i];  // Add '('
        }
        else if (ch == ')' && in_sub_command) {
            // We detect the close of a sub-string $()
            current_word += ch;
            splited_command.push_back(current_word); // Add sub-command
            current_word.clear();
            in_sub_command = false;
        }
        else {
            // Add characters to current word
            current_word += ch;
        }
    }

    // If there is a work which needs to be added
    if (!current_word.empty()) {
        splited_command.push_back(current_word);
    }

    return splited_command;
}

int execute_system_commands(std::vector<std::string> command) {
    std::string command_name = command[0];
    std::vector<std::string> command_path;

    for (int i = 0; i < command.size(); ++i) {
        command[i] = replace_home_abreviation(command[i]);
    }

    size_t start = 0;
    size_t end = 0;
    while ((end = path_variable.find(":", start)) != std::string::npos) {
        command_path.push_back(path_variable.substr(start, end - start));
        start = end + 1;
    }
    if (start < path_variable.size()) {
        command_path.push_back(path_variable.substr(start));
    }

   int argc = command.size();
   char** exec_args = new char*[argc + 1];
   for (int i = 0; i < argc; ++i) {
       exec_args[i] = const_cast<char*>(command[i].c_str());
   }
   exec_args[argc] = nullptr;

    bool found = false;
    std::string complete_command;
    for (const auto& dir : command_path) {
        std::string complete_path = dir + "/" + command[0];
        if (access(complete_path.c_str(), X_OK) == 0) {
            found = true;
            complete_command = complete_path;
            break;
        }
    }

    if (!found) {
        delete [] exec_args;
        return 127;
    }

    pid_t pid = fork();
    if (pid < 0) {
        std::cerr << t.get("ecmd_exec");
        delete[] exec_args;
        return 1;
    }
    else if (pid == 0) {
        execv(complete_command.c_str(), exec_args);
    }
    else {
        int status;
        waitpid(pid, &status, 0);

        delete[] exec_args;

        if (WIFEXITED(status)) {
            return WEXITSTATUS(status);
        }
        else {
            return 1;
        }
    }
    delete[] exec_args;
    return 127;
}

int execute_mega_commands(std::vector<std::string> commands) {
    std::vector<std::vector<std::string>> separated_commands;
    std::vector<std::string> current_command;

    int in_file = -1, out_file = -1;
    bool is_piped = false;

    // Separate commands based on pipes and handle redirection
    for (size_t i = 0; i < commands.size(); ++i) {
        if (commands[i] == "|") {
            separated_commands.push_back(current_command);
            current_command.clear();
            is_piped = true;
        }
        else if (commands[i] == "<" || commands[i] == ">" || commands[i] == ">>") {
            // Handle input/output redirection
            if (commands[i] == "<") {
                if (i + 1 < commands.size()) {
                    in_file = open(commands[i + 1].c_str(), O_RDONLY);
                    if (in_file == -1) {
                        perror("open");
                        return 1;
                    }
                    ++i;
                }
            } else {
                int flags = O_WRONLY | O_CREAT | (commands[i] == ">" ? O_TRUNC : O_APPEND);
                if (i + 1 < commands.size()) {
                    out_file = open(commands[i + 1].c_str(), flags, 0644);
                    if (out_file == -1) {
                        perror("open");
                        return 1;
                    }
                    ++i;
                }
            }
        }
        else {
            current_command.push_back(commands[i]);
        }
    }
    if (!current_command.empty()) {
        separated_commands.push_back(current_command);
    }

    int num_commands = separated_commands.size();
    int pipes[num_commands - 1][2];

    // Create pipes for piped commands
    for (int i = 0; i < num_commands - 1; ++i) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe");
            return 1;
        }
    }

    // Execute commands
    for (int i = 0; i < num_commands; i++) {
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            return 1;
        } else if (pid == 0) {
            // Handle input redirection
            if (in_file != -1) {
                dup2(in_file, STDIN_FILENO);
                close(in_file);
            }
            // Handle output redirection
            if (out_file != -1) {
                dup2(out_file, STDOUT_FILENO);
                close(out_file);
            }
            // Handle piping
            if (i > 0) {
                dup2(pipes[i - 1][0], STDIN_FILENO);
            }
            if (i < num_commands - 1) {
                dup2(pipes[i][1], STDOUT_FILENO);
            }

            // Close all pipes
            for (int j = 0; j < num_commands - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            // Prepare arguments for execvp
            std::vector<char*> args;
            for (auto& arg : separated_commands[i]) {
                args.push_back(const_cast<char*>(arg.c_str()));
            }
            args.push_back(nullptr);

            execvp(args[0], args.data());
            perror("execvp");
            exit(1);
        }
    }

    // Close all pipes in parent process
    for (int i = 0; i < num_commands - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    // Wait for all child processes
    for (int i = 0; i < num_commands; i++) {
        wait(nullptr);
    }

    return 0;
}

int execute_file_command(std::vector<std::string> command) {
    for (int i = 0; i < command.size(); ++i) {
        command[i] = replace_home_abreviation(command[i]);
    }
    std::string commandFile = command[0];

    char** exec_args = new char*[command.size() + 1];
    for (int i = 0; i < command.size(); ++i) {
        exec_args[i] = const_cast<char*>(command[i].c_str());
    }
    exec_args[command.size()] = nullptr;

    bool found = false;
    std::string complete_command;
    if (access(commandFile.c_str(), X_OK) == 0) {
        found = true;
    }

    if (!found) {
        delete[] exec_args;
        return 127;
    }

    pid_t pid = fork();
    if (pid < 0) {
        std::cerr << t.get("ebin_exec");
        delete[] exec_args;
        return 1;
    }
    else if (pid == 0) {
        execv(commandFile.c_str(), exec_args);
    }
    else {
        int status;
        waitpid(pid, &status, 0);

        delete[] exec_args;

        if (WIFEXITED(status)) {
            return WEXITSTATUS(status);
        }
        else {
            return 1;
        }
    }
    delete[] exec_args;
    return 127;
}

std::string get_command_return(std::string& text) {
    size_t first_delimiter = text.find("$(");
    if (first_delimiter == std::string::npos) return text;

    while (first_delimiter != std::string::npos) {
        size_t last_delimiter = text.find(")", first_delimiter + 2);
        if (last_delimiter == std::string::npos) {
            std::cerr << "Error: Falta cerrar el comando con ')'\n";
            return text; // Avoid process an incomplet command
        }

        std::string command_name = text.substr(first_delimiter + 2, last_delimiter - (first_delimiter + 2));

        // Execute command using popen()
        std::array<char, 128> buffer;
        std::string result;
        FILE* pipe = popen(command_name.c_str(), "r");

        if (!pipe) {
            std::cerr << "Error al ejecutar el comando: " << command_name << "\n";
            return text; // We return the original text without modify
        }

        while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
            result += buffer.data();
        }

        // Close correctly the pipe
        int exit_status = pclose(pipe);
        if (exit_status == -1) {
            std::cerr << "Error al cerrar el pipe para: " << command_name << "\n";
            return text;
        }

	// Remove character of the result new line
        if (!result.empty() && result.back() == '\n') {
            result.pop_back();
        }

	// Replace in the original text
        text.replace(first_delimiter, last_delimiter - first_delimiter + 1, result);

	// Search more instances of `$()` in the updated string
        first_delimiter = text.find("$(", first_delimiter + result.length());
    }

    return text;
}

int change_dir(std::vector<std::string> command) {
    if (command.size() == 1) {
        std::string new_path = "/home/";
        new_path.append(getenv("USER")).append("/");
        chdir(new_path.c_str());
        return 0;
    }
    std::string new_path = command[1];
    if (new_path.find("~") != std::string::npos) {
        size_t start_pos = new_path.find("~");
        std::string user_name = getenv("USER");
        std::string user_dir = "/home/" + user_name + "/";
        new_path.replace(start_pos, 1, user_dir);
    }
    int status = chdir(new_path.c_str());
    return status;
}

void set_raw_mode(bool enable) {
    struct termios term;
    tcgetattr(STDIN_FILENO, &term);

    if (enable) {
        term.c_lflag &= ~(ICANON | ECHO);
        term.c_cc[VMIN] = 1;
        term.c_cc[VTIME] = 0;
    }
    else {
        term.c_lflag |= (ICANON | ECHO);
    }

    tcsetattr(STDIN_FILENO, TCSANOW, &term);
}

std::string read_command() {
    std::string path = get_current_path();
    std::cout << path << " ~~> ";


    set_raw_mode(true);
    std::string input;
    char ch;
    int cursor_pos = 0;
    command_history_index = command_history.size();

    while (true) { // NOTE: SPAGHETTI LOOP
        ch = getchar();
        if (ch == '\n') break;

        if (ch == 27) {
            char next = getchar();
            if (next == 91) {
                char arrow = getchar();
                switch (arrow) {
                    case 'C':
                        if (cursor_pos < input.size()) {
                            cursor_pos++;
                            std::cout << "\033[C";
                        }
                        break;
                    case 'D':
                        if (cursor_pos > 0) {
                            cursor_pos--;
                            std::cout << "\033[D";
                        }
                        break;
                    case 'A':
                        if (command_history_index > 0) {
                            command_history_index--;
                            input = command_history[command_history_index];
                            cursor_pos = input.size();
                            std::cout << "\r" << path << " ~~> " << input << "\033[K";

                            if (cursor_pos < input.size()) {
                                std::cout << "\033[" << (input.size() - cursor_pos) << "D";
                            }
                            std::cout.flush();
                        }
                        break;
                    case 'B':
                        if (command_history_index < command_history.size()) {
                            command_history_index++;
                            input = (command_history_index < command_history.size()) ? command_history[command_history_index] : "";
                            cursor_pos = input.size();
                            std::cout << "\r" << path << " ~~> " << input << "\033[K";

                            if (cursor_pos < input.size()) {
                                std::cout << "\033[" << (input.size() - cursor_pos) << "D";
                            }
                            std::cout.flush();
                        }
                }
            }
        }
        else if (ch == 127 || ch == 8) {
            if (cursor_pos > 0) {
                input.erase(cursor_pos-1, 1);
                cursor_pos--;

                std::cout << "\r" << path << " ~~> " << "\033[K";
                std::cout.flush();

                std::cout << input;
                std::cout.flush();

                if (cursor_pos < input.size()) {
                    std::cout << "\033[" << (input.size() - cursor_pos) << "D";
                }
                std::cout.flush();
            }
            else {
                if (check_boolean_var("SBELL_BEEP", true)) std::cout << "\a" << std::flush;
            }
        }
        else {
            input.insert(cursor_pos, 1, ch);
            cursor_pos++;

            std::cout << "\r" << path << " ~~> " << "\033[K";
            std::cout.flush();

            std::cout << input;
            std::cout.flush();

            if (cursor_pos < input.size()) {
                std::cout << "\033[" << (input.size() - cursor_pos) << "D";
            }
        }
    }
    set_raw_mode(false);
    std::cout << "\n";
    return input;
}

void signal_handler(int signum) {
    //(void*)0;
}

int execute_interpreter_commands(std::vector<std::string> command) { //NOTE: SPAGHETTI CODE (PLS, FIX IT AS SOON AS POSSIBLE)
    if (command[0] == "exit") {
        if (command.size() == 1) exit(0);
            try {
                exit(std::stoi(command[1]));
            }
            catch (const std::exception& e) {
                exit(1);
            }
        }
    else if (command[0] == "cd") {
        if (change_dir(command) != 0) {
            std::cout << t.get("ecd");
        };
        return 0;
    }
    else if (command[0] == "export") {
        bool export_in_file = false;
        if (command.size() < 3) {
            std::cerr << "export: error: required at least 2 args\n";
            return 1;
        }
        if (command.size() > 3) {
            if (command[3] == "--all-sessions") export_in_file = true;
        }
        setenv(command[1].c_str(), command[2].c_str(), 1);

        if (export_in_file) {
            set_line("export  " + command[1] + " " + command[2], command[1]);
        }

        return 0;
    }
    else if (command[0] == "alias") {
        bool export_in_conf_file = false;
        if (command.size() == 1) {
            for (const auto& current_alias : alias_vector) {
                std::cerr << current_alias.abreviated_name << "='" << current_alias.command << "'\n";
            }
            return 0;
        }

        if (command.size() == 2) {
            std::cerr << "alias: error: required at least 2 args\n";
        }

	    std::string alias_args;
	    for (int i = 2; i < command.size(); ++i) {
		    if (command[i] == "--all-sessions") {
			    export_in_conf_file = true;
			    break;
		    }
		    alias_args.append(command[i] + " ");
	    }

        alias_vector.push_back(alias{command[1], alias_args});
        if (export_in_conf_file) {
            set_line("alias " + command[1] + " " + alias_args, command[1]);
        }
        return 0;
    }
    else if (command[0] == "rmhist") {
        command_history.clear();
        std::ofstream file(HIST_FILE, std::ofstream::out | std::ios::trunc);
        file.close();
	return 0;
    }
    else if (command[0] == "exec" || command[0] == "./") {
	if (command.size() < 2) {
	    std::cerr << "exec" << t.get("c:arg1");
	    return -1;
	}
        std::vector<std::string> to_execute;
        for (int i = 1; i < command.size(); ++i) {
            to_execute.push_back(command[i]);
        }
        return execute_file_command(to_execute);
    }
    return 5;
}

int executeAlias(std::string alias_name) {
    for (const auto& current_alias : alias_vector) {
        if (current_alias.abreviated_name == alias_name) {
            std::vector<std::string> command = split_command(current_alias.command);

            if (command.empty()) continue;

            if (execute_interpreter_commands(command) != 5) continue;

            for (int i = 0; i < command.size(); ++i) {
                command[i] = replace_variable_symbol(command[i]);
                command[i] = get_command_return(command[i]);
            }

            int status = execute_system_commands(command);
            if (status == 127) {
                std::cerr << "command: " << command[0] << " not found\n";
            }
            return status;
            if (status == 0) continue;
            std::cout << status << "\n";
        }
    }
    return 5;
}

void read_conf_file() {
    std::ifstream file(CONF_FILE);
    if (!file.is_open()) {
        std::ofstream first_file(CONF_FILE);
        first_file << "// SBELL CONFIG FILE !/\n";
        first_file << "// THIS MAKE THE WHOLE LINE BE A COMMENT, YOU CAN UNCOMMENT NEXT LINES !/\n";
        first_file << "// export SBELL_WELCOME false !/\n";
        first_file << "// export SBELL_WELCOMEMSG 'Message' !/\n";
        first_file << "// export SBELL_BEEP false !/\n";
        first_file << "// export SBELL_LANGDIR /path/to/your/custom/lang/dir/";
        return;
    }
    std::string line;

    while (getline(file, line)) {
	if (line.find("//") != std::string::npos) {
	    size_t first_comment_sep = line.find("//");
	    if (line.find("!/") != std::string::npos) {
	        size_t last_comment_sep = line.find("!/");
		line.erase(first_comment_sep, last_comment_sep + 2);
	    }
	    else {
	        line.erase(first_comment_sep);
	    }
	}

        std::vector<std::string> command = split_command(line);
        if (command.empty()) continue;

        for (int i = 0; i < command.size(); ++i) {
            command[i] =  replace_variable_symbol(command[i]);
            command[i] = get_command_return(command[i]);
	    command[i] = replace_home_abreviation(command[i]);
        }

        if (execute_interpreter_commands(command) != 5) continue;
	if (executeAlias(command[0]) != 5) continue;

        int status = execute_system_commands(command);
        if (status == 127) {
            std::cerr << t.get("e127") << command[0] << "\n";
        }
        if (status == 0) continue;
        std::cout << status << "\n";
    }
}

struct command_with_uniter {
    std::string command;
    std::string uniter = "";
};

std::vector<std::string> splite_line_in_commands(const std::string line) {
    std::vector<std::string> splited_line;

    size_t start = 0;
    size_t end = line.find("&&");

    while (end != std::string::npos) {
        splited_line.push_back(line.substr(start, end - start));
        start = end + 2;
        end = line.find("&&", start);
    }
    splited_line.push_back(line.substr(start));
    return splited_line;
}

int main(int argc, char **argv) {
    signal(SIGINT, signal_handler);
    signal(SIGTSTP, signal_handler);

    set_interpreter_variable("CURRENT_SHELL", defs::sbell::shell);
    set_interpreter_variable("SBELL_AUTHOR", defs::sbell::author);
    set_interpreter_variable("SBELL_VERSION", defs::sbell::version);
    set_interpreter_variable("SBELL_LICENSE", defs::sbell::license);
    set_interpreter_variable("ILOVELINUX", "Me too :3");
    set_interpreter_variable("SBELL_LANGDIR", "/etc/sbell/lang/");
    set_interpreter_variable("SBELL_WEB", defs::sbell::web);

    read_conf_file();
    t = Translator(getenv("SBELL_LANGDIR"));

    if (argc > 1) {
    	change_dir({"cd ", argv[1]});
    }

    if (check_boolean_var("SBELL_WELCOME", true)) {
        if (getenv("SBELL_WELCOMEMSG") == nullptr) {
            std::cerr << t.get("welcome");
        }
        else {
            std::cerr << getenv("SBELL_WELCOMEMSG") << "\n";
        }
    }

    load_command_history();


    while (true) {
        t = Translator(getenv("SBELL_LANGDIR"));
        path_variable = getenv("PATH");
        std::cout << "\033[0m";
        std::string input;
        input = read_command();
        if (input.empty()) continue;
        std::vector<std::string> splited_line = splite_line_in_commands(input);
        if (splited_line.empty()) continue;
        if (check_boolean_var("SBELL_SAVEHIST"), true) {
            save_command_history(input);
        }

        for (const auto& element : splited_line) {
            std::vector<std::string> command = split_command(element);
            if (command.empty()) continue;
            setenv("HIST", get_unified_string(command_history, "\n").c_str(), 1); //FIXME: FATAL BUG, PLS DONT EXECUTE `command + $:HIST$`

            for (int i = 0; i < command.size(); ++i) {
                command[i] = replace_variable_symbol(command[i]);
                command[i] = get_command_return(command[i]);
            }

            bool commandExecuted = false;

            for (const auto& arg : command) {
                if (arg.find(">") != std::string::npos ||
                    arg.find(">>") != std::string::npos ||
                    arg.find("<") != std::string::npos ||
                    arg.find("|") != std::string::npos) {
                        execute_mega_commands(command);
                        commandExecuted = true;
                    }
            }
            if (commandExecuted) continue;

            if (execute_interpreter_commands(command) != 5) continue;
            if (executeAlias(command[0]) != 5) continue;

            int status = execute_system_commands(command);
            if (status == 127) {
                std::cerr << t.get("e127") << command[0] << "\n";
            }
            if (status == 0) continue;
            std::cout << status << "\n";
        }
    }

    std::cout << "Goodbye!\n";
    return 0;
}
