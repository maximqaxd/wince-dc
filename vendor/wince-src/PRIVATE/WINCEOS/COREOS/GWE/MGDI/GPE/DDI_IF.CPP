/*

Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.

*/
#include "precomp.h"
#include "swemul.h"

DEFINE_GDI_ENTRY_POINTS(EMPTYPARM,LEFTENTRY,RIGHTENTRY)

void RegisterDDHALAPI();
extern int AllocConverters();
extern void FreeConverters();

EXTERN_C
int
__stdcall
DllMain(
    void *  hmod,
    DWORD dwReason,
    void *  lpvReserved
    )
{
    switch (dwReason) {
    case DLL_PROCESS_ATTACH:
#ifdef DEBUG
		RegisterDbgZones( (HINSTANCE)hmod, &dpCurSettings );
#endif
		RegisterDDHALAPI();
        break;
    case DLL_PROCESS_DETACH:
        break;
    case DLL_THREAD_ATTACH:
        break;
    case DLL_THREAD_DETACH:
        break;
   }

    return TRUE;
}

/******************************Public*Data*********************************\
* MIX translation table
*
* Translates a mix 1-16, into an old style Rop 0-255.
*
\**************************************************************************/

const BYTE gaMix[] =
{
    0xFF,  // R2_WHITE        - Allow rop = gaMix[mix & 0x0F]
    0x00,  // R2_BLACK
    0x05,  // R2_NOTMERGEPEN
    0x0A,  // R2_MASKNOTPEN
    0x0F,  // R2_NOTCOPYPEN
    0x50,  // R2_MASKPENNOT
    0x55,  // R2_NOT
    0x5A,  // R2_XORPEN
    0x5F,  // R2_NOTMASKPEN
    0xA0,  // R2_MASKPEN
    0xA5,  // R2_NOTXORPEN
    0xAA,  // R2_NOP
    0xAF,  // R2_MERGENOTPEN
    0xF0,  // R2_COPYPEN
    0xF5,  // R2_MERGEPENNOT
    0xFA,  // R2_MERGEPEN
    0xFF   // R2_WHITE        - Allow rop = gaMix[mix & 0xFF]
};


#define CLIP_LIMIT 50
typedef struct _CLIPENUM {
    LONG    c;
    RECTL   arcl[CLIP_LIMIT];   // Space for enumerating complex clipping

} CLIPENUM;                         /* ce, pce */


inline GPE *SurfobjToGPE( SURFOBJ *pso )
{
	return (GPE *)(pso->dhpdev);
}

class TmpGPESurf
{
	GPESurf *m_pGPESurf;
	GPESurf m_GPESurf;
public:
	operator GPESurf *() { return m_pGPESurf; }
	TmpGPESurf
	(
		SURFOBJ *pso1,
		SURFOBJ *pso2 = (SURFOBJ *)NULL,
		TmpGPESurf *pTmp2 = (TmpGPESurf *)NULL
	);
	~TmpGPESurf() {}
};

TmpGPESurf::TmpGPESurf
(
	SURFOBJ *pso1,
	SURFOBJ *pso2,
	TmpGPESurf *pTmp2
)
{
	if( !pso1 )
	{
		m_pGPESurf = (GPESurf *)NULL;
	}
	else if( pso1 == pso2 )
	{
		m_pGPESurf = pTmp2->m_pGPESurf;
	}
	else if( pso1->dhsurf )
	{
		m_pGPESurf = (GPESurf *)(pso1->dhsurf);
	}
	else
	{
		m_pGPESurf = &m_GPESurf;
		m_pGPESurf->Init( pso1->sizlBitmap.cx, pso1->sizlBitmap.cy,
    					    pso1->pvScan0, pso1->lDelta,
							IFormatToEGPEFormat[pso1->iBitmapFormat] );
	}
}

const DRVENABLEDATA pDrvFn =
{
    {   DrvEnablePDEV           },
    {   DrvDisablePDEV          },
    {   DrvEnableSurface        },
    {   DrvDisableSurface       },
    {   DrvCreateDeviceBitmap   },
    {   DrvDeleteDeviceBitmap   },
    {   DrvRealizeBrush         },
    {   DrvStrokePath           },
    {   DrvFillPath             },
    {   DrvPaint                },
    {   DrvBitBlt               },
    {   DrvCopyBits             },
    {   DrvAnyBlt               },
    {   DrvTransparentBlt       },
    {   DrvSetPalette           },
    {   DrvSetPointerShape      },
    {   DrvMovePointer          },
    {   DrvGetModes             },
    {   DrvRealizeColor         },
    {   DrvGetMasks             },
    {   DrvUnrealizeColor       },
    {   DrvContrastControl      },
    {   DrvPowerHandler         },
    {   NULL /* DrvEndDoc    */ },
    {   NULL /* DrvStartDoc  */ },
    {   NULL /* DrvStartPage */ },
    {   DrvEscape               }
};

#ifdef FIXED_EXE_EXPORTS

BOOL APIENTRY GPEEnableDriver(
	ULONG          iEngineVersion,
	ULONG          cj,
	DRVENABLEDATA *pded)
{
//	RegisterDbgZones( hmod, &dpCurSettings );
	DEBUGMSG(GPE_ZONE_ENTER,(TEXT("Entering DrvEnableDriver\r\n")));
	if ( iEngineVersion != DDI_DRIVER_VERSION || cj != sizeof(DRVENABLEDATA) )
	{
		DEBUGMSG(GPE_ZONE_ENTER,(TEXT("Leaving DrvEnableDriver\r\n")));
		return FALSE;
	}
	*pded = pDrvFn;
	
	DEBUGMSG(GPE_ZONE_ENTER,(TEXT("Leaving DrvEnableDriver\r\n")));
	return TRUE;
}

#else


