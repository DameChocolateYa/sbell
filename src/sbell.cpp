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

std::string pathVariable = getenv("PATH");

const std::string HISTFILE = std::string(getenv("HOME")) + "/.sbell_hist";

std::vector<std::string> commandHistory;
int commandHistoryIndex = -1;

struct alias {
    std::string abreviatedName;
    std::string command;
};

std::vector<alias> aliasVector;

bool checkBooleanVar(const char* variable, bool byDefault=false) {
    if (getenv(variable) == nullptr) return byDefault;

    return std::strcmp(getenv(variable), "true") == 0;
}

std::string replaceHomeAbreviation(std::string& text) {
    if (text.find("~") != std::string::npos) {
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

void loadCommandHistory() {
    std::ifstream file(HISTFILE);
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

void saveCommandHistory(const std::string& command) {
    if (!commandHistory.empty() && command == commandHistory[commandHistoryIndex-1]) return;

    std::ofstream file(HISTFILE, std::ios::app);
    file << command << "\n";
    file.close();
    commandHistory.push_back(command);
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
        std::cerr << "Error executing command\n";
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
        std::cerr << "Error executing binary file\n";
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
        changeDir(command);
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
    std::string line;

    while (getline(file, line)) {
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
            std::cerr << "command: " << command[0] << " not found\n";
        }
        if (status == 0) continue;
        std::cout << status << "\n";
    }
}

struct commandWithUniter {
    std::string command;
    std::string uniter = "";
};

/*std::vector<std::string> splitLineInCommands(std::string line) {
    // TODO: ME DA PUTA PEREZA
    }*/

int main(int argc, char **argv) {
    signal(SIGINT, signalHandler);
    signal(SIGTSTP, signalHandler);

    readConfFile();

    loadCommandHistory();

    std::cout << "Welcome to Sbell\n";
    setInterpreterVariable("CURRENT_SHELL", "sbell");
    setInterpreterVariable("SBELL_AUTHOR", "SAMUEL JORGE FRA");
    setInterpreterVariable("ILOVELINUX", "Me too :3");

    while (true) {
        pathVariable = getenv("PATH");
        std::cout << "\033[0m";
        std::string input;
        input = readCommand();
        std::vector<std::string> command = splitCommand(input);

        if (command.empty()) continue;
        if (checkBooleanVar("SBELL_SAVEHIST", true)) {
            saveCommandHistory(input);
        }
        setenv("HIST", getUnifiedString(commandHistory, "\n").c_str(), 1); //FIXME: QUE PUTA MIERDA

        for (int i = 0; i < command.size(); ++i) {
            command[i] = replaceVariableSymbol(command[i]);
            command[i] = getCommandReturn(command[i]);
        }

        if (executeInterpreterCommands(command) != 5) continue;
	if (executeAlias(command[0]) != 5) continue;

        int status = executeSystemCommand(command);
        if (status == 127) {
            std::cerr << "command: " << command[0] << " not found\n";
        }
        if (status == 0) continue;
        std::cout << status << "\n";
    }

    std::cout << "Goodbye!\n";
    return 0;
}
