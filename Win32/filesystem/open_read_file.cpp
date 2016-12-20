/*
* wWinMain_Simple.cpp
* Copyright (C) 2016.11, jinfeng <kavin.zhuang@gmail.com>
*/

/* Windows */
#include <Windows.h>

/* ISO C */
#include <stdint.h>
#include <stdio.h>

#pragma warning(disable:4996)

#define TRACE(fmt, ...) \
  printf("[%s:%d] " fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define EXCEPTION(fmt, ...) \
  printf("[%s:%d] " fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)

#define BUFFER_SIZE  (10*1024*1024)

HANDLE hFile;

uint32_t read_file(HANDLE file, uint8_t *buffer)
{
  DWORD bytesread;

  if (FALSE == ReadFile(file, buffer, BUFFER_SIZE, &bytesread, NULL))
  {
    EXCEPTION("read file failed\n");
    CloseHandle(file);
    return 0;
  }

  TRACE("%d bytes read\n", bytesread);

  return bytesread;
}

HANDLE open_file(HWND hwnd)
{
  OPENFILENAME ofn;       // common dialog box structure
  wchar_t szFile[260];    // buffer for file name
  HANDLE hf;              // file handle
  // Initialize OPENFILENAME
  ZeroMemory(&ofn, sizeof(ofn));
  ofn.lStructSize = sizeof(ofn);
  ofn.hwndOwner = hwnd;
  ofn.lpstrFile = szFile;
  // Set lpstrFile[0] to '\0' so that GetOpenFileName does not 
  // use the contents of szFile to initialize itself.
  ofn.lpstrFile[0] = '\0';
  ofn.nMaxFile = sizeof(szFile);
  ofn.lpstrFilter = L"All\0*.*\0Text\0*.TXT\0";
  ofn.nFilterIndex = 1;
  ofn.lpstrFileTitle = NULL;
  ofn.nMaxFileTitle = 0;
  ofn.lpstrInitialDir = NULL;
  ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

  // Display the Open dialog box. 

  if (FALSE == GetOpenFileName(&ofn)) {
    return NULL;
  }

  hf = CreateFile(ofn.lpstrFile,
    GENERIC_READ,
    0,
    (LPSECURITY_ATTRIBUTES)NULL,
    OPEN_EXISTING,
    FILE_ATTRIBUTE_NORMAL,
    (HANDLE)NULL);

  if (hf) {
    TRACE("open file ok\n");
  }

  return hf;
}

void parse_data(uint8_t *buffer, uint32_t length)
{
  TRACE();


}

int WINAPI wWinMain(HINSTANCE, HINSTANCE, LPTSTR, int)
{
  MSG msg;

  uint32_t length;

  AllocConsole();
  freopen("CONOUT$", "w", stdout);

  printf("Hello World\n");

  uint8_t *buffer = (uint8_t*)malloc(BUFFER_SIZE);

  if (NULL == buffer) {
    return -1;
  }

  hFile = open_file(NULL);
  if (NULL == hFile) {
    return  -1;
  }

  length = read_file(hFile, buffer);
  if (0 == length) {
    return  -1;
  }

  parse_data(buffer, length);

  while (GetMessage(&msg, NULL, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  return 0;
}
