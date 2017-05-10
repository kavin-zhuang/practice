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

/* �򵥿��豸���豸�� */
#define SIMPLE_BLKDEV_MAJOR             200

/* �򵥿��豸���С���� */
#define SIMPLE_BLKDEV_BLOCK_SIZE		512

#define RAMDEV_NAME_MAX 32

/* �򵥿��豸���� */

struct sblkdev_struct
{
       struct block_device	blkdev;
       char					*addr;
       char                 name[32];
       int 					devnum;
};

/* ������� */

/* �� */
int sblkdev_read(struct block_device *blkdev, int start, int blocks, char *buffer);

/* д */
int sblkdev_write(struct block_device *blkdev, int start, int blocks, char *buffer);

/* ���� */
int sblkdev_ioctl(void *registered, int cmd, int arg);

/* ��λ */
int sblkdev_reset(struct block_device *blkdev);

/* ���״̬ */
int sblkdev_check(struct block_device *blkdev);

/* ��ʾ��Ϣ */
int sblkdev_show(struct block_device *blkdev);

#ifdef __cplusplus
}
#endif

#endif

