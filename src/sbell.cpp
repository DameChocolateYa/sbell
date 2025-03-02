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

std::string pathVariable = getenv("PATH");

const std::string HISTFILE = std::string(getenv("HOME")) + "/.sbell_hist";

std::vector<std::string> commandHistory;
int commandHistoryIndex = -1;

struct alias {
    std::string abreviatedName;
    std::string command;
};

std::vector<alias> aliasVector;

Translator t;

bool checkBooleanVar(const char* variable, bool byDefault=false) {
    if (getenv(variable) == nullptr) return byDefault;

    return std::strcmp(getenv(variable), "true") == 0;
}

std::string replaceHomeAbreviation(std::string& text) {
    while (text.find("~") != std::string::npos) {
        size_t start_pos = text.find("~");
        std::string username = getenv("USER");
        std::string userDir = "/home/" + username;
        text.replace(start_pos, 1, userDir);
    }
    return text;
}

std::string getUnifiedString(std::vector<std::string> vector, std::string separator = "") {
    std::string result;
    for (int i = 0; i < vector.size(); ++i) {
        result.append(vector[i] + separator);
    }
    return result;
}

void saveCommandHistory(const std::string& command) {
    if (!commandHistory.empty() && command == commandHistory[commandHistoryIndex-1]) return;

    std::ofstream file(HISTFILE, std::ios::app);
    file << command << "\n";
    file.close();
    commandHistory.push_back(command);
}

void loadCommandHistory() {
    std::ifstream file(HISTFILE);
    if (!file.is_open()) {
        saveCommandHistory("echo 'Enjoy :)'");
        return;
    }
    std::string line;
    while (std::getline(file, line)) {
        commandHistory.push_back(line);
    }
    file.close();

    if (!commandHistory.empty()) {
    	commandHistoryIndex = commandHistory.size();
    }
    else {
    	commandHistoryIndex = 0;
    }
}

std::string getCurrentPath() {
    char cwd[1024];
    getcwd(cwd, sizeof(cwd));
    std::string pwd(cwd);

    std::string username = getenv("USER");
    username.append("");

    pwd.erase(0, pwd.find(username) + username.length());
    if (std::string(cwd).find(username) != std::string::npos) {
         std::string path = "(" + username + ")" + pwd;
         return path;
    }

    return std::string(cwd);
}


std::vector<std::string> splitCommand(std::string command) {
    std::vector<std::string> splitedCommand;
    std::string currentWord;
    bool inQuotes = false;
    bool inSubCommand = false;  // Para detectar $()

    // Iterar sobre cada carácter de la cadena
    for (size_t i = 0; i < command.size(); ++i) {
        char ch = command[i];

        // Manejo de comillas
        if (ch == '"') {
            if (inQuotes) {
                splitedCommand.push_back(currentWord); // Agregar palabra entre comillas
                currentWord.clear();
                inQuotes = false;
            } else {
                inQuotes = true; // Iniciar una cadena entre comillas
            }
        }
        else if (ch == ' ' && !inQuotes && !inSubCommand) {
            // Si no estamos dentro de comillas ni en una subcomando $()
            if (!currentWord.empty()) {
                splitedCommand.push_back(currentWord);
                currentWord.clear();
            }
        }
        else if (ch == '$' && i + 1 < command.size() && command[i + 1] == '(') {
            // Detectamos el inicio de una subcadena $()
            inSubCommand = true;
            currentWord += ch;  // Agregar '$'
            i++; // Saltamos el '('
            currentWord += command[i];  // Agregar '('
        }
        else if (ch == ')' && inSubCommand) {
            // Detectamos el cierre de una subcadena $()
            currentWord += ch;
            splitedCommand.push_back(currentWord); // Agregar subcomando
            currentWord.clear();
            inSubCommand = false;
        }
        else {
            // Agregar caracteres a la palabra actual
            currentWord += ch;
        }
    }

    // Si hay una palabra pendiente por agregar
    if (!currentWord.empty()) {
        splitedCommand.push_back(currentWord);
    }

    return splitedCommand;
}