BOOL APIENTRY GPEEnableDriver(
	ULONG          iEngineVersion,
	ULONG          cj,
	DRVENABLEDATA *pded,
	PENGCALLBACKS  pEngCallbacks)
{
//	RegisterDbgZones( hmod, &dpCurSettings );

	DEBUGMSG(GPE_ZONE_ENTER,(TEXT("Entering DrvEnableDriver\r\n")));

	BRUSHOBJ_pvAllocRbrush = pEngCallbacks->BRUSHOBJ_pvAllocRbrush;
	BRUSHOBJ_pvGetRbrush = pEngCallbacks->BRUSHOBJ_pvGetRbrush;
	CLIPOBJ_cEnumStart = pEngCallbacks->CLIPOBJ_cEnumStart;
	CLIPOBJ_bEnum = pEngCallbacks->CLIPOBJ_bEnum;
	PALOBJ_cGetColors = pEngCallbacks->PALOBJ_cGetColors;
	PATHOBJ_vEnumStart = pEngCallbacks->PATHOBJ_vEnumStart;
	PATHOBJ_bEnum = pEngCallbacks->PATHOBJ_bEnum;
	PATHOBJ_vGetBounds = pEngCallbacks->PATHOBJ_vGetBounds;
	XLATEOBJ_cGetPalette = pEngCallbacks->XLATEOBJ_cGetPalette;
	EngCreateDeviceSurface = pEngCallbacks->EngCreateDeviceSurface;
	EngDeleteSurface = pEngCallbacks->EngDeleteSurface;
	EngCreateDeviceBitmap = pEngCallbacks->EngCreateDeviceBitmap;
	EngCreatePalette = pEngCallbacks->EngCreatePalette;

	if ( iEngineVersion != DDI_DRIVER_VERSION || cj != sizeof(DRVENABLEDATA) )
		return FALSE;

	*pded = pDrvFn;
	
	DEBUGMSG(GPE_ZONE_ENTER,(TEXT("Leaving DrvEnableDriver\r\n")));

	return TRUE;
}


#endif

BOOL APIENTRY DrvContrastControl(
	DHPDEV     dhpdev,
    ULONG      cmd,
    ULONG     *pValue)
{
	GPE *pGPE = (GPE *)dhpdev;
    return pGPE->ContrastControl(cmd, pValue);
}

VOID APIENTRY DrvPowerHandler(
	DHPDEV     dhpdev,
    BOOL       bOff)
{
	GPE *pGPE = (GPE *)dhpdev;
    pGPE->PowerHandler(bOff);
}

ULONG APIENTRY DrvEscape(
	DHPDEV   dhpdev,
    SURFOBJ *pso,
    ULONG    iEsc,
    ULONG    cjIn,
    PVOID    pvIn,
    ULONG    cjOut,
    PVOID    pvOut)
{
	GPE *pGPE = (GPE *)dhpdev;
    return pGPE->DrvEscape(pso, iEsc, cjIn, pvIn, cjOut, pvOut);
}

VOID DrvDisableDriver()
{
	// by the time this is called, the driver has already been shut down.
	DEBUGMSG(GPE_ZONE_ENTER,(TEXT("Entering DrvDisableDriver\r\n")));
	DEBUGMSG(GPE_ZONE_ENTER,(TEXT("Leaving DrvDisableDriver\r\n")));
}

// Configuration / Intialization funtions

VOID APIENTRY DrvDisablePDEV(
	DHPDEV dhpdev)
{
	DEBUGMSG(GPE_ZONE_ENTER,(TEXT("Entering DrvDisablePDEV\r\n")));
	FreeConverters();
	delete ((GPE *)dhpdev);
	DEBUGMSG(GPE_ZONE_ENTER,(TEXT("Leaving DrvDisablePDEV\r\n")));

}

VOID  APIENTRY DrvDisableSurface(
	DHPDEV dhpdev)
{
	DEBUGMSG(GPE_ZONE_ENTER,(TEXT("Entering DrvDisableSurface\r\n")));
	EngDeleteSurface( (HSURF)(((GPE *)dhpdev)->GetHSurf()) );
	DEBUGMSG(GPE_ZONE_ENTER,(TEXT("Leaving DrvDisableSurface\r\n")));
}

HSURF APIENTRY DrvEnableSurface(
	DHPDEV dhpdev)
{
	HSURF hsurf;
	GPE *pGPE = (GPE *)dhpdev;
	SIZEL sizl;
	GPESurf *pSurf;

	DEBUGMSG(GPE_ZONE_ENTER,(TEXT("Entering DrvEnableSurface\r\n")));

	pSurf = pGPE->PrimarySurface();
	sizl.cx = pGPE->ScreenWidth();
	sizl.cy = pGPE->ScreenHeight();

	DEBUGMSG(GPE_ZONE_INIT,(TEXT("Primary surface is at 0x%08x\r\n"), pSurf ));
	DEBUGMSG(GPE_ZONE_INIT,(TEXT("Format of primary surface is %d\r\n"),pSurf->Format() ));
	
	hsurf = EngCreateDeviceSurface(
		(DHSURF)pSurf,
		sizl,
		EGPEFormatToIFormat[pSurf->Format()]);
	
	pGPE->SetHSurf( (unsigned long)hsurf );

	DEBUGMSG(GPE_ZONE_ENTER,(TEXT("Leaving DrvEnableSurface\r\n")));

	return hsurf;
}

// Surface creation

HBITMAP APIENTRY DrvCreateDeviceBitmap(
	DHPDEV dhpdev,
	SIZEL sizl,
	ULONG iFormat)
{
	GPE *pGPE = (GPE *)dhpdev;
	GPESurf *pSurf;

	DEBUGMSG(GPE_ZONE_ENTER,(TEXT("Entering DrvCreateDeviceBitmap\r\n")));

	if( FAILED( pGPE->AllocSurface(
		&pSurf,
		sizl.cx,
		sizl.cy,
		IFormatToEGPEFormat[iFormat],
		GPE_PREFER_VIDEO_MEMORY ) )	)
	{
		DEBUGMSG(GPE_ZONE_ENTER,(TEXT("Leaving DrvEnableSurface\r\n")));

		return (HBITMAP)0xFFFFFFFF;
	}

	HBITMAP hbm = EngCreateDeviceBitmap(
		(DHSURF)pSurf,
		sizl,
		iFormat );

	pSurf->m_nHandle = (unsigned long)hbm;

	DEBUGMSG(GPE_ZONE_ENTER,(TEXT("Leaving DrvEnableSurface\r\n")));

	if( !hbm )
		return (HBITMAP)0xFFFFFFFF;
	else
		return hbm;
}

VOID APIENTRY DrvDeleteDeviceBitmap(
	DHSURF dhsurf)
{
	DEBUGMSG(GPE_ZONE_ENTER,(TEXT("Entering DrvDeleteDeviceBitmap\r\n")));

	GPESurf *pSurf = (GPESurf *)dhsurf;
	HBITMAP hbm = (HBITMAP)(pSurf->m_nHandle);
	EngDeleteSurface( (HSURF)hbm );
	delete pSurf;
	DEBUGMSG(GPE_ZONE_ENTER,(TEXT("Leaving DrvDeleteDeviceBitmap\r\n")));
}

