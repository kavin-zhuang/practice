/*
* draw_memory_image.cpp
* Copyright (C) 2016.11, jinfeng <kavin.zhuang@gmail.com>
*/

void draw_rect(HWND hwnd, HDC hdc)
{
  Graphics graphics(hdc);

  /* 调色板 */
  Gdiplus::Color color(255, 0, 0);
  
  /* 准备画笔画刷 */
  Gdiplus::Pen line_pen(color, 3);
  Gdiplus::SolidBrush brush(color);
    
  /* 准备100x100的画布 */
  Gdiplus::Bitmap bitmap(100, 100);
  Graphics *g = Graphics::FromImage(&bitmap);

  /* 挥洒笔墨 */
  
  g->DrawLine(&line_pen, 0, 0, 100, 100);
  g->DrawEllipse(&line_pen, 0, 0, 50, 50);

  /* 显示出来 */
  graphics.DrawImage(&bitmap, 100, 100, 100, 100);
}