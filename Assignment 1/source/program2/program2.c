#include <linux/module.h>
#include <linux/sched.h>
#include <linux/pid.h>
#include <linux/kthread.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/printk.h>
#include <linux/jiffies.h>
#include <linux/kmod.h>
#include <linux/fs.h>

MODULE_LICENSE("GPL");

int my_WIFEXITED(int status){return ((status&0x7f)==0);}
int my_WEXITSTATUS(int status){return ((status&0xff00)>>8);}
int my_WIFSIGNALED(int status){return ((((status&0x7f)+1)>>1)>0);}
int my_WTERMSIG(int status){return (status&0x7f);}
int my_WSTOPSIG(int status){return my_WEXITSTATUS(status);}

static struct task_struct *task;

struct wait_opts
{
	enum pid_type wo_type;
	int	wo_flags;
	struct pid *wo_pid;
	struct waitid_info *wo_info;
	int	wo_stat;
	struct rusage *wo_rusage;
	wait_queue_entry_t child_wait;
	int	notask_error;
};

extern pid_t kernel_clone(struct kernel_clone_args *kargs);
extern long do_wait(struct wait_opts *wo);
extern int do_execve(struct filename *filename,
						const char __user *const __user *__argv,
						const char __user *const __user *__envp);
extern struct filename *getname_kernel(const char *filename);

char *sig_list[]=
{
	"SIGHUP","SIGINT","SIGQUIT","SIGILL","SIGTRAP","SIGABRT","SIGBUS","SIGFPE",
	"SIGKILL","SIGUSR1","SIGSEGV","SIGUSR2","SIGPIPE","SIGALRM","SIGTERM","SIGSTKFLT",
	"SIGCHLD","SIGCONT","SIGSTOP","SIGTSTP","SIGTTIN","SIGTTOU","SIGURG","SIGXCPU",
	"SIGXFSZ","SIGVTALRM","SIGPROF","SIGWINCH","SIGIO","SIGPWR","SIGSYS"
};

int my_exec(void)
{
	int result;
	const char path[]="/tmp/test";
	const char *argv[]={path,NULL};
	const char *envp[]={NULL};
	printk("[program2] : child process\n");
	struct filename *file_name=getname_kernel(path);
	result=do_execve(file_name,NULL,NULL);
	//int result = call_usermodehelper(path, (char **)argv, (char **)envp, UMH_WAIT_PROC);
	//printk("%d is result of do_execve\n",result);
	if(!result)
		return 0;
	do_exit(result);
}

int my_wait(pid_t pid)
{
	struct wait_opts wo;
	wo.wo_type=PIDTYPE_PID;
	wo.wo_flags=WEXITED|WUNTRACED;
	wo.wo_pid=find_get_pid(pid);
	wo.wo_info=NULL;
	wo.wo_stat=-1;
	wo.wo_rusage=NULL;

	int tmp;
    tmp=do_wait(&wo);
	put_pid(wo.wo_pid);
	//printk("start to wait\n");
	return wo.wo_stat;
}

//implement fork function
int my_fork(void *argc){
	//set default sigaction for current process
	int i;
	struct k_sigaction *k_action = &current->sighand->action[0];
	for(i=0;i<_NSIG;i++){
		k_action->sa.sa_handler = SIG_DFL;
		k_action->sa.sa_flags = 0;
		k_action->sa.sa_restorer = NULL;
		sigemptyset(&k_action->sa.sa_mask);
		k_action++;
	}
	
	/* fork a process using kernel_clone or kernel_thread */
	int status;
	/*struct kernel_clone_args args;
	args.stack=(unsigned long)&my_exec;
	args.flags=SIGCHLD;
	args.exit_signal=SIGCHLD;
	args.stack_size=0;
	args.parent_tid=NULL;
	args.child_tid=NULL;*/
	struct kernel_clone_args args=
	{
		.stack=(unsigned long)&my_exec,
		.flags=CLONE_FS|SIGCHLD,
		.exit_signal=SIGCHLD,
		.stack_size=0,
		.parent_tid=NULL,
		.child_tid=NULL,
	};

	pid_t pid=kernel_clone(&args);
	printk("[program2] : The child process has pid = %d\n",pid);
	printk("[program2] : This is the parent process, pid = %d\n",current->pid);

	/* execute a test program in child process */
	status=my_wait(pid);
	//printk("[program 2] : status=%d\n",status);
	//printk("process signal number=%d\n",my_WIFSIGNALED(status));
	/* wait until child process terminates */
	if(my_WIFEXITED(status))
	{
		status=status%128;
		printk("[program2] : Child proccess get normal exit with return signal = %d\n",my_WEXITSTATUS(status));
	}
	else if(my_WSTOPSIG(status)==19)
	{
		printk("[program2] : get SIGSTOP signal\n");
		status=19;
	}
	else if(my_WIFSIGNALED(status))
	{
		status=status%128;
		printk("[program2] : get %s signal\n",sig_list[my_WTERMSIG(status)-1]);
	}

	printk("[program2] : child process terminated\n");
	printk("[program2] : The return signal is %d\n",status);
	
	return 0;
}

static int __init program2_init(void){

	printk("[program2] : module_init\n");
	printk("[program2] : module_init create kthread start\n");
	/* create a kernel thread to run my_fork */

	task=kthread_create(&my_fork,NULL,"my_fork");
	if(!IS_ERR(task))
	{
		printk("[program2] : module_init kthreads start\n");
		wake_up_process(task);
	}
	return 0;
}

static void __exit program2_exit(void){
	printk("[program2] : module_exit\n");
}

module_init(program2_init);
module_exit(program2_exit);