int executeSystemCommand(std::vector<std::string> command) {
    std::string commandName = command[0];
    std::vector<std::string> commandPath;

    for (int i = 0; i < command.size(); ++i) {
        command[i] = replaceHomeAbreviation(command[i]);
    }

    size_t start = 0;
    size_t end = 0;
    while ((end = pathVariable.find(":", start)) != std::string::npos) {
        commandPath.push_back(pathVariable.substr(start, end - start));
        start = end + 1;
    }
    if (start < pathVariable.size()) {
        commandPath.push_back(pathVariable.substr(start));
    }

   int argc = command.size();
   char** execArgs = new char*[argc + 1];
   for (int i = 0; i < argc; ++i) {
       execArgs[i] = const_cast<char*>(command[i].c_str());
   }
   execArgs[argc] = nullptr;

    bool found = false;
    std::string completeCommand;
    for (const auto& dir : commandPath) {
        std::string completePath = dir + "/" + command[0];
        if (access(completePath.c_str(), X_OK) == 0) {
            found = true;
            completeCommand = completePath;
            break;
        }
    }

    if (!found) {
        delete [] execArgs;
        return 127;
    }

    pid_t pid = fork();
    if (pid < 0) {
        std::cerr << t.get("ecmd_exec");
        delete[] execArgs;
        return 1;
    }
    else if (pid == 0) {
        execv(completeCommand.c_str(), execArgs);
    }
    else {
        int status;
        waitpid(pid, &status, 0);

        delete[] execArgs;

        if (WIFEXITED(status)) {
            return WEXITSTATUS(status);
        }
        else {
            return 1;
        }
    }
    delete[] execArgs;
    return 127;
}

int executeMegaCommands(std::vector<std::string> commands) {
    std::vector<std::vector<std::string>> separatedCommands;
    std::vector<std::string> currentCommand;

    int inFile = -1, outFile = -1;
    bool isPiped = false;

    bool isBackground = false;

    if (commands.back() == "&") {
        isBackground = true;
        commands.pop_back();
    }

    // Separate commands based on pipes and handle redirection
    for (size_t i = 0; i < commands.size(); ++i) {
        if (commands[i] == "|") {
            separatedCommands.push_back(currentCommand);
            currentCommand.clear();
            isPiped = true;
        }
        else if (commands[i] == "<" || commands[i] == ">" || commands[i] == ">>") {
            // Handle input/output redirection
            if (commands[i] == "<") {
                if (i + 1 < commands.size()) {
                    inFile = open(commands[i + 1].c_str(), O_RDONLY);
                    if (inFile == -1) {
                        perror("open");
                        return 1;
                    }
                    ++i;
                }
            } else {
                int flags = O_WRONLY | O_CREAT | (commands[i] == ">" ? O_TRUNC : O_APPEND);
                if (i + 1 < commands.size()) {
                    outFile = open(commands[i + 1].c_str(), flags, 0644);
                    if (outFile == -1) {
                        perror("open");
                        return 1;
                    }
                    ++i;
                }
            }
        }
        else {
            currentCommand.push_back(commands[i]);
        }
    }
    if (!currentCommand.empty()) {
        separatedCommands.push_back(currentCommand);
    }

    int numCommands = separatedCommands.size();
    int pipes[numCommands - 1][2];

    // Create pipes for piped commands
    for (int i = 0; i < numCommands - 1; ++i) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe");
            return 1;
        }
    }

    // Execute commands
    for (int i = 0; i < numCommands; i++) {
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            return 1;
        } else if (pid == 0) {
            // Handle input redirection
            if (inFile != -1) {
                dup2(inFile, STDIN_FILENO);
                close(inFile);
            }
            // Handle output redirection
            if (outFile != -1) {
                dup2(outFile, STDOUT_FILENO);
                close(outFile);
            }
            // Handle piping
            if (i > 0) {
                dup2(pipes[i - 1][0], STDIN_FILENO);
            }
            if (i < numCommands - 1) {
                dup2(pipes[i][1], STDOUT_FILENO);
            }

            // Close all pipes
            for (int j = 0; j < numCommands - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            // Prepare arguments for execvp
            std::vector<char*> args;
            for (auto& arg : separatedCommands[i]) {
                args.push_back(const_cast<char*>(arg.c_str()));
            }
            args.push_back(nullptr);

            execvp(args[0], args.data());
            perror("execvp");
            exit(1);
        }
        else if (!isBackground) {
            wait(nullptr);
        }
    }

    // Close all pipes in parent process
    for (int i = 0; i < numCommands - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    // Wait for all child processes
    for (int i = 0; i < numCommands; i++) {
        wait(nullptr);
    }

    return 0;
}

