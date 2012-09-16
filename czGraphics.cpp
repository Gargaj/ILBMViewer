/*

 UD directdraw library by Gargaj & Vercy

 we've been rolling this code in front of us since 2001
 so shut up.

*/

#include "czGraphics.h"
//#include "czDebug.h"
//#include "resource.h"
#include <tchar.h>

#pragma comment(lib,"ddraw.lib")
#pragma comment(lib,"dxguid.lib")

czGraphics::czGraphics( int x, int y, bool fs ) {
  CZ_WIDTH = x;
  CZ_HEIGHT = y;

  quit=0;

  ZeroMemory(&g_rcWindow  ,sizeof(RECT));
  ZeroMemory(&g_rcViewport,sizeof(RECT));
  ZeroMemory(&g_rcScreen  ,sizeof(RECT));

  bpp = 32;
  mouseclicked = 0;
  mouserelease = 0;
  fullscreen = fs;
  doublesize = 0;
  ontop = 0;

  mousex = 0;
  mousey = 0;
  active = 1;
}
czGraphics::~czGraphics() {}


#define CZDEBUG_ASSERT_RETURN(x) if (!(x)) return 0;
  
LRESULT czGraphics::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg) {

  case WM_ACTIVATE:                           
    {
      active = (LOWORD(wParam)!=0) && (HIWORD(wParam)==0);
    } break;
    case WM_MOVE: {
      GetWindowRect(hWnd, &g_rcWindow);
      GetClientRect(hWnd, &g_rcViewport);
      GetClientRect(hWnd, &g_rcScreen);
      ClientToScreen(hWnd, (POINT*)&g_rcScreen.left);
      ClientToScreen(hWnd, (POINT*)&g_rcScreen.right);
    } break;

    case WM_SYSKEYDOWN:
      {
        if (wParam == VK_RETURN)
        {
          DestroyDirectDraw();
          fullscreen = !fullscreen;

          DWORD wStyle = WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
          if (!fullscreen) wStyle |= WS_OVERLAPPED | WS_CAPTION;
          SetWindowLong( hWnd, GWL_STYLE, wStyle );

          DWORD wExStyle = WS_EX_APPWINDOW | WS_EX_TOOLWINDOW;
          RECT wr={0,0,CZ_WIDTH,CZ_HEIGHT};
          AdjustWindowRectEx(&wr, wStyle, FALSE, wExStyle);

          if (!fullscreen)
          {
            int cx = (GetSystemMetrics(SM_CXSCREEN)-CZ_WIDTH)/2;
            int cy = (GetSystemMetrics(SM_CYSCREEN)-CZ_HEIGHT)/2;
            wr.left += cx;
            wr.top += cy;
            wr.right += cx;
            wr.bottom += cy;
          }

          SetWindowPos( hWnd, NULL, wr.left, wr.top, wr.right-wr.left, wr.bottom-wr.top, SWP_NOZORDER | SWP_SHOWWINDOW );


          InitDirectDraw();
        }
      } break;

    case WM_KEYDOWN:
      switch(wParam)
      {
        case VK_ESCAPE:
          {
          PostMessage(hWnd, WM_QUIT, 0, 0);
          quit=1;
          return 0L;
          }
      } break;

    case WM_LBUTTONDOWN: {
      mouseclicked = 1;
      int x = LOWORD(lParam); 
      int y = HIWORD(lParam);
      
      mousex = x;
      mousey = y;

      if (!(33 < x && x < 33+25 && 13 < y && y < 13+94))
        SendMessage(hWnd, WM_NCLBUTTONDOWN, HTCAPTION, NULL);
    } break;

    case WM_MOUSEMOVE: {
      int x = LOWORD(lParam); 
      int y = HIWORD(lParam);
      
      mousex = x;
      mousey = y;
    } break;

    case WM_NCLBUTTONUP: 
    case WM_LBUTTONUP: 
      { 
        int x = LOWORD(lParam); 
        int y = HIWORD(lParam);
        mouseclicked=0; 
        mouserelease=1;
        mousex = x;
        mousey = y;
        } break; 

    case WM_CLOSE:
    case WM_DESTROY: {
      quit = 1;
      PostMessage(hWnd, WM_QUIT, 0, 0);
    } break;

    case WM_SYSCOMMAND: {
      switch (wParam) {
        case SC_SCREENSAVE:         
        case SC_MONITORPOWER:       
        return 0;             
      }
      break;                  
    }

  }
  return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

