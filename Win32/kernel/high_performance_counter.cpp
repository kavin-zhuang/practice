/*
* high_performance_counter.cpp
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

  int64_t counter;
  int64_t precnt = 0;

  int64_t cnt_per_sec;

  double sec_per_cnt;

  QueryPerformanceFrequency((LARGE_INTEGER*)&cnt_per_sec);

  printf("%lld\n", cnt_per_sec);

  sec_per_cnt = 1.0 / cnt_per_sec;

  printf("%0.12lf\n", sec_per_cnt);

  while (1) {
    QueryPerformanceCounter((LARGE_INTEGER*)&counter);
    printf("%lld equals %lf sec\n", counter - precnt, sec_per_cnt * (counter - precnt));
    precnt = counter;
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

  /* make this task only run on Core 1 */
  SetThreadAffinityMask(hThread, (1<<1));

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
