#include <linux/utsname.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

#define VERSION_MODIFIED _IOR('v', 1, bool)
#define VERSION_RESET _IO('v', 2)

#define MAX_VERSION_LENGTH 256

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Votre Nom");
MODULE_DESCRIPTION("Un module pour gérer la version du noyau");

static char *version;
static bool version_modified = false;

static char *initial_version;

static ssize_t version_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
    size_t len = strlen(version);

    if (*ppos >= len)
        return 0;

    if (count > len - *ppos)
        count = len - *ppos;

    if (copy_to_user(buf, version + *ppos, count))
        return -EFAULT;

    *ppos += count;
    return count;
}

static long version_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    switch (cmd) {
    case VERSION_MODIFIED:
		{
			bool is_modified = (strcmp(version, initial_version) != 0);
        	if (copy_to_user((int __user *)arg, &is_modified, sizeof(bool)))
            	return -EFAULT;
        	break;
		}
	case VERSION_RESET:
		{
			strncpy(version, initial_version, MAX_VERSION_LENGTH);
			version_modified = false;
			break;
		}
    default:
        return -ENOTTY;
    }
    return 0;
}

static ssize_t version_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{

	if (count >= MAX_VERSION_LENGTH)
		return -EINVAL;

	if (copy_from_user(version, buf, count))
		return -EFAULT;

	version[count] = '\0';
	version_modified = true;
	return count;
}

static const struct file_operations version_fops = {
    .owner = THIS_MODULE,
    .read = version_read,
    .write = version_write,
    .unlocked_ioctl = version_ioctl,
};


static struct miscdevice version_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "version",
    .fops = &version_fops,
};

static int __init version_init(void)
{
 
	misc_register(&version_device);
	struct new_utsname *uname = utsname();
	version = kmalloc(MAX_VERSION_LENGTH, GFP_KERNEL);
	initial_version = kmalloc(MAX_VERSION_LENGTH, GFP_KERNEL);
	if (!version || !initial_version)
	{
		pr_err("version: kmalloc failed\n");
		return -ENOMEM;
	}
	strncpy(version, uname->version, MAX_VERSION_LENGTH);
	strncpy(initial_version, uname->version, MAX_VERSION_LENGTH);
	int len = strlen(uname->version);
	if (len < MAX_VERSION_LENGTH - 1)
	{
		version[len] = '\n';
		version[len + 1] = '\0';
		initial_version[len] = '\n';
		initial_version[len + 1] = '\0';
	}
	pr_info("version module loaded\n");
	return 0;
}

static void __exit version_exit(void)
{
    misc_deregister(&version_device);
    kfree(version);
    pr_info("version module unloaded\n");
}

module_init(version_init);
module_exit(version_exit);


