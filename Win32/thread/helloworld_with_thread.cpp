/*
* helloworld_with_thread.cpp
*
* Copyright (C) 2016.11, jinfeng <kavin.zhuang@gmail.com>
*/

/* Windows */
#include <Windows.h>

/* Multithread */
#include <process.h>

/* ISO C */
#include <stdint.h>
#include <stdio.h>

#pragma warning(disable:4996)

#define TRACE(fmt, ...) printf("[%s:%d] " fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)

static unsigned int __stdcall thread_entry(void* args)
{
  DWORD id = GetCurrentThreadId();

  while (1) {
    printf("thread(%d)\n", id);
    Sleep(1000);
  }

  return 0;
}

void thread_init(void)
{
  HANDLE hThread;

  unsigned id;

  hThread = (HANDLE)_beginthreadex(NULL, 0, &thread_entry, NULL, 0, &id);
  if (NULL == hThread) {
    TRACE("thread create failed\n");
    return;
  }

  TRACE("thread(%d) created\n", id);
}

int WINAPI wWinMain(HINSTANCE, HINSTANCE, LPTSTR, int)
{
  MSG msg;

  AllocConsole();
  freopen("CONOUT$", "w", stdout);

  printf("Hello World\n");

  thread_init();

  while (GetMessage(&msg, NULL, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  return 0;
}
