#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>

char *sig_list[]=
{
	"SIGHUP","SIGINT","SIGQUIT","SIGILL","SIGTRAP","SIGABRT","SIGBUS","SIGFPE",
	"SIGKILL","SIGUSR1","SIGSEGV","SIGUSR2","SIGPIPE","SIGALRM","SIGTERM","SIGSTKFLT",
	"SIGCHLD","SIGCONT","SIGSTOP","SIGTSTP","SIGTTIN","SIGTTOU","SIGURG","SIGXCPU",
	"SIGXFSZ","SIGVTALRM","SIGPROF","SIGWINCH","SIGIO","SIGPWR","SIGSYS"
};

int main(int argc,char *argv[]){
	pid_t pid;
	int status;
	/* fork a child process */
	printf("Process start to fork\n");
	pid=fork();
	if(pid==-1)
	{
		perror("fork");
		exit(1);
	}
	else
	{
		if(pid==0)
		{
			printf("I'm the child process,my pid = %d\n",getpid());
			char *arg[argc];
			for(int i=0;i<argc-1;i++)
				arg[i]=argv[i+1];
			arg[argc-1]=NULL;
			/* execute test program */ 
			printf("Child process start to execute test program\n");
			execve(arg[0],arg,NULL);
			exit(1);
		}
		else
		{
			printf("I'm the parent process,my pid = %d\n",getpid());
			/* wait for child process terminates */
			waitpid(pid,&status,WUNTRACED);
			printf("Parent process receives SIGCHLD signal\n");
			/* check child process'  termination status */
			if(!WIFEXITED(status))
			{
				if(WIFSTOPPED(status))
					printf("Child process get %s signal\n",sig_list[WSTOPSIG(status)-1]);
				else if(WIFSIGNALED(status))
					printf("Child process get %s signal\n",sig_list[WTERMSIG(status)-1]);
			}
			else
				printf("Normal termination with EIXT STATUS %d\n",status);
		}
	}

	return 0;
}
