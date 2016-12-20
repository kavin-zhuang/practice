/*
 * task_monitor.c
 *
 * Copyright (C) 2016.11, jinfeng <kavin.zhuang@gmail.com>
 *
 * add field to TCB, to log thread last start time.
 *
 * reference: cpuuse_show()
 *
 * Q: use thread_t as thread ID, is it OK?
 *
 * should take sample for 1min, so the storage size is confirmed based on reality.
 */

#include <sys/types.h>
#include <RTOS/thread.h>
#include <pthread_hook.h>
#include <driver.h>

#include "ringbuf.h"

#define TASK_RUNINFO_ITEM_SIZE		(8+8+4)
#define TASK_RUNINFO_STORAGE_SIZE	(60*1024*TASK_RUNINFO_ITEM_SIZE) // ~1MB

#define MONITOR_MAJOR		150
#define MONITOR_MIN_START 	0

struct thread_runinfo {
	u64 start_timestamp;
};

typedef struct {
	char path[64];
	unsigned int dev;
} monitor_priv_t;

/* thread_runinfo's offset in TCB */
static size_t thread_runinfo_offset;

static ringbuf_t task_runinfo_ring;
static unsigned char task_runinfo_storage[TASK_RUNINFO_STORAGE_SIZE];
static boolean flag_ringbuf_full = false;

static monitor_priv_t monitor_priv;

static void switch_hook(thread_t executing, thread_t heir)
{
	int ret;

	u64 end_timestamp = sys_timestamp();
	u64 start_timestamp = (*(struct thread_runinfo **)((void*)executing+thread_runinfo_offset))->start_timestamp;
	u32 thread_id = executing;

	ret = ringbuf_put(&task_runinfo_ring, &start_timestamp, sizeof(start_timestamp));
	if(!ret) {
		goto execption;
	}
	ret = ringbuf_put(&task_runinfo_ring, &end_timestamp, sizeof(end_timestamp));
	if(!ret) {
		goto execption;
	}
	ret = ringbuf_put(&task_runinfo_ring, &thread_id, sizeof(thread_id));
	if(!ret) {
		goto execption;
	}

	return;

execption:
	flag_ringbuf_full = true;
}

static int monitor_data_get(void *opened, uint8_t *buffer, size_t count)
{
	int datalen, ret;

	datalen = ringbuf_get_datalen(&task_runinfo_ring);

	datalen = (datalen > count) ? count : datalen;

	ret = ringbuf_get(&task_runinfo_ring, buffer, datalen);

    return ret;
}

struct file_operations monitor_ops = {
		read: monitor_data_get,
};

int task_monitor_init(void)
{
	dev_t major, minor;

	/* 注册数据存储区 */
	ringbuf_init(&task_runinfo_ring, task_runinfo_storage, TASK_RUNINFO_STORAGE_SIZE);

	/* 注册数据源 */
	if (OS_OK != add_registered_field("thread_runinfo", NULL, &thread_runinfo_offset)) {
		return OS_ERROR;
	}

	/* 注册采样点 */
	if(OS_OK != pthread_switch_hook_add(switch_hook)){
		return OS_ERROR;
	}

	/* 注册用户获取数据的接口 */
	if(OS_OK != register_driver(MONITOR_MAJOR, &monitor_ops, &major)) {
		return OS_ERROR;
	}
	if(OS_OK != chrdev_minor_request(MONITOR_MAJOR, MONITOR_MIN_START, 1, MONITOR_MIN_START+1, &minor)) {
		return OS_ERROR;
	}
	monitor_priv.dev = MKDEV(major, minor);
	sprintf(monitor_priv.path, "/dev/task_monitor");
	register_chrdev(monitor_priv.path, monitor_priv.dev, &monitor_priv);

	return 0;
}

int task_monitor_storage_status(void)
{
	return ringbuf_get_datalen(&task_runinfo_ring);
}
