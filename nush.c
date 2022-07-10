#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <assert.h>
#include <fcntl.h>
#include "tokens.h"
#include "ast.h"

int parseArgs(ast* tokens);

// executes the array of arguments passed to it or returns failed exit code
int
execute(char* args[])
{
    int exit_code = 1;
    int cpid;
    if ((cpid = fork())) {
        int status;
        waitpid(cpid, &status, 0);
        exit_code = WEXITSTATUS(status);
    }
    else {
        execvp(args[0], args);
        _exit(1);
    }
    return exit_code;
}

// runs code on left, if its exit code is 0 runs code on right
int logicAnd(ast* left, ast* right) {
    int exit_code = parseArgs(left);
    if (exit_code == 0 && exit_code != 2) {
        exit_code = parseArgs(right);
    }
    return exit_code;
}

// runs code on left, it exit code is 1 runs code on right
int logicOr(ast* left, ast* right) {
    int exit_code = parseArgs(left);
    if (exit_code != 0 && exit_code != 2) {
        exit_code = parseArgs(right);
    }
    return exit_code;
}

// runs code on left, then code on right
int semicolon(ast* left, ast* right) {
    int exit_code = parseArgs(left);
    if (exit_code != 2) {
        exit_code = parseArgs(right);
    }
    return exit_code;
}

int background(ast* left) {
    int cpid;
    if((cpid = fork())) {

    } else {
        parseArgs(left);
        _exit(0);
    }
    return 0;
}
 
// close stdin, make the right argument the new stdin, run code on left.
// if the specified file doesn't exist, create it
int redirectIn(ast* left, ast* right) {
    int cpid;
    int exit_code = 1;
    if((cpid = fork())) {
        int status;
        waitpid(cpid, &status, 0);
        exit_code = WEXITSTATUS(status);
    } else {     
        int fd = open(right->value[0], O_RDWR | O_CREAT | O_SYNC, 0644);
        assert(fd != 0);
        close(0);
        int fd2 = dup(fd);
        close(fd);
        assert(fd2 == 0);
        exit_code = parseArgs(left);
        close(fd2);
        _exit(exit_code);
    }
    return exit_code;
}

// close stdout, make the right argument the new stdout, run code on left.
// if a file doesn't exit, create it
int redirectOut(ast* left, ast* right) {
    int cpid;
    int exit_code = 1;
    if((cpid = fork())) {
        int status;
        waitpid(cpid, &status, 0);
        exit_code = WEXITSTATUS(status);
    } else {
        int fd = open(right->value[0], O_RDWR | O_CREAT | O_SYNC, 0644);
        assert(fd != 1);
        close(1);
        int fd2 = dup(fd);
        close(fd);
        assert(fd2 == 1);
        exit_code = parseArgs(left);
        close(fd2);
        _exit(exit_code);
    }
    return exit_code;
}

// close both stdin and stdout and rerout them to fd[0] and fd[1], and then run the code
// on each side.
int pipeCmd(ast* left, ast* right) {
    int cpid;
    int exit_code = 0;

    if((cpid = fork())) {
        int status;
        waitpid(cpid, &status, 0);
        exit_code = WEXITSTATUS(status);

    } else {
        int cpid2;
        int fds[2];
        pipe(fds);
        int exit2 = 1;

        if ((cpid2 = fork())) {
            int status2;
            exit_code = waitpid(cpid, &status2, 0);
            close(fds[1]);

        } else {
        
            close(1); 
            int fd = dup(fds[1]);
            assert(fd == 1);   

            exit_code = parseArgs(left);
            _exit(exit_code);
        }

        if ((cpid2 = fork())) {
            int status2;
            exit2 = waitpid(cpid, &status2, 0);
            close(fds[0]);
        } else {
     
            close(0); 
            int fd = dup(fds[0]);
            assert(fd == 0);
            parseArgs(right);
            _exit(exit_code);
        }
        _exit(exit_code || exit2);      
    }
    return exit_code;
}

int parens(ast* left) {
    int cpid;
    if ((cpid = fork())) {
        int status;
        waitpid(cpid, &status, 0);
    } else {
        int exit_code;
        exit_code = parseArgs(left);
        _exit(exit_code);
    }
    return 0;
}


