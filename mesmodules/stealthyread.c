#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kprobes.h>
#include <linux/uaccess.h> // Pour copy_from_user

MODULE_LICENSE("GPL");
MODULE_AUTHOR("tibo.wav");
MODULE_DESCRIPTION("Hook ksys_read using kprobes");

static struct kretprobe krp;
static char kernel_buf[256]; // Tampon temporaire

static int ret_handler(struct kretprobe_instance *ri, struct pt_regs *regs) {
    ssize_t ret = regs_return_value(regs); // Nombre d'octets lus
    char __user *user_buf;

    pr_info("Return value from read: %zd\n", ret);

    if (ret <= 0) {
        pr_info("Read returned no data or an error\n");
        return 0;
    }

    // Obtenez l'adresse du buffer utilisateur depuis les registres
    user_buf = (char __user *)regs->si;
    pr_info("User buffer address (regs->si): %p\n", user_buf);

    // Si le buffer est NULL, ignorez cet appel
    if (!user_buf) {
        pr_warn("Ignoring read with NULL buffer\n");
        return 0;
    }

    // Vérifiez si l'adresse utilisateur est accessible
    if (!access_ok(user_buf, ret)) {
        pr_warn("User buffer is not accessible\n");
        return 0;
    }

    // Limitez la taille copiée
    if (ret > sizeof(kernel_buf)) {
        ret = sizeof(kernel_buf);
    }

    // Tentez de copier les données depuis l'utilisateur
    if (copy_from_user(kernel_buf, user_buf, ret) == 0) {
        pr_info("Buffer content: %.*s\n", (int)ret, kernel_buf);
    } else {
        pr_err("Failed to copy data from user space (ret=%zd, si=%p, di=%lu)\n",
               ret, (void *)user_buf, (unsigned long)regs->di);
    }

    return 0;
}



static int __init hook_init(void) {
    krp.kp.symbol_name = "vfs_read"; // Changer la cible ici
    krp.handler = ret_handler;

    if (register_kretprobe(&krp)) {
        pr_err("[-] Failed to register kretprobe\n");
        return -1;
    }

    pr_info("[+] ksys_read hooked successfully\n");
    return 0;
}

static void __exit hook_exit(void) {
    unregister_kretprobe(&krp);
    pr_info("[+] ksys_read unhooked successfully\n");
}

module_init(hook_init);
module_exit(hook_exit);


