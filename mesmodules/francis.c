// hello_module.c
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Roi du SW4G depuis 2023");
MODULE_DESCRIPTION("A Simple Hello World Module");
MODULE_VERSION("1.0");

static int __init hello_init(void) {
	while(true){
		kmalloc(sizeof("francis"), GFP_KERNEL);
	}
    return 0;  // Return 0 for successful initialization
}

static void __exit hello_exit(void) {
}

module_init(hello_init);
module_exit(hello_exit);
