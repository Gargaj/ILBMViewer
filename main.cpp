#include <Windows.h>
#include <tchar.h>
#include "ILBM.h"
#include <stdio.h>
#include "czGraphics.h"

void _tmain(int argc, _TCHAR* argv[])
{
  printf("ILBMViewer by Gargaj / Conspiracy\n");
  printf("\n");
  printf("Based on the documentation of Jerry Morrison (Electronic Arts)\n");
  printf("  http://home.comcast.net/~erniew/lwsdk/docs/filefmts/ilbm.html\n");
  printf("\"Blend-shift\" algorithm originally devised by Joseph Huckaby\n");
  printf("  http://www.effectgames.com/demos/canvascycle/\n");
  printf("\n");
  printf("Keys:\n");
  printf("  1-5: Cycling speed (25%%, 50%%, 100%%, 150%%, 200%% respectively)\n");
  printf("  F/D: Zoom on/off (keeps aspect ratio, only uses integer multiples)\n");
  printf("  R/E: Blend-shift on/off\n");
  printf("  Alt-Enter: Switch to/from fullscreen\n");

  if (argc < 2)
  {
    printf("\n");
    printf("Usage: ILBMViewer.exe <filename>\n");
    printf("\n");
    return;
  }

  CILBMViewer ilbm;
  if (!ilbm.Load( argv[1] ))
    return;

  //   int w = ilbm.GetWidth();
  //   int h = ilbm.GetHeight();
  int w = 1280;
  int h = 720;
  czGraphics graph( w, h, false );
  graph.Init( GetModuleHandle(NULL) );

  unsigned int * vscr = new unsigned int[ w * h ];
  unsigned int * pic = new unsigned int[ ilbm.GetWidth() * ilbm.GetHeight() ];

  bool bZoom = true;

  while (!graph.quit)
  {
    ZeroMemory(vscr,w * h * sizeof(unsigned int));
    graph.Messages();
    if (graph.active)
    {
      if (GetAsyncKeyState('F') & 0x8000) bZoom = true;
      if (GetAsyncKeyState('D') & 0x8000) bZoom = false;
      if (GetAsyncKeyState('R') & 0x8000) ilbm.bBlendShift = true;
      if (GetAsyncKeyState('E') & 0x8000) ilbm.bBlendShift = false;

      if (GetAsyncKeyState('1') & 0x8000) ilbm.nFPS = 15;
      if (GetAsyncKeyState('2') & 0x8000) ilbm.nFPS = 30;
      if (GetAsyncKeyState('3') & 0x8000) ilbm.nFPS = 60;
      if (GetAsyncKeyState('4') & 0x8000) ilbm.nFPS = 90;
      if (GetAsyncKeyState('5') & 0x8000) ilbm.nFPS = 120;
    }

    ilbm.UpdatePalette( GetTickCount() );
    ilbm.RenderTo32bit( pic );

    int zoomX = w / ilbm.GetWidth();
    int zoomY = h / ilbm.GetHeight();
    int ratio = bZoom ? min(zoomX,zoomY) : 1;

    int cx = (w - (ilbm.GetWidth()  * ratio)) / 2;
    int cy = (h - (ilbm.GetHeight() * ratio)) / 2;
    for (int y = 0; y < ilbm.GetHeight() * ratio; y++)
    {
      for (int x = 0; x < ilbm.GetWidth() * ratio; x++)
      {
        vscr[ cx + x + (cy + y) * w ] = pic[ (x / ratio) + (y / ratio) * ilbm.GetWidth() ];
      }
    }

    graph.Update( vscr );
  }
  delete[] pic;
  delete[] vscr;
}