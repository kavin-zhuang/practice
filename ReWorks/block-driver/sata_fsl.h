#ifndef _REWORKS_SIMPLE_BLKDEV_H_
#define _REWORKS_SIMPLE_BLKDEV_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <string.h>
#include <stdio.h>
#include <reworksio.h>
#include <driver.h>
#include <fs.h>

/* 简单块设备主设备号 */
#define SIMPLE_BLKDEV_MAJOR             200

/* 简单块设备块大小定义 */
#define SIMPLE_BLKDEV_BLOCK_SIZE		512

#define RAMDEV_NAME_MAX 32

/* 简单块设备定义 */

struct sblkdev_struct
{
       struct block_device	blkdev;
       char					*addr;
       char                 name[32];
       int 					devnum;
};

/* 定义操作 */

/* 读 */
int sblkdev_read(struct block_device *blkdev, int start, int blocks, char *buffer);

/* 写 */
int sblkdev_write(struct block_device *blkdev, int start, int blocks, char *buffer);

/* 控制 */
int sblkdev_ioctl(void *registered, int cmd, int arg);

/* 复位 */
int sblkdev_reset(struct block_device *blkdev);

/* 检查状态 */
int sblkdev_check(struct block_device *blkdev);

/* 显示信息 */
int sblkdev_show(struct block_device *blkdev);

#ifdef __cplusplus
}
#endif

#endif

