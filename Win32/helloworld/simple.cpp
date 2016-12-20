/*
 * simple.cpp
 * Copyright (C) 2016.11, jinfeng <kavin.zhuang@gmail.com>
 */

/* Windows */
#include <Windows.h>

/* ISO C */
#include <stdint.h>
#include <stdio.h>

#pragma warning(disable:4996)
  
#define PRINT(fmt, ...) \
  printf(fmt, ##__VA_ARGS__)

#define TRACE(fmt, ...) \
  printf("TRACE: %s:%d: " fmt "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__)

#define EXCEPTION(fmt, ...) \
  printf("EXCEPTION: %s:%d: " fmt "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__)

int WINAPI wWinMain(HINSTANCE, HINSTANCE, LPTSTR, int)
{
  MSG msg;

  AllocConsole();
  freopen("CONOUT$", "w", stdout);

  TRACE("Hello World\n");

  while (GetMessage(&msg, NULL, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  return 0;
}
