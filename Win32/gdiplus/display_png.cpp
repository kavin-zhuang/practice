/*
* display_png.cpp
*
* Copyright (C) 2016.11, jinfeng <kavin.zhuang@gmail.com>
*/

#include <Windows.h>
#include <gdiplus.h>

#include <stdio.h>

#pragma warning(disable:4996)

/* 链接用 */
#pragma comment(lib,"gdiplus")

/* 编译用-包含多个命名空间的情况下，或者是C++标准需要的 */
using namespace Gdiplus;

void draw_image(HWND hWnd, HDC hdc)
{
  int width, height;

  Image image(TEXT("头像.png"));
  if (image.GetLastStatus() != Status::Ok){
    MessageBox(hWnd, TEXT("图片无效!"), NULL, MB_OK);
    return;
  }

  width = image.GetWidth();
  height = image.GetHeight();

  Graphics graphics(hdc);
  graphics.DrawImage(&image, 0, 0, width, height);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  HDC         hdc;
  PAINTSTRUCT ps;
  RECT rect;

  static int pretick = 0;
  static int fps = 0;

  switch (uMsg)
  {
  case WM_TIMER:
    GetClientRect(hWnd, &rect);
    InvalidateRect(hWnd, &rect, FALSE);
    return 0;

  case WM_CREATE:
    SetTimer(hWnd, NULL, 10, NULL);
    return 0;
  case WM_PAINT:

    if (GetTickCount() - pretick > 1000) {
      printf("fps: %d\n", fps);
      fps = 0;
      pretick = GetTickCount();
    }

    hdc = BeginPaint(hWnd, &ps);

    draw_image(hWnd, hdc);

    EndPaint(hWnd, &ps);

    fps++;

    return 0;
  case WM_DESTROY:
    PostQuitMessage(0);
    return 0;
  }
  return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
  MSG msg;
  WNDCLASSEX wce = { 0 };
  HWND hWnd;

  /* Console for DEBUG */
  AllocConsole();
  freopen("CONOUT$", "w", stdout);

  printf("Hello World\n");

  wce.cbSize = sizeof(wce);
  wce.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
  wce.hCursor = LoadCursor(NULL, IDC_ARROW);
  wce.hIcon = LoadIcon(NULL, IDI_APPLICATION);
  wce.hInstance = hInstance;
  wce.lpfnWndProc = WndProc;
  wce.lpszClassName = TEXT("MyClassName");
  wce.style = CS_HREDRAW | CS_VREDRAW;
  if (!RegisterClassEx(&wce))
    return 1;

  wchar_t* title = TEXT("Win32SDK GDI+ 图像显示示例程序");
  hWnd = CreateWindowEx(0, TEXT("MyClassName"), title, WS_OVERLAPPEDWINDOW,
    CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
    NULL, NULL, hInstance, NULL);
  if (hWnd == NULL)
    return 1;

  //GdiPlus初始化
  ULONG_PTR gdiplusToken;
  GdiplusStartupInput gdiplusInput;
  GdiplusStartup(&gdiplusToken, &gdiplusInput, NULL);

  //窗口接收文件拖放
  DragAcceptFiles(hWnd, TRUE);

  ShowWindow(hWnd, nShowCmd);
  UpdateWindow(hWnd);

  while (GetMessage(&msg, NULL, 0, 0)){
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  //GdiPlus 取消初始化
  GdiplusShutdown(gdiplusToken);

  return msg.wParam;
}
