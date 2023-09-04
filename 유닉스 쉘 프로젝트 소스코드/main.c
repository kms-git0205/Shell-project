//main.c

#include "header.h"
const char * prompt = "myshell> ";
char *commandlookup[] = {"cd", "exit", "\0"};

static sigjmp_buf jmpbuf;
pid_t shellpid; //save shell pid

void sig_handler(int errno){

        switch(errno){
                case SIGINT:
                case SIGQUIT:
                        if(getpid() == shellpid){       //shell
                                printf("\n");
                                siglongjmp(jmpbuf, 1);
                        }
                        else{   //child
                                exit(0);
                        }
                        break;
                case SIGCHLD:
                        waitpid(-1, NULL, 0);
                //      cmdline[0] = '\0';
                        break;
        }
}


int main(int argc, char**argv){
  
  //signal handling
  shellpid = getpid();	//get shell pid

  static struct sigaction act;
  act.sa_handler = sig_handler;
  act.sa_flags = SA_RESTART;
  sigfillset(&(act.sa_mask));
  sigaction(SIGINT, &act, NULL);
  sigaction(SIGQUIT, &act, NULL);
  sigaction(SIGCHLD, &act, NULL); 
  //signal handling



  pid_t pid;
  while (1) {
	sigsetjmp(jmpbuf, 1);  
        fputs(prompt, stdout);
	fgets(cmdline, BUFSIZ, stdin);
        cmdline[strlen(cmdline) -1] = '\0';
//mycode/////////////////////////////////////////////
	

	//background////////////////////////////////
	int background = 0;	//handle background process
	int target = 0;		
	while(1){
		for(int i=0; cmdline[i]!=0; i++){
			if(cmdline[i] == '&'){
				background = 1;
				target = i;
				break;
			}
		}

		if(background == 0) break;
		if(background == 1){
			cmdline[target] = '\0';
			makelist(cmdline, " \t", cmdvector, MAX_CMD_ARG);
			pid = fork();
			if(pid == 0){	//child
				
				int pid2;
				pid2 = fork();
				if(pid2 == 0){
				
					blockintquit();	//signal handling
					execvp(cmdvector[0], cmdvector);	//grandchild
				}
					else waitpid(pid2, NULL, WNOHANG);		//child	

				return 0;
			}
			else if(pid == -1){
				fatal("main()");
			}
			else{		//parent
				int j=0;
				for(int i=target+1; cmdline[i]!='\0'; i++){
					cmdline[j] = cmdline[i];
					j++;
				}
				cmdline[j] = '\0';
				background = 0;
				waitpid(pid, NULL, 0);
			}
		}
	}
	

	//background//////////////////////////////////////////

	
	
//mycode/////////////////////////////////////////////

	execute_pipe(cmdline);
  }
  return 0;
}