// Cursor control functions

VOID APIENTRY DrvMovePointer(
	SURFOBJ *pso,
	LONG x,
	LONG y,
	RECTL *prcl)
{
//	unsigned long zones = 1; // dpCurSettings.ulZoneMask;
//	DEBUGMSG(1,(TEXT("Zones = 0x%08x\r\n"),zones ));
//dpCurSettings.ulZoneMask = 0x0000ffff;
	DEBUGMSG(GPE_ZONE_ENTER,(TEXT("Entering DrvMovePointer\r\n")));

	SurfobjToGPE(pso)->MovePointer(x,y);
	DEBUGMSG(GPE_ZONE_ENTER,(TEXT("Leaving DrvMovePointer\r\n")));
}

ULONG APIENTRY DrvSetPointerShape(
	SURFOBJ  *pso,
	SURFOBJ  *psoMask,
	SURFOBJ  *psoColor,
	XLATEOBJ *pxlo,
	LONG      xHot,
	LONG      yHot,
	LONG      x,
	LONG      y,
	RECTL    *prcl,
	FLONG     fl)
{

	// dpCurSettings.ulZoneMask = 0x0000ffff;
	
	DEBUGMSG(GPE_ZONE_ENTER,(TEXT("Entering DrvSetPointerShape\r\n")));

	TmpGPESurf pMask(psoMask);
	TmpGPESurf pColor(psoColor);
	GPE *pGPE = SurfobjToGPE(pso);

	if( !pGPE )
	{
		DEBUGMSG(GPE_ZONE_ENTER,(TEXT("Leaving DrvSetPointerShape\r\n")));

		return SPS_ERROR;
	}
	if( FAILED( pGPE->SetPointerShape(pMask,pColor,xHot,yHot,
		psoMask?(psoMask->sizlBitmap.cx):0, psoMask?(psoMask->sizlBitmap.cy>>1):0 ) ) )
	{
		DEBUGMSG(GPE_ZONE_ENTER,(TEXT("Leaving DrvSetPointerShape\r\n")));

		return SPS_ERROR;
	}
	pGPE->MovePointer(x,y);
	
	DEBUGMSG(GPE_ZONE_ENTER,(TEXT("Leaving DrvSetPointerShape\r\n")));

	return SPS_ACCEPT_NOEXCLUDE;    // It is up to GPE to avoid cursor interference
}

// Palette Control Functions

BOOL APIENTRY DrvSetPalette(
	DHPDEV  dhpdev,
	PALOBJ *ppalo,
	FLONG   fl,
	ULONG   iStart,
	ULONG   cColors)
{
	DEBUGMSG(GPE_ZONE_ENTER,(TEXT("Entering DrvSetPalette\r\n")));

	unsigned long Colors[256];
	if( cColors > 256 )
		return FALSE;
	cColors = PALOBJ_cGetColors( ppalo, iStart, cColors, Colors );
	if( !cColors ||
		FAILED( ( (GPE *)dhpdev )->SetPalette( (PALETTEENTRY *)Colors,
			(unsigned short)iStart, (unsigned short)cColors ) ) )
	{
		DEBUGMSG(GPE_ZONE_ENTER,(TEXT("Leaving DrvSetPalette\r\n")));

		return FALSE;
	}
	DEBUGMSG(GPE_ZONE_ENTER,(TEXT("Leaving DrvSetPalette\r\n")));

	return TRUE;
}

// Drawing functions

