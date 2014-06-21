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
#include <linux/genhd.h>
#include <linux/hdreg.h>

/* -----------------------------------------------------------------------------
 * Define
 * -------------------------------------------------------------------------- */
#define KERNEL_SECTOR_SIZE 512
#define LOGICAL_SECTOR_SIZE 512
#define LOGICAL_SECTOR_NUMBER 2048
#define STORAGE_DEVICE_NAME "moon0"

/* -----------------------------------------------------------------------------
 * Function Prototype
 * -------------------------------------------------------------------------- */


/* -----------------------------------------------------------------------------
 * Manage Moon Device Parameter
 * -------------------------------------------------------------------------- */
static struct moon_device_param {
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
 * Storage Device Request
 * -------------------------------------------------------------------------- */
static int __init kernel_module_init_function(void);
static void __exit kernel_module_exit_function(void);
static void moon_device_request(struct request_queue *queue);
static void moon_device_transfer(struct moon_device *dev, sector_t sector,
				unsigned long nsect, char *buffer, int write);

/*
 * The HDIO_GETGEO ioctl is handled in blkdev_ioctl(), which
 * calls this. We need to implement getgeo, since we can't
 * use tools such as fdisk to partition the drive otherwise.
 */
int sbd_getgeo(struct block_device *block_device, struct hd_geometry *geo) {
	long size;

	/* We have no real geometry, of course, so make something up. */
	size = Device.size * (LOGICAL_SECTOR_SIZE / KERNEL_SECTOR_SIZE);
	geo->cylinders = (size & ~0x3f) >> 6;
	geo->heads = 4;
	geo->sectors = 16;
	geo->start = 0;
	return 0;
}

/*
 * The device operations structure.
 */
static struct block_device_operations moon_device_ops = {
	.owner  = THIS_MODULE,
	.getgeo = sbd_getgeo
};

/* -----------------------------------------------------------------------------
 *
 * -------------------------------------------------------------------------- */
static void moon_device_transfer(struct moon_device *dev, sector_t sector,
				unsigned long nsect, char *buffer, int write) {
	unsigned long offset = sector * LOGICAL_SECTOR_SIZE;
	unsigned long nbytes = nsect * LOGICAL_SECTOR_SIZE;

	if ((offset + nbytes) > dev->size) {
		printk (KERN_NOTICE "sbd: Beyond-end write (%ld %ld)\n", offset, nbytes);
		return;
	}
	if (write)
		memcpy(dev->data + offset, buffer, nbytes);
	else
		memcpy(buffer, dev->data + offset, nbytes);
}

static void moon_device_request(struct request_queue *q)
{
	struct request *req;

	req = blk_fetch_request(q);
	while (req != NULL) {
		// blk_fs_request() was removed in 2.6.36 - many thanks to
		// Christian Paro for the heads up and fix...
		// if (!blk_fs_request(req)) {
		if (req == NULL || (req->cmd_type != REQ_TYPE_FS)) {
			printk (KERN_NOTICE "Skip non-CMD request\n");
			__blk_end_request_all(req, -EIO);
			continue;
		}

		moon_device_transfer(&Device, blk_rq_pos(req), blk_rq_cur_sectors(req),
			req->buffer, rq_data_dir(req));

		if (!__blk_end_request_cur(req, 0)) {
			req = blk_fetch_request(q);
		}
	}
}


/* -----------------------------------------------------------------------------
 * Kernel module initialize function
 * -------------------------------------------------------------------------- */
static int __init kernel_module_init_function(void)
{
	/*
	 * =ブロックデバイスのバッファ取得=
	 * ブロックデバイスが使用するバッファ用のメモリを取得
	 */
	Device.size = (LOGICAL_SECTOR_SIZE * LOGICAL_SECTOR_NUMBER);
	spin_lock_init(&Device.lock);
	Device.data = vmalloc(Device.size);
	if (Device.data == NULL) return -ENOMEM;

	/*
	 * =I/Oスケジューラ=
	 * blk_init_queue関数を使用して、ブロックI/O要求のスケジュールに
	 * 使用するキューを作成する
	 * -引数1: キューに対して入出力が生じたら呼び出される関数
	 * -引数2: スピンロック
	 * -返り値: 作成されたI/Oスケジューラのキュー
	 *
	 * [参考文献]
	 * IOスケジューラ: http://wiki.bit-hive.com/linuxkernelmemo/pg/I/Oスケジューラ
	 */
	Moon.queue = blk_init_queue(moon_device_request, &Device.lock);
	if (Moon.queue == NULL) {
		vfree(Device.data);
		return -ENOMEM;
	}

	/*
	 * =ブロック型デバイスのドライバをカーネルに登録=
	 * register_blkdev関数を使用してドライバを登録する
	 * -引数1: メジャー番号(分からない場合は0を設定する)
	 * -引数2: デバイス名
	 * -返り値: メジャー番号(エラーの場合は、0以下)
	 *
	 * ブロック型デバイスを登録する。メジャー番号は、分からないので
	 * 引数を0に設定する事で取得する。
	 *
	 * [予備知識]
	 * -メジャー番号: デバイスドライバを識別する番号
	 * -マイナー番号: デバイスドライバが捜査する機器を識別する番号
	 * [参考文献]
	 * -デバイスファイル: http://ja.wikipedia.org/wiki/デバイスファイル
	 */
	Moon.major_number = register_blkdev(0, STORAGE_DEVICE_NAME);
	if (Moon.major_number < 0) {
		vfree(Device.data);
		return -ENOMEM;
	}

	/*
         * 仮想ディスクの作成
         * alloc_disk関数を使用して仮想的なディスクを作成します
         * -引数1: マイナー番号数(捜査を行う機器の数)
         *  (例)HDDの場合には、作成可能なパーティションの数
         */
	Device.gd = alloc_disk(1);
	if (Device.gd == NULL) {
		unregister_blkdev(Moon.major_number, STORAGE_DEVICE_NAME);
		vfree(Device.data);
		return -ENOMEM;
	}
	Device.gd->major = Moon.major_number;
	Device.gd->first_minor = 0;

	// ディスクをオープンしたときなどに呼び出される関数の登録
	Device.gd->fops = &moon_device_ops;
	Device.gd->private_data = &Device;

	// スペシャルファイル名(例)[/dev/sda]
	strcpy(Device.gd->disk_name, STORAGE_DEVICE_NAME);
	set_capacity(Device.gd, LOGICAL_SECTOR_NUMBER);

	// ブロックデバイスにキューを結びつける
	Device.gd->queue = Moon.queue;

	/*
	 * =仮想ディスクを登録=
	 * スペシャルファイルは自動的に/devに作られる
	 */
	add_disk(Device.gd);

	return 0;
}

/* ----------------------------------------------------------------------------
 * Kernel module settlement function
 * ------------------------------------------------------------------------- */
static void __exit kernel_module_exit_function(void)
{
	del_gendisk(Device.gd);
	put_disk(Device.gd);

	/*
	 * キューの削除
	 */
	blk_cleanup_queue(Moon.queue);

	/*
	 * ブロック型デバイスドライバの登録解除
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

/* -----------------------------------------------------------------------------
 * Kernel Module Description
 * -------------------------------------------------------------------------- */
MODULE_AUTHOR("Tuntunkun");
MODULE_DESCRIPTION("Moon File System");
MODULE_LICENSE("GPL");

