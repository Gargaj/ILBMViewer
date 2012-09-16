#include <windows.h>
#include <ddraw.h>
#include <tchar.h>

class czGraphics
{
public:
  czGraphics( int x, int y, bool fullscreen );

  ~czGraphics();

  LRESULT WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

  int Init(HINSTANCE hInst);

  int InitDirectDraw( );
  void Close();

  void DestroyDirectDraw();
  void Messages();
  void Update(void*vscr);

  HWND hWnd;

  int CZ_WIDTH;
  int CZ_HEIGHT;

  LPDIRECTDRAW7 g_pDD;
  LPDIRECTDRAWSURFACE7 g_pDDSPrimary;
  LPDIRECTDRAWSURFACE7 g_pDDSBack;

  int quit;
  int bpp;

  RECT g_rcWindow  ;
  RECT g_rcViewport;
  RECT g_rcScreen  ;

  int mouseclicked;
  int mouserelease;
  bool fullscreen;
  int doublesize;
  int ontop;

  int mousex;
  int mousey;
  int active;

};
