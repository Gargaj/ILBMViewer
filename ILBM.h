#pragma once
#include <windows.h>

//////////////////////////////////////////////////////////////////////////
// most of this is from
// http://home.comcast.net/~erniew/lwsdk/docs/filefmts/ilbm.html
// only slightly updated, kinda.
// (really should convert most of them to enums but oh well)

#pragma pack(1)

typedef unsigned char Masking;  /* Choice of masking technique. */

#define mskNone   0
#define mskHasMask   1
#define mskHasTransparentColor   2
#define mskLasso  3

typedef unsigned char Compression;    /* Choice of compression algorithm
  applied to the rows of all source and mask planes.  "cmpByteRun1"
  is the byte run encoding described in Appendix C.  Do not compress
  across rows! */
#define cmpNone   0
#define cmpByteRun1  1

typedef struct {
  unsigned short w, h;             /* raster width & height in pixels      */
  signed short x, y;             /* pixel position for this image        */
  unsigned char nPlanes;          /* # source bitplanes                   */
  Masking masking;
  Compression compression;
  unsigned char pad1;             /* unused; ignore on read, write as 0   */
  unsigned short transparentColor; /* transparent "color number" (sort of) */
  unsigned char xAspect, yAspect; /* pixel aspect, a ratio width : height */
  signed short pageWidth, pageHeight;  /* source "page" size in pixels   */
} BitMapHeader;

typedef struct {
  unsigned char red, green, blue;       /* color intensities 0..255 */
} ColorRegister;                 /* size = 3 bytes */

#define RNG_ACTIVE 1
#define RNG_REVERSE 2

typedef struct {
  unsigned short pad1;       /* reserved for future use; store 0 here    */
  unsigned short rate;       /* color cycle rate                         */
  unsigned short flags;      /* see below                                */
  unsigned char low, high;  /* lower and upper color registers selected */
} CycleRange;

#pragma pack()

class CILBM
{
protected:
  BitMapHeader sLBMHeader;
  ColorRegister pColorPalette[256];
  int nCycleRanges;
  CycleRange pCycleRanges[256];
  unsigned int nDataCompressedSize;
  unsigned char * pDataCompressed;

  bool ReadChunk( HANDLE h );

public:
  CILBM(void);
  ~CILBM(void);

  unsigned short GetWidth();
  unsigned short GetHeight();

  void Reset();
  bool Load( TCHAR * szFilename );
  void Decompress( unsigned char * pTarget );
};

class CILBMViewer : public CILBM
{
protected:
  ColorRegister pCurrentColorPalette[256];
  unsigned char * pChunkyData;

  void PlanarToChunky( unsigned char * pTempData );

public:
  CILBMViewer();
  ~CILBMViewer();

  bool bBlendShift;
  int nFPS;

  bool Load( TCHAR * szFilename );

  void UpdatePalette( unsigned int frame );
  void RenderTo32bit( unsigned int * p );
};