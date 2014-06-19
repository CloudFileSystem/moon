//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
//_/			Moon Distributed File System
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

/*
 * Kernel Module & Object Storage Driver Headers
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>

#define DEVICE_NAME "moon0"

MODULE_AUTHOR("Tuntunkun");
MODULE_DESCRIPTION("Speed File System");
MODULE_LICENSE("GPL");

/*
 * The internal representation of our device.
 */
static struct sbd_device {
	unsigned long size;
	spinlock_t lock;
	u8 *data;
	struct gendisk *gd;
} Device;

/*
 * Kernel Registration Operation
 */
int dev_major	= 0;
char *dev_name	= DEVICE_NAME;

int init_module(void)
{
	printk(KERN_ALERT "REGISTER OPERATION\n");
	dev_major = register_blkdev(0, dev_name);

	return 0;
}

void cleanup_module(void)
{
	printk(KERN_ALERT "UNREGISTER OPERATION\n");
	unregister_blkdev(dev_major, dev_name);
};


