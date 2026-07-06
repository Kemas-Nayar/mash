#include <cstddef>
#include <iterator>
#include <sys/time.h>
#include <time.h>
#include <bits/types/struct_timeval.h>
#include <termios.h>
#include <cstdio>
#include <cstdlib>
#include <limits.h>
#include <linux/limits.h>
#include <sys/wait.h>
#include <string.h>
#include <iostream>
#include <string>
#include <vector> 
#include <sstream>
#include <unistd.h>
#include <stdlib.h>

std::vector<std::string> history;

termios original_termios;

void redraw_terminal(const std::string& prompt, const std::string& line, std::size_t cursor){
  std::cout << "\r\033[K" << prompt << line << std::flush;

  std::size_t back = line.size() - cursor;
  if(back > 0){
    std::cout << "\033[" << back << "D" << std::flush;
  }
}

void disableRawMode(){
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_termios);
}

void enableRawMode(){
  tcgetattr(STDIN_FILENO, &original_termios);
  atexit(disableRawMode);

  struct termios raw = original_termios;
  raw.c_lflag &= ~(ECHO | ICANON);

  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

// pwd command
bool pwd(const std::vector<std::string>& args){
  char cwd[PATH_MAX];
  if(getcwd(cwd, sizeof(cwd)) != NULL){
    std::cout << cwd << "\n";
  } else {
    std::cerr << "cwd failed!\n";
  }
  
  return true;
}

// echo command
bool echo(const std::vector<std::string>& args){
  for(size_t i{0}; i<args.size(); ++i){
    std::cout << args[i];
    if(i+1 < args.size()){
      std::cout << " " << std::flush;
    }
  }
  std::cout << "\n";

  return true;
}


// line jadi vector of string
std::vector<std::string> parse_input(const std::string& line){
  std::vector<std::string> tokens;
  std::stringstream ss(line);
  std::string token;

  while (ss >> token) {
    tokens.push_back(token);
  }

  return tokens;
}

void clear_terminal(){
  std::cout << "\033[2J\033[1;1H";
}

// cd command
bool cd(const std::vector<std::string>& args){
  char oldcwd[PATH_MAX];

  if(getcwd(oldcwd, sizeof(oldcwd)) == NULL){
    perror("getcwd");
    return true;
  }

  const char *target = nullptr;

  if(args.size() == 1 || args[1] == "~"){
    target = getenv("HOME");
    if(target == NULL){
      std::cerr << "HOME is not set";
      return 1;
    }
  } 

  else if(args[1] == "-"){
    target = getenv("OLDPWD");
    if(target == NULL){
      std::cerr << "OLDWD is not set!";
      return true;
    }
  }

  else {
    target = args[1].c_str();
  }

  if(chdir(target) != 0){
    return true;
  }

  setenv("OLDPWD", oldcwd, 1);

  char newcwd[PATH_MAX];
  if(getcwd(newcwd, sizeof(newcwd))){
    setenv("PWD", newcwd, 1);
  }

  return false;
}

// hadling built in commands
bool built_in(const std::vector<std::string>& args){
  if(args.empty()){
    return true;
  }

  if(args[0] == "exit"){
    clear_terminal();
    disableRawMode();
    exit(0);
  }

  if(args[0] == "cd"){
    cd(args);
    return true;
  }

  if(args[0] == "cwd"){
    pwd(args);
    return true;
  }

  if(args[0] == "history"){
    for(std::size_t i{0}; i<history.size(); ++i){
      std::cout << i << " " << history[i] << "\n";
    }
    return true;
  }

  return false;
}


// handling external commands
bool external_command(const std::vector<std::string>& args){
  int my_pid = fork();

  if(my_pid < 0){
    std::cerr << "fork failed!" << "\n";
  } 

  else if (my_pid == 0){
    std::vector<char*> argv;

    for(auto &s : args){
      argv.push_back(const_cast<char*>(s.c_str()));
    }
    argv.push_back(nullptr);

    execvp(argv[0], argv.data());

    std::cerr << "mash: " << argv[0] << ": command not found\n";
    exit(EXIT_FAILURE);
  } 

  else {
    wait(NULL);
    return true;
  }

  return false;
}

void throw_error(const std::vector<std::string>& args){
  std::cerr << "command: " << args[0] << " not found!\n";
}

int main()
{
  int history_index = 0;
  // clear_terminal();

  while(true){
    timeval start, end;

    enableRawMode();
    atexit(disableRawMode);
    
    std::string line;
    history_index = history.size();

    std::size_t cursor = 0;

    char cwd[PATH_MAX];
    getcwd(cwd, sizeof(cwd));

    char *folder_name = strrchr(cwd, '/');
    std::stringstream prompt_stream;

    if(strcmp(cwd, getenv("HOME")) == 0){
      prompt_stream << "[bash@mash ~]$ ";
    } 

    else if(folder_name != NULL){
      if(*(folder_name + 1) == '\0'){
        prompt_stream << "[bash@mash " << "/" << "]$ ";
      } else {
        prompt_stream << "[bash@mash " << folder_name + 1 << "]$ ";
      }
    } else{
        prompt_stream << "[bash@mash " << cwd << "]$ ";
    }
    std::string prompt = prompt_stream.str();

    std::cout << prompt << std::flush;

    char c;
    while(read(STDIN_FILENO, &c, 1) == 1){
      if(c == '\x1b'){
        char seq[2];
        read(STDIN_FILENO, &seq[0], 1);
        read(STDIN_FILENO, &seq[1], 1);

        if(seq[0] == '['){
        switch (seq[1]){
          case 'A':
            // Up Arrow Key
            if(history_index > 0){
              history_index--;
              line = history[history_index];
              cursor = line.size();
              redraw_terminal(prompt, line, cursor);
            }

            break;

          case 'B':
            //down
            if(history_index < history.size()){
              history_index++;
              if(history_index == line.size()){
                line.clear();
              } else{
                line = history[history_index];
              }
              cursor = line.size();
              redraw_terminal(prompt, line, cursor);
            }
            break;

          case 'C':
            //Right
            if (cursor < line.size()){
              cursor++;
              std::cout << "\033[C" << std::flush;
            }
            break;


          case 'D':
            //Left
            if(cursor > 0){
              cursor++;
              std::cout << "\033[D" << std::flush;
            }
            break;
          }
        }
      }

      else{
        if(c == '\n'){
          write(STDOUT_FILENO, &c, 1);
          disableRawMode();
          break;
        } 

        else if (c == 127 || c == '\b') { // Backspace
          if(!line.empty()){
            line.pop_back();
            write(STDOUT_FILENO, "\b \b", 3);
          }
        }

        else{
          line.push_back(c);
          write(STDOUT_FILENO, &c, 1);
        }
      }
    }
    gettimeofday(&start, NULL);

    history.push_back(line);
    std::vector<std::string> args = parse_input(line);

    if(args.empty()) continue;

    if(!built_in(args))
      if(!external_command(args)) 
          throw_error(args);

    gettimeofday(&end, NULL);
    // std::cout << "end time: " << end.tv_usec- start.tv_usec << std::endl;
    
  }
  disableRawMode();

  return 0;
}