int executeFileCommand(std::vector<std::string> command) {
    for (int i = 0; i < command.size(); ++i) {
        command[i] = replaceHomeAbreviation(command[i]);
    }
    std::string commandFile = command[0];

    char** execArgs = new char*[command.size() + 1];
    for (int i = 0; i < command.size(); ++i) {
        execArgs[i] = const_cast<char*>(command[i].c_str());
    }
    execArgs[command.size()] = nullptr;

    bool found = false;
    std::string completeCommand;
    if (access(commandFile.c_str(), X_OK) == 0) {
        found = true;
    }

    if (!found) {
        delete[] execArgs;
        return 127;
    }

    pid_t pid = fork();
    if (pid < 0) {
        std::cerr << t.get("ebin_exec");
        delete[] execArgs;
        return 1;
    }
    else if (pid == 0) {
        execv(commandFile.c_str(), execArgs);
    }
    else {
        int status;
        waitpid(pid, &status, 0);

        delete[] execArgs;

        if (WIFEXITED(status)) {
            return WEXITSTATUS(status);
        }
        else {
            return 1;
        }
    }
    delete[] execArgs;
    return 127;
}

std::string getCommandReturn(std::string& text) {
    size_t firstDelimiter = text.find("$(");
    if (firstDelimiter == std::string::npos) return text;

    while (firstDelimiter != std::string::npos) {
        size_t lastDelimiter = text.find(")", firstDelimiter + 2);
        if (lastDelimiter == std::string::npos) {
            std::cerr << "Error: Falta cerrar el comando con ')'\n";
            return text; // Evitamos procesar un comando incompleto
        }

        std::string commandName = text.substr(firstDelimiter + 2, lastDelimiter - (firstDelimiter + 2));

        // Ejecutar el comando usando popen()
        std::array<char, 128> buffer;
        std::string result;
        FILE* pipe = popen(commandName.c_str(), "r");

        if (!pipe) {
            std::cerr << "Error al ejecutar el comando: " << commandName << "\n";
            return text; // Devolvemos el texto original sin modificar
        }

        while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
            result += buffer.data();
        }

        // Cerrar el pipe correctamente
        int exitStatus = pclose(pipe);
        if (exitStatus == -1) {
            std::cerr << "Error al cerrar el pipe para: " << commandName << "\n";
            return text;
        }

        // Remover caracteres de nueva línea del resultado
        if (!result.empty() && result.back() == '\n') {
            result.pop_back();
        }

        // Reemplazar en el texto original
        text.replace(firstDelimiter, lastDelimiter - firstDelimiter + 1, result);

        // Buscar más ocurrencias de `$()` en la cadena actualizada
        firstDelimiter = text.find("$(", firstDelimiter + result.length());
    }

    return text;
}

int changeDir(std::vector<std::string> command) {
    if (command.size() == 1) {
        std::string newPath = "/home/";
        newPath.append(getenv("USER")).append("/");
        chdir(newPath.c_str());
        return 0;
    }
    std::string newPath = command[1];
    if (newPath.find("~") != std::string::npos) {
        size_t start_pos = newPath.find("~");
        std::string username = getenv("USER");
        std::string userDir = "/home/" + username + "/";
        newPath.replace(start_pos, 1, userDir);
    }
    int status = chdir(newPath.c_str());
    return status;
}