BOOL APIENTRY DrvFillPath(
	SURFOBJ  *pso,
	PATHOBJ  *ppo,
	CLIPOBJ  *pco,
	BRUSHOBJ *pbo,
	POINTL   *pptlBrushOrg,
	MIX       mix,
	FLONG     flOptions)	// Simply winding mode - we ignore for now.
{

#ifdef DEBUG
	if( (GPE_ZONE_POLY) && !(GPE_ZONE_BLT_HI) )
	{
   		ulong oldSettings = dpCurSettings.ulZoneMask;
		dpCurSettings.ulZoneMask |= 0x0034;	// enter, exit & blt hi and lo
		BOOL v = DrvFillPath(pso,ppo,pco,pbo,pptlBrushOrg,mix,flOptions);
		dpCurSettings.ulZoneMask = oldSettings;
		return v;
	}
#endif

	DEBUGMSG(GPE_ZONE_ENTER,(TEXT("Entering DrvFillPath\r\n")));
   BOOL bFailed=false;
	EdgeList edgeList( ppo->cCurves );
	int bMore;
	int moreClipLists;
	PATHDATA pd;
	ULONG cptfx;
	SCODE sc1=0;
	SCODE sc2;
	RECTL *prclCurr;
	GPEBltParms parms;
	int i;
	// FIX xFirst, yFirst, xLast, yLast;
	POINTFIX firstPoint, lastPoint;

	// Set up Blt parameters for GPEPolygon::Fill to use
	TmpGPESurf pDst(pso);
	GPE *pGPE = SurfobjToGPE(pso);
	
	parms.pDst = pDst;
	parms.pSrc = (GPESurf *)NULL;
	parms.pMask = (GPESurf *)NULL;
	parms.prclDst = (RECTL *)NULL;
	parms.prclMask = (RECTL *)NULL;
	parms.prclClip = (RECTL *)NULL;		// while we prepare blt function
	parms.xPositive = parms.yPositive = 1;	// we don't support overlapped brush & dst
	parms.pptlBrush = pptlBrushOrg;
	parms.bltFlags = 0;
	parms.rop4 = (((ROP4)gaMix[(mix>>8)&0x0f])<<8) | gaMix[mix&0x0f];	// mix -> rop4
//	parms.rop4 = 0;	// TEMPORARY BLACKNESS
	parms.pBrush = (GPESurf *)NULL;
	parms.pLookup = (unsigned long *)NULL;
	parms.pConvert = NULL;
	parms.solidColor = pbo->iSolidColor;

	if( pbo )
	{
		if( pbo->iSolidColor == 0xffffffff )
		{
			if( pbo->pvRbrush == NULL )
				parms.pBrush = (GPESurf *)( BRUSHOBJ_pvGetRbrush( pbo ) );
			else
				parms.pBrush = (GPESurf *)( pbo->pvRbrush );
		}
	}



	if( FAILED( pGPE->BltPrepare( &parms ) ) )
	{
		DEBUGMSG(GPE_ZONE_ERROR,(TEXT("Failed to prepare blt for fillpoly operations\r\n")));
		DEBUGMSG(GPE_ZONE_ENTER,(TEXT("Leaving DrvFillPath\r\n")));
		return FALSE;
	}
	
	// Create GPEPolygon(s) for the path


	PATHOBJ_vEnumStart(ppo);
	do
	{
		bMore = PATHOBJ_bEnum( ppo, &pd );

		cptfx = pd.count;

		if( !cptfx ){
			break;
      }

		if( pd.flags & PD_BEGINSUBPATH ){
			// Start a new subpath - just remember the first point
			firstPoint = pd.pptfx[0];
	   } else {
			// Continue old subpath
			if FAILED(edgeList.AddEdge( lastPoint.x, lastPoint.y, pd.pptfx[0].x, pd.pptfx[0].y )){
            bMore=false;
            bFailed=true;
			}
      }
		for( i=0; i<((int)cptfx)-1; i++ ){
			if FAILED(edgeList.AddEdge( pd.pptfx[i].x, pd.pptfx[i].y, pd.pptfx[i+1].x, pd.pptfx[i+1].y )){
            bMore=false;         //The reason we don't return an error immediately is that
            bFailed=true;        //we used to draw a partial path. For BC we will continue to.
			}
      }
		if( pd.flags & PD_ENDSUBPATH ){
			// close the subpath
			if FAILED(edgeList.AddEdge( pd.pptfx[i].x, pd.pptfx[i].y, firstPoint.x, firstPoint.y )){
            bMore=false;
            bFailed=true;
			}
		}else{
			// continue this subpath with next enumeration
			lastPoint = pd.pptfx[i];
      }
	} while(bMore);

	// Now, loop through the cliprect(s), calling GPEPolygon::Fill (via GPEPolygonList)

	if( ( pco == NULL ) || ( pco->iDComplexity == DC_TRIVIAL ) )
	{
		DEBUGMSG(GPE_ZONE_POLY,(TEXT("Calling PolygonList::Fill() with no clipping\r\n")));
		sc1 = edgeList.Fill( &parms, (RECTL *)NULL, pGPE);
	}
	else if( pco->iDComplexity == DC_RECT )
	{
		DEBUGMSG(GPE_ZONE_POLY,(TEXT("Calling PolygonList::Fill() with single cliprect\r\n")));
		sc1 = edgeList.Fill( &parms, &pco->rclBounds, pGPE );
	}
	else
	{
		DEBUGMSG(GPE_ZONE_POLY,(TEXT("Iterating through complex clipping for fillpoly\r\n")));
		
		CLIPENUM ce;
		for( ce.c = 0, moreClipLists=1; ce.c || moreClipLists; )	// <- Note , & ;
		{
			if( ce.c == 0 )
			{
				// Get next list of cliprects from clipobj
				DEBUGMSG(GPE_ZONE_POLY,(TEXT("Calling CLIPOBJ_bEnum\r\n")));
				moreClipLists = CLIPOBJ_bEnum( pco, sizeof(ce), (ULONG *)&ce );
				prclCurr = ce.arcl;
				if( !ce.c )		// empty list !?
					continue;
			}
			ce.c--;

			DEBUGMSG(GPE_ZONE_POLY,(TEXT("Calling PolygonList::Fill() with complex cliprect\r\n")));
			if( FAILED(sc1 = edgeList.Fill( &parms, prclCurr++, pGPE ) ) )
				break;
		}
	}

	sc2 = pGPE->BltComplete( &parms );


	return ((!FAILED(sc1)) && (!FAILED(sc2)) && (!bFailed));

}

BOOL APIENTRY DrvRealizeBrush(
	BRUSHOBJ *pbo,
	SURFOBJ  *psoTarget,
	SURFOBJ  *psoPattern,
	SURFOBJ  *psoMask,
	XLATEOBJ *pxlo,
	ULONG    iHatch)
{
	DEBUGMSG(GPE_ZONE_ENTER,(TEXT("Entering DrvRealizeBrush\r\n")));

	TmpGPESurf tpSrc(psoPattern);
	TmpGPESurf tpTrg(psoTarget,psoPattern,&tpSrc);
	GPESurf *pSrc = tpSrc;
	GPESurf *pTrg = tpTrg;
	GPESurf *pDst;
	void *pBits;
	unsigned long stride;
	EGPEFormat patFormat = (pTrg->Format() == gpe24Bpp) ? gpe32Bpp : pTrg->Format();
	int temporaryPattern = (( pSrc->Format() != patFormat ) || (pxlo->flXlate != XO_TRIVIAL ) );

	// If the Target is 24Bpp, we convert the brush to 32Bpp because the EmulatedBlt routine
	// doesn't handle 24Bpp patterns (they cross 32-bit boundaries and this requires
	// extra code)

	// If the pattern and target have different formats, we need to create a GPESurf to
	// contain the pattern but since GDI destroys the brush without notifying the driver,
	// the bits associated with the pattern surface must be in the same memory allocation
	// to avoid memory leaks.

	int memoryRequired = sizeof(GPESurf);

	if( temporaryPattern )
	{
		stride = ((( pSrc->Width() * EGPEFormatToBpp[patFormat] + 7 ) / 8 + 3 )
			& 0xfffffffc );
		memoryRequired += stride * pSrc->Height();
	}
	else
		stride = pSrc->Stride();

	pDst = (GPESurf *)BRUSHOBJ_pvAllocRbrush( pbo, memoryRequired );

	if( !pDst )
		return FALSE;	// memory allocation failed
		
	if( temporaryPattern )
		pBits = (void *)((unsigned long)(((unsigned char *)pDst)+sizeof(GPESurf)+3)&0xfffffffc);
	else
		pBits = pSrc->Buffer();

	pDst->Init( pSrc->Width(), pSrc->Height(), pBits, stride, patFormat );

	if( temporaryPattern )
	{
		GPEBltParms parms;

		// Create a color converter to handle depth changes

		ColorConverter::InitConverter(
			pxlo,
			&parms.pColorConverter,
			&parms.pConvert,
			&parms.pLookup );

		// Blt the bits to the temporary pattern buffer

		RECTL rcl;
		rcl.top = 0;
		rcl.bottom = pSrc->Height();
		rcl.left = 0;
		rcl.right = pSrc->Width();

		parms.pDst = pDst;
		parms.pSrc = pSrc;
		parms.pMask = (GPESurf *)NULL;
		parms.pBrush = (GPESurf *)NULL;
		parms.prclDst = &rcl;
		parms.prclSrc = &rcl;
		parms.prclClip = (RECTL *)NULL;
		parms.solidColor = 0xffffffff;
		parms.bltFlags = 0;
		parms.rop4 = 0xCCCC;	// SRCCOPY
		parms.prclMask = (RECTL *)NULL;
		parms.pptlBrush = (POINTL *)NULL;
		parms.xPositive = parms.yPositive = 1;

		SurfobjToGPE(psoTarget)->EmulatedBlt( &parms );
	}

	DEBUGMSG(GPE_ZONE_ENTER,(TEXT("Leaving DrvRealizeBrush\r\n")));
	return TRUE;
}



