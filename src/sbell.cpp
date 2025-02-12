#include <iostream>
#include <cstring>
#include <unistd.h>
#include <vector>
#include <sstream>
#include <cstdlib>
#include <sys/wait.h>
#include <termios.h>
#include <fstream>

std::string pathVariable = getenv("PATH");
const std::string HISTFILE = std::string(getenv("HOME")) + "/.sbell_hist";

std::vector<std::string> commandHistory;
int commandHistoryIndex = -1;

std::string replaceHomeAbreviation(std::string& text) {
    if (text.find("~") != std::string::npos) {
        size_t start_pos = text.find("~");
        std::string username = getenv("USER");
        std::string userDir = "/home/" + username + "/";
        text.replace(start_pos, 1, userDir);
    }
    return text;
}


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

void loadCommandHistory() {
    std::ifstream file(HISTFILE);
    std::string line;
    while (std::getline(file, line)) {
        commandHistory.push_back(line);
    }
    file.close();
    commandHistoryIndex = commandHistory.size();
}

void saveCommandHistory(const std::string& command) {
    if (command == commandHistory[commandHistoryIndex-1]) return;

    std::ofstream file(HISTFILE, std::ios::app);
    file << command << "\n";
    file.close();
    commandHistory.push_back(command);
}

std::string getCurentPath() {
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
    std::stringstream ss(command);
    std::string word;
    std::vector<std::string> splitedCommand;

    while (ss >> word) {
        splitedCommand.push_back(word);
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
    std::string path = getCurentPath();
    std::cout << path << " ~~>  ";


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
                            cursorPos = std::min(cursorPos, (int)input.size());
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
                            cursorPos = std::min(cursorPos, (int)input.size());
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
    (void*)0;
}

int main(int argc, char **argv) {
    signal(SIGINT, signalHandler);
    signal(SIGTSTP, signalHandler);

    loadCommandHistory();

    std::cout << "Welcome to Sbell\n";

    while (true) {
        std::string input;
        input = readCommand();
        std::vector<std::string> command = splitCommand(input);
        
        if (command.empty()) continue;
        saveCommandHistory(input);

        for (int i = 0; i < command.size(); ++i) {
            command[i] = replaceVariableSymbol(command[i]);
        }

        if (command[0] == "exit") {
            if (command.size() == 1) return 0;
            try {
                return std::stoi(command[1]);
            }
            catch (const std::exception& e) {
                exit(1);
            }
        }
        else if (command[0] == "cd") {
            changeDir(command);
            continue;
        }
        else if (command[0] == "export") {
            if (command.size() < 3) {
                std::cerr << "export: error: required at least 2 args\n";
                continue;
            }
            setenv(command[1].c_str(), command[2].c_str(), 1);
            continue;
        }

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
