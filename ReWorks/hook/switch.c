/*
 * switch.c
 *
 * Copyright (C) 2017.1, Jinfeng
 */

#include <sys/types.h>
#include <RTOS/thread.h>
#include <pthread_hook.h>

/* thread_runinfo's offset in TCB */
static size_t offset_in_tcb;

static void switch_hook(thread_t executing, thread_t heir)
{
	// ((void*)executing+offset_in_tcb)
}

int user_hook_init(void)
{
	if (0 != add_registered_field("sys_task_start", NULL, &offset_in_tcb)) {
		return -1;
	}
	
	if(0 != pthread_switch_hook_add(switch_hook)){
		return -1;
	}

	return 0;
}
