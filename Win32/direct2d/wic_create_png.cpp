/*
* main.cpp
* Copyright (C) 2016.11, jinfeng <kavin.zhuang@gmail.com>
*
* 思路：
* 1. 创建WIC位图，创建WIC的Render（包括属于该Render的画刷）绘画
* 2. 将WIC位图转换成D2D位图，注意，这不是指针操作，使用HWND的Render显示D2D位图后，释放D2D位图
* 3. 更新WIC位图，重复第2步
*/

/* Windows API */
#include <Windows.h>

/* GDI+ */
#include <gdiplus.h>

/* Direct2D */
#include <d2d1.h>
#include <d2d1_1.h>
#include <d2d1helper.h>
#include <dwrite.h>

/* WICBitmap */
#include <wincodec.h>

/* ISO C99 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <wchar.h>

/*
*==============================================================================
* Namespaces
*==============================================================================
*/

using namespace Gdiplus;

/*
*==============================================================================
* Compiler Control
*==============================================================================
*/

#pragma warning(disable:4996)

/*
*==============================================================================
* Librarys
*==============================================================================
*/

#pragma comment(lib,"gdiplus")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "Windowscodecs.lib")

/*
*==============================================================================
* Defines
*==============================================================================
*/

#define TRACE(fmt, ...) \
  printf("[%s:%d] " fmt, __FUNCTION__, __LINE__, ##__VAR_ARGS__);

/*
*==============================================================================
* Types
*==============================================================================
*/

typedef struct _timeline {
  uint64_t start;
  uint64_t stop;
  uint32_t task;
} timeline_t;

/*
*==============================================================================
* Local Varibles
*==============================================================================
*/

static TCHAR winclass[] = TEXT("mainwindow");
static TCHAR wintitle[] = TEXT("demo");
static int win_width = 800;
static int win_height = 260;
HWND mainwindow;

static IWICImagingFactory *pWICFactory = NULL;
static ID2D1Factory1* pD2DFactory = NULL;
static IDWriteFactory *pDWriteFactory = NULL;

static ID2D1HwndRenderTarget *pRenderTarget = NULL;
static ID2D1RenderTarget *pWicRenderTarget = NULL;

static ID2D1SolidColorBrush *pBlackBrush = NULL;
static ID2D1SolidColorBrush *pWhiteBrush = NULL;
static ID2D1SolidColorBrush *pRedBrush = NULL;
static ID2D1SolidColorBrush *pWicBlackBrush = NULL;

static IDWriteTextFormat *pTextFormat = NULL;

static IWICBitmap *pWicBitmap = NULL;

static float data_ratio = 1.0;

static IWICStream *pWicStream;
static IWICBitmapEncoder *pWicBitmapEncoder;
static IWICBitmapFrameEncode *pIWICBitmapFrameEncode;

static WICPixelFormatGUID format = GUID_WICPixelFormatDontCare;

/*
*==============================================================================
* Local Functions
*==============================================================================
*/

static int create_d2d_factory(void)
{
  HRESULT hr;
  RECT rc;

  /* Init Direct2D Engine
  *
  * NOTE: Work in Single Thread Mode
  */
  hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &pD2DFactory);
  if (S_OK != hr) {
    printf("2D Factory Inited failed\n");
    return S_FALSE;
  }
}

static int create_text_factory(void)
{
  HRESULT hr;

  hr = DWriteCreateFactory(
    DWRITE_FACTORY_TYPE_SHARED,
    __uuidof(IDWriteFactory),
    (IUnknown**)&pDWriteFactory);
  if (S_OK != hr) {
    printf("DWrite Factory Inited failed\n");
    return S_FALSE;
  }

  hr = pDWriteFactory->CreateTextFormat(
    L"segoe print",
    NULL,
    DWRITE_FONT_WEIGHT_NORMAL,
    DWRITE_FONT_STYLE_NORMAL,
    DWRITE_FONT_STRETCH_NORMAL,
    17.0f,
    L"en-us", //locale
    &pTextFormat);
  if (S_OK != hr) {
    printf("TextFormat Inited failed\n");
    return S_FALSE;
  }

  // Center the text horizontally and vertically.
  //pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
  //pTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
}