typedef struct _DRVDEVMODEW
{
	DEVMODEW        devmodew;       // DDI definition of mode
	GPEMode         gpeMode;        // GPE definition of mode
} DRVDEVMODEW;



ULONG APIENTRY DrvGetModes(
	HANDLE    hDriver,
	ULONG     cjSize,
	DEVMODEW *pdm)
{
	DEBUGMSG(GPE_ZONE_ENTER,(TEXT("Entering DrvGetModes\r\n")));

	GPE *pGPE = GetGPE();
	int numModes = pGPE->NumModes();
	ULONG bytesReqd = numModes * sizeof(DRVDEVMODEW);
	DRVDEVMODEW *pMode = (DRVDEVMODEW *)pdm;
	GPEMode *pGPEMode;
	int modeNo;

	if( !pdm )
	{
		// GDI is asking how much memory is required for the entire mode list
		DEBUGMSG(GPE_ZONE_ENTER,(TEXT("Leaving DrvGetModes\r\n")));

		return bytesReqd;
	}

	if( cjSize != bytesReqd )
	{
		DEBUGMSG(GPE_ZONE_ENTER,(TEXT("Leaving DrvGetModes\r\n")));
		return 0;       // In this DDI, we insist that GDI gives 0 or sizeof(modelist)
	}

	memset( pMode, 0, cjSize );

	for( modeNo = 0; modeNo < numModes; modeNo++, pMode++ )
	{
		pGPEMode = &(pMode->gpeMode);
		pGPE->GetModeInfo( pGPEMode, modeNo );
		memcpy( &(pMode->devmodew.dmDeviceName), TEXT("GPE"), 8 );
		pMode->devmodew.dmSize = sizeof(DEVMODEW);
		pMode->devmodew.dmDriverExtra = sizeof(GPEMode);
		pMode->devmodew.dmFields = ( DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT |
			DM_DISPLAYFREQUENCY | DM_DISPLAYFLAGS );
		pMode->devmodew.dmBitsPerPel = pGPEMode->Bpp;
		pMode->devmodew.dmPelsWidth = pGPEMode->width;
		pMode->devmodew.dmPelsHeight = pGPEMode->height;
		pMode->devmodew.dmDisplayFrequency = pGPEMode->frequency;
	}
	DEBUGMSG(GPE_ZONE_ENTER,(TEXT("Leaving DrvGetModes\r\n")));
	return cjSize;
}

DHPDEV APIENTRY DrvEnablePDEV(
	DEVMODEW *pdm,
	LPWSTR    pwszLogAddress,
	ULONG     cPat,
	HSURF    *phsurfPatterns,
	ULONG     cjCaps,
	ULONG    *pdevcaps,
	ULONG     cjDevInfo,
	DEVINFO  *pdi,
	HDEV      hdev,
	LPWSTR    pwszDeviceName,
	HANDLE    hDriver)
{
    // Make pGPE static to allow this func to be called more than once
	static GPE *pGPE;

	DEBUGMSG(GPE_ZONE_ENTER,(TEXT("Entering DrvEnablePDEV\r\n")));

    // Only initialize display driver once.  This may be called again by
    // printer drivers that leave most of the rendering to GPE.
    if( pGPE == NULL )
    {
	    if( ( pGPE = GetGPE() ) == NULL )
	    {
		    DEBUGMSG(GPE_ZONE_ERROR,(TEXT("ERROR: Failed to instantiate GPE object\r\n")));
		    DEBUGMSG(GPE_ZONE_ENTER,(TEXT("Leaving DrvEnablePDEV\r\n")));
		    return (DHPDEV)NULL;
	    }

		if( !AllocConverters() )
		{
		    DEBUGMSG(GPE_ZONE_ENTER,(TEXT("Failed Allocate color converters! Leaving DrvEnablePDEV\r\n")));
		    return (DHPDEV)NULL;
		}
		
	    if( FAILED( pGPE->SetMode( ((DRVDEVMODEW *)pdm)->gpeMode.modeId,
	    	&(pdi->hpalDefault) ) ) )
	    {
		    DEBUGMSG(GPE_ZONE_ENTER,(TEXT("Failed to set mode! Leaving DrvEnablePDEV\r\n")));
		    return (DHPDEV)NULL;
	    }

    }

    GDIINFO *pgdiinfo = (GDIINFO *)pdevcaps;

    pgdiinfo->ulVersion     = DDI_DRIVER_VERSION;
    pgdiinfo->ulTechnology  = DT_RASDISPLAY;
    pgdiinfo->ulHorzSize    = 64;
    pgdiinfo->ulVertSize    = 60;
    pgdiinfo->ulHorzRes     = pdm->dmPelsWidth;
    pgdiinfo->ulVertRes     = pdm->dmPelsHeight;
    pgdiinfo->ulLogPixelsX  = 96;
    pgdiinfo->ulLogPixelsY  = 96;
    pgdiinfo->cBitsPixel    = pdm->dmBitsPerPel;
    pgdiinfo->cPlanes       = 1;
    pgdiinfo->ulNumColors   = 1 << pdm->dmBitsPerPel;
    pgdiinfo->ulAspectX     = 1;
    pgdiinfo->ulAspectY     = 1;
    pgdiinfo->ulAspectXY    = 1;
	pgdiinfo->flRaster		= RC_BITBLT
								| RC_STRETCHBLT
								| ((pGPE->IsPaletteSettable())?RC_PALETTE:0);

    // DEVINFO Graphics capabilities beyond normal uDDI caps; eg, grayscale text output
    pdi->flGraphicsCaps     = pGPE->GetGraphicsCaps();

	DEBUGMSG(1,(TEXT("Bits-per-pixel: %d\r\n"), pgdiinfo->cBitsPixel ));

	DEBUGMSG(GPE_ZONE_ENTER,(TEXT("Leaving DrvEnablePDEV\r\n")));

	return (DHPDEV)pGPE;
}					