void setRawMode(bool enable) {
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

std::string readCommand() {
    std::string path = getCurrentPath();
    std::cout << path << " ~~> ";


    setRawMode(true);
    std::string input;
    char ch;
    int cursorPos = 0;
    commandHistoryIndex = commandHistory.size();

    while (true) {
        ch = getchar();
        if (ch == '\n') break;

        if (ch == 27) {
            char next = getchar();
            if (next == 91) {
                char arrow = getchar();
                switch (arrow) {
                    case 'C':
                        if (cursorPos < input.size()) {
                            cursorPos++;
                            std::cout << "\033[C";
                        }
                        break;
                    case 'D':
                        if (cursorPos > 0) {
                            cursorPos--;
                            std::cout << "\033[D";
                        }
                        break;
                    case 'A':
                        if (commandHistoryIndex > 0) {
                            commandHistoryIndex--;
                            input = commandHistory[commandHistoryIndex];
                            cursorPos = input.size();
                            std::cout << "\r" << path << " ~~> " << input << "\033[K";

                            if (cursorPos < input.size()) {
                                std::cout << "\033[" << (input.size() - cursorPos) << "D";
                            }
                            std::cout.flush();
                        }
                        break;
                    case 'B':
                        if (commandHistoryIndex < commandHistory.size()) {
                            commandHistoryIndex++;
                            input = (commandHistoryIndex < commandHistory.size()) ? commandHistory[commandHistoryIndex] : "";
                            cursorPos = input.size();
                            std::cout << "\r" << path << " ~~> " << input << "\033[K";

                            if (cursorPos < input.size()) {
                                std::cout << "\033[" << (input.size() - cursorPos) << "D";
                            }
                            std::cout.flush();
                        }
                }
            }
        }
        else if (ch == 127 || ch == 8) {
            if (cursorPos > 0) {
                input.erase(cursorPos-1, 1);
                cursorPos--;

                std::cout << "\r" << path << " ~~> " << "\033[K";
                std::cout.flush();

                std::cout << input;
                std::cout.flush();

                if (cursorPos < input.size()) {
                    std::cout << "\033[" << (input.size() - cursorPos) << "D";
                }
                std::cout.flush();
            }
            else {
                if (checkBooleanVar("SBELL_BEEP", true)) std::cout << "\a" << std::flush;
            }
        }
        else {
            input.insert(cursorPos, 1, ch);
            cursorPos++;

            std::cout << "\r" << path << " ~~> " << "\033[K";
            std::cout.flush();

            std::cout << input;
            std::cout.flush();

            if (cursorPos < input.size()) {
                std::cout << "\033[" << (input.size() - cursorPos) << "D";
            }
        }
    }
    setRawMode(false);
    std::cout << "\n";
    return input;
}

void signalHandler(int signum) {
    //(void*)0;
}

int executeInterpreterCommands(std::vector<std::string> command) { //NOTE: código spaghetti - arreglarlo cuanto antes
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
        if (changeDir(command) != 0) {
            std::cout << t.get("cderr");
        };
        return 0;
    }
    else if (command[0] == "export") {
        bool exportInFile = false;
        if (command.size() < 3) {
            std::cerr << "export: error: required at least 2 args\n";
            return 1;
        }
        if (command.size() > 3) {
            if (command[3] == "--all-sessions") exportInFile = true;
        }
        setenv(command[1].c_str(), command[2].c_str(), 1);

        if (exportInFile) {
            setLine("export  " + command[1] + " " + command[2], command[1]);
        }

        return 0;
    }
    else if (command[0] == "alias") {
        bool exportInConfFile = false;
        if (command.size() == 1) {
            for (const auto& currentAlias : aliasVector) {
                std::cerr << currentAlias.abreviatedName << "='" << currentAlias.command << "'\n";
            }
            return 0;
        }

        if (command.size() == 2) {
            std::cerr << "alias: error: required at least 2 args\n";
        }

	    std::string aliasArgs;
	    for (int i = 2; i < command.size(); ++i) {
		    if (command[i] == "--all-sessions") {
			    exportInConfFile = true;
			    break;
		    }
		    aliasArgs.append(command[i] + " ");
	    }

        aliasVector.push_back(alias{command[1], aliasArgs});
        if (exportInConfFile) {
            setLine("alias " + command[1] + " " + aliasArgs, command[1]);
        }
        return 0;
    }
    else if (command[0] == "rmhist") {
	commandHistory.clear();
        std::ofstream file(HISTFILE, std::ofstream::out | std::ios::trunc);
        file.close();
	return 0;
    }
    else if (command[0] == "exec") {
	if (command.size() < 2) {
	    std::cerr << "exec" << t.get("c:arg1");
	    return -1;
	}
        std::vector<std::string> toExecute;
        for (int i = 1; i < command.size(); ++i) {
            toExecute.push_back(command[i]);
        }
        return executeFileCommand(toExecute);
    }
    return 5;
}

