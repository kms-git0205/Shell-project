//fn.c

#include "header.h"


void fatal(char *str){
        perror(str);
        exit(1);
}

int makelist(char *s, const char *delimiters, char** list, int MAX_LIST){
  int i = 0;
  int numtokens = 0;
  char *snew = NULL;

  if( (s==NULL) || (delimiters==NULL) ) return -1;

  snew = s + strspn(s, delimiters);     /* Skip delimiters */
 
  if( (list[numtokens]=strtok(snew, delimiters)) == NULL )
    return numtokens;

  numtokens = 1;

  while(1){
     if( (list[numtokens]=strtok(NULL, delimiters)) == NULL)
        break;
     if(numtokens == (MAX_LIST-1)) return -1;
     numtokens++;
  }
  return numtokens;
}

void specialcase(int handlenum, int vsize){
	switch(handlenum){
		case 0:	//cd
			cmd_cd(cmdvector[1], vsize);
			break;
		case 1:	//exit
			exit(0);
			break;

	}

}

void cmd_cd(char *path, int vsize ){
	
	char ch[256];
	getcwd(ch, 256);
	
	if(vsize==1) {
		chdir(getenv("HOME"));
		return;
	}

	if(path[0] == '~'){
		// cd ~
		if( strcmp(path, "~") == 0 ) chdir(getenv("HOME"));
		else {
			path++;	//path = "/path"

			if( path[0] == '/' ){	//cd ~/path
				chdir(getenv("HOME"));
				path++;	//path = "path"
				if( chdir(path) == -1 ){
					chdir(ch);
					perror("cmd_cd");
					return;
				}
			}
			else{	//cd ~name
				chdir(getenv("HOME"));
				chdir("..");
				if( chdir(path) == -1 ){
					chdir(ch);
                                	perror("cmd_cd");
                                        return;
				}
					
			}


		}

	}
	else{
		if( chdir(path) == -1 ){
			perror("cmd_cd");
                        return;
	       	}
	}
}

void blockintquit(){ //block SIGINT and SIGQUIT
        sigset_t set1;
        sigemptyset(&set1);
        sigaddset(&set1, SIGINT);
        sigaddset(&set1, SIGQUIT);
        sigprocmask(SIG_SETMASK, &set1, NULL);
}


void execute_small(char *cmd, int fd0, int fd1){

	if(cmd[0] == '\0') return;
	char *commandlookup[] = {"cd", "exit", "\0"};
	int jj; //restore cmdline

	char *cmdtmp = (char*)malloc(sizeof(char)*MAX_CMD_ARG);
	int a;
	for(a=0; cmdline[a]!='\0'; a++) cmdtmp[a] = cmdline[a];
	cmdtmp[a] = '\0';

	makelist(cmd, " \t", cmdvector, MAX_CMD_ARG);	


        for(jj=0; cmdtmp[jj]!='\0'; jj++) cmdline[jj] = cmdtmp[jj];
        cmdline[jj] = '\0';	//restore cmdline

        int vsize = 0;          //same as argc
        for(vsize; cmdvector[vsize]!='\0'; vsize++) ;
        int flag=0;             //handle specialcase
        int handlenum=-1;


        for(int i=0; commandlookup[i][0]!='\0'; i++){   //special case (cd, exit...)
                if( strcmp(commandlookup[i], cmdvector[0]) == 0){
                        flag = 1;
                        handlenum = i;
                        break;
                }
        }
        if(flag == 1){
                specialcase(handlenum, vsize);
                return;
        }



	pid_t pid;
        char *arg;
        int fd;
	int j;
	switch(pid=fork()){
        case 0:

		if(fd1 != -1){
                	dup2(fd1, 1);
	        }
       		if(fd0 !=-1){   //for pipe
                	dup2(fd0, 0);
       		}

		//redirection
                for(int i=0; cmdline[i]!='\0'; i++){

                        switch(cmdline[i]){
                                case '<':
                                        arg = strtok(&cmdline[i+1], " \t><");
			 		if( (fd=open(arg, O_RDONLY|O_CREAT, 0644)) < 0) fatal("file open error");

					for(jj=0; cmdtmp[jj]!='\0'; jj++) cmdline[jj] = cmdtmp[jj];
        				cmdline[jj] = '\0';     //restore cmdline

                                        dup2(fd, 0);
                                        close(fd);
                                        cmdline[i] = '\0';
                                        cmdtmp[i] = '\0';
					break;
                                case '>':

					arg = strtok(&cmdline[i+1], " \t><");
					
					if( (fd=open(arg, O_WRONLY|O_CREAT|O_TRUNC, 0644)) < 0) fatal("file open error");

					for(jj=0; cmdtmp[jj]!='\0'; jj++) cmdline[jj] = cmdtmp[jj];
        				cmdline[jj] = '\0';     //restore cmdline
                                        dup2(fd, 1);
                                        close(fd);
                                        cmdline[i] = '\0';
					cmdtmp[i] = '\0';
                                        break;

                                default: break;

                        }
                }
		//redirection
		makelist(cmd, " \t", cmdvector, MAX_CMD_ARG);
		execvp(cmdvector[0], cmdvector);
                fatal("main()");
        case -1:
                fatal("main()");
        default:
                waitpid(pid, NULL, 0);
        }
	

}


void execute_pipe(char *cmd){
	char *cmdgrp[MAX_CMD_ARG];

	makelist(cmd, "|", cmdgrp, MAX_CMD_ARG);

	
	

	if(cmdgrp[1] == '\0'){
		execute_small(cmd, -1, -1);
		return;
	}

	int tmpfd[2];
//	dup2(0, tmpfd[0]);
//	dup2(1, tmpfd[1]);

	int fd[2][2];
	int a=0;
	int b=1;
	if(pipe(fd[0]) == -1) fatal("fail to call pipe");
	if(pipe(fd[1]) == -1) fatal("fail to call pipe");
	int grpsize = 0;
	for(grpsize = 0; cmdgrp[grpsize]!='\0'; grpsize++) continue;
	//accessible : cmdgrp[grpsize-1]

	for(int i=0; cmdgrp[i]!='\0'; i++){
		if(i==0) {//first
			execute_small(cmdgrp[i], -1, fd[a][1]);
		}
		else if(i==grpsize-1) {	//last
			execute_small(cmdgrp[i], fd[a][0], -1);	
		}


		else{	//middle
			execute_small(cmdgrp[i], fd[a][0], fd[b][1]);
			int tmp=a;
                        a=b;
                        b=tmp;
		}
	}//for
	close(fd[0][0]);
	close(fd[0][1]);
	close(fd[1][0]);
	close(fd[1][1]);	
}
