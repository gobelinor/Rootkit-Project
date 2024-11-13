// hello_module.c
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Dakou");
MODULE_DESCRIPTION("A Simple Hello World Module");
MODULE_VERSION("1.0");

void hello(void);

static int __init hello_init(void) {
    printk(KERN_INFO "Hello, World!\n");
    return 0;  // Return 0 for successful initialization
}

static void __exit hello_exit(void) {
    printk(KERN_INFO "Goodbye, World!\n");
}

void hello(void) {
    printk(KERN_INFO "Hello from myModuleHello\n");
}
EXPORT_SYMBOL(hello);

module_init(hello_init);
module_exit(hello_exit);