//  BLT Functions

// All exposed Drv Blt functions are translated into a call to
// AnyBlt directly or indirectly:
BOOL APIENTRY AnyBlt(
	SURFOBJ  *psoTrg,
	SURFOBJ  *psoSrc,
	SURFOBJ  *psoMask,
	CLIPOBJ  *pco,
	XLATEOBJ *pxlo,
	RECTL    *prclTrg,
	RECTL    *prclSrc,
	POINTL   *pptlMask,
	BRUSHOBJ *pbo,
	POINTL   *pptlBrush,
	ROP4      rop4,
	unsigned long	bltFlags);



BOOL APIENTRY DrvPaint(
SURFOBJ*  pso,
CLIPOBJ*  pco,
BRUSHOBJ* pbo,
POINTL*   pptlBrush,
MIX       mix)
{
	DEBUGMSG(GPE_ZONE_ENTER,(TEXT("Entering DrvPaint\r\n")));

	DEBUGMSG(GPE_ZONE_ENTER,(TEXT("DrvPaint (mix=0x%04x)\r\n"), mix));

	BOOL rc;
    ROP4 rop4;

    rop4 = (((ROP4)gaMix[(mix >> 8)&0xf]) << 8) | gaMix[mix & 0xf];

    // Since our DrvFillPath routine handles almost all fills, DrvPaint
    // won't get called all that much (mainly via PaintRgn, FillRgn, or
    // complex clipped polygons).  As such, we save some code and simply
    // allow DrvBitBlt to handle it:

    rc = DrvBitBlt(pso, NULL, NULL, pco, NULL, &pco->rclBounds, NULL,
		     NULL, pbo, pptlBrush, rop4 );

	DEBUGMSG(GPE_ZONE_ENTER,(TEXT("Leaving DrvPaint\r\n")));

	return rc;
}

BOOL APIENTRY DrvCopyBits(
	SURFOBJ  *psoDest,
	SURFOBJ  *psoSrc,
	CLIPOBJ  *pco,
	XLATEOBJ *pxlo,
	RECTL    *prclDest,
	POINTL   *pptlSrc)
{
	DEBUGMSG(GPE_ZONE_ERROR,(TEXT("Entering DrvCopyBits! - SHOULD ONLY BE USED BY PRINTER DRIVER \r\n")));
	ASSERT(0);
	return FALSE;
}


BOOL APIENTRY DrvAnyBlt(
	SURFOBJ         *psoDest,
	SURFOBJ         *psoSrc,
	SURFOBJ         *psoMask,
	CLIPOBJ         *pco,
	XLATEOBJ        *pxlo,
	POINTL          *pptlHTOrg,         // Halftone brush origin
	RECTL           *prclDest,
	RECTL           *prclSrc,
	POINTL          *pptlMask,
	BRUSHOBJ        *pbo,
	POINTL          *pptlBrush,
	ROP4             rop4,
	ULONG            iMode,             // DrvStretchBlt iMode: eg, COLORONCOLOR, HALFTONE
	ULONG	         bltFlags)
{
	DEBUGMSG(GPE_ZONE_ENTER,(TEXT("Entering DrvAnyBlt\r\n")));

	return AnyBlt(psoDest, psoSrc, psoMask, pco, pxlo, prclDest, prclSrc,
		pptlMask, pbo, pptlBrush, rop4, bltFlags );
}



BOOL APIENTRY DrvTransparentBlt(
	SURFOBJ         *psoDest,
	SURFOBJ         *psoSrc,
	CLIPOBJ         *pco,
	XLATEOBJ        *pxlo,
	RECTL           *prclDest,
	RECTL           *prclSrc,
	ULONG            TransColor)
{
	DEBUGMSG(GPE_ZONE_ENTER,(TEXT("Entering DrvTransparentBlt\r\n")));

	unsigned long bltFlags = BLT_TRANSPARENT;
    BRUSHOBJ bo;
    bo.iSolidColor = TransColor;


	// Since this may be called with an actual stretch or a 1:1, check
	// the actual dimensions.


	return AnyBlt(psoDest, psoSrc, NULL, pco, pxlo, prclDest, prclSrc,
		NULL, &bo, (POINTL *)NULL, 0xCCCC, bltFlags );
}



BOOL APIENTRY DrvBitBlt(
	SURFOBJ  *psoTrg,
	SURFOBJ  *psoSrc,
	SURFOBJ  *psoMask,
	CLIPOBJ  *pco,
	XLATEOBJ *pxlo,
	RECTL    *prclTrg,
	POINTL   *pptlSrc,
	POINTL   *pptlMask,
	BRUSHOBJ *pbo,
	POINTL   *pptlBrush,
	ROP4      rop4)
{
	DEBUGMSG(GPE_ZONE_ENTER,(TEXT("Entering DrvBitBlt\r\n")));
	RECTL rclSrc;
	if( pptlSrc )
	{
		rclSrc.top = pptlSrc->y;
		rclSrc.left = pptlSrc->x;
	}
	else
	{
		rclSrc.top = 0;
		rclSrc.left = 0;
	}
	rclSrc.bottom = rclSrc.top + prclTrg->bottom - prclTrg->top;
	rclSrc.right = rclSrc.left + prclTrg->right - prclTrg->left;

	return AnyBlt( psoTrg, psoSrc, psoMask, pco, pxlo, prclTrg, &rclSrc,
		pptlMask, pbo, pptlBrush, rop4, 0 );
}

SCODE ClipBlt( GPE *pGPE, GPEBltParms *pBltParms );

// The AnyBlt function can handle any stretching, color translation, masking, etc
// and services all of the other DrvBlt style functions