static int create_wic_factory(UINT32 width, UINT32 height)
{
  HRESULT hr;

  CoInitialize(NULL);

  /* WIC Init
  *
  * WARNING: CLSID_WICImagingFactory not work on Win7
  * MSDN-"CLSID_WICImagingFactory: breaking change since VS11 Beta on Windows"
  */
  hr = CoCreateInstance(
    CLSID_WICImagingFactory1,
    NULL,
    CLSCTX_INPROC_SERVER,
    IID_IWICImagingFactory,
    (LPVOID*)&pWICFactory
    );
  if (S_OK != hr) {
    printf("err = %x\n", GetLastError());
    return S_FALSE;
  }

  /* Create bitmap for draw */

  /* WARNING: will error if parameters not correct. */
  hr = pWICFactory->CreateBitmap(width, height, GUID_WICPixelFormat32bppPBGRA, WICBitmapCacheOnLoad, &pWicBitmap);
  if (S_OK != hr) {
    printf("%d err = %x\n", __LINE__, GetLastError());
    return S_FALSE;
  }
}

static int create_wic_bitmap_render_target(void)
{
  HRESULT hr;

  /* Create Render target for WIC bitmap */
  hr = pD2DFactory->CreateWicBitmapRenderTarget(
    pWicBitmap,
    D2D1::RenderTargetProperties(),
    &pWicRenderTarget);
  if (S_OK != hr) {
    printf("%d err = %x\n", __LINE__, GetLastError());
    return S_FALSE;
  }
  printf("create WIC Bitmap Render Target OK\n");

  /* WARNING: the brush should be created by correct render target. */
  hr = pWicRenderTarget->CreateSolidColorBrush(
    D2D1::ColorF(D2D1::ColorF::White),
    &pWhiteBrush
    );
  if (S_OK != hr) {
    return S_FALSE;
  }

  hr = pWicRenderTarget->CreateSolidColorBrush(
    D2D1::ColorF(D2D1::ColorF::Red),
    &pRedBrush
    );
  if (S_OK != hr) {
    return S_FALSE;
  }

  hr = pWicRenderTarget->CreateSolidColorBrush(
    D2D1::ColorF(D2D1::ColorF::Black),
    &pWicBlackBrush
    );
  if (S_OK != hr) {
    return S_FALSE;
  }

  return 0;
}

static void draw_bitmap(void)
{
  /* WARNING: render works between begin & end procedure */

  pWicRenderTarget->BeginDraw();

  pWicRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::White));

  pWicRenderTarget->DrawLine(D2D1::Point2F(10, 100), D2D1::Point2F(100, 100), pRedBrush);

  pWicRenderTarget->EndDraw();
}

static int create_draw_room(void)
{
  HRESULT hr;
  RECT rc;

  /* 创立图像处理工厂 */
  create_d2d_factory();

  /* 创建字体资源工厂 */
  create_text_factory();

  /* 创建图片资源工厂 */
  create_wic_factory(win_width, win_height);

  /* 图像处理工厂：绘图器 */
  create_wic_bitmap_render_target();

  return 0;
}

static void create_one_frame(IWICBitmapFrameEncode *frameEncode, int width, int height)
{
  // Just One Frame
  frameEncode->Initialize(NULL);
  frameEncode->SetSize(width, height);
  frameEncode->SetPixelFormat(&format);

  frameEncode->WriteSource(pWicBitmap, NULL);

  frameEncode->Commit();
}

static void save_bitmap_as_file(void)
{
  pWICFactory->CreateStream(&pWicStream);
  pWicStream->InitializeFromFilename(L"demo.png", GENERIC_WRITE);
  pWICFactory->CreateEncoder(GUID_ContainerFormatPng, NULL, &pWicBitmapEncoder);
  pWicBitmapEncoder->Initialize(pWicStream, WICBitmapEncoderNoCache);
  pWicBitmapEncoder->CreateNewFrame(&pIWICBitmapFrameEncode, NULL);

  create_one_frame(pIWICBitmapFrameEncode, win_width, win_height);

  pWicBitmapEncoder->Commit();
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nShowCmd)
{
  /* Console for DEBUG */
  AllocConsole();
  freopen("CONOUT$", "w", stdout);

  printf("Hello World\n");

  create_draw_room();

  draw_bitmap();

  save_bitmap_as_file();

  while (1)
  {
    Sleep(1000);
  }

  return 0;
}
