// hello_module.c
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Roi du SW4G depuis 2023");
MODULE_DESCRIPTION("A Simple Hello World Module");
MODULE_VERSION("1.0");

static int __init hello_init(void) {
    static char nathan[4];
	for (unsigned int i=0; 1; i++)
	{
		nathan[i]='x';
	}
	return 0;  // Return 0 for successful initialization
}

static void __exit hello_exit(void) {
}

module_init(hello_init);
module_exit(hello_exit);
