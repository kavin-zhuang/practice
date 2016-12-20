﻿/*
* hello_child_window.cpp
* Copyright (C) 2016.11, jinfeng <kavin.zhuang@gmail.com>
*/

/* Windows API */
#include <Windows.h>

/* ISO C99 */
#include <stdint.h>
#include <stdio.h>

#pragma warning(disable:4996)

#define TRACE(fmt, ...) printf("[%s:%d] " fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)

#define CONTENT_MAX 256

static TCHAR winclass[] = TEXT("mainwindow");
static TCHAR wintitle[] = TEXT("demo");
static int win_left = 500;
static int win_top = 200;
static int win_width = 800;
static int win_height = 600;

static HWND hwndEdit = NULL;

static LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  HDC         hdc;
  PAINTSTRUCT ps;

  wchar_t editContent[CONTENT_MAX];

  switch (uMsg)
  {
  case WM_CREATE:
    hwndEdit = CreateWindow(L"edit", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER, 0, 0, 100, 20, hwnd, NULL, NULL, NULL);
    return 0;
  case WM_COMMAND:
    if ((HWND)lParam == hwndEdit) {
      //printf("msg from edit: ID=%x MSG=%x\n", LOWORD(wParam), HIWORD(wParam));
      // EN_SETFOCUS
      if (EN_MAXTEXT == HIWORD(wParam)) {
        //printf("[MSG] text area is full\n");
      }
      if (EN_CHANGE == HIWORD(wParam)) {
        GetWindowText(hwndEdit, editContent, CONTENT_MAX);
        wprintf(L"%s\n", editContent);
      }
    }
    return 0;
  case WM_PAINT:
    hdc = BeginPaint(hwnd, &ps);

    EndPaint(hwnd, &ps);
    return 0;
  case WM_DESTROY:
    PostQuitMessage(0);
    return 0;
  }
  return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

static int register_window(HINSTANCE hInstance)
{
  WNDCLASSEX wce = { 0 };

  wce.cbSize = sizeof(wce);
  wce.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
  wce.hCursor = LoadCursor(NULL, IDC_ARROW);
  wce.hIcon = LoadIcon(NULL, IDI_APPLICATION);
  wce.hInstance = hInstance;
  wce.lpfnWndProc = WndProc;
  wce.lpszClassName = winclass;
  wce.style = CS_HREDRAW | CS_VREDRAW;
  if (!RegisterClassEx(&wce)) {
    return 1;
  }

  return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
  MSG msg;
  HWND hwnd;

  /* Console for DEBUG */
  AllocConsole();
  freopen("CONOUT$", "w", stdout);

  printf("Hello World\n");

  /* Windows Register */
  if (0 != register_window(hInstance)) {
    return 1;
  }

  /* Create Windows & Do Message Loop */
  hwnd = CreateWindowEx(0, winclass, wintitle, WS_OVERLAPPEDWINDOW,
    win_left, win_top, win_width, win_height,
    NULL, NULL, hInstance, NULL);
  if (hwnd == NULL) {
    return 1;
  }

  ShowWindow(hwnd, nShowCmd);
  UpdateWindow(hwnd);

  while (GetMessage(&msg, NULL, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  return msg.wParam;
}
