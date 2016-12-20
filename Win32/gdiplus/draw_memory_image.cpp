/*
* draw_memory_image.cpp
* Copyright (C) 2016.11, jinfeng <kavin.zhuang@gmail.com>
*/

void draw_rect(HWND hwnd, HDC hdc)
{
  Graphics graphics(hdc);

  /* ��ɫ�� */
  Gdiplus::Color color(255, 0, 0);
  
  /* ׼�����ʻ�ˢ */
  Gdiplus::Pen line_pen(color, 3);
  Gdiplus::SolidBrush brush(color);
    
  /* ׼��100x100�Ļ��� */
  Gdiplus::Bitmap bitmap(100, 100);
  Graphics *g = Graphics::FromImage(&bitmap);

  /* ������ī */
  
  g->DrawLine(&line_pen, 0, 0, 100, 100);
  g->DrawEllipse(&line_pen, 0, 0, 50, 50);

  /* ��ʾ���� */
  graphics.DrawImage(&bitmap, 100, 100, 100, 100);
}