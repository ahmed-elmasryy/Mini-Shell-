
/*
 * 
 *
 * Template file.
 * You will need to add more code here to execute the command table.
 *
 * NOTE: You are responsible for fixing any bugs this code may have!
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <time.h>
#include <glob.h>
#include <wordexp.h>
#include <vector>
#include <string>
#include <cctype>
#include <iostream>
#include <sstream>
#include "command.h"
#include "tokenizer.h"


void parse(std::vector<Token> &tokens)
{   
    //  Command currentCommand; -> local variable removed to avoid memory leak, use Command::_currentCommand instead
    //  SimpleCommand *currentSimpleCommand = new SimpleCommand();
    //  currentCommand.execute(); 

    // Use Command::_currentCommand directly, do not declare a local one
    Command::_currentSimpleCommand = new SimpleCommand();

    if (tokens.empty() || tokens[0].type == TOKEN_EOF)
    {
        return;
    }

    int i = 0;
    while (tokens[i].type != TOKEN_EOF){
        TokenType type = tokens[i].type;

        if (type == TOKEN_COMMAND || type == TOKEN_ARGUMENT){
            Command::_currentSimpleCommand->insertArgument(strdup(tokens[i].value.c_str()));
        }
        else if (type == TOKEN_PIPE){
            if (Command::_currentSimpleCommand->_numberOfArguments == 0){
                std::cerr << "Error: Missing command before pipe." << std::endl;
                Command::_currentCommand.clear();
                return; 
            }
            if (tokens[i + 1].type == TOKEN_BACKGROUND || tokens[i + 1].type == TOKEN_EOF){
                std::cerr << "Error: Missing command after pipe." << std::endl;
                Command::_currentCommand.clear();
                return; 
            }
            Command::_currentCommand.insertSimpleCommand(Command::_currentSimpleCommand);
            Command::_currentSimpleCommand = new SimpleCommand();
        }
        else if (type == TOKEN_REDIRECT || type == TOKEN_APPEND || type == TOKEN_INPUT || type == TOKEN_ERROR || type == TOKEN_REDIRECT_AND_ERROR){
            Token nextToken = tokens[i+1];
            if (nextToken.type != TOKEN_COMMAND && nextToken.type != TOKEN_ARGUMENT){
                std::cerr << "Error: Missing filename for output redirection." << std::endl;
                Command::_currentCommand.clear();
                return; 
            }
            
            // Command::_currentCommand._outFile = strdup(nextToken.value.c_str());

            if(tokens[i].type == TOKEN_REDIRECT){
                if( Command::_currentCommand._outFile != nullptr){
                std::cerr << "Error: Multiple output redirections are not allowed." << std::endl;
                Command::_currentCommand.clear();
                return; 
        }
                free(Command::_currentCommand._outFile);
                Command::_currentCommand._outFile = strdup(nextToken.value.c_str());
                Command::_currentCommand._append = 0; // Reset append flag for '>' redirection
                i += 1;
            }
            else if(tokens[i].type == TOKEN_APPEND){
                if( Command::_currentCommand._outFile != nullptr){
                std::cerr << "Error: Multiple output redirections are not allowed." << std::endl;
                Command::_currentCommand.clear();
                return; 
                }
                free(Command::_currentCommand._outFile);
                Command::_currentCommand._outFile = strdup(nextToken.value.c_str());
                Command::_currentCommand._append = 1; // Set append flag for '>>' redirection
                i += 1;
            }
            else if(tokens[i].type == TOKEN_INPUT){
                if( Command::_currentCommand._inputFile != nullptr){
                std::cerr << "Error: Multiple input redirections are not allowed." << std::endl;
                Command::_currentCommand.clear();
                return; 
                }
                free(Command::_currentCommand._inputFile);
                Command::_currentCommand._inputFile = strdup(nextToken.value.c_str());
                i += 1; 
            }
            else if(tokens[i].type == TOKEN_ERROR){
                if( Command::_currentCommand._errFile != nullptr){
                std::cerr << "Error: Multiple error redirections are not allowed." << std::endl;
                Command::_currentCommand.clear();
                return; 
                }
                free(Command::_currentCommand._errFile);
                Command::_currentCommand._errFile = strdup(nextToken.value.c_str());
                i += 1;
            }
            else if(tokens[i].type == TOKEN_REDIRECT_AND_ERROR){
                free(Command::_currentCommand._outFile);
                free(Command::_currentCommand._errFile);
                Command::_currentCommand._outFile = strdup(nextToken.value.c_str());
                Command::_currentCommand._errFile = strdup(nextToken.value.c_str());
                Command::_currentCommand._out_error = 1; // Set flag for '>>&' redirection
                i += 1;
            }
        }
        else if (type == TOKEN_BACKGROUND){
            if( tokens[i + 1].type != TOKEN_EOF){
                std::cerr << "Error: Background operator '&' must be at the end of the command." << std::endl;
                return; 
            }
            Command::_currentCommand._background = 1;
        }
        i += 1;
        }    // end of while 

        if(Command::_currentSimpleCommand->_numberOfArguments > 0){
            Command::_currentCommand.insertSimpleCommand(Command::_currentSimpleCommand);
        }
        else {
            free(Command::_currentSimpleCommand);
            std::cerr << "Error: No command provided." << std::endl;
            Command::_currentCommand.clear();
            return;
        }
    
   Command::_currentCommand.execute();
}

SimpleCommand::SimpleCommand()
{
    _numberOfAvailableArguments = 5;
    _numberOfArguments = 0;
    _arguments = (char **)malloc(_numberOfAvailableArguments * sizeof(char *));
}

void SimpleCommand::insertArgument(char *argument)
{
    if (_numberOfAvailableArguments == _numberOfArguments + 1)
    {
        _numberOfAvailableArguments *= 2;
        _arguments = (char **)realloc(_arguments,
                                      _numberOfAvailableArguments * sizeof(char *));
    }

    _arguments[_numberOfArguments] = argument;
    _arguments[_numberOfArguments + 1] = NULL;

    _numberOfArguments++;
}

Command::Command()
{
    _numberOfAvailableSimpleCommands = 1;
    // Allocate storage based on AVAILABLE capacity not current count
    // The buffer allocation must always be sized by the capacity not current size
    _simpleCommands = (SimpleCommand **)
        malloc(_numberOfAvailableSimpleCommands * sizeof(SimpleCommand *));

    _numberOfSimpleCommands = 0;
    _outFile = 0;
    _inputFile = 0;
    _errFile = 0;
    _background = 0;
    // Initialize redirection flags to 0 
    _append = 0;
    _out_error = 0;
}

void Command::insertSimpleCommand(SimpleCommand *simpleCommand)
{
    if (_numberOfAvailableSimpleCommands == _numberOfSimpleCommands)
    {
        _numberOfAvailableSimpleCommands *= 2;
        _simpleCommands = (SimpleCommand **)realloc(_simpleCommands,
                                                    _numberOfAvailableSimpleCommands * sizeof(SimpleCommand *));
    }

    _simpleCommands[_numberOfSimpleCommands] = simpleCommand;
    _numberOfSimpleCommands++;
}

void Command::clear()
{
    for (int i = 0; i < _numberOfSimpleCommands; i++)
    {
        for (int j = 0; j < _simpleCommands[i]->_numberOfArguments; j++)
        {
            free(_simpleCommands[i]->_arguments[j]);
        }

        free(_simpleCommands[i]->_arguments);
        free(_simpleCommands[i]);
    }

    if (_outFile)
    {
        free(_outFile);
    }

    if (_inputFile)
    {
        free(_inputFile);
    }

    if (_errFile)
    {
        free(_errFile);
    }

    _numberOfSimpleCommands = 0;
    _outFile = 0;
    _inputFile = 0;
    _errFile = 0;
    _background = 0;
    // Reset flags so previous commands do not contaminate subsequent commands
    _append = 0;
    _out_error = 0;
}

void Command::print()
{
    printf("\n\n");
    printf("              COMMAND TABLE                \n");
    printf("\n");
    printf("  #   Simple Commands\n");
    printf("  --- ----------------------------------------------------------\n");

    for (int i = 0; i < _numberOfSimpleCommands; i++)
    {
        printf("  %-3d ", i);
        for (int j = 0; j < _simpleCommands[i]->_numberOfArguments; j++)
        {
            printf("\"%s\" \t", _simpleCommands[i]->_arguments[j]);
        }
    }

    printf("\n\n");
    printf("  Output       Input        Error        Err&Out       Background\n");
    printf("  ------------ ------------ ------------ ------------ ------------\n");
    printf("  %-12s %-12s %-12s %-12s %-12s\n", _outFile ? _outFile : "default",
           _inputFile ? _inputFile : "default", _errFile ? _errFile : "default", _out_error == 1 ? _errFile : "default"
           ,_background ? "YES" : "NO");
    printf("\n\n");
}

void Command::execute()
{   
    if(_numberOfSimpleCommands == 0){
        std::cerr << "Error: No command to execute." << std::endl;
        clear();
        return; 
    }

    char *firstCmd = _simpleCommands[0]->_arguments[0];

    if(strcmp(firstCmd, "exit") == 0){
        printf("Good bye!!\n");
        clear();
        exit(0);
    }

    if(strcmp(firstCmd, "cd") == 0){
        print();
        int numArgs = _simpleCommands[0]->_numberOfArguments;

        if(numArgs > 2){
            std::cerr << "Error: cd: too many arguments." << std::endl;
        }
        else{
            const char *targetDir;
            if(numArgs == 1){
                targetDir = getenv("HOME");
            }
            else{
                targetDir = _simpleCommands[0]->_arguments[1];
            }

            if(targetDir == nullptr || chdir(targetDir) < 0){
                std::cerr << "Error: cd: unable to change directory to: " << (targetDir ? targetDir : "HOME") << std::endl;
            }
        }
        clear();
        return;
    }

    print();

    int tmpin = dup(0);
    int tmpout = dup(1);
    int tmperr = dup(2);

    if(_errFile != nullptr){
        int err_flags = O_WRONLY | O_CREAT | (_out_error ? O_APPEND : O_TRUNC);
        int errorFd = open(_errFile, err_flags, 0666);
        if(errorFd < 0) {
            std::cerr << "Error: Unable to open error file: " << _errFile << std::endl; 
            close(tmpin);
            close(tmpout);
            close(tmperr);
            clear();
            return;
        }
        dup2(errorFd, STDERR_FILENO);
        close(errorFd);
    }
    
    int fdin;
    if(_inputFile != nullptr){
        fdin = open(_inputFile, O_RDONLY);
        if (fdin < 0) {
            std::cerr << "Error: Unable to open input file: " << _inputFile << std::endl; 
            dup2(tmperr, STDERR_FILENO);
            close(tmpin);
            close(tmpout);
            close(tmperr);
            clear();
            return;
        }
    }
    else {
        fdin = dup(tmpin);
    }

    int fdout;
    std::vector<pid_t> childPids;

    for(int i = 0; i < _numberOfSimpleCommands; i++){
        dup2(fdin, STDIN_FILENO);
        close(fdin);

        if(i == _numberOfSimpleCommands - 1){
            if(_outFile != nullptr){
                int out_flags = O_WRONLY | O_CREAT | ((_append || _out_error) ? O_APPEND : O_TRUNC);
                fdout = open(_outFile, out_flags, 0666);
                if (fdout < 0) {
                    std::cerr << "Error: Unable to open output file: " << _outFile << std::endl; 
                    dup2(tmpin, STDIN_FILENO);
                    dup2(tmpout, STDOUT_FILENO);
                    dup2(tmperr, STDERR_FILENO);
                    close(tmpin);
                    close(tmpout);
                    close(tmperr);
                    clear();
                    return;
                }
            }
            else{
                fdout = dup(tmpout);
            }
        }
        else{
            int fdpipe[2];
            pipe(fdpipe);
            fdout = fdpipe[1];
            fdin = fdpipe[0];
        }

        dup2(fdout, STDOUT_FILENO);
        close(fdout);

        pid_t pid = fork(); 
        if(pid < 0){
            std::cerr << "Error: Fork failed." << std::endl;
            dup2(tmpin, STDIN_FILENO);
            dup2(tmpout, STDOUT_FILENO);
            dup2(tmperr, STDERR_FILENO);
            close(tmpin);
            close(tmpout);
            close(tmperr);
            clear();
            return; 
        }
        else if(pid == 0){
            close(tmpin);
            close(tmpout);
            close(tmperr);

            if(i < _numberOfSimpleCommands - 1){
                close(fdin);
            }

            SimpleCommand *cmd = _simpleCommands[i];
            char *cmdName = cmd->_arguments[0];
            char **cmdArgs = cmd->_arguments;

            if(!_background){
             signal(SIGINT, SIG_DFL);
            }

            execvp(cmdName, cmdArgs);
            std::cerr << "Error: execvp failed for command: " << cmdName << std::endl;
            exit(1);
        }
        else{
            childPids.push_back(pid); 
        }
    }

    dup2(tmpin, STDIN_FILENO);
    dup2(tmpout, STDOUT_FILENO);
    dup2(tmperr, STDERR_FILENO);
    close(tmpin);
    close(tmpout);
    close(tmperr);

    if(!_background){
     for(size_t i = 0; i < childPids.size(); i++){
         int status;
         pid_t waitedPid = waitpid(childPids[i], &status, 0);
         if(waitedPid > 0){
             int logFd = open("log.txt", O_WRONLY | O_CREAT | O_APPEND, 0666);
             if(logFd >= 0){
                 std::string logMsg = "Child process " + std::to_string(waitedPid) + " terminated\n";
                 write(logFd, logMsg.c_str(), logMsg.length());
                 close(logFd);
                }
            }
        }
    }

    clear();
}

void Command::prompt()
{
    std::string input;
    while (true)
    {   
        printf("myshell>");
        fflush(stdout);

        if (!std::getline(std::cin, input)) {
            break;
        }
        std::vector<Token> tokens = tokenize(input);
        parse(tokens);
    }
}

Command Command::_currentCommand;
SimpleCommand *Command::_currentSimpleCommand;

void handle_sigchld(int sig)
{
    (void)sig; 
    int status;
    pid_t pid;

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        int logFd = open("log.txt", O_WRONLY | O_CREAT | O_APPEND, 0666);
        if (logFd >= 0) {
            std::string logMsg = "Child process " + std::to_string(pid) + " terminated\n";
            write(logFd, logMsg.c_str(), logMsg.length());
            close(logFd);
        }
    }
}

int main()
{
    signal(SIGINT, SIG_IGN);
    signal(SIGCHLD, handle_sigchld);

    Command::_currentCommand.prompt();
    return 0;
}