int parseArgs(ast* tokens) {
    int exit_code = 0;
    int is_assignment = 0;
    // if the ast contains values
    if (is_value(tokens)) {
        char** value = tokens->value;
        // check for variable assignments and existing env vars
        for (int i = 0; i < tokens->valSize; i++) {
            if (*value[i] == '$') {
                const char* env = value[i] + 1;
                value[i] = strdup(getenv(env));
            } else {
                int c = 0;
                char str1[strlen(value[i])];
                char str2[strlen(value[i])];
                while (value[i][c] != 0) {
                    if (value[i][c] == '=') {
                        is_assignment = 1;
                        str1[c] = 0;
                        for (int r = 0; r < strlen(value[i]) - c - 1; r++) {
                            str2[r] = value[i][r + c + 1];
                        }
                        str2[strlen(value[i]) - c] = 0;
                        setenv(str1, str2, 1);
                    }
                    str1[c] = value[i][c];
                    c++;
                }
            }
        }
        // if the command is change directory, do so
        if (strcmp(value[0], "cd") == 0) {
            assert(value[1] != 0);
            chdir(value[1]);
        }
        // if the command is to exit the program, do so
        else if(strcmp(value[0], "exit") == 0) {
            exit_code = 2;    
        // if the command is not a variable assignment, execute it
        } else if (!is_assignment) {
            // loop through each of the tokens, finding the quotes and reconstructing the tokens
            // without them
            // Credit to StackOverflow user Sam for the algorithm
            for (int i = 0; i < tokens->valSize; i++) {
                int c = 0;
                while (c < strlen(value[i])) {
                    if (value[i][c] == '"') {
                        for (int r = c; r < strlen(value[i]); r++) {
                           value[i][r] = value[i][r+1];                          
                        }
                    } else {                       
                        c++;
                    }                    
                }
            }
            exit_code = execute(value);
        }
    } 
    // if the ast is an operator, run the appropriate code and get the exit code
    else if (is_operator(tokens)) { 
        char* op = tokens->op;
        if(strcmp(op, ";") == 0) {
            exit_code = semicolon(tokens->left, tokens->right);
        }
        else if(strcmp(op, "&&") == 0) {
            exit_code = logicAnd(tokens->left, tokens->right);
        }
        else if(strcmp(op, "||") == 0) {
            exit_code = logicOr(tokens->left, tokens->right);
        }
        else if(strcmp(op, "|") == 0) {
            exit_code = pipeCmd(tokens->left, tokens->right);
        }
        else if(strcmp(op, "&") == 0) {
            exit_code = background(tokens->left);
            if (tokens->right) {
                exit_code = parseArgs(tokens->right);
            }
        }
        else if(strcmp(op, ">") == 0) {
            exit_code = redirectOut(tokens->left, tokens->right);
        }
        else if(strcmp(op, "<") == 0) {
            exit_code = redirectIn(tokens->left, tokens->right);
        }
        else if(strcmp(op, "()") == 0) {
            exit_code = parens(tokens->left);
        } else {
            puts("Illegal Operator");
            exit(1);
        }
    } else {
        exit_code = 0;
    }
    // return the exit code so the program knows how to progress
    return exit_code;
}     

int needsMoreInput(char* cmd) {
    return cmd[strlen(cmd)-2] == '\\';
}

char* getInput() {
    char cmd[256];
    fflush(stdout);
    return fgets(cmd, 256, stdin);
}
    
int
main(int argc, char* argv[])
{
    char cmd[256];
    int exit_code = 0;
    // take normal input
    if (argc == 1) {  
	    while(1) {    
		    printf("nush$ ");
		    fflush(stdout);
            memset(cmd, 0, 256);
		    char* sv = fgets(cmd, 256, stdin);
		    if (!sv) {
			    printf("\n");
			    exit(0);
		    }
            if (needsMoreInput(cmd)) {
                char* temp = getInput();
                int length = strlen(cmd);
                for (int i = 0; i < strlen(temp); i++) {
                    cmd[i + length - 2] = temp[i];
                }
            } 
            ast* tokens = tokenize(cmd);
            exit_code = parseArgs(tokens);
            free_ast(tokens);
            if(exit_code == 2) {
                exit(0);
            }
	    }
    // take input from file
    } else {
        int fd = open(argv[1], O_RDONLY);
        assert(fd != 0);
        close(0);
        int fd2 = dup(fd);
        close(fd);
        assert(fd2 == 0);
        while(1) {
            fflush(stdout);
            memset(cmd, 0, 256);
		    char* sv = fgets(cmd, 256, stdin);
		    if (!sv) {
			    exit(0);
		    }
            if (needsMoreInput(cmd)) {
                char* temp = getInput();
                int length = strlen(cmd);
                for (int i = 0; i < strlen(temp); i++) {
                    cmd[i + length - 2] = temp[i];
                }
            } 
            ast* tokens = tokenize(cmd);
            exit_code = parseArgs(tokens);
            free_ast(tokens);
            if(exit_code == 2) {
                exit(0);
            }
        }
    }
    return 0;
}
