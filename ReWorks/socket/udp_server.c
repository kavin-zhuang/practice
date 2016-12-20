/*
 * udpcli.c
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
	struct sockaddr_in cliaddr;
	socklen_t addrlen = sizeof(cliaddr);
	int bytesrecv = 0;

	/* 分配资源 */

	uint8_t *buffer = (uint8_t*)malloc(BUFFER_SIZE);
	if(NULL == buffer) {
		goto end;
	}

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if(-1 == fd) {
		EXCEPTION("create failed\n");
		goto end;
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(SERVER_PORT);
	addr.sin_addr.s_addr = INADDR_ANY;

	ret = bind(fd, &addr, sizeof(addr));
	if(-1 == ret) {
		EXCEPTION("bind failed\n");
		goto end;
	}

	while(1) {
		ret = recvfrom(fd, buffer, BUFFER_SIZE, 0, &cliaddr, &addrlen);
		if(-1 == ret) {
			EXCEPTION("recvfrom failed\n");
			goto end;
		}

		bytesrecv = ret;

		ret = sendto(fd, buffer, bytesrecv, 0, &cliaddr, addrlen);
		if(-1 == ret) {
			EXCEPTION("sendto failed\n");
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

int udp_server_init(void)
{
	int ret;
	pthread_t pid;

	ret = pthread_create2(&pid, "udpsvr", 100, 0, 4096, thread_entry, NULL);
	if(0 != ret) {
		EXCEPTION("create thread failed\n");
		return -1;
	}
}
