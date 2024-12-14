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
MODULE_DESCRIPTION("Hook __x64_sys_getdents using kretprobes");

struct linux_dirent64 {
    __u64           d_ino;       /* Numéro d'inode */
    __s64           d_off;       /* Décalage vers l'entrée suivante */
    unsigned short  d_reclen;    /* Longueur de cette entrée */
    unsigned char   d_type;      /* Type de fichier (DT_UNKNOWN, DT_DIR, DT_REG, etc.) */
    char            d_name[];    /* Nom du fichier (terminé par \0) */
};

static int entry_handler(struct kretprobe_instance *ri, struct pt_regs *regs)
{
    /* Rien à faire dans l'entrée pour ce cas */
    return 0;
}

static int ret_handler(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	struct linux_dirent64 *output = (struct linux_dirent64*)regs_return_value(regs);
	while (output != NULL) {
		char *name = kmalloc(output->d_reclen, GFP_KERNEL);
		if (name == NULL) {
			pr_err("[-] Failed to allocate memory\n");
			return -1;
		}
		if (copy_from_user(name, output->d_name, output->d_reclen)) {
			pr_err("[-] Failed to copy data from user space\n");
			kfree(name);
			return -1;
		}
		pr_info("[+] File: %s\n", name);
		kfree(name);
		if (output->d_off == 0) {
			break;
		}
		output = (struct linux_dirent64*)((char*)output + output->d_off);
	}
	return 0;
}

static struct kretprobe krp = {
    .handler = ret_handler,
    .entry_handler = entry_handler,
    .data_size = 0,
    .maxactive = 20,
};

static int __init hook_init(void)
{
    krp.kp.symbol_name = "__x64_sys_getdents64";
    if (register_kretprobe(&krp)) {
        pr_err("[-] Failed to register kretprobe\n");
        return -1;
    }
    pr_info("[+] __x64_sys_getdents hooked successfully at address: %p\n", krp.kp.addr);
    return 0;
}

static void __exit hook_exit(void)
{
    unregister_kretprobe(&krp);
    pr_info("[+] __x64_sys_getdents unhooked successfully\n");
}

module_init(hook_init);
module_exit(hook_exit);

