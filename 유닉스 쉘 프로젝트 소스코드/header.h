//header.h

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <signal.h>
#include <setjmp.h>

#define MAX_CMD_ARG 10
#define BUFSIZ 256

extern const char *prompt;
char* cmdvector[MAX_CMD_ARG];
char cmdline[BUFSIZ];


char *commandlookup[];

void fatal(char *str);
int makelist(char *s, const char *delimiters, char** list, int MAX_LIST);
void spacialcase(int handlenum, int vsize);
void cmd_cd(char *path, int vsize);
void blockintquit();
void execute_small(char * cmd, int fd0, int fd1);
void execute_pipe(char *cmd);
