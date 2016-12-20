#include <driver.h>
#include <semaphore.h>

#include "sata_fsl.h"

// 操作接口

int sblkdev_read(struct block_device *blkdev, int start, int blocks, char *buffer)
{
   struct sblkdev_struct *sblkdev = (struct sblkdev_struct *)blkdev;
   
   printf("%s: start=%d blocks=%d\n", __func__, start, blocks);

   register int byte_per_block = sblkdev->blkdev.bd_bytesPerBlk;

   bcopy(sblkdev->addr + (start * byte_per_block),

			buffer,

			blocks * byte_per_block);

   return 0;
}


int sblkdev_write(struct block_device *blkdev, int start, int blocks, char *buffer)
{
   struct sblkdev_struct *sblkdev = (struct sblkdev_struct *)blkdev;

   register int byte_per_block = sblkdev->blkdev.bd_bytesPerBlk;
   
   printf("%s: start=%d blocks=%d\n", __func__, start, blocks);

   bcopy(buffer,
			sblkdev->addr + (start * byte_per_block),
			blocks * byte_per_block);

   return 0;
}

int sblkdev_show(struct block_device *blkdev)
{
   struct sblkdev_struct *ramdev = (struct sblkdev_struct *)blkdev;

   printf("%-12s %-12s %-12s %-12s %-12s %-12s\n",
		"name", "startaddress",
				 "bytesperblk",
				 "blkspertrack",
				 "blocks",
				 "devnum");

    printf("---------------------------------------------------------------\n");

    printf("%-12s 0x%-10x %-12d %-12d %-12d %-12d\n",
                  ramdev->name,
                  (u32)ramdev->addr,
                  ramdev->blkdev.bd_bytesPerBlk,
                  ramdev->blkdev.bd_blksPerTrack,
                  ramdev->blkdev.bd_nBlocks,
                  ramdev->devnum);

    return 0;
}

int ramdev_ioctl(struct block_device *blkdev, int cmd, int arg)
{
	printf("%s %d\n", __func__, __LINE__);
	
	return 0;
}

int ramdev_reset(struct block_device *blkdev)
{
	printf("%s %d\n", __func__, __LINE__);
	
	return 0;
}

int ramdev_check(struct block_device *blkdev)
{
	printf("%s %d\n", __func__, __LINE__);
	
	return 0;
}

// 模块注册接口
int ramdev_module_init(const char *devname, int size)
{
    dev_t registered_major = 0;

    static ramdev_module_init_flag = 0;

    if (ramdev_module_init_flag) {
        return 0;
    }

    struct sblkdev_struct *ramdev;

    int blocks;

    dev_t minor = 0;

    char fullpath[PATH_MAX];

    /* 检查参数 */

    if (0 >= size) {
        printf("[ERROR] ramdev size error: %d\n", size);
        return -1;
    }

    /* 设备路径 */
    memset(fullpath, 0, PATH_MAX);

    /* 相对路径 */
    if ('/' != devname[0]) {
        strcpy(fullpath, "/dev/");
        strcat(fullpath, devname);
    }

    else {
        strcpy(fullpath, devname);
    }

    /* 分配内存硬盘 */
    if (NULL == (ramdev = (struct sblkdev_struct *)malloc(sizeof(struct sblkdev_struct)))) {
        printf("[ERROR] cannot create ramdev desc\n");
        return -1;
    }

    memset(ramdev, 0, sizeof(struct sblkdev_struct));

    /* 分配存储区 */

    blocks = (size << 20) / SIMPLE_BLKDEV_BLOCK_SIZE;

    if (NULL == (ramdev->addr = (char *)malloc(size << 20))) {
        printf("[ERROR] cannot malloc for ramdev\n");
        free(ramdev);
        return -1;
    }
    
    printf("block address = 0x%08x\n", ramdev->addr);

    /* 设置属性 */

    memset(ramdev->name, 0, RAMDEV_NAME_MAX);

    strncpy(ramdev->name, devname, RAMDEV_NAME_MAX - 1);

    /*  过长则截断 */

    ramdev->devnum = MKDEV(SIMPLE_BLKDEV_MAJOR, minor++);

    ramdev->blkdev.bd_blkRd         = sblkdev_read;
    ramdev->blkdev.bd_blkWrt        = sblkdev_write;
    ramdev->blkdev.bd_ioctl			= ramdev_ioctl;
    ramdev->blkdev.bd_reset			= ramdev_reset;
    ramdev->blkdev.bd_statusChk		= ramdev_check;
    ramdev->blkdev.bd_devShow		= sblkdev_show;
    ramdev->blkdev.bd_nBlocks  		= blocks;
    ramdev->blkdev.bd_nOffset 		= 0;
    ramdev->blkdev.bd_bytesPerBlk	= SIMPLE_BLKDEV_BLOCK_SIZE;
    ramdev->blkdev.bd_blksPerTrack	= 4;
    ramdev->blkdev.bd_nHeads		= 1;
    ramdev->blkdev.bd_mode			= O_RDONLY | O_WRONLY | O_RDWR;

    /* 注册块设备 */
    register_blkdev(fullpath, RAMDISK_MAJOR, ramdev);

    ramdev_module_init_flag = 1;

    return 0;

}
