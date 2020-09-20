/* Author: benjamin ramon
 * title: csce 313 PA2, linux shell
 * date: 9/20/2020
*/
#include <iostream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <fcntl.h>
#include <time.h>
#include <algorithm>
#include <unistd.h>
#include <sys/wait.h>
#include <regex>

using namespace std;

// reference https://www.techiedelight.com/trim-string-cpp-remove-leading-trailing-spaces/#:~:text=We%20can%20use%20combination%20of,functions%20to%20trim%20the%20string.
// includes up to trim
string ltrim(string input) {
    return regex_replace(input, regex("^\\s+"), string(""));
}

string rtrim(string input) {
    return regex_replace(input, regex("\\s+$"), string(""));
}

string trim(string input) {
    return ltrim(rtrim(input));
}
// end of referenced fucntions from first website

// taken from geeksforgeeks
char *removeSpaces(char *str) {
    int i = 0, j = 0;
    while (str[i]) {
        if (str[i] != ' ')
            str[j++] = str[i];
        i++;
    }
    str[j] = '\0';
    return str;
}

// reference  https://www.indradhanush.github.io/blog/writing-a-unix-shell-part-2
char** parser(char* input, bool awkCom) {
    char** command = static_cast<char **>(malloc(8 * sizeof(char *)));
    if (command == NULL) {
        perror("malloc failed");
        exit(1);
    }
    const char* delim = " ";
    const char* awkDelim = "\'";
    char* parsed;
    int idx = 0;
    if (awkCom) {
        parsed = strtok(input, awkDelim);
        while (parsed != NULL) {
            command[idx] = parsed;
            idx++;
            parsed = strtok(NULL, awkDelim);
        }
        command[0] = removeSpaces(command[0]);
        command[idx] = NULL;
    } else {
        parsed = strtok(input, delim);
        while (parsed != NULL) {
            command[idx] = parsed;
            idx++;
            parsed = strtok(NULL, delim);
        }
        command[idx] = NULL;
    }
    return command;
}

// reference https://stackoverflow.com/questions/298510/how-to-get-the-current-directory-in-a-c-program
char * curDir() {
    char buff[FILENAME_MAX];
    getcwd(buff, FILENAME_MAX );
    std::string current_working_dir(buff);
    char* curr_working_dir = get_current_dir_name();
    return curr_working_dir;
}

int redirect(string input) {
    for (int i = 0; i < input.size(); i++) {
        if (input.find('|') != string::npos) {
            return -1;
        }
        if (input.at(i) == '>' || input.at(i) == '<') {
            return i;
        }
    }
    return -1;
}

bool validEcho(string input) {
    int count = 0;
    for (int i = 0; i < input.length(); i++) {
        if (input.at(i) == '\"' || input.at(i) == '\'') count++;
    }
    if (count == 2) return true;
    return false;
}

string outFileName(string input) {
    string toReturn;
    size_t pos = input.find(">");
    toReturn = input.substr(pos+1);
    return trim(toReturn);
}

string inFileName(string input) {
    string toReturn;
    size_t pos = input.find("<");
    if (input.find(">") != string::npos) {
        int sidx = input.find("<");
        int eidx = input.find(">");
        toReturn = input.substr(sidx+1,eidx-sidx-1);
    } else {
        toReturn = input.substr(pos + 1);
    }
    return trim(toReturn);
}

vector<string> split(string input, char delim) {
    vector<string> tok;
    stringstream chec1(input);
    string inter;
    while (getline(chec1, inter, delim)) {
        tok.push_back(trim(inter));
    }
    return tok;
}

void execute(string input) {
    if (input.substr(0,4) == "echo") { // echo command still prints with "" or '', need to rid of quotes
        bool validCom = false;
        bool out = false;
        if (validEcho(input)) validCom = true;
        if (input.find("\"") != string::npos && input.find(">") != string::npos) {
            if ((int)input.find(">") > (int)input.find_last_of('\"')) {
                out = true;
            }
        }
        if (input.find("\'") != string::npos && input.find(">") != string::npos) {
            if ((int) input.find(">") > (int) input.find_last_of('\'')) {
                out = true;
            }
        }
        if (validCom && out) {
            string tempCom = input.substr(0, input.find(">"));
            string file = outFileName(input);
            char *filename = (char *) file.c_str();
            int fd = open(filename, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
            dup2(fd, 1);
            char *command = (char *) tempCom.c_str();
            char **comAry = parser(command, false);
            if(execvp(comAry[0], comAry) < 0) {
                perror(comAry[0]);
                exit(1);
            }
            close(fd);
        } else if (validCom) {
            string necho;
            char *command;
            if (input.find("-e")!=string::npos) {
                necho = input.substr(9);
                necho = necho.substr(0, necho.size() - 1);
                command = (char *) necho.c_str();
                if(execlp("echo","echo", "-e", command, NULL) < 0) {
                    perror(reinterpret_cast<const char *>(command[0]));
                    exit(1);
                }
            } else {
                necho = input.substr(6);
                necho = necho.substr(0, necho.size() - 1);
                command = (char *) necho.c_str();
                if(execlp("echo","echo",command,NULL) < 0) {
                    perror(reinterpret_cast<const char *>(command[0]));
                    exit(1);
                }
            }
        } else {
            perror("Invalid syntax for echo command!\n");
            exit(1);
        }
    } else if (input.find("<")!=string::npos && input.find("awk")!=string::npos) {
        string file = inFileName(input);
        int comIdx = input.find("<") - 1;
        string coms = input.substr(0,comIdx);
        char *filename = (char *) file.c_str();
        int fd = open(filename, O_RDONLY | S_IRUSR);
        dup2(fd, 0);
        char *command = (char *) coms.c_str();
        char **comAry = parser(command, true);
        if (execvp(comAry[0], comAry) < 0) {
            perror(comAry[0]);
            exit(1);
        } else {
            wait(NULL);
        }
        close(fd);
    } else if (redirect(input) != -1) {
        bool out = false;
        bool in = false;
        if (input.find(">") != string::npos && input.find("<") != string::npos) {
            out = true;
            in = true;
        }
        if (input.at(redirect(input)) == '>') out = true;
        if (input.at(redirect(input)) == '<') in = true;
        if (in && out) {
            string tempCom = input.substr(0, input.find("<"));
            tempCom = trim(tempCom);
            char *command = (char *) tempCom.c_str();
            char **comAry = parser(command, false);
            string file = inFileName(input);
            char *filename = (char *) file.c_str();
            int fd = open(filename, O_RDONLY, 0644);
            dup2(fd, 0);
            string file1 = outFileName(input);
            char *filename1 = (char *) file1.c_str();
            int fd1 = open(filename1, O_CREAT | O_WRONLY | O_TRUNC, 0644);
            dup2(fd1, 1);
            close(fd1);
            close(fd);
            if (execvp(comAry[0], comAry) < 0) {
                perror(comAry[0]);
                exit(1);
            } else {
                wait(NULL);
            }
        }
        if (in) {
            string tempCom = input.substr(0, input.find("<"));
            tempCom = trim(tempCom);
            string file = inFileName(input);
            char *filename = (char *) file.c_str();
            int fd = open(filename, O_RDONLY | S_IRUSR);
            dup2(fd, 0);
            char *command = (char *) tempCom.c_str();
            char **comAry = parser(command, false);
            if (execvp(comAry[0], comAry) < 0) {
                perror(comAry[0]);
                exit(1);
            } else {
                wait(NULL);
            }
            close(fd);
        }
        if (out) {
            string tempCom = input.substr(0, input.find(">"));
            tempCom = trim(tempCom);
            string file = outFileName(input);
            char *filename = (char *) file.c_str();
            int fd = open(filename, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
            dup2(fd, 1);
            char *command = (char *) tempCom.c_str();
            char **comAry = parser(command, false);
            if (execvp(comAry[0], comAry) < 0) {
                perror(comAry[0]);
                exit(1);
            } else {
                wait(NULL);
            }
            close(fd);
        }
    } else {
        char *command = (char *) input.c_str();
        // trim whitespace and re assign string here
        bool awkCom = false;
        if (input.find("awk") != string::npos) awkCom = true;
        char **comAry = parser(command, awkCom);
        if(execvp(removeSpaces(comAry[0]), comAry) < 0) {
            perror(comAry[0]);
            exit(1);
        }
    }
}

int cd(char* path) {
    return chdir(path);
}

int main (){
    time_t tt;
    vector<int> bgs;
    vector<char*> dirs;
    struct tm * timeinfo;
    char buffer[80];
    int backup = dup(0);
    while (true){
        dup2(backup, 0);
        for (int i = 0; i < bgs.size(); i++) {
            if (waitpid(bgs[i], 0, WNOHANG) == bgs[i]) {
                cerr << "Process at: [" << i << "] ended, PID: " << bgs[i] << endl;
                bgs.erase(bgs.begin() + i);
                i--;
            }
        }
        char* p = getenv("USER");
        if (p == NULL) return EXIT_FAILURE;
        time(&tt);
        timeinfo = localtime(&tt);
        strftime(buffer, sizeof(buffer), "%d-%m-%Y %H:%M:%S",timeinfo);
        string curtime(buffer);
        char*  cwd = curDir();
        dirs.emplace_back(cwd);
        cerr << p << "@" << curtime << ":" << cwd << "$ "; //print a prompt
        string inputline;
        getline (cin, inputline); //get a line from standard input
        if (inputline == string("exit")){
            break;
        }
        bool bg = false;
        if (inputline[inputline.size() - 1] == '&') {
            bg = true;
            inputline = inputline.substr(0, inputline.size() - 1);
        }
        if (inputline.substr(0,2) == "cd") {
            if (inputline.find("-") != string::npos) {
                char *command = dirs[dirs.size()-2];
                command = removeSpaces(command);
                if (cd(command) < 0) {
                    cerr << "cd failed" << endl;
                }
                continue;
            } else {
                char *command = (char *) inputline.c_str();
                char **comAry = parser(command, false);
                if (cd(comAry[1]) < 0) {
                    cerr << "cd failed" << endl;
                }
                continue;
            }
        }
        int pid = fork();
        if (pid == 0) { //child process
        // preparing the input command for execution
             if (inputline.find("echo") != string::npos) {
                bool validCom = false;
                bool out = false;
                if (validEcho(inputline)) validCom = true;
                if (inputline.find("\"") != string::npos && inputline.find(">") != string::npos) {
                    if ((int)inputline.find(">") > (int)inputline.find_last_of('\"')) {
                        out = true;
                    }
                }
                if (inputline.find("\'") != string::npos && inputline.find(">") != string::npos) {
                    if ((int) inputline.find(">") > (int) inputline.find_last_of('\'')) {
                        out = true;
                    }
                }
                if (validCom && !out) {
                    string necho;
                    char *command;
                    if (inputline.find("-e")!=string::npos) {
                        if (inputline.find("-e") < inputline.find("\'") || inputline.find("-e") < inputline.find("\"")) {
                            necho = inputline.substr(9);
                            necho = necho.substr(0, necho.size() - 1);
                            command = (char *) necho.c_str();
                            if (execlp("echo", "echo", "-e", command, NULL) < 0) {
                                perror(reinterpret_cast<const char *>(command[0]));
                                exit(1);
                            }
                        }
                    } else {
                        necho = inputline.substr(6);
                        necho = necho.substr(0, necho.size() - 1);
                        command = (char *) necho.c_str();
                        if(execlp("echo","echo",command,NULL) < 0) {
                            perror(reinterpret_cast<const char *>(command[0]));
                            exit(1);
                        }
                    }
                } else if (validCom && out) {
                    string tempCom = inputline.substr(0, inputline.find(">"));
                    string file = outFileName(inputline);
                    char *filename = (char *) file.c_str();
                    int fd = open(filename, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
                    dup2(fd, 1);
                    char *command = (char *) tempCom.c_str();
                    char **comAry = parser(command, false);
                    if(execvp(comAry[0], comAry) < 0) {
                        perror(comAry[0]);
                        exit(1);
                    }
                    close(fd);
                } else {
                    perror("Invalid syntax for echo command!\n");
                    exit(1);
                }
            } else if (inputline.find("|") != string::npos) {
                 vector<string> c = split(inputline, '|');
                 for (int i = 0; i < c.size(); i++) {
                     int fd[2];
                     pipe(fd);
                     int cid = fork();
                     if (!cid) {
                         if (i < c.size() - 1) {
                             dup2(fd[1], 1);
                         }
                         execute(c[i]);
                     } else {
                         if (i == c.size() - 1) {
                             waitpid(cid, 0, 0);
                         }
                         dup2(fd[0], 0);
                         close(fd[1]);
                     }
                 }
             } else if (inputline.find("<")!=string::npos && inputline.find("awk")!=string::npos) {
                 string file = inFileName(inputline);
                 int comIdx = inputline.find("<") - 1;
                 string coms = inputline.substr(0,comIdx);
                 char *filename = (char *) file.c_str();
                 int fd = open(filename, O_RDONLY | S_IRUSR);
                 dup2(fd, 0);
                 char *command = (char *) coms.c_str();
                 char **comAry = parser(command, true);
                 if (execvp(comAry[0], comAry) < 0) {
                     perror(comAry[0]);
                     exit(1);
                 } else {
                     wait(NULL);
                 }
                 close(fd);
             } else if (redirect(inputline) != -1) {
                bool out = false;
                bool in = false;
                if (inputline.find(">") != string::npos && inputline.find("<") != string::npos) {
                    out = true;
                    in = true;
                }
                if (inputline.at(redirect(inputline)) == '>') out = true;
                if (inputline.at(redirect(inputline)) == '<') in = true;
                if (in && out) {
                    string tempCom = inputline.substr(0, inputline.find("<"));
                    tempCom = trim(tempCom);
                    char *command = (char *) tempCom.c_str();
                    char **comAry = parser(command, false);
                    string file = inFileName(inputline);
                    char *filename = (char *) file.c_str();
                    int fd = open(filename, O_RDONLY, 0644);
                    dup2(fd, 0);
                    string file1 = outFileName(inputline);
                    char *filename1 = (char *) file1.c_str();
                    int fd1 = open(filename1, O_CREAT | O_WRONLY | O_TRUNC, 0644);
                    dup2(fd1, 1);
                    close(fd1);
                    close(fd);
                    if (execvp(comAry[0], comAry) < 0) {
                        perror(comAry[0]);
                        exit(1);
                    } else {
                        wait(NULL);
                    }
                }
                if (in) {
                    string tempCom = inputline.substr(0, inputline.find("<"));
                    tempCom = trim(tempCom);
                    string file = inFileName(inputline);
                    char *filename = (char *) file.c_str();
                    int fd = open(filename, O_RDONLY | S_IRUSR);
                    dup2(fd, 0);
                    char *command = (char *) tempCom.c_str();
                    char **comAry = parser(command, false);
                    if (execvp(comAry[0], comAry) < 0) {
                            perror(comAry[0]);
                            exit(1);
                    } else {
                        wait(NULL);
                    }
                    close(fd);
                }
                if (out) {
                    string tempCom = inputline.substr(0, inputline.find(">"));
                    tempCom = trim(tempCom);
                    string file = outFileName(inputline);
                    char *filename = (char *) file.c_str();
                    int fd = open(filename, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
                    dup2(fd, 1);
                    char *command = (char *) tempCom.c_str();
                    char **comAry = parser(command, false);
                    if (execvp(comAry[0], comAry) < 0) {
                        perror(comAry[0]);
                        exit(1);
                    } else {
                        wait(NULL);
                    }
                    close(fd);
                }
            } else {
                char *command = (char *) inputline.c_str();
                bool awkCom = false;
                if (inputline.find("awk") != string::npos) awkCom = true;
                char **comAry = parser(command, awkCom);
                if(execvp(removeSpaces(comAry[0]), comAry) < 0) {
                    perror(comAry[0]);
                    exit(1);
                } else {
                    wait(NULL);
                }
            }
        } else {
            if (!bg) {
                waitpid(pid, 0, 0); //parent waits for child process
            } else {
                bgs.push_back(pid);
            }
        }
    }
}
