#include <linux/module.h>
#include <linux/kernel.h>


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Will");
MODULE_DESCRIPTION("X");



static int __init init_hello(void){
	printk("hello Will\n");
	return 0;
}

static void __exit exit_hello(void){
	printk("bye Will\n");
}

module_init(init_hello);
module_exit(exit_hello);