BOOL APIENTRY AnyBlt(
	SURFOBJ  *psoTrg,
	SURFOBJ  *psoSrc,
	SURFOBJ  *psoMask,
	CLIPOBJ  *pco,
	XLATEOBJ *pxlo,
	RECTL    *prclTrg,
	RECTL    *prclSrc,
	POINTL   *pptlMask,
	BRUSHOBJ *pbo,
	POINTL   *pptlBrush,
	ROP4      rop4,
	unsigned long bltFlags)
{

//	if( pbo && (pbo->iSolidColor == 0xffffffff ) && !(GPE_ZONE_BLT_HI) )
//		{
//        DebugBreak();
//	   		ulong oldSettings = dpCurSettings.ulZoneMask;
//		dpCurSettings.ulZoneMask |= 0x0034;	// enter, exit & blt hi and lo
//			BOOL v = DrvBitBlt(psoTrg,psoSrc,psoMask,pco,pxlo,prclTrg,pptlSrc,pptlMask,pbo,pptlBrush,rop4);
//			dpCurSettings.ulZoneMask = oldSettings;
//			return v;
//		}

	DEBUGMSG(GPE_ZONE_ENTER,(TEXT("AnyBlt (rop4 = 0x%04x) rclTrg=t:%d,l:%d,b:%d,r:%d\r\n"),
		rop4, prclTrg->top, prclTrg->left, prclTrg->bottom, prclTrg->right ));

	GPE *pGPE = SurfobjToGPE(psoTrg);
	RECTL *prclCurr;
	GPEBltParms parms;
	TmpGPESurf pDst(psoTrg);
	TmpGPESurf pSrc(psoSrc,psoTrg,&pDst);
	TmpGPESurf pMask(psoMask);
	RECTL rclMask;
	int moreClipLists;
	SCODE sc1=0;
	SCODE sc2;


	if( pptlMask )
	{
		rclMask.top = pptlMask->y;
		rclMask.left = pptlMask->x;
		rclMask.bottom = pptlMask->y + prclTrg->bottom - prclTrg->top;
		rclMask.right = pptlMask->x + prclTrg->right - prclTrg->left;
	}
	else
		rclMask = *prclSrc;

	parms.pDst = pDst;
	parms.prclDst = prclTrg;
	parms.xPositive = 1;
	parms.yPositive = 1;
	parms.rop4 = rop4;
	parms.pLookup = (unsigned long *)NULL;
	parms.pConvert = NULL;
	parms.pBrush = (GPESurf *)NULL;
	parms.solidColor = 0xffffffff;

	// See if source matters
	if( ( rop4 ^ ( rop4 >> 2 ) ) & 0x3333 )
	{
		parms.prclSrc = prclSrc;
		parms.pSrc = pSrc;
		if( !prclTrg || !prclSrc )
			return 0;
		if( (prclTrg->right - prclTrg->left != prclSrc->right - prclSrc->left )
			|| ( prclTrg->bottom - prclTrg->top != prclSrc->bottom - prclSrc->top ))
			bltFlags |= BLT_STRETCH;
	}
	else
	{
		parms.prclSrc = (RECTL *)NULL;
		parms.pSrc = (GPESurf *)NULL;
		bltFlags &= ~BLT_STRETCH;
	}
	parms.bltFlags = bltFlags;

	// See if pattern matters
	if( ( rop4 ^ ( rop4 >> 4 ) ) & 0x0F0F )
	{
		parms.pptlBrush = pptlBrush;
		if( pbo )
		{
			if( pbo->iSolidColor == 0xffffffff )
			{
				if( pbo->pvRbrush == NULL )
					parms.pBrush = (GPESurf *)( BRUSHOBJ_pvGetRbrush( pbo ) );
				else
					parms.pBrush = (GPESurf *)( pbo->pvRbrush );
			}
			else
			{
				parms.solidColor = pbo->iSolidColor;
				parms.pptlBrush = (POINTL *)NULL;
			}
		}
		else
		{
			DEBUGMSG(GPE_ZONE_ERROR,(TEXT("Blt w/ rop4=0x%04x and pbo==NULL !\r\n"),rop4));
			return FALSE;
		}
	}
	else
	{
		parms.pptlBrush = (POINTL *)NULL;
		if( pbo )
			parms.solidColor = pbo->iSolidColor;	// This is used as the transparent color
	}

	// See if mask matters
	if( ( rop4 ^ ( rop4 >> 8 ) ) & 0x00FF )
	{
		parms.prclMask = &rclMask;
		parms.pMask = pMask;
	}
	else
	{
		parms.prclMask = (RECTL *)NULL;
		parms.pMask = (GPESurf *)NULL;
	}


	ULONG iDir = CD_ANY;	// default to clip enumeration in any order

	if( (GPESurf *)(parms.pDst) == (GPESurf *)(parms.pSrc) )	// <-- Won't happen on stretch Blts
	{
		// Check for overlap since source and dest surfaces are the same
		if( 	prclSrc->bottom > prclTrg->top
			&& 	prclSrc->top < prclTrg->bottom
			&&	prclSrc->right > prclTrg->left
			&&	prclSrc->left < prclTrg->right )
		{
			if( prclSrc->top == prclTrg->top )
			{
				// Horizontal blt, just set xPositive appropriately
				parms.xPositive = prclSrc->left >= prclTrg->left;
			}
			else
				// Non horizontal blts, just set yPositive appropriately
				parms.yPositive = prclSrc->top >= prclTrg->top;

			// In case we enumerate cliprects - determine the order to use
			if( prclSrc->top > prclTrg->top )
				iDir = ( prclSrc->left > prclTrg->left ) ? CD_RIGHTDOWN : CD_LEFTDOWN;
			else
				iDir = ( prclSrc->left > prclTrg->left ) ? CD_RIGHTUP : CD_LEFTUP;
		}
	}
	else if ( parms.pSrc )
	{
		ColorConverter::InitConverter(
			pxlo,
			&parms.pColorConverter,
			(unsigned long (ColorConverter::** )(unsigned long))&parms.pConvert,
			&parms.pLookup );
	}

	parms.prclClip = (RECTL *)NULL;
	if( pco )
		if( pco->iDComplexity == DC_RECT )
			parms.prclClip = &(pco->rclBounds);

	if( FAILED(pGPE->BltPrepare( &parms ) ) )
	{
		DEBUGMSG(GPE_ZONE_ERROR,(TEXT("failed to prepare blt\r\n")));
		DEBUGMSG(GPE_ZONE_ENTER,(TEXT("Leaving DrvBitBlt\r\n")));
		return FALSE;
	}

	if( pco && pco->iDComplexity == DC_COMPLEX )
	{
		// Non-trivial clipping - set up the Blt, then iterate through the cliprects & complete the blt
		CLIPENUM ce;

		// Request cliprects in a specific order to handle overlaps
		CLIPOBJ_cEnumStart( pco, TRUE, CT_RECTANGLES, iDir, 0 );	// don't need to count

		for( ce.c = 0, moreClipLists=1; ce.c || moreClipLists; )
		{
			if( ce.c == 0 )
			{
				// Get next list of cliprects from clipobj
				moreClipLists = CLIPOBJ_bEnum( pco, sizeof(ce), (ULONG *)&ce );
				prclCurr = ce.arcl;
				if( !ce.c )
					continue;
			}

			parms.prclClip = prclCurr++;
			ce.c--;

			DEBUGMSG(GPE_ZONE_BLT_HI,(TEXT("Calling GPE::Blt with complex cliprect\r\n")));
//#define ALLOW_MODIFY_BLTPARMS_IN_CLIP_BLT
#ifdef ALLOW_MODIFY_BLTPARMS_IN_CLIP_BLT
			GPEBltParms parmsCopy = parms;
			if( FAILED( sc1 = ClipBlt( pGPE, &parmsCopy ) ) )
#else
			if( FAILED( sc1 = ClipBlt( pGPE, &parms ) ) )
#endif
			{
				DEBUGMSG(GPE_ZONE_ENTER,(TEXT("Leaving DrvBitBlt\r\n")));
				break;
			}
		}
	}
	else
		sc1 = ClipBlt( pGPE, &parms );

	sc2 = pGPE->BltComplete( &parms );

	DEBUGMSG(GPE_ZONE_ENTER,(TEXT("Leaving DrvBitBlt\r\n")));

	return !FAILED(sc1) && !FAILED(sc2);
}


