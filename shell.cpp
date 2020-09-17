/* Author */
#include <iostream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <fcntl.h>
#include <time.h>
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

// reference  https://www.indradhanush.github.io/blog/writing-a-unix-shell-part-2
char** parser(char* input) {
    char** command = static_cast<char **>(malloc(8 * sizeof(char *)));
    if (command == NULL) {
        perror("malloc failed");
        exit(1);
    }
    const char* delim = " ";
    char* parsed;
    int idx = 0;

    parsed = strtok(input, delim);
    while (parsed != NULL) {
        command[idx] = parsed;
        idx++;
        parsed = strtok(NULL, delim);
    }

    command[idx] = NULL;
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

int barCount(string input) {
    char delim = '|';
    int count = 0;
    for (int i = 0; i < input.size(); i++) {
        if (input.at(i) == delim) count++;
    }
    return count;
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
        int idx = 0;
        for (int i = input.find("<") + 1; i < input.find(">"); i++) {
            toReturn = input.at(i);
            if (input.at(i + 1)=='>') break;
            idx++;
        }
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
    /*for (int i = 0; i < tok.size(); i++) {
        cout << tok[i] << endl;
    }*/
    return tok;
}

void execute(string input) {
    if (input.substr(0,4) == "echo") { // echo command still prints with "" or '', need to rid of quotes
        bool validCom = false;
        if ((input.at(5) == '\"') && (input.at(input.size() - 1) == '\"')) validCom = true;
        if ((input.at(5) == '\'') && (input.at(input.size() - 1) == '\'')) validCom = true;
        if (validCom) {
            string necho = input.substr(6, input.length() - 1);
            necho = necho.substr(0, necho.size() - 1);
            cout << necho << endl;
        } else {
            cout << "Invalid syntax for echo command!\n";
        }
    } else if (redirect(input) != -1) {
        bool out = false;
        bool in = false;
        if (input.at(redirect(input)) == '>') out = true;
        if (input.at(redirect(input)) == '<') in = true;
        if (out) {
            //cout << "trying to output\n";
            string tempCom = input.substr(0, redirect(input) - 1);
            string file = outFileName(input);
            char *filename = (char *) file.c_str();
            int fd = open(filename, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
            dup2(fd, 1);
            char *command = (char *) tempCom.c_str();
            char **comAry = parser(command);
            //cerr << &comAry << endl;
            if(execvp(comAry[0], comAry) < 0) {
                perror(comAry[0]);
                exit(1);
            }
            close(fd);
        }
        if (in) {
            //cout << "trying to input\n";
            string tempCom = input.substr(0, redirect(input) - 1);
            string file = inFileName(input);
            //cout << "trying to input from: " << file << endl;
            char *filename = (char *) file.c_str();
            int fd = open(filename, O_RDONLY, 0);
            dup2(fd, STDIN_FILENO);
            char *command = (char *) tempCom.c_str();
            char **comAry = parser(command);
            if (input.find(">") != string::npos && input.find("<") != string::npos) {
                string tempCom1 = input.substr(0, redirect(input) - 1);
                string file1 = outFileName(input);
                char *filename1 = (char *) file1.c_str();
                int fd1 = open(filename1, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
                dup2(fd1, 1);
                char *command1 = (char *) tempCom1.c_str();
                char **comAry1 = parser(command1);
                if(execvp(comAry1[0], comAry1) < 0) {
                    perror(comAry1[0]);
                    exit(1);
                }
            } else {
                if(execvp(comAry[0], comAry) < 0) {
                    perror(comAry[0]);
                    exit(1);
                }
            }
            close(fd);
        }
    } else {
        char *command = (char *) input.c_str();
        // trim whitespace and re assign string here
        char **comAry = parser(command);
        if(execvp(comAry[0], comAry) < 0) {
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
    struct tm * timeinfo;
    char buffer[80];
    int backup = dup(0);
    while (true){
        dup2(backup, 0);
        for (int i = 0; i < bgs.size(); i++) {
            if (waitpid(bgs[i], 0, WNOHANG) == bgs[i]) {
                cout << "Process at: [" << i << "] ended, PID: " << bgs[i] << endl;
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
        cout << p << "@" << curtime << ":" << cwd << "$ "; //print a prompt
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
            char* command = (char *) inputline.c_str();
            char ** comAry = parser(command);
            if (cd(comAry[1]) < 0) {
                cerr << "cd failed" << endl;
            }
            continue;
        }
        int pid = fork();
        if (pid == 0) { //child process
        // preparing the input command for execution
            if (inputline.substr(0,4) == "echo") { // echo command still prints with "" or '', need to rid of quotes
                bool validCom = false;
                if ((inputline.at(5) == '\"') && (inputline.at(inputline.size() - 1) == '\"')) validCom = true;
                if ((inputline.at(5) == '\'') && (inputline.at(inputline.size() - 1) == '\'')) validCom = true;
                if (validCom) {
                    string necho = inputline.substr(6, inputline.length() - 1);
                    necho = necho.substr(0, necho.size() - 1);
                    cout << necho << endl;
                } else {
                    cout << "Invalid syntax for echo command!\n";
                }
            } else if (redirect(inputline) != -1) {
                bool out = false;
                bool in = false;
                if (inputline.at(redirect(inputline)) == '>') out = true;
                if (inputline.at(redirect(inputline)) == '<') in = true;
                if (out) {
                    //cout << "trying to output\n";
                    string tempCom = inputline.substr(0, redirect(inputline) - 1);
                    string file = outFileName(inputline);
                    //cout << "trying to output to: " << file << endl;
                    char *filename = (char *) file.c_str();
                    int fd = open(filename, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
                    dup2(fd, 1);
                    char *command = (char *) tempCom.c_str();
                    char **comAry = parser(command);
                    //cerr << &comAry << endl;
                    if(execvp(comAry[0], comAry) < 0) {
                        perror(comAry[0]);
                        exit(1);
                    }
                    close(fd);
                }
                if (in) {
                    //cout << "trying to input\n";
                    string tempCom = inputline.substr(0, redirect(inputline) - 1);
                    string file = inFileName(inputline);
                    //cout << "trying to input from: " << file << endl;
                    char *filename = (char *) file.c_str();
                    int fd = open(filename, O_RDONLY, 0);
                    dup2(fd, STDIN_FILENO);
                    char *command = (char *) tempCom.c_str();
                    char **comAry = parser(command);
                    if (inputline.find(">") != string::npos && inputline.find("<") != string::npos) {
                        string tempCom1 = inputline.substr(0, redirect(inputline) - 1);
                        string file1 = outFileName(inputline);
                        char *filename1 = (char *) file1.c_str();
                        int fd1 = open(filename1, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
                        dup2(fd1, 1);
                        char *command1 = (char *) tempCom1.c_str();
                        char **comAry1 = parser(command1);
                        if(execvp(comAry1[0], comAry1) < 0) {
                            perror(comAry1[0]);
                            exit(1);
                        }
                        close(fd1);
                    } else {
                        if(execvp(comAry[0], comAry) < 0) {
                            perror(comAry[0]);
                            exit(1);
                        }
                    }
                    close(fd);
                }
            } else if (inputline.find("|") != string::npos) {
                //cout << "parsing\n";
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
            } else {
                char *command = (char *) inputline.c_str();
                // trim whitespace and re assign string here
                char **comAry = parser(command);
                if(execvp(comAry[0], comAry) < 0) {
                    perror(comAry[0]);
                    exit(1);
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
