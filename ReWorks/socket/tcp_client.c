/*
 * tcpcli.c
 * Copyright (C) 2016.11, jinfeng <kavin.zhuang@gmail.com>
 *
 * descriptions
 */

#include <stdio.h>
#include <stdint.h>

#include <socket.h>
#include <netinet/in.h>

/*
 *==============================================================================
 * Macros
 *==============================================================================
 */

#define PRINT(fmt, ...) \
  printf(fmt, ##__VA_ARGS__)

#define TRACE(fmt, ...) \
  printf("TRACE: %s:%d: " fmt "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__)

#define EXCEPTION(fmt, ...) \
  printf("EXCEPTION: %s:%d: " fmt "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__)

#define SERVER_IP	"192.168.10.254"
#define SERVER_PORT	8090

#define BUFFER_SIZE	1024

/*
 *==============================================================================
 * Local Functions
 *==============================================================================
 */

static void thread_entry(void *arg)
{
	int ret;
	int fd = 0;
	struct sockaddr_in addr;

	/* 分配资源 */

	uint8_t *buffer = (uint8_t*)malloc(BUFFER_SIZE);
	if(NULL == buffer) {
		goto end;
	}

	fd = socket(AF_INET, SOCK_STREAM, 0);
	if(-1 == fd) {
		EXCEPTION("create failed\n");
		goto end;
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(SERVER_PORT);
	addr.sin_addr.s_addr = inet_addr(SERVER_IP);

	ret = connect(fd, &addr, sizeof(addr));
	if(-1 == ret) {
		EXCEPTION("connect failed\n");
		goto end;
	}

	while(1) {
		strcpy(buffer, "Hello Server\n");
		ret = send(fd, buffer, strlen(buffer), 0);
		if(-1 == ret) {
			EXCEPTION("connect failed\n");
			goto end;
		}
		usleep(100*1000);
	}

end:

	/* 释放资源 */

	if(fd >= 0) {
		close(fd);
	}
	if(buffer) {
		free(buffer);
	}

	TRACE("thread end\n");
}

/*
 *==============================================================================
 * Global Functions
 *==============================================================================
 */

int tcp_client_init(void)
{
	int ret;
	pthread_t pid;

	ret = pthread_create2(&pid, "tcpcli", 100, 0, 4096, thread_entry, NULL);
	if(0 != ret) {
		EXCEPTION("create thread failed\n");
		return -1;
	}
}
