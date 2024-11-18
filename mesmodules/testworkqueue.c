#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/workqueue.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("tibo.wav");
MODULE_DESCRIPTION("Example of using a workqueue");

/* Déclaration de la workqueue */
static struct workqueue_struct *my_workqueue;

/* Structure pour stocker les données de travail */
struct work_data {
    struct work_struct work;
    char command[64];
};

/* Fonction exécutée dans la workqueue */
static void work_handler(struct work_struct *work) {
    struct work_data *wd = container_of(work, struct work_data, work);
    char *argv[] = { "/bin/sh", "-c", wd->command, NULL };
    char *envp[] = { "HOME=/", "PATH=/sbin:/bin:/usr/sbin:/usr/bin", NULL };

    pr_info("[+] Executing command: %s\n", wd->command);
    call_usermodehelper(argv[0], argv, envp, UMH_WAIT_EXEC);

    kfree(wd); // Libérer la mémoire après exécution
}

/* Initialisation du module */
static int __init my_workqueue_init(void) {
    /* Vérifie si une workqueue existe déjà */
    if (my_workqueue) {
        pr_err("Workqueue already exists!\n");
        return -EINVAL;
    }

    /* Créer une workqueue */
    my_workqueue = create_singlethread_workqueue("communication_wq");
    if (!my_workqueue) {
        pr_err("[-] Failed to create workqueue\n");
        return -ENOMEM;
    }

    /* Planifie une tâche différée comme exemple */
    struct work_data *wd = kmalloc(sizeof(struct work_data), GFP_KERNEL);
    if (!wd) {
        destroy_workqueue(my_workqueue);
        pr_err("[-] Failed to allocate memory for work\n");
        return -ENOMEM;
    }

    INIT_WORK(&wd->work, work_handler);
    snprintf(wd->command, sizeof(wd->command), "echo 'Workqueue example' > /tmp/workqueue_example");
    queue_work(my_workqueue, &wd->work);

    pr_info("[+] Workqueue module loaded successfully\n");
    return 0;
}

/* Nettoyage du module */
static void __exit my_workqueue_exit(void) {
    if (my_workqueue) {
        flush_workqueue(my_workqueue); // Assure que toutes les tâches différées sont terminées
        destroy_workqueue(my_workqueue); // Détruit la workqueue
        my_workqueue = NULL;
    }
    pr_info("[+] Workqueue module unloaded successfully\n");
}

module_init(my_workqueue_init);
module_exit(my_workqueue_exit);

