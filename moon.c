//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
//_/			Moon Distributed File System
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

/* -----------------------------------------------------------------------------
 * Kernel Module & Object Storage Driver Headers
 * -------------------------------------------------------------------------- */
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include <linux/vmalloc.h>
#include <linux/errno.h>

#include <linux/fs.h>
#include <linux/blkdev.h>

/* -----------------------------------------------------------------------------
 * Kernel Module Description
 * -------------------------------------------------------------------------- */
MODULE_AUTHOR("Tuntunkun");
MODULE_DESCRIPTION("Moon File System");
MODULE_LICENSE("GPL");

#define LOGICAL_BLOCK_SIZE 512
#define LOGICAL_SECTOR_SIZE 512
#define STORAGE_DEVICE_NAME "moon0"

/* -----------------------------------------------------------------------------
 * Manage Moon Device Parameter
 * -------------------------------------------------------------------------- */
static struct moon_param {
	int major_number;
	struct request_queue *queue;
} Moon;

static struct moon_device {
	unsigned long size;
	spinlock_t lock;
	u8 *data;
	struct gendisk *gd;
} Device;

/* -----------------------------------------------------------------------------
 * Kernel module initialize function
 * -------------------------------------------------------------------------- */
static int __init kernel_module_init_function(void)
{
	int major_num = 0;

	/*
	 * Set up our internal device.
	 */
	Device.size = LOGICAL_SECTOR_SIZE * LOGICAL_BLOCK_SIZE;
	spin_lock_init(&Device.lock);
	Device.data = vmalloc(Device.size);
	if (Device.data == NULL) return -ENOMEM;

	/*
	 * Get a request queue.
	 */
	//Moon.queue = blk_init_queue(sbd_request, &Device.lock);

	/*
	 * Register Block Device
	 */
	major_num = register_blkdev(major_num, STORAGE_DEVICE_NAME);
	Moon.major_number = major_num;
	if (major_num < 0) {
		vfree(Device.data);
		return -ENOMEM;
	}

	/*
         * And the gendisk structure.
         */
	/*
	Device.gd = alloc_disk(16);
	if (Device.gd == NULL) {
		unregister_blkdev(Moon.major_number, STORAGE_DEVICE_NAME);
		vfree(Device.data);
		return -ENOMEM;
	}
	*/


	return 0;
}

/* ----------------------------------------------------------------------------
 * Kernel module settlement function
 * ------------------------------------------------------------------------- */
static void __exit kernel_module_exit_function(void)
{

	/*
	 * Unregister Block Device
	 */
	unregister_blkdev(Moon.major_number, STORAGE_DEVICE_NAME);

	/*
	 * free device data
	 */
	vfree(Device.data);

	return;
}

/* ----------------------------------------------------------------------------
 * Register init & exit function
 * ------------------------------------------------------------------------- */
module_init(kernel_module_init_function);
module_exit(kernel_module_exit_function);