czGraphics * pGlobalDisp = NULL;

LRESULT CALLBACK DlgFuncHook( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam){
  if (uMsg == WM_CREATE) {
    CREATESTRUCT * s = (CREATESTRUCT*)lParam; // todo: split to multiple hWnd-s! (if needed)
    pGlobalDisp = (czGraphics*)s->lpCreateParams;
  }

  if (!pGlobalDisp->WndProc(hWnd,uMsg,wParam,lParam))
    return 0;

  return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

int czGraphics::Init(HINSTANCE hInst) {
  int xres = CZ_WIDTH;
  int yres = CZ_HEIGHT;

  if (doublesize) {
    xres<<=1;
    yres<<=1;
  }

  DWORD wExStyle = WS_EX_APPWINDOW | WS_EX_TOOLWINDOW;
  DWORD wStyle = WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
  if (!fullscreen) wStyle |= WS_OVERLAPPED | WS_CAPTION;

  WNDCLASS WC;
  WC.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
  WC.lpfnWndProc = &DlgFuncHook;
  WC.cbClsExtra = 0;
  WC.cbWndExtra = 0;
  WC.hInstance = hInst;
  WC.hIcon = NULL;
  WC.hCursor = LoadCursor(NULL, IDC_ARROW);
  WC.hbrBackground = NULL;
  WC.lpszMenuName = NULL;
  WC.lpszClassName = _T("ud2dwindow");

  //LOG.Printf("[gfx] RegisterClass()...\n");
  CZDEBUG_ASSERT_RETURN(RegisterClass(&WC));

  RECT wr={0,0,xres,yres};
  if (!fullscreen) 
    AdjustWindowRectEx(&wr, wStyle, FALSE, wExStyle);

  //LOG.Printf("[gfx] CreateWindow()...\n");
  hWnd = CreateWindowEx(wExStyle,WC.lpszClassName,_T("czGraphics"),wStyle,
                        (GetSystemMetrics(SM_CXSCREEN)-xres)/2,
                        (GetSystemMetrics(SM_CYSCREEN)-yres)/2,
                        wr.right-wr.left, wr.bottom-wr.top,
                        NULL,NULL,hInst,this);
  CZDEBUG_ASSERT_RETURN(hWnd);

  if (ontop)
    SetWindowPos( hWnd, HWND_TOPMOST, NULL, NULL, NULL, NULL, SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE);

  ShowWindow(hWnd, SW_SHOW);
  SetForegroundWindow(hWnd);
  SetFocus(hWnd);

  GetClientRect(hWnd, &g_rcViewport);
  GetClientRect(hWnd, &g_rcScreen);
  ClientToScreen(hWnd, (POINT*)&g_rcScreen.left);
  ClientToScreen(hWnd, (POINT*)&g_rcScreen.right);

  //////////////////////////////////////////////////////////////////////////
  
  if (InitDirectDraw() == 0)
    return 0;


  return 1;
}

void czGraphics::Close() {
  //LOG.Printf("[gfx] Close()...\n");
  DestroyDirectDraw();

  DestroyWindow(hWnd);
}

void czGraphics::Messages() {
  MSG msg;
  if( PeekMessage( &msg, NULL, 0U, 0U, PM_REMOVE ) ) {
      TranslateMessage( &msg );
      DispatchMessage( &msg );
  }
}

void czGraphics::Update(void*vscr) {

  unsigned int * rot = (unsigned int *)vscr;
  DDSURFACEDESC2  ddsdBack;

  memset(&ddsdBack, 0, sizeof(DDSURFACEDESC));
  ddsdBack.dwSize = sizeof (ddsdBack);
  g_pDDSBack->Lock (0, &ddsdBack, DDLOCK_SURFACEMEMORYPTR  | DDLOCK_WAIT ,0);

  long wta = ddsdBack.lPitch-ddsdBack.dwWidth*((bpp+1) / 8);
  LPVOID surf = ddsdBack.lpSurface;
//  int* vscr = scr32;

  switch(bpp){         
    case 32 : __asm{
      push  es
      mov   ax,ds
      mov   es,ax
      mov   esi,[rot]
      mov   edi,[surf]
      mov   ecx,[ddsdBack.dwHeight]
    Oneline32:
      push  ecx
      mov   ecx,[ddsdBack.dwWidth]
      rep   movsd
      add   edi,[wta]
      pop   ecx  
      loop  Oneline32
      pop   es
    } break;
    case 24 : __asm{
      push  es
      mov   ax,ds
      mov   es,ax
      mov   esi,[rot]
      mov   edi,[surf]
      mov   ecx,[ddsdBack.dwHeight]
    Oneline24:
      push  ecx
      mov   ecx,[ddsdBack.dwWidth]
    Onepix24:
      movsw
      movsb
      inc   esi
      loop  Onepix24
      add   edi,[wta]
      pop   ecx  
      loop  Oneline24
      pop   es
    } break;
    case 16 : __asm{
      push  es
      mov   ax,ds
      mov   es,ax
      mov   esi,[rot]
      mov   edi,[surf]
      mov   ecx,[ddsdBack.dwHeight]
    Oneline16:
      push  ecx
      mov   ecx,[ddsdBack.dwWidth]
    Onepix16:
      lodsd
      shr   ax,2
      shl   al,2
      shr   ax,3
      mov   bx,ax
      shr   eax,8
      and   ax,1111100000000000b
      or    ax,bx
      stosw
      loop  Onepix16
      add   edi,[wta]
      pop   ecx
      loop  Oneline16
      pop   es 
          } break;
    case 15 : __asm{
      push  es
      mov   ax,ds
      mov   es,ax
      mov   esi,[rot]
      mov   edi,[surf]
      mov   ecx,[ddsdBack.dwHeight]
    Oneline15:
      push  ecx
      mov   ecx,[ddsdBack.dwWidth]
    Onepix15:
      lodsd
      shr   ax,3
      shl   al,3
      shr   ax,3
      mov   bx,ax
      shr   eax,9
      and   ax,0111110000000000b
      or    ax,bx
      stosw
      loop  Onepix15
      add   edi,[wta]
      pop   ecx
      loop  Oneline15
      pop   es
    } break;
  }

  g_pDDSBack->Unlock(NULL);

  if (fullscreen)
    g_pDDSPrimary->Flip(NULL,NULL);
  else
    g_pDDSPrimary->Blt(&g_rcScreen, g_pDDSBack,&g_rcViewport,DDBLT_WAIT,NULL);

  mouserelease = 0;
}

int czGraphics::InitDirectDraw( )
{
  int xres = CZ_WIDTH;
  int yres = CZ_HEIGHT;

  int h;
  //LOG.Printf("[gfx] DirectDrawCreateEx()...\n");
  h = DirectDrawCreateEx(NULL,(VOID**)&g_pDD, IID_IDirectDraw7, NULL);
  CZDEBUG_ASSERT_RETURN( h == DD_OK );

  if (fullscreen) {
    //LOG.Printf("[gfx] SetCooperativeLevel()...\n");
    h=g_pDD->SetCooperativeLevel(hWnd, DDSCL_EXCLUSIVE| DDSCL_FULLSCREEN);
    CZDEBUG_ASSERT_RETURN( h == DD_OK );

    //LOG.Printf("[gfx] SetDisplayMode(32)...\n");
    h = g_pDD->SetDisplayMode(xres, yres, bpp=32, 0, 0);
    if(h != DD_OK) {
      //LOG.Printf("[gfx] SetDisplayMode(24)...\n");
      h = g_pDD->SetDisplayMode(xres, yres, bpp=24, 0, 0);
      if(h != DD_OK) {
        //LOG.Printf("[gfx] SetDisplayMode(16)...\n");
        h = g_pDD->SetDisplayMode(xres, yres, bpp=16, 0, 0);
        if(h != DD_OK) {
          CZDEBUG_ASSERT_RETURN( h == DD_OK );
        } else {
          DDSURFACEDESC2 ddsd;
          ZeroMemory(&ddsd, sizeof(ddsd));
          ddsd.dwSize = sizeof(ddsd);
          //LOG.Printf("[gfx] GetDisplayMode(16)...\n");
          g_pDD->GetDisplayMode(&ddsd);
          if(bpp == 16){
            if(ddsd.ddpfPixelFormat.dwRBitMask == 0x7c00){
              bpp = 15;
            }else if(ddsd.ddpfPixelFormat.dwRBitMask != 0xf800){
              CZDEBUG_ASSERT_RETURN( h == DD_OK );
              exit(0);
            }
          }
        }
      }
    }
    //LOG.Printf("[gfx] bpp == %d...\n",bpp);

    DDSURFACEDESC2 ddsd;
    ZeroMemory(&ddsd,sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_FLIP | DDSCAPS_COMPLEX;
    ddsd.dwBackBufferCount = 1;
    //LOG.Printf("[gfx] CreateSurface()...\n");
    h = g_pDD->CreateSurface( &ddsd, &g_pDDSPrimary, NULL);
    CZDEBUG_ASSERT_RETURN( h == DD_OK );

    DDSCAPS2 ddscaps;
    ZeroMemory(&ddscaps, sizeof(ddscaps));
    ddscaps.dwCaps = DDSCAPS_BACKBUFFER;
    //LOG.Printf("[gfx] GetAttachedSurface()...\n");
    h = g_pDDSPrimary->GetAttachedSurface(&ddscaps, &g_pDDSBack);
    CZDEBUG_ASSERT_RETURN( h == DD_OK );

    ShowCursor(FALSE);
  }
  else 
  {
    //LOG.Printf("[gfx] SetCooperativeLevel()...\n");
    h=g_pDD->SetCooperativeLevel(hWnd, DDSCL_NORMAL);
    CZDEBUG_ASSERT_RETURN( h == DD_OK );

    DDSURFACEDESC2 ddsd;
    ZeroMemory(&ddsd,sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
    //LOG.Printf("[gfx] CreateSurface(g_pDDSPrimary)...\n");
    h = g_pDD->CreateSurface(&ddsd, &g_pDDSPrimary, NULL);
    CZDEBUG_ASSERT_RETURN( h == DD_OK );

    LPDIRECTDRAWCLIPPER pClipper;
    //LOG.Printf("[gfx] CreateClipper()...\n");
    h = g_pDD->CreateClipper(0, &pClipper, NULL);
    CZDEBUG_ASSERT_RETURN( h == DD_OK );

    pClipper->SetHWnd(0, hWnd);
    //LOG.Printf("[gfx] SetClipper()...\n");
    h = g_pDDSPrimary->SetClipper(pClipper);
    pClipper->Release();
    CZDEBUG_ASSERT_RETURN( h == DD_OK );

    ddsd.dwFlags        = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS;
    ddsd.dwWidth        = xres;
    ddsd.dwHeight       = yres;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    //LOG.Printf("[gfx] CreateSurface(g_pDDSBack)...\n");
    h = g_pDD->CreateSurface(&ddsd, &g_pDDSBack, NULL);
    CZDEBUG_ASSERT_RETURN( h == DD_OK );

    bpp = GetDeviceCaps(GetDC(hWnd),BITSPIXEL);
  }
  return 1;
}

void czGraphics::DestroyDirectDraw()
{
  if (g_pDD != NULL){
    g_pDD->SetCooperativeLevel(hWnd, DDSCL_NORMAL);
    if (g_pDDSBack != NULL) {
      g_pDDSBack->Release();
      g_pDDSBack = NULL;
    }
    if (g_pDDSPrimary != NULL) {
      g_pDDSPrimary->Release();
      g_pDDSPrimary = NULL;
    }
    g_pDD->Release();
  }
}