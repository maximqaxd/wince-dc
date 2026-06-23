/*

Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.

*/
#include "precomp.h"

// Flip and round flags:

#define DEBUGTEMP 0

#define FL_H_ROUND_DOWN         0x00000080L     // .... .... 1... ....
#define FL_V_ROUND_DOWN         0x00008000L     // 1... .... .... ....

#define FL_FLIP_D               0x00000005L     // .... .... .... .1.1
#define FL_FLIP_V               0x00000008L     // .... .... .... 1...
#define FL_FLIP_SLOPE_ONE       0x00000010L     // .... .... ...1 ....
#define FL_FLIP_HALF            0x00000002L     // .... .... .... ..1.
#define FL_FLIP_H               0x00000200L     // .... ..1. .... ....

#define FL_ROUND_MASK           0x0000001CL     // .... .... ...1 11..
#define FL_ROUND_SHIFT          2

#define FL_RECTLCLIP_MASK       0x0000000CL     // .... .... .... 11..
#define FL_RECTLCLIP_SHIFT      2

#define FL_STRIP_MASK           0x00000003L     // .... .... .... ..11
#define FL_STRIP_SHIFT          0

#define FL_SIMPLE_CLIP          0x00000020      // .... .... ..1. ....
#define FL_COMPLEX_CLIP         0x00000040      // .... .... .1.. ....
#define FL_CLIP                (FL_SIMPLE_CLIP | FL_COMPLEX_CLIP)

#define FL_STYLED               0x00000400L     // .... .1.. .... ....
#define FL_ALTERNATESTYLED      0x00001000L     // ...1 .... .... ....

#define FL_STYLE_MASK           0x00000400L
#define FL_STYLE_SHIFT          10

#define FL_LAST_PEL_INCLUSIVE   0x00002000L     // ..1. .... .... ....


#define CLIP_LIMIT 50
typedef struct _CLIPENUM {
    LONG    c;
    RECTL   arcl[CLIP_LIMIT];   // Space for enumerating complex clipping

} CLIPENUM;                         /* ce, pce */

#define SWAP(a,b,type) { type tmp=(a); a=(b); b=tmp; }

const FLONG gaflRound[] = {
    FL_H_ROUND_DOWN | FL_V_ROUND_DOWN, // no flips
    FL_H_ROUND_DOWN | FL_V_ROUND_DOWN, // FL_FLIP_D
    FL_H_ROUND_DOWN,                   // FL_FLIP_V
    FL_V_ROUND_DOWN,                   // FL_FLIP_V | FL_FLIP_D
    FL_V_ROUND_DOWN,                   // FL_FLIP_SLOPE_ONE
    0xbaadf00d,                        // FL_FLIP_SLOPE_ONE | FL_FLIP_D
    FL_H_ROUND_DOWN,                   // FL_FLIP_SLOPE_ONE | FL_FLIP_V
    0xbaadf00d                         // FL_FLIP_SLOPE_ONE | FL_FLIP_V | FL_FLIP_D
};

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