SCODE ClipBlt(
	GPE *pGPE,
	GPEBltParms *pBltParms )
{
	DEBUGMSG(GPE_ZONE_BLT_LO,(TEXT("ClipBlt continue phase\r\n")));

#ifdef ALLOW_MODIFY_BLTPARMS_IN_CLIP_BLT

	if( pBltParms->prclClip && !( pBltParms->bltFlags & BLT_STRETCH ) )
	{
		RECTL rclDst = *(pBltParms->prclDst);
		RECTL &rclClip = *(pBltParms->prclClip);

		if( rclDst.left < rclClip.left )
			rclDst.left = rclClip.left;
		if( rclDst.top < rclClip.top )
			rclDst.top = rclClip.top;
		if( rclDst.bottom > rclClip.bottom )
			rclDst.bottom = rclClip.bottom;
		if( rclDst.right > rclClip.right )
			rclDst.right = rclClip.right;
		if( rclDst.right <= rclDst.left || rclDst.bottom <= rclDst.top )
			return S_OK;	// the clipping left nothing to do

		int OffLeft   = rclDst.left   - pBltParms->prclDst->left;
		int OffTop    = rclDst.top    - pBltParms->prclDst->top;
		int OffRight  = rclDst.right  - pBltParms->prclDst->right;
		int OffBottom = rclDst.bottom - pBltParms->prclDst->bottom;

		pBltParms->prclDst = &rclDst;

		if (OffLeft || OffTop || OffRight || OffBottom)
		{
			if( pBltParms->prclSrc )
			{
				register RECTL * const prclSrc = pBltParms->prclSrc;
				prclSrc->left += OffLeft;
				prclSrc->right += OffRight;
				prclSrc->top += OffTop;
				prclSrc->bottom += OffBottom;
			}

			if( pBltParms->prclMask )
			{
				register RECTL * const prclMask = pBltParms->prclMask;
				prclMask->left += OffLeft;
				prclMask->right += OffRight;
				prclMask->top += OffTop;
				prclMask->bottom += OffBottom;
			}
		}

		pBltParms->prclClip = (RECTL *)NULL;

		// Perform actual Blt
		return (pGPE->*(pBltParms->pBlt))( pBltParms );
	}
	else
		// Perform actual Blt
		return (pGPE->*(pBltParms->pBlt))( pBltParms );

#else // !ALLOW_MODIFY_BLTPARMS_IN_CLIP_BLT

	if( pBltParms->prclClip && !( pBltParms->bltFlags & BLT_STRETCH ) )
	{
		RECTL rclDst;
		RECTL rclSrc;
		RECTL rclMask;
		RECTL rclClip = *(pBltParms->prclClip);
		GPEBltParms parms = *pBltParms;
		parms.prclDst = &rclDst;
		rclDst = *(pBltParms->prclDst);
		if( rclDst.left < rclClip.left )
			rclDst.left = rclClip.left;
		if( rclDst.top < rclClip.top )
			rclDst.top = rclClip.top;
		if( rclDst.bottom > rclClip.bottom )
			rclDst.bottom = rclClip.bottom;
		if( rclDst.right > rclClip.right )
			rclDst.right = rclClip.right;
		if( rclDst.right <= rclDst.left || rclDst.bottom <= rclDst.top )
			return S_OK;	// the clipping left nothing to do

		int OffLeft   = rclDst.left   - pBltParms->prclDst->left;
		int OffTop    = rclDst.top    - pBltParms->prclDst->top;
		int OffRight  = rclDst.right  - pBltParms->prclDst->right;
		int OffBottom = rclDst.bottom - pBltParms->prclDst->bottom;

		if (OffLeft || OffTop || OffRight || OffBottom)
		{
			if( parms.prclSrc )
			{
				rclSrc = *(parms.prclSrc);
				parms.prclSrc = &rclSrc;
				rclSrc.left += OffLeft;
				rclSrc.right += OffRight;
				rclSrc.top += OffTop;
				rclSrc.bottom += OffBottom;
			}

			if( parms.prclMask )
			{
				rclMask = *(parms.prclMask);
				parms.prclMask = &rclMask;
				rclMask.left += OffLeft;
				rclMask.right += OffRight;
				rclMask.top += OffTop;
				rclMask.bottom += OffBottom;
			}
		}

		parms.prclClip = (RECTL *)NULL;

		// Perform actual Blt
		return (pGPE->*(pBltParms->pBlt))( &parms );
	}
	else
		// Perform actual Blt
		return (pGPE->*(pBltParms->pBlt))( pBltParms );

#endif // !ALLOW_MODIFY_BLTPARMS_IN_CLIP_BLT

}
