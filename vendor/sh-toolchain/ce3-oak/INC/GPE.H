/*++

Copyright (c) 1985-2000 Microsoft Corporation.  All rights reserved.

Module Name:

    gpe.h

Abstract:

    Base class definitions for Graphics Primitive Engine (GPE) -based display drivers.

--*/

#ifndef __GPE_H__
#define __GPE_H__

#ifdef __cplusplus
extern "C" {
#endif


class GPESurf;			// Object represents a surface (bitmap)
class GPE;				// Object represents a display device
class ColorConverter;	// Each converter represents an XLATEOBJ
class Node2D;			// Optional helper class

struct GPEBltParms;
struct GPELineParms;
struct GPEFillParms;
struct GPEMode;

typedef unsigned long COLOR;
typedef unsigned long ROP4;
typedef unsigned long ADDRESS;

enum EGPEFormat
{
	gpe1Bpp,
	gpe2Bpp,
	gpe4Bpp,
	gpe8Bpp,
	gpe16Bpp,
	gpe24Bpp,
	gpe32Bpp,
	gpe16YCrCb,
	gpeDeviceCompatible,
	gpeUndefined
};

#define  INSTANTIATE_GPE	\
	const int EGPEFormatToBpp[] =     { 1, 2, 4, 8, 16, 24, 32, 16, 0 };	\
	const int EGPEFormatToIFormat[] = { 1, 0, 2, 3, 4,  5,  6,  0,  0  };	/* 0 is non-std formats */	\
	const EGPEFormat IFormatToEGPEFormat[] = { gpeDeviceCompatible, gpe1Bpp, gpe4Bpp, gpe8Bpp, gpe16Bpp, gpe24Bpp, gpe32Bpp, gpeUndefined, gpeUndefined, gpe2Bpp };

extern const int EGPEFormatToBpp[];
extern const int EGPEFormatToIFormat[];
extern const EGPEFormat IFormatToEGPEFormat[];

enum EGPEPhase
{
	gpeSingle,
	gpePrepare,
	gpeContinue,
	gpeComplete
};


// CLASS GPESurf
//
// This is the class used for surfaces created in system memory
// Derived classes are used for surfaces stored in video memory

class GPESurf
{
protected:
	ADDRESS			m_pVirtAddr;
	unsigned long	m_nStrideBytes;
	EGPEFormat		m_eFormat;
	int				m_fInVideoMemory;
	int				m_fOwnsBuffer;
	int				m_nWidth;
	int				m_nHeight;
	unsigned long	m_nOffsetInVideoMemory;
public:
	unsigned long	m_nHandle;
					GPESurf() 
					{	
					    m_fOwnsBuffer = 0; 	// Null constructor; call Init() to initialize
					}   
					GPESurf
					(						// Create system-memory surface
						int width,			// ... and allocate storage
						int height,
						EGPEFormat format
					);
	void			Init
					(						// Create GPESurf representing existing
						int width,			// ... system-memory surface (i.e. DIB)
						int height,
						void *pBits,
						int stride,
						EGPEFormat format
					);
					GPESurf
					(						// Create GPESurf representing existing
						int width,			// ... system-memory surface (i.e. DIB)
						int height,
						void *pBits,
						int stride,
						EGPEFormat format
					)
					{
						Init( width, height, pBits, stride, format );
					}
	virtual			~GPESurf();
	int				Stride() { return m_nStrideBytes; }
	EGPEFormat		Format() { return m_eFormat; }
	void *			Buffer() { return (void *)m_pVirtAddr; }
	int				Width() { return m_nWidth; }
	int				Height() { return m_nHeight; }
	int				InVideoMemory() { return m_fInVideoMemory; }
	unsigned long	OffsetInVideoMemory() { return m_nOffsetInVideoMemory; }
};


struct GPEMode
{
	int				modeId;
	int				width;
	int				height;
	int				Bpp;
	int				frequency;
	EGPEFormat		format;
};


// CLASS GPE
//
// This class serves as the base class for device specific drivers
// Emulation functions for performing graphics operations by software
// are also placed here

class GPE
{
protected:
	GPESurf			*m_pPrimarySurface;
	int				m_nScreenWidth;
	int				m_nScreenHeight;
	GPEMode			*m_pMode;
	unsigned long	m_hSurf;	// hsurf of display surface	

public:
					GPE();
	virtual			~GPE();
	virtual SCODE	BltPrepare( GPEBltParms *pBltParms )=0;
	virtual SCODE	BltComplete( GPEBltParms *pBltParms )=0;
	virtual SCODE	Line(
						GPELineParms *pLineParms,
						EGPEPhase phase = gpeSingle )=0;
	virtual SCODE	AllocSurface(
						GPESurf **ppSurf,
						int width,
						int height,
						EGPEFormat format,
						int surfaceFlags )=0;
	virtual SCODE	SetPointerShape(
						GPESurf *pMask,
						GPESurf *pColorSurf,
						int xHot,
						int yHot,
						int cx,
						int cy )=0;
	virtual SCODE	MovePointer(
						int x,
						int y )=0;
	virtual SCODE	SetPalette(
						const PALETTEENTRY *src,
						unsigned short firstEntry,
						unsigned short numEntries )=0;
	virtual SCODE	GetModeInfo(
						GPEMode *pMode,
						int modeNo )=0;
	virtual int		NumModes()=0;
	virtual SCODE	SetMode
					(
						int modeId,
						HPALETTE *pPaletteHandle
					)=0;
	unsigned long	GetHSurf() { return m_hSurf; }
	void			SetHSurf( unsigned long hSurf ) { m_hSurf = hSurf; }
	virtual int		InVBlank()=0;
	int				GetModeId() { return m_pMode->modeId; }	// -1 means not initialized yet
	int				ScreenHeight() { return m_nScreenHeight; }
	int				ScreenWidth() { return m_nScreenWidth; }
	GPESurf			*PrimarySurface() { return m_pPrimarySurface; }
	SCODE			EmulatedBlt( GPEBltParms *pParms );
	SCODE			EmulatedLine( GPELineParms *pParms );
	virtual int		IsPaletteSettable();
    // These functions should only be implemented for devices as appropriate
    virtual BOOL    ContrastControl(
                        ULONG cmd,
                        ULONG *pValue);
    virtual VOID    PowerHandler(
                        BOOL bOff);
    virtual ULONG   DrvEscape(
                        SURFOBJ *pso,
                        ULONG    iEsc,
                        ULONG    cjIn,
                        PVOID    pvIn,
                        ULONG    cjOut,
                        PVOID    pvOut);
	virtual ULONG	GetGraphicsCaps();
	// The following are only required for DDHALs
	virtual void	GetPhysicalVideoMemory(
						unsigned long *pPhysicalMemoryBase,
						unsigned long *pVideoMemorySize );
	virtual void	SetVisibleSurface( GPESurf *pSurf );
	virtual int		FlipInProgress();
	virtual void	WaitForVBlank();
	virtual int		SurfaceBusyFlipping( GPESurf *pSurf );
	virtual int		IsBusy();
	virtual void	WaitForNotBusy();
	virtual unsigned long
					AvailableVideoMemory();
	virtual int		ScanLine();
	virtual SCODE	ProcessCommandBlock( unsigned char *pBlock );	// Only D3D Drivers!
};
// Flags to surface allocator
#define GPE_REQUIRE_VIDEO_MEMORY 	0x00000001
#define GPE_PREFER_VIDEO_MEMORY		0x00000002

GPE *GetGPE();	// The driver implements this to allow instantiation of device

////////////////////////////////////////////////////////////////////////////////
// Color Conversion
//

// The ColorConverter class is used to convert pixels from one color format
// to another.
// InitConverter is called to set up the converter for a given XLATEOBJ, and
// writes the address of a translation table to be used or a translation
// function to be called for conversion.  If both are NULL, no conversion is
// required.


class ColorConverter
{

public:

static void				InitConverter(
							XLATEOBJ *pxo,
							ColorConverter **ppConvertObj,
							unsigned long (ColorConverter::**ppConvertFn)(unsigned long),
							unsigned long **ppLookup );
};


struct GPEBltParms
{
	SCODE 			(GPE::*pBlt)(GPEBltParms *);	// Pointer to specific blt function that will perform blt
	GPESurf			*pDst;
	GPESurf			*pSrc;
	GPESurf			*pMask;
	GPESurf			*pBrush;
	RECTL			*prclDst;
	RECTL			*prclSrc;
	RECTL			*prclClip;
	COLOR			solidColor;
	unsigned long	bltFlags;
	ROP4			rop4;
	RECTL			*prclMask;
	POINTL			*pptlBrush;
	int				xPositive;
	int				yPositive;
	unsigned long	*pLookup;		// For src=palette conversions
//	unsigned long	(*pConvert)(ColorConverter*,unsigned long);
	unsigned long	(ColorConverter::*pConvert)(unsigned long);
	ColorConverter	*pColorConverter;
};

struct GPELineParms
{
	SCODE 			(GPE::*pLine)(GPELineParms *);
	long			xStart;
	long			yStart;
	int				cPels;
	unsigned long	dM;
	unsigned long	dN;
	long			llGamma;
	int				iDir;
	unsigned long	style;
	int				styleState;
	GPESurf 		*pDst;
	COLOR			solidColor;
	RECTL			*prclClip;
//	int				state;
	unsigned short	mix;
};

struct GPEFillParms
{
	SCODE 			(GPE::*pFill)(GPEFillParms *);
	GPESurf 		*pDst;
	POINTL 			*aptlLeft;
	POINTL 			*aptlRight;
	unsigned long 	bottom;
	RECTL 			*prclClip;
	COLOR 			solidColor;
};



class Node2D
{
private:
	int					m_nX;
	int					m_nY;
	int					m_nWidth;
	int					m_nHeight;
	int					m_nAlign;
	Node2D				*m_pParent;
	Node2D				*m_pChild1;
	Node2D				*m_pChild2;

	void				GetClosest
						(
							int width,
							int height,
							int *pLeastWaste,
							Node2D **ppBestFit
						);
	Node2D				*Split
						(
							int width,
							int height
						);

public:
						Node2D
						(
							int width,
							int height,
							int x=0,
							int y=0,
							int align=4,	// = multiple to align to
							Node2D *pParent=(Node2D *)NULL
						);
	void				Free();
	Node2D				*Alloc
						(
							int width,
							int height
						);
	int					Top() { return m_nY; }
	int					Left() { return m_nX; }
	unsigned long		AvailSpace();
};

#ifdef DEBUG

#define GPE_ZONE_ERROR              DEBUGZONE(0)
#define GPE_ZONE_WARNING    		DEBUGZONE(1)
#define GPE_ZONE_PERF               DEBUGZONE(2)
#define GPE_ZONE_TEMP               DEBUGZONE(3)

#define GPE_ZONE_ENTER              DEBUGZONE(4)
#define GPE_ZONE_INIT               DEBUGZONE(5)
#define GPE_ZONE_BLT_HI             DEBUGZONE(6)
#define GPE_ZONE_BLT_LO             DEBUGZONE(7)

#define GPE_ZONE_CREATE             DEBUGZONE(8)
#define GPE_ZONE_FLIP               DEBUGZONE(9)
#define GPE_ZONE_LINE               DEBUGZONE(10)
#define GPE_ZONE_HW                 DEBUGZONE(11)

#define GPE_ZONE_POLY               DEBUGZONE(12)
#define GPE_ZONE_CURSOR             DEBUGZONE(13)
// #define GPE_ZONE_               DEBUGZONE(14)
// #define GPE_ZONE_               DEBUGZONE(15)

// NOTE:  One file should use INSTANTIATE_GPE_ZONES.  This allows it to be pre-compiled
// initialZones should typically be 0x0003
// modname is a string such as "DDI driver"
// opt1 and 2 are user defined (device-specific) zones e.g. "Reg0 Write"

#define INSTANTIATE_GPE_ZONES(initialZones,modname,opt1,opt2) 		\
DBGPARAM dpCurSettings =                                \
{                                                       \
	TEXT(modname),                                      \
	{                                                   \
		TEXT("Errors"),                 /* 0  */        \
		TEXT("Warnings"),               /* 1  */        \
		TEXT("Performance"),    		/* 2  */        \
		TEXT("Temporary tests"), 		/* 3  */        \
		TEXT("Enter,Exit"),             /* 4  */        \
		TEXT("Initialize"),             /* 5  */        \
		TEXT("Blt Calls"),              /* 6  */        \
		TEXT("Blt Verbose"),    		/* 7  */        \
		TEXT("Surface Create"), 		/* 8  */        \
		TEXT("Flip"),                   /* 9  */        \
		TEXT("Line"),                   /* 10 */        \
		TEXT("Hardware"),               /* 11 */        \
		TEXT("Polygon"),                /* 12 */        \
		TEXT("Cursor"),                 /* 13 */        \
		TEXT(opt1),                     /* 14 */        \
		TEXT(opt2),                     /* 15 */        \
	},                                                  \
	(initialZones)										\
};

extern DBGPARAM dpCurSettings;
#define DEBUGWAIT(cond) { if(cond) Sleep(50); }
#else	// NOT DEBUG
#define INSTANTIATE_GPE_ZONES(initialZones,MODNAME,opt1,opt2)
#define DEBUGWAIT(cond)
#endif	// DEBUG


#ifdef __cplusplus
}
#endif

#endif // __GPE_H__