BOOL APIENTRY DrvStrokePath(
	SURFOBJ   *pso,
	PATHOBJ   *ppo,
	CLIPOBJ   *pco,
	XFORMOBJ  *pxo,
	BRUSHOBJ  *pbo,
	POINTL    *pptlBrushOrg,
	LINEATTRS *plineattrs,
	MIX        mix)
{
	DEBUGMSG(GPE_ZONE_ENTER,(TEXT("Entering DrvStrokePath\r\n")));

	CLIPENUM ce;						// Clip enumeration allocation
	int moreClipLists;					// Continue flag for clip enumeration
	PATHDATA pd;						// Path enumeration allocation
	int morePointLists;					// Continue flag for path enumeration
	GPELineParms parms;					// Line parameters passed to individual line drawing calls
	GPE *pGPE = SurfobjToGPE(pso);		// Extract pGPE for this device from SURFOBJ
	TmpGPESurf pDst(pso);				// Either actual pSurf or temporary one for dest surf
	RECTL *prclCurr;					// Pointer into cliprect list
	RECTL arclClip[8];					// Cliprect rotated/flipped
	RECTL *prclFlipped;					// Pointer into arclClip[]
	unsigned long pointNo;				// Point counter within subpath
	POINTFIX ptfxStartFigure;			// X,Y of first point in subpath
	POINTFIX ptfxLast;					// X,Y of last point from previous point sequence
	POINTFIX *pptfxPrev;				// &[X,Y] of previous point
	POINTFIX *pptfxBuf;					// &[X,Y] of current point
	ULONG cptfx;						// Remaining points in current point list
	FIX fx0, fy0, fx1, fy1;				// Endpoints of line segment in 28.4 format
	unsigned long fl;					// Current flip/rotate flags for line segment
	unsigned long x,y;					// Integer components of starting point on line
	FIX M0, N0;							// Fractional component of fx0,fy0 after flipping (0..15)
	FIX M1, N1;							// Fractional component of fx1, fy1 after flipping
	FIX dM, dN;							// dX and dY after flipping  ( 28.4 format )
	unsigned long x0, y0;				// Starting point wrt initial x,y ( +0..+2 each )
	unsigned long x1, y1;				// End point wrt initial x,y
	long llGamma;						// Error term
	long llBeta;						// Error term... minor dimension
	long errorTerm;
	int cStylePels;						// Number of pixels in unclipped line
	int cPels;							// Number of pixels in clipped line
	int xLeft, xRight, yTop, yBottom;	// Rotated/reflected cliprect
	long xStart,yStart;					// Final first pixel coordinates
//	long ll;
//	unsigned long ulDelta;
	int iDir;							// Octant number for line


#if DEBUGTEMP
					int orig_x0 = 0xcccccccc;
					int orig_x1 = 0xcccccccc;
					int orig_y0 = 0xcccccccc;
					int x0_v1 = 0xcccccccc;
					int y0_v1 = 0xcccccccc;
					int x0_v2 = 0xcccccccc;
					int y0_v2 = 0xcccccccc;
					int orig_y1 = 0xcccccccc;
					int x1_v3 = 0xcccccccc;
					int y1_v3 = 0xcccccccc;
					int dM_v = 0xcccccccc;
					int dN_v = 0xcccccccc;
					int llGamma_v = 0xcccccccc;
#endif

	// Populate GPELineParms elements which are consistent along path

	// struct GPELineParms
	// {
	// 	SCODE 			(GPE::*pLine)(GPELineParms *);
	// 	GPESurf 		*pDst;
	// 	unsigned long	x1,	y1, x2, y2;
	// 	COLOR			solidColor;
	// 	RECTL			*prclClip;
	// 	int				state;
	// 	unsigned short	mix;
	// };

	parms.solidColor = pbo->iSolidColor;
	DEBUGMSG(GPE_ZONE_LINE,(TEXT("Mix:%04x Line color: %08x\r\n"), mix, pbo->iSolidColor));
	parms.mix = (unsigned short)mix;
	parms.pDst = pDst;
	parms.style = 0;

	if( plineattrs && (((mix>>8)&0x00ff)!=(mix&0x00ff)))
	{
		if( plineattrs->fl & LA_ALTERNATE )	// Every other pixel on
			parms.style = 0xaaaaaaaa;
		else if( plineattrs->pstyle && plineattrs->cstyle  )
		{
			int bitno=0;
			int bitval=0;
			int dashno;
			int dashpixel;
			for( dashno=0;
				bitno < 32;
				dashno++ )
			{
				dashno %= plineattrs->cstyle;
				for( dashpixel=0;
					( dashpixel < plineattrs->pstyle[dashno].l ) && ( bitno < 32 );
					dashpixel++, bitno++ )
						parms.style |= (bitval<<bitno);
				bitval ^= 1;
				if( dashno == 64 )		// in case all of the segments have 0 length
				{
					parms.style = 0;
					break;
				}
			}
		}
		if( plineattrs->fl & LA_STARTGAP )	// First dash is actually a gap
			parms.style = ~(parms.style);
	}











	// Call device specific line prepare
	// This can set up hardware mode (such as color) and must populate pLineParms->pLine

	DEBUGMSG(GPE_ZONE_LINE,(TEXT("Calling GPE::Line [prepare]\r\n")));

	if( FAILED( pGPE->Line( &parms, gpePrepare ) ) )
	{
		DEBUGMSG(GPE_ZONE_ENTER,(TEXT("Leaving DrvStrokePath\r\n")));
		return FALSE;
	}

	// Examine clipping

	if ( ( pco == NULL ) || ( pco->iDComplexity == DC_TRIVIAL ) )
	{
		DEBUGMSG(GPE_ZONE_LINE,(TEXT("DrvStrokePath: no clipping\r\n")));
		prclCurr = (RECTL *)NULL;
		moreClipLists = 0;
	}
	else if ( pco->iDComplexity == DC_RECT )
	{
		DEBUGMSG(GPE_ZONE_LINE,(TEXT("DrvStrokePath: single (DC_RECT) cliprect\r\n")));
		prclCurr = &pco->rclBounds;
		moreClipLists = 0;
	}
	else
	{
		DEBUGMSG(GPE_ZONE_LINE,(TEXT("DrvStrokePath: complex clipping region\r\n")));
		moreClipLists = 1;
	}

	// Iterate through each rectangle in clip list (once if no list)

	for( ce.c = 1-moreClipLists; ce.c || moreClipLists; prclCurr++, ce.c-- )
	{
		if( ce.c == 0 )
		{
			// Get next list of cliprects from clipobj
			moreClipLists = CLIPOBJ_bEnum( pco, sizeof(ce), (ULONG *)&ce );
			prclCurr = ce.arcl;
			if( !ce.c )
				continue;
		}












		parms.styleState = 0;


		// Flip cliprect into possible orientations

        arclClip[0]        =  *prclCurr;

    // FL_FLIP_D:

        arclClip[1].top    =  prclCurr->left;
        arclClip[1].left   =  prclCurr->top;
        arclClip[1].bottom =  prclCurr->right;
        arclClip[1].right  =  prclCurr->bottom;

    // FL_FLIP_V:

        arclClip[2].top    = -prclCurr->bottom + 1;
        arclClip[2].left   =  prclCurr->left;
        arclClip[2].bottom = -prclCurr->top + 1;
        arclClip[2].right  =  prclCurr->right;

    // FL_FLIP_V | FL_FLIP_D:

        arclClip[3].top    =  prclCurr->left;
        arclClip[3].left   = -prclCurr->bottom + 1;
        arclClip[3].bottom =  prclCurr->right;
        arclClip[3].right  = -prclCurr->top + 1;


    // FL_FLIP_H:

        arclClip[4].top    =  prclCurr->top;
        arclClip[4].left   =  -prclCurr->right + 1;
        arclClip[4].bottom =  prclCurr->bottom;
        arclClip[4].right  =  -prclCurr->left + 1;

    // FL_FLIP_H | FL_FLIP_D:

        arclClip[5].top    =  -prclCurr->right + 1;
        arclClip[5].left   =  prclCurr->top;
        arclClip[5].bottom =  -prclCurr->left + 1;
        arclClip[5].right  =  prclCurr->bottom;

    // FL_FLIP_H | FL_FLIP_V:

        arclClip[6].top    = -prclCurr->bottom + 1;
        arclClip[6].left   = -prclCurr->right + 1;
        arclClip[6].bottom = -prclCurr->top + 1;
        arclClip[6].right  = -prclCurr->left + 1;

    // FL_FLIP_H | FL_FLIP_V | FL_FLIP_D:

        arclClip[7].top    =  -prclCurr->right + 1;
        arclClip[7].left   =  -prclCurr->bottom + 1;
        arclClip[7].bottom =  -prclCurr->left + 1;
        arclClip[7].right  =  -prclCurr->top + 1;

		// Loop through the line segments in the path
		PATHOBJ_vEnumStart(ppo);
		do
		{
			morePointLists = PATHOBJ_bEnum( ppo, &pd );
			cptfx = pd.count;
			if( !cptfx )
				break;
			if( pd.flags & PD_BEGINSUBPATH )
			{
				ptfxStartFigure = *pd.pptfx;
				pptfxPrev = pd.pptfx;
				pptfxBuf = pd.pptfx+1;
				cptfx--;
			}
			else
			{
				pptfxPrev = &ptfxLast;
				pptfxBuf = pd.pptfx;
			}
			for( pointNo=0; pointNo<=cptfx; pointNo++ )
			{
//				parms.solidColor = (pointNo & 1 ) ? 2 : 4;	 // old test to alternate line colors

				if( pointNo < cptfx )
				{
					fx0 = pptfxPrev->x;
					fy0 = pptfxPrev->y;
					fx1 = pptfxBuf->x;
					fy1 = pptfxBuf->y;
					pptfxPrev = pptfxBuf++;
				}
				else
				{
					ptfxLast = pd.pptfx[pd.count-1];

					if( ! ( pd.flags & PD_CLOSEFIGURE ) )
						break;
					fx0 = ptfxLast.x;
					fy0 = ptfxLast.y;
					fx1 = ptfxStartFigure.x;
					fy1 = ptfxStartFigure.y;

				}

				DEBUGMSG(GPE_ZONE_LINE,(TEXT("Line Seg %d.%d,%d.%d - %d.%d,%d.%d\r\n"),
					fx0>>4, fx0&0x000f,
					fy0>>4, fy0&0x000f,
					fx1>>4, fx1&0x000f,
					fy1>>4, fy1&0x000f ));

				// Rotate/Flip the line segment into octant 0  ( dX>=0, dY>=0, dX>=dY )
				fl = 0;

//				if( fx1 < fx0 )
//				{
//					// Reverse direction of line to make dX>0
//					SWAP(fx0,fx1,FIX)
//					SWAP(fy0,fy1,FIX)
//					fl |= FL_FLIP_H;
//				}

				if( fx1 < fx0 )
				{
					// Reflect across x=0 to make dX>0
					fx0 = -fx0;
					fx1 = -fx1;
					fl |= FL_FLIP_H;
				}
				
				if( fy1 < fy0 )
				{
					// Reflect across y=0 to make dY>0
					fy0 = -fy0;
					fy1 = -fy1;
					fl |= FL_FLIP_V;
				}

				if( ( fy1 - fy0 ) > ( fx1 - fx0 ) )
				{
					// Reflect across y=x to make dX>=dY
					SWAP(fx0,fy0,FIX)
					SWAP(fx1,fy1,FIX)
					fl |= FL_FLIP_D;
				}

				dM = fx1 - fx0;
				dN = fy1 - fy0;
				
				if( dM == dN )
					fl |= FL_FLIP_SLOPE_ONE;	// take note of 1:1 slope

				// Based on the flips, set FL_H_ROUND_DOWN &/or FL_V_ROUND_DOWN
				fl |= gaflRound[ (fl & FL_ROUND_MASK) >>FL_ROUND_SHIFT];

				x = fx0>>4;
				y = fy0>>4;
				M0 = fx0 & 0x000f;
				N0 = fy0 & 0x000f;

				// Calculate remainder term  [ dM * ( N0 + F/2 ) - M0 * dN ]

				llGamma = (dM * ( N0 + 8 ) - dN * M0 - ((fl&FL_V_ROUND_DOWN)?1:0)) >> 4;
				llBeta = ~llGamma;
				//.... N0,M0 are 0..15 so this is ok if dM,dN < about 8 million pixels

				// Calculate N1,M1 - the fractional part of the line ends
				M1 = ( M0 + dM ) & 0x000f;
				N1 = ( N0 + dN ) & 0x000f;

				// x0,y0..x1,y1 are the integer parts of the line ends where the origin
				// of the line is at 0.M0,0.N0
				// Thus x0, and y0 start off as 0 but may become 1 or 2 based on the exact
				// starting point of the line to be rendered (and whether the last point
				// is excluded)
				
				x1 = ( M0 + dM ) >> 4;
				

//				if( fl & FL_FLIP_H )
//				{
//					// Unflipped line goes right to left so x1,y1 is start
//					x0 = 1;
//					if( N1 == 0 )
//					{
//						if( M1 > ((fl & FL_H_ROUND_DOWN) ? 8 : 7 ) )
//							x1++;
//					}
//					else if ( ((N1<8)?(8-N1):(N1-8)) + M1 > 16 )
//						x1++;
//					if((fl & (FL_FLIP_SLOPE_ONE | FL_H_ROUND_DOWN)) == FL_FLIP_SLOPE_ONE)
//					{
//						if(( N1 > 0 ) && ( M1 == N1+8 ))
//							x1++;
//						if(( N0 > 0 ) && ( M0 == N0+8 ))
//							x0 = 2;
//					}
//					if( x0 != 2 )
//					{
//						if( N0 == 0 )
//						{
//							if( M0 > ((fl & FL_H_ROUND_DOWN) ? 8 : 7 ) )
//								x0 = 2;
//						}
//						else if ( ((N0<8)?(8-N0):(N0-8)) + M0 > 16 )
//							x0 = 2;
//					}
//					ulDelta = ( x0 == 2 ) ? dN : 0;
//
//					ll = llGamma + ulDelta;
//
//					if( ll >= (long)( 2 * dM - dN ) )
//					{
//						y0 = 2;
//					}
//					else if( ll >= (long)( dM - dN ) )
//					{
//						y0 = 1;
//					}
//					else
//					{
//						y0 = 0;
//					}
//					errorTerm=ll;
//					while(errorTerm>0)
//						errorTerm -= dM;
//
//				}
//				else
				{
					// Line is left to right ( x0,y0 is start )

					x1--;

					if( M1 > 0 )
					{
						if( N1 == 0 )
						{
							if( M1 > ((fl & FL_H_ROUND_DOWN) ? 8 : 7 ) )
								x1++;
						}
						else if ( ((N1<8)?(8-N1):(N1-8)) <= M1 )
							x1++;
					}
					x0 = 0;
					if((fl & (FL_FLIP_SLOPE_ONE|FL_H_ROUND_DOWN))
						== (FL_FLIP_SLOPE_ONE|FL_H_ROUND_DOWN))
					{
						if(( M1 > 0 ) && ( N1 == M1+8 ))
							x1--;
						if(( M0 > 0 ) && ( N0 == M0+8 ))
							goto left_to_right_compute_y0;
					}
					if( M0 > 0 )
					{
						if( N0 == 0 )
						{
							if( M0 > ((fl & FL_H_ROUND_DOWN) ? 8 : 7 ) )
								x0 = 1;
						}
						else if ( ((N0<8)?(8-N0):(N0-8)) <= M0 )
							x0 = 1;
					}
					
					left_to_right_compute_y0:

					if( llGamma >= (long)(dM-(dN&(-(long)x0))) )
					{
						errorTerm = llGamma + ( dN & ~(x0-1) ) - 2 * dM;
						y0 = 1;
					}
					else
					{
						errorTerm = llGamma + ( dN & ~(x0-1) ) - dM;
						y0 = 0;
					}

				}


				llGamma = errorTerm;
				llBeta = ~llGamma;

				//  if( ( fl & FL_FLIP_H ) || ( fl & FL_FLIP_D ) || ( fl & FL_FLIP_SLOPE_ONE ) || ( fl & FL_FLIP_V ) )
				// if( ( fl & FL_FLIP_H ) )
				//	continue;

				// Calculate length of unclipped line in pixels for style tracking
				cStylePels = x1 - x0 + 1;
				if( cStylePels <= 0 )
					continue;		// This line segment was too short to be visible!
	

				


				// Normalize cliprect to origin

				prclFlipped = &arclClip[(fl&FL_RECTLCLIP_MASK) >> FL_RECTLCLIP_SHIFT];
				if( fl & FL_FLIP_H )
					prclFlipped += 4;
				xRight = prclFlipped->right - x;
				xLeft = prclFlipped->left - x;
				yTop = prclFlipped->top - y;
				yBottom = prclFlipped->bottom - y;

				// Clip line to cliprect

				DEBUGMSG(GPE_ZONE_LINE,(TEXT("x0,y0=%d,%d x1=%d. Clip: %d,%d - %d,%d.  llGamma = %d, llBeta = %d\r\n"),
					x0,y0,x1, xLeft,yTop,xRight,yBottom, llGamma, llBeta ));

				if( (long)y0 >= yBottom || (long)x0 >= xRight || (long)x1 < xLeft )
					continue;  // totally clipped  ( note, we don't know y1 yet )

				if( (long)x1 >= xRight )
					x1 = xRight - 1;

				if( dM == 0 || dN == 0 )
				{
					DEBUGMSG(GPE_ZONE_LINE,(TEXT("Horizontal or Vertical\r\n")));
					if( (long)x0 < xLeft )
						x0 = (unsigned long)xLeft;
					if( (long)y0 < yTop )
						continue;
					y1 = y0;
				}
				else
				{
#if DEBUGTEMP
					orig_x0 = (int)x0;
					orig_x1 = (int)x1;
					orig_y0 = (int)y0;
#endif

					if( (long)x0 < xLeft )
					{
						y0 = y0 + (unsigned long)( ( xLeft - (long)x0 ) * dN + llGamma + dM ) / dM;
						x0 = (unsigned long)xLeft;
						DEBUGMSG(GPE_ZONE_LINE,(TEXT("new y0 = %d\r\n"),y0));
						if( (long)y0 >= yBottom )
							continue;
					}
#if DEBUGTEMP
					x0_v1 = (int)x0;
					y0_v1 = (int)y0;
#endif


					if( (long)y0 < yTop )
					{
						x0 = 1 + x0 + (unsigned long)( (yTop-(long)y0-1) * dM - llGamma - 1 ) / dN;
						y0 = (unsigned long)yTop;
						DEBUGMSG(GPE_ZONE_LINE,(TEXT("new x0 = %d\r\n"),x0));
						if( (long)x0 >= xRight )
							continue;
					}
#if DEBUGTEMP
					x0_v2 = (int)x0;
					y0_v2 = (int)y0;
#endif


					y1 = y0 + (unsigned long)(((long)( (x1-x0) * dN ) + llGamma + dM ) / dM);

				
#if DEBUGTEMP
					orig_y1 = (int) y1;
#endif

					if( (long)y1 < yTop )
						continue;

					DEBUGMSG(GPE_ZONE_LINE,(TEXT("y1 = %d\r\n"),y1));

					if( (long)y1 >= yBottom )
					{
						y1 = (unsigned long)(yBottom-1);
						x1 = (unsigned long)( x0 + ( (long)( y1 - y0 ) * dM - llGamma - 1 ) / dN );
						if( (long)x1 < xLeft )
							continue;
					}
#if DEBUGTEMP
					x1_v3 = (int)x1;
					y1_v3 = (int)y1;
					dM_v = (int)dM;
					dN_v = (int)dN;
					llGamma_v = (int)llGamma;
#endif
				}
				

				
				// Unflip the endpoint coordinates

				xStart = x + x0;
				yStart = y + y0;
				if( fl & FL_FLIP_D )
				{
					SWAP( xStart,yStart,LONG )
				}
				if( fl & FL_FLIP_V )
					yStart = -yStart;
				if( fl & FL_FLIP_H )
					xStart = -xStart;

				iDir = ( ( fl & FL_FLIP_V ) ? 7 : 0 ) ^ ( ( fl & FL_FLIP_D ) ? 1 : 0 )  ^ ( ( fl & FL_FLIP_H ) ? 3 : 0 );
				cPels = x1 - x0 + 1;
				if( cPels <= 0 )	// this can happen if line just touches cliprect
					continue;

				DEBUGMSG(GPE_ZONE_LINE,(TEXT("X,Y=%d,%d dM,dN=%d,%d llGamma=%d errorTerm = %d cPels=%d\r\n"),
					xStart,yStart,dM,dN,llGamma,errorTerm,cPels));

				parms.xStart = xStart;
				parms.yStart = yStart;
				parms.cPels = cPels;
				parms.dM = dM;
				parms.dN = dN;
				parms.llGamma = errorTerm;
				parms.iDir = iDir;
				parms.prclClip = prclCurr;

				if( FAILED((pGPE->*(parms.pLine))(&parms)))
				{
					DEBUGMSG(GPE_ZONE_ENTER,(TEXT("Leaving DrvStrokePath\r\n")));
					return FALSE;
				}

				parms.styleState = ( parms.styleState + cStylePels ) & 31;

			}
		} while ( morePointLists );

		

	}

	pGPE->Line( &parms, gpeComplete );
	return TRUE;
}
