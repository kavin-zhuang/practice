/*
 * draw_rectangle.cpp
 * Copyright (C) 2016.11, jinfeng <kavin.zhuang@gmail.com>
 *
 * 本例程混合了GDIplus和Direct2D编程。
 */

/* Windows API */
#include <Windows.h>

/* GDI+ */
#include <gdiplus.h>

/* Direct2D */
#include <d2d1.h>
#include <d2d1helper.h>
#include <dwrite.h>

/* ISO C99 */
#include <stdint.h>
#include <stdio.h>

using namespace Gdiplus;

#pragma warning(disable:4996)

#pragma comment(lib,"gdiplus")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

static TCHAR winclass[] = TEXT("mainwindow");
static TCHAR wintitle[] = TEXT("demo");
static int win_width = 800;
static int win_height = 600;

static ID2D1Factory* pD2DFactory = NULL;
static ID2D1HwndRenderTarget *pRenderTarget = NULL;
static ID2D1SolidColorBrush *pBlackBrush = NULL;

static int CreateD2DResource(HWND hwnd)
{
  HRESULT hr;
  RECT rc;

  /* Init Direct2D Engine: Work in Single Thread Mode */
  hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &pD2DFactory);
  if (S_OK != hr) {
    printf("2D Factory Inited failed\n");
    return S_FALSE;
  }

  GetClientRect(hwnd, &rc);

  /* create Direct2D Render area */
  hr = pD2DFactory->CreateHwndRenderTarget(
    D2D1::RenderTargetProperties(),
    D2D1::HwndRenderTargetProperties(
    hwnd,
    D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top)
    ),
    &pRenderTarget
    );
  if (S_OK != hr) {
    return S_FALSE;
  }

  /* create Direct2D brush */
  hr = pRenderTarget->CreateSolidColorBrush(
    D2D1::ColorF(D2D1::ColorF::Black),
    &pBlackBrush
    );
  if (S_OK != hr) {
    return S_FALSE;
  }

  return S_OK;
}

static void DrawRectangle(HWND hwnd)
{
  pRenderTarget->BeginDraw();

  pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::White));

  pRenderTarget->DrawRectangle(
    D2D1::RectF(100.f, 100.f, 500.f, 500.f),
    pBlackBrush);

  pRenderTarget->EndDraw();
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  HDC         hdc;
  PAINTSTRUCT ps;

  switch (uMsg)
  {
  case WM_CREATE:
    if (S_OK != CreateD2DResource(hwnd)) {
      printf("D2D Init failed\n");
    }
    return 0;
  case WM_PAINT:
    hdc = BeginPaint(hwnd, &ps);
    DrawRectangle(hwnd);
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
    return S_FALSE;
  }

  return S_OK;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nShowCmd)
{
  MSG msg;
  HWND hwnd;

  /* Console for DEBUG */
  AllocConsole();
  freopen("CONOUT$", "w", stdout);

  printf("Hello World\n");

  /* GDI+ Init */
  ULONG_PTR gdiplusToken;
  GdiplusStartupInput gdiplusInput;
  GdiplusStartup(&gdiplusToken, &gdiplusInput, NULL);

  /* Windows Register */
  if (0 != register_window(hInstance)) {
    return S_FALSE;
  }

  /* Create Windows & Do Message Loop */
  hwnd = CreateWindowEx(0, winclass, wintitle, WS_OVERLAPPEDWINDOW,
    CW_USEDEFAULT, CW_USEDEFAULT, win_width, win_height,
    NULL, NULL, hInstance, NULL);
  if (NULL == hwnd) {
    return S_FALSE;
  }

  ShowWindow(hwnd, nShowCmd);
  UpdateWindow(hwnd);

  while (GetMessage(&msg, NULL, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  return msg.wParam;
}
