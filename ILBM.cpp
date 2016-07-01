#include <windows.h>
#include "ILBM.h"
#include <stdio.h>

// Based on the documentation of Jerry Morrison (Electronic Arts)
// http://home.comcast.net/~erniew/lwsdk/docs/filefmts/ilbm.html

CILBM::CILBM(void)
{
  pDataCompressed = NULL;
  Reset();
}

CILBM::~CILBM(void)
{
  if (pDataCompressed)
  {
    delete[] pDataCompressed;
    pDataCompressed = NULL;
  }
}

void CILBM::Reset()
{
  ZeroMemory(&sLBMHeader,sizeof(BitMapHeader));
  ZeroMemory(&pColorPalette,sizeof(ColorRegister) * 256);
  nCycleRanges = 0;
  nDataCompressedSize = 0;
  if (pDataCompressed)
  {
    delete[] pDataCompressed;
    pDataCompressed = NULL;
  }
}

bool CILBM::Load( TCHAR * szFilename )
{
  Reset();

  HANDLE h = CreateFile( szFilename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL );
  if (h == INVALID_HANDLE_VALUE) 
    return false;

  ReadChunk( h );

  return true;
}

#define RESHIFTBIT(width,from,to) (( ((width)>>(from)) & 0xFF) << (to))

#define FLIPMWORD(width) ( RESHIFTBIT(width,8,0) | RESHIFTBIT(width,0,8) )
#define FLIPMDWORD(width) ( RESHIFTBIT(width,24,0) | RESHIFTBIT(width,16,8) | RESHIFTBIT(width,8,16) | RESHIFTBIT(width,0,24) )

bool CILBM::ReadChunk( HANDLE h )
{
  unsigned int dwSignature = 0;
  unsigned int dwSize = 0;
  unsigned long dwRead = 0;
  if (ReadFile( h, &dwSignature, sizeof(unsigned int), &dwRead, NULL ) == FALSE || dwRead == 0)
    return false;
  if (ReadFile( h, &dwSize, sizeof(unsigned int), &dwRead, NULL ) == FALSE || dwRead == 0)
    return false;

  dwSize = FLIPMDWORD(dwSize);
  switch(FLIPMDWORD(dwSignature))
  {
    case 'FORM':
      {
        unsigned int dwLBMSignature = 0;
        if (ReadFile( h, &dwLBMSignature, sizeof(unsigned int), &dwRead, NULL ) == FALSE || dwRead == 0)
          return false;
        while (true)
        {
          if (!ReadChunk(h))
            break;
        }
      } break;
    case 'BMHD':
      {
        if (ReadFile( h, &sLBMHeader, dwSize, &dwRead, NULL ) == FALSE || dwRead == 0)
          return false;
        sLBMHeader.w = FLIPMWORD(sLBMHeader.w);
        sLBMHeader.h = FLIPMWORD(sLBMHeader.h);
        sLBMHeader.pageWidth  = FLIPMWORD(sLBMHeader.pageWidth);
        sLBMHeader.pageHeight = FLIPMWORD(sLBMHeader.pageHeight);
      } break;
    case 'CMAP':
      {
        if (ReadFile( h, &pColorPalette[0], dwSize, &dwRead, NULL ) == FALSE || dwRead == 0)
          return false;
      } break;
    case 'CRNG':
      {
        if (ReadFile( h, &pCycleRanges[nCycleRanges], dwSize, &dwRead, NULL ) == FALSE || dwRead == 0)
          return false;
        pCycleRanges[nCycleRanges].rate  = FLIPMWORD(pCycleRanges[nCycleRanges].rate);
        pCycleRanges[nCycleRanges].flags = FLIPMWORD(pCycleRanges[nCycleRanges].flags);
        nCycleRanges++;
      } break;
    case 'BODY':
      {
        nDataCompressedSize = dwSize;
        pDataCompressed = new unsigned char[ dwSize ];
        if (ReadFile( h, pDataCompressed, dwSize, &dwRead, NULL ) == FALSE || dwRead == 0)
          return false;
      } break;
    default:
      {
        SetFilePointer( h, dwSize, NULL, FILE_CURRENT );
      } break;
  }
  return true;
}

unsigned short CILBM::GetWidth()
{
  return sLBMHeader.w;
}

unsigned short CILBM::GetHeight()
{
  return sLBMHeader.h;
}

void CILBM::Decompress( unsigned char * pTarget )
{
  if (sLBMHeader.compression == cmpNone)
  {
    CopyMemory( pTarget, pDataCompressed, nDataCompressedSize );
    return;
  }

  unsigned char * out = pTarget;
  for (unsigned int i=0; i < nDataCompressedSize; i++)
  {
    signed char c = (signed char)pDataCompressed[i++];
    if (0 <= c && c <= 127)
    {
      for (int j = 0; j< c + 1; j++)
      {
        *(out++) = pDataCompressed[i++];
      }
      i--;
    }
    else if (-127 <= c && c <= -1)
    {
      for (int j = 0; j<-c+1; j++)
      {
        *(out++) = pDataCompressed[i];
      }
    }
    else if (-128 == c)
    {
      // do nothing.
    }
  }
}


CILBMViewer::CILBMViewer()
{
  pChunkyData = NULL;
  nFPS = 60;
  bBlendShift = true;
}

CILBMViewer::~CILBMViewer()
{
  if (pChunkyData)
  {
    delete[] pChunkyData;
    pChunkyData = NULL;
  }
}

void CILBMViewer::RenderTo32bit( unsigned int * p )
{
  ZeroMemory( p, sizeof(unsigned int) * sLBMHeader.h * sLBMHeader.w );
  for (int y = 0; y < sLBMHeader.h; y++)
  {
    for (int x = 0; x < sLBMHeader.w; x++)
    {
      *p |= pCurrentColorPalette[ pChunkyData[ x + y * sLBMHeader.w ] ].blue;
      *p |= pCurrentColorPalette[ pChunkyData[ x + y * sLBMHeader.w ] ].green << 8;
      *p |= pCurrentColorPalette[ pChunkyData[ x + y * sLBMHeader.w ] ].red << 16;
      p++;
    }
  }
}

void CILBMViewer::UpdatePalette( unsigned int frame )
{
  int nStepsPerSecond = 16384;

  CopyMemory( pCurrentColorPalette, pColorPalette, sizeof(ColorRegister) * 256 );
  for (int i = 0; i < nCycleRanges; i++)
  {
    if (!(pCycleRanges[i].flags & RNG_ACTIVE)) continue;

    int nSpeed = (pCycleRanges[i].rate * nFPS) / (nStepsPerSecond / 256);
    int step256 = (frame * nSpeed) / 1000;
    int shift = step256 & 0xFF;
    int step = step256 / 256;

    int range = (pCycleRanges[i].high - pCycleRanges[i].low + 1);

    for (int j = pCycleRanges[i].low; j <= pCycleRanges[i].high; j++)
    {
      unsigned int index = 0;
      if (pCycleRanges[i].flags & RNG_REVERSE) 
        index = pCycleRanges[i].low + (step + j - pCycleRanges[i].low) % range;
      else
        index = pCycleRanges[i].low + (step + range - (j - pCycleRanges[i].low)) % range;

      if (bBlendShift)
      {
        // Joseph Huckaby's blend-shift idea (I'm not sure how he did it but probably similar to this)

        unsigned int index2 = 0;
        if (pCycleRanges[i].flags & RNG_REVERSE) 
          index2 = pCycleRanges[i].low + (step + 1 + j - pCycleRanges[i].low) % range;
        else
          index2 = pCycleRanges[i].low + (step + 1 + range - (j - pCycleRanges[i].low)) % range;

        pCurrentColorPalette[ j ].red   = pColorPalette[ index ].red   + (pColorPalette[ index2 ].red   - pColorPalette[ index ].red  ) * shift / 256;
        pCurrentColorPalette[ j ].green = pColorPalette[ index ].green + (pColorPalette[ index2 ].green - pColorPalette[ index ].green) * shift / 256;
        pCurrentColorPalette[ j ].blue  = pColorPalette[ index ].blue  + (pColorPalette[ index2 ].blue  - pColorPalette[ index ].blue ) * shift / 256;
      } 
      else 
      {
        pCurrentColorPalette[ j ] = pColorPalette[ index ];
      }
    }
  }
}

void CILBMViewer::PlanarToChunky( unsigned char * pPlanar )
{
  ZeroMemory( pChunkyData, sLBMHeader.w * sLBMHeader.h );
  unsigned char * pIn = pPlanar;
  for (int y = 0; y < sLBMHeader.h; y++)
  {
    for (int b = 0; b < sLBMHeader.nPlanes; b++)
    {
      for (int x = 0; x < sLBMHeader.w; x++)
      {
        if (*pIn & ( 1 << ( 7 - ( x & 7 ) ) ) )
          pChunkyData[ x + y * sLBMHeader.w ] |= (1 << b);
        if (( x & 7 ) == 7)
          pIn++;
      }
    }
  }
}

bool CILBMViewer::Load( TCHAR * szFilename )
{
  if (!CILBM::Load(szFilename))
    return false;

  unsigned char * pTempData = new unsigned char[ sLBMHeader.w * sLBMHeader.h ];
  Decompress( pTempData );

  pChunkyData = new unsigned char[ sLBMHeader.w * sLBMHeader.h ];
  PlanarToChunky( pTempData );

  delete[] pTempData;

  UpdatePalette( 0 );

  return true;
}