int executeAlias(std::string aliasName) {
    for (const auto& currentAlias : aliasVector) {
        if (currentAlias.abreviatedName == aliasName) {
            std::vector<std::string> command = splitCommand(currentAlias.command);

            if (command.empty()) continue;

            if (executeInterpreterCommands(command) != 5) continue;

            for (int i = 0; i < command.size(); ++i) {
                command[i] = replaceVariableSymbol(command[i]);
                command[i] = getCommandReturn(command[i]);
            }

            int status = executeSystemCommand(command);
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

void readConfFile() {
    std::ifstream file(CONFFILE);
    if (!file.is_open()) {
        std::ofstream firstFile(CONFFILE);
        firstFile << "// SBELL CONFIG FILE !/\n";
        firstFile << "// THIS MAKE THE WHOLE LINE BE A COMMENT, YOU CAN UNCOMMENT NEXT LINES !/\n";
        firstFile << "// export SBELL_WELCOME false !/\n";
        firstFile << "// export SBELL_WELCOMEMSG 'Message' !/\n";
        firstFile << "// export SBELL_BEEP false !/\n";
        return;
    }
    std::string line;

    while (getline(file, line)) {
	if (line.find("//") != std::string::npos) {
	    size_t firstCommentSep = line.find("//");
	    if (line.find("!/") != std::string::npos) {
	        size_t lastCommentSep = line.find("!/");
		line.erase(firstCommentSep, lastCommentSep + 2);
	    }
	    else {
	        line.erase(firstCommentSep);
	    }
	}

        std::vector<std::string> command = splitCommand(line);
        if (command.empty()) continue;

        for (int i = 0; i < command.size(); ++i) {
            command[i] =  replaceVariableSymbol(command[i]);
            command[i] = getCommandReturn(command[i]);
	    command[i] = replaceHomeAbreviation(command[i]);
        }

        if (executeInterpreterCommands(command) != 5) continue;
	if (executeAlias(command[0]) != 5) continue;

        int status = executeSystemCommand(command);
        if (status == 127) {
            std::cerr << t.get("e127") << command[0] << "\n";
        }
        if (status == 0) continue;
        std::cout << status << "\n";
    }
}

struct commandWithUniter {
    std::string command;
    std::string uniter = "";
};

std::vector<std::string> splitLineInCommands(const std::string line) {
    std::string tempLine = line;
    std::vector<std::string> splittedLine;

    size_t start = 0;
    size_t end = line.find("&&");

    while (end != std::string::npos) {
        splittedLine.push_back(line.substr(start, end - start));
        start = end + 2;
        end = line.find("&&", start);
    }
    splittedLine.push_back(line.substr(start));
    return splittedLine;
}

int main(int argc, char **argv) {
    signal(SIGINT, signalHandler);
    signal(SIGTSTP, signalHandler);

    readConfFile();

    if (checkBooleanVar("SBELL_WELCOME", true)) {
        if (getenv("SBELL_WELCOMEMSG") == nullptr) {
            std::cerr << t.get("welcome");
        }
        else {
            std::cerr << getenv("SBELL_WELCOMEMSG") << "\n";
        }
    }

    loadCommandHistory();

    setInterpreterVariable("CURRENT_SHELL", defs::sbell::shell);
    setInterpreterVariable("SBELL_AUTHOR", defs::sbell::author);
    setInterpreterVariable("SBELL_VERSION", defs::sbell::version);
    setInterpreterVariable("SBELL_LICENSE", defs::sbell::license);
    setInterpreterVariable("ILOVELINUX", "Me too :3");

    while (true) {
        t = Translator();
        pathVariable = getenv("PATH");
        std::cout << "\033[0m";
        std::string input;
        input = readCommand();
        std::vector<std::string> splittedLine = splitLineInCommands(input);
        if (splittedLine.empty()) continue;
        if (checkBooleanVar("SBELL_SAVEHIST"), true) {
            saveCommandHistory(input);
        }

        for (const auto& element : splittedLine) {
            std::vector<std::string> command = splitCommand(element);
            setenv("HIST", getUnifiedString(commandHistory, "\n").c_str(), 1); //FIXME: QUE PUTA MIERDA

            for (int i = 0; i < command.size(); ++i) {
                command[i] = replaceVariableSymbol(command[i]);
                command[i] = getCommandReturn(command[i]);
            }

            bool commandExecuted = false;

            for (const auto& arg : command) {
                if (arg.find(">") != std::string::npos ||
                    arg.find(">>") != std::string::npos ||
                    arg.find("<") != std::string::npos ||
                    arg.find("|") != std::string::npos ||
                    arg.find("&") != std::string::npos) {
                        executeMegaCommands(command);
                        commandExecuted = true;
                    }
            }
            if (commandExecuted) continue;

            if (executeInterpreterCommands(command) != 5) continue;
            if (executeAlias(command[0]) != 5) continue;

            int status = executeSystemCommand(command);
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
