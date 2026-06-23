/*

Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.

*/
#include "precomp.h"

unsigned long ProcessROP3(
	unsigned long dstValue,
	unsigned long srcValue,
	unsigned long brushValue,
	unsigned char rop3,
	unsigned char dstBitsPerPixel);



class PixelIterator
{
public:
	// Fixed upon creation:
	unsigned char		*RowPtr;
	int					RowIncrement;
	unsigned long		CacheStateNewDWord;
	unsigned long		CacheStateNewRow;
	unsigned long		CacheStateIncrement;
	unsigned long		CacheStateIncrementDirty;
	int					Is24Bit;
	unsigned long		Mask;
	int					BytesPerAccess;
	int					Bpp;
	unsigned char		MaskShiftXor;
	// IteratorState
	unsigned char		*Ptr;
	unsigned long		Cache;
	unsigned long		Value;
	unsigned long		CacheState;
	void				InitPixelIterator(
							GPESurf *pSurf,
							int xPositive,
							int yPositive,
							RECTL *prcl,
							int xSkip,
							int ySkip );
};

class BrushPixelIterator : public PixelIterator
{
public:
	int					PixelsRemaining;			// Pixels still available this row of brush incl cache
	int					PixelsPerRow;				// Width of source surface
	int					RowInitialPixelsRemaining;	// PixelsRemaining at start of each Dst row
	int					RowsRemaining;				// Rows (incl this) left in pattern
	int					Rows;						// Total rows in pattern
	unsigned char		*FirstRowPtr;				// Pointer to first DWord to use in top/bottom row
	unsigned char		*LeftRowPtr;				// Ptr to *left* of pattern of current row
													//		(RowPtr is *start* of next row)
	void				InitBrushPixelIterator(
							GPESurf *pSurf,
							int xPositive,
							int yPositive,
							RECTL *prcl );

};

void BrushPixelIterator::InitBrushPixelIterator(
	GPESurf *pSurf,
	int xPositive,
	int yPositive,
	RECTL *prcl)
{
	DEBUGMSG(GPE_ZONE_BLT_LO,(TEXT("BrushPixelIterator::InitBrushPixelIterator\r\n")));
	InitPixelIterator( pSurf, xPositive, yPositive, prcl, 0, 0 );
	
	PixelsPerRow = pSurf->Width();
	Rows = pSurf->Height();
	RowInitialPixelsRemaining = xPositive?PixelsPerRow-prcl->left:prcl->right;
	RowsRemaining = yPositive?Rows-prcl->top:prcl->bottom;
	FirstRowPtr = RowPtr - RowIncrement * ( Rows - RowsRemaining );
	DEBUGMSG(GPE_ZONE_BLT_LO,(TEXT("PixelsPerRow=%d, Rows=%d, RowInitPixRem=%d, RowsRem=%d, FirstRowPtr=%08x\r\n"),
		PixelsPerRow, Rows, RowInitialPixelsRemaining, RowsRemaining, FirstRowPtr ));
}


void PixelIterator::InitPixelIterator(
	GPESurf *pSurf,
	int xPositive,
	int yPositive,
	RECTL *prcl,
	int xSkip,
	int ySkip )
{
	DEBUGMSG(GPE_ZONE_BLT_LO,(TEXT("PixelIterator::PixelIterator, prcl= l:%d,t:%d - r:%d,b:%d Bpp=%d\r\n"),
		prcl->left, prcl->top, prcl->right, prcl->bottom, EGPEFormatToBpp[pSurf->Format()]
		));

	// Set pointer to start of first row to use
	RowPtr = (unsigned char *)(pSurf->Buffer());

	if( yPositive )
	{
		RowIncrement = pSurf->Stride();
		RowPtr += pSurf->Stride() * (prcl->top + ySkip );
	}
	else
	{
		RowIncrement = -pSurf->Stride();
		RowPtr += pSurf->Stride() *  (prcl->bottom-1-ySkip);
	}

	int StartX;
	Bpp = EGPEFormatToBpp[pSurf->Format()];

	MaskShiftXor = (Bpp<8)?(8-Bpp):0;

	CacheStateNewDWord = (32/Bpp)<<8;		// Initally 32/Bpp pixels in dword, 0 offset
	if( xPositive )
	{
		StartX = prcl->left + xSkip;
		CacheStateIncrement = ((Bpp<<8) - 1)<<8;
	}
	else
	{
		StartX = prcl->right-1-xSkip;
		CacheStateIncrement = (((-Bpp)<<8) - 1)<<8;
		CacheStateNewDWord |= (32 - Bpp)<<16;		// Initial offset points to last pixel in dword
	}
	CacheStateIncrementDirty = CacheStateIncrement+1;
	
	if( Is24Bit = ( pSurf->Format() == gpe24Bpp ) ) // deliberate assignment
	{
		RowPtr += 3 * StartX;
		BytesPerAccess = 3;
	}
	else
	{
		int StartBit = Bpp * StartX;
		RowPtr += (StartBit&~31) >> 3;
		// Since the first pixel on row in prcl may not be on dword alignment:
		CacheStateNewRow = CacheStateNewDWord;
		while( (( CacheStateNewRow >> 16 ) ^ StartBit ) & 31 )
			CacheStateNewRow += CacheStateIncrement;
		BytesPerAccess = 4;
	}
	if( !xPositive )
		BytesPerAccess = -BytesPerAccess;
	Mask = ( 2 << (Bpp - 1) ) - 1;

	DEBUGMSG(GPE_ZONE_BLT_LO,(TEXT("BytesPerAccess=%d,CacheStateNewRow=%06x,CacheStateIncrement=%06x\r\n"),
		BytesPerAccess, CacheStateNewRow, CacheStateIncrement ));
	DEBUGMSG(GPE_ZONE_BLT_LO,(TEXT("Mask=%08x, CacheStateNewDWord=%08x, StartX=0x%04x(%d)\r\n"),
		Mask, CacheStateNewDWord, StartX, StartX ));
	DEBUGMSG(GPE_ZONE_BLT_LO,(TEXT("Buffer=0x%08x\r\n"),
		(pSurf->Buffer()) ));

}
#undef SWAP
#define SWAP(a,b,type) { type tmp=a; a=b; b=tmp; }


SCODE GPE::EmulatedBlt( GPEBltParms *pParms )
{
	DEBUGMSG(GPE_ZONE_BLT_LO,(TEXT("New Blt. rop4=%04X\r\n"), pParms->rop4));


	unsigned char rop3 = (unsigned char)pParms->rop4;
	unsigned char maskedROP3 = (unsigned char)( pParms->rop4 >> 8 );
	unsigned char unmaskedROP3 = rop3;
	int complexBlt = 0;
	int quickWrite = 0;
	int width = pParms->prclDst->right - pParms->prclDst->left;
	int height = pParms->prclDst->bottom - pParms->prclDst->top;
	int x;
	RECTL rclBrush;
	int dstMatters = ((( pParms->rop4 >> 1 ) ^ pParms->rop4 ) & 0x5555 ) != 0;
	int dstXPositive = pParms->xPositive;	// These differ from pParms->?Positive if flipping is being performed
	int dstYPositive = pParms->yPositive;
	RECTL *prclDst = pParms->prclDst;
	RECTL tmpRclDst;
	int xShrinkStretch = 0;
	int xShrink=0;
	int xStretch=0;
	int yShrinkStretch = 0;
	int yShrink=0;
	int yStretch=0;
	int srcWidth;
	int srcHeight;
	int rowXAccum, xAccum, xDMajor, xDMinor;	// only valid if xShrinkStretch
	int yAccum, yDMajor, yDMinor;	// only valid if yShrinkStretch
	int originalMaskRowInc, originalSrcRowInc;	// Used if yShrinkStretch
	unsigned char *prevMaskPtr, *prevSrcPtr;
	unsigned long prevMaskCache, prevSrcCache;
	unsigned long prevMaskCacheState, prevSrcCacheState;
	unsigned long originalSrc;
	int dstXStartSkip = 0;	// Number of dst pixels to not write (left if dstXPositive else right)
	int dstYStartSkip = 0;	// Number of rows to ignore (top if dstYPositive)
	int srcXStartSkip = 0;
	int srcYStartSkip = 0;

	if(	pParms->pDst->InVideoMemory() ||
		( pParms->pSrc && pParms->pSrc->InVideoMemory() ) ||
		( pParms->pMask && pParms->pMask->InVideoMemory() ) )
	{
		// If we have a pending blt and now attempt a software operation using
		// video memory, the pipeline must be flushed.
		WaitForNotBusy();
	}

//	DebugBreak();

	// Handle improperly ordered prclDst resulting from stretch Blt
	// Note that only prclDst is ever improperly ordered
	if( width < 0 || height < 0 )
	{
		tmpRclDst = *prclDst;
		prclDst = &tmpRclDst;
		if( width < 0 )
		{
			SWAP( tmpRclDst.left, tmpRclDst.right, LONG )
			width = -width;
			dstXPositive = !dstXPositive;
		}
		if( height < 0 )
		{
			SWAP( tmpRclDst.top, tmpRclDst.bottom, LONG )
			height = -height;
			dstYPositive = !dstYPositive;
		}
	}
	// prclDst is now properly ordered


	if( pParms->bltFlags & BLT_STRETCH )	// Check for stretching or shrinking vertically &/or horizontally
	{
		srcWidth = pParms->prclSrc->right - pParms->prclSrc->left;
		srcHeight = pParms->prclSrc->bottom - pParms->prclSrc->top;

		if( width > srcWidth )
		{
			xStretch = 1;
			xDMajor = width;
			xDMinor = srcWidth;
		}
		if( width < srcWidth )
		{
			xShrink = 1;
			xDMajor = srcWidth;
			xDMinor = width;
		}
		if( xStretch || xShrink)
		{
			xShrinkStretch = 1;
			// Convert to Bresenham parameters
			xDMinor *= 2;
			xDMajor = xDMinor - 2 * xDMajor;
			rowXAccum = xShrink?(2*width - srcWidth):(3*srcWidth - 2*width);
				// loaded into xAccum at start of each row
		}
		if( height > srcHeight )
		{
			yStretch = 1;
			yDMajor = height;
			yDMinor = srcHeight;
		}
		if( height < srcHeight )
		{
			yShrink = 1;
			yDMajor = srcHeight;
			yDMinor = height;
		}
		if( yStretch || yShrink)
		{
			yShrinkStretch = 1;
			// Convert to Bresenham parameters
			yDMinor *= 2;
			yDMajor = yDMinor - 2 * yDMajor;
			yAccum = yShrink?(2*height - srcHeight):(3*srcHeight - 2*height);
		}

		if( pParms->prclClip )	// ONLY happens if stretch blting
		{
			RECTL rclClipped = *prclDst;
			if( rclClipped.left < pParms->prclClip->left )
				rclClipped.left = pParms->prclClip->left;
			if( rclClipped.top < pParms->prclClip->top )
				rclClipped.top = pParms->prclClip->top;
			if( rclClipped.bottom > pParms->prclClip->bottom )
				rclClipped.bottom = pParms->prclClip->bottom;
			if( rclClipped.right > pParms->prclClip->right )
				rclClipped.right = pParms->prclClip->right;
			if( rclClipped.right <= rclClipped.left || rclClipped.bottom <= rclClipped.top )
				return S_OK;	// the clipping left nothing to do
			dstXStartSkip = dstXPositive?(rclClipped.left-prclDst->left):(prclDst->right-rclClipped.right);
			dstYStartSkip = dstYPositive?(rclClipped.top-prclDst->top):(prclDst->bottom-rclClipped.bottom);
			width = rclClipped.right - rclClipped.left;		// Calculate fully clipped destination width
			height = rclClipped.bottom - rclClipped.top;	// Fully clipped height
		}

		if( xShrink )
		{
			while( rowXAccum < 0 )
			{
				rowXAccum += xDMinor;
				srcXStartSkip++;
			}
			rowXAccum += xDMajor; 
		}
		int skipCount;
		for( skipCount = dstXStartSkip; skipCount; skipCount-- )
		{
			if( xShrink )
			{
				while( rowXAccum < 0 )
				{
					rowXAccum += xDMinor;
					srcXStartSkip++;
				}
				srcXStartSkip++;	//  <--- this is the srcSkip inherent with a dstSkip
				rowXAccum += xDMajor; 
			}
			else if( xStretch )
			{
				if( rowXAccum < 0 )
					rowXAccum += xDMinor;
				else
				{
					rowXAccum += xDMajor;
					srcXStartSkip++;
				}
			}
            else
                srcXStartSkip++;
		}
		if( yShrink )
		{
			yAccum += dstYStartSkip * yDMajor;
			srcYStartSkip += dstYStartSkip;	//  <--- this is the srcSkip inherent with a dstSkip
		}
		else if( yStretch )
		{
			for( skipCount = dstYStartSkip; skipCount; skipCount-- )
			{
				if( yAccum < 0 )
					yAccum += yDMinor;
				else
				{
					yAccum += yDMajor;
					srcYStartSkip++;
				}
			}
		}
        else
            srcYStartSkip += dstYStartSkip;
 	}

	int transparentBlt = pParms->bltFlags & BLT_TRANSPARENT;
	unsigned long transparentColor = pParms->solidColor;

	DEBUGMSG(GPE_ZONE_BLT_LO,(TEXT("pSrc=0x%08x, pDst=0x%08x, pBrush=%08x, pMask=%08x\r\n"),
		pParms->pSrc, pParms->pDst, pParms->pBrush, pParms->pMask ));

	if( pParms->pBrush )
	{
		// Calculate an rclBrush so that the correct starting pixel is chosen for the brush iterator
		DEBUGMSG(GPE_ZONE_BLT_LO,(TEXT("Pattern depth: %d Bpp\r\n"), EGPEFormatToBpp[pParms->pBrush->Format()] ) );
		int topIndent = (pParms->pptlBrush)?pParms->pBrush->Height()-pParms->pptlBrush->y:0;
		int leftIndent = (pParms->pptlBrush)?pParms->pBrush->Width()-pParms->pptlBrush->x:0;
		rclBrush.left = ( leftIndent + prclDst->left ) % pParms->pBrush->Width();
		rclBrush.right = ( rclBrush.left + pParms->pBrush->Width() - 1 ) % pParms->pBrush->Width();
		rclBrush.top = ( topIndent + prclDst->top ) % pParms->pBrush->Height();
		rclBrush.bottom = ( rclBrush.top + pParms->pBrush->Height() - 1 ) % pParms->pBrush->Height();
	}

	PixelIterator src, dst, mask;
	BrushPixelIterator brush;
	src.Ptr = src.RowPtr = mask.Ptr = mask.RowPtr = brush.Ptr = brush.RowPtr = (unsigned char *)0;

	if( pParms->pSrc )
	{
		DEBUGMSG(GPE_ZONE_BLT_LO,(TEXT("pSrc depth: %d Bpp\r\n"), EGPEFormatToBpp[pParms->pSrc->Format()] ) );
		src.InitPixelIterator( pParms->pSrc, pParms->xPositive, pParms->yPositive, pParms->prclSrc,
			srcXStartSkip, srcYStartSkip );
	}
	DEBUGMSG(GPE_ZONE_BLT_LO,(TEXT("pDst depth: %d Bpp\r\n"), EGPEFormatToBpp[pParms->pDst->Format()] ) );

	if( pParms->pBrush )
	{
		DEBUGMSG(GPE_ZONE_BLT_LO,(TEXT("pBrush depth: %d Bpp\r\n"), EGPEFormatToBpp[pParms->pBrush->Format()] ) );
		brush.InitBrushPixelIterator( pParms->pBrush, pParms->xPositive, pParms->yPositive, &rclBrush );
	}

	if( ( maskedROP3 != unmaskedROP3 ) && !(pParms->pMask) )
	{
		DEBUGMSG(GPE_ZONE_ERROR,(TEXT("Rop4 = %04x but pMask is NULL\r\n"), pParms->rop4 ));
		return E_INVALIDARG;
	}

	if( pParms->pMask )
		mask.InitPixelIterator( pParms->pMask, pParms->xPositive, pParms->yPositive, pParms->prclMask,
			srcXStartSkip, srcYStartSkip );
    else
        mask.RowIncrement = 0;

	if( ( maskedROP3 != unmaskedROP3 ) && !(mask.RowPtr) )
	{
		DEBUGMSG(GPE_ZONE_ERROR,(TEXT("Rop4 = %04x but mask.RowPtr is NULL\r\n"), pParms->rop4 ));
		return E_INVALIDARG;
	}

	if( ( dst.Bpp = EGPEFormatToBpp[pParms->pDst->Format()] ) >= 8 )
	{
		quickWrite = 1;
		if( !dstYPositive )
		{
			dst.RowIncrement = -pParms->pDst->Stride();
			dst.RowPtr = (unsigned char *)pParms->pDst->Buffer() + pParms->pDst->Stride() * ( prclDst->bottom - 1 - dstYStartSkip );
		}
		else
		{
			dst.RowIncrement = pParms->pDst->Stride();
			dst.RowPtr = (unsigned char *)pParms->pDst->Buffer() + pParms->pDst->Stride() * ( dstYStartSkip + prclDst->top );
		}
		if( !dstXPositive )
		{
			dst.BytesPerAccess = -dst.Bpp/8;
			dst.RowPtr -= dst.BytesPerAccess * ( prclDst->right - 1 - dstXStartSkip );
		}
		else
		{
			dst.BytesPerAccess = dst.Bpp/8;
			dst.RowPtr += dst.BytesPerAccess * ( dstXStartSkip + prclDst->left );
		}
		dst.Mask = ( 2 << (dst.Bpp - 1) ) - 1;
		dst.CacheState=0;	// so it is not marked as dirty
	}
	else
		dst.InitPixelIterator( pParms->pDst, dstXPositive, dstYPositive, prclDst,
			dstXStartSkip, dstYStartSkip );


	if( yShrinkStretch )
	{
		// Back up the source and mask row increments because they will be altered dynamically
		originalMaskRowInc = mask.RowIncrement;
		originalSrcRowInc = src.RowIncrement;
		DEBUGMSG(GPE_ZONE_BLT_LO,(TEXT("yStretch:%d yShrink:%d yDMinor:%d yDMajor:%d yAccum:%d\n"),
			yStretch, yShrink, yDMinor, yDMajor, yAccum ));
	}

	if( pParms->rop4 == 0x0000 )
	{
		src.Value = 0;
	}
	else if( pParms->rop4 == 0xffff )
	{
		src.Value = dst.Mask;
	}
	else
	{
		src.Value = pParms->solidColor & dst.Mask;
		if(	   ( maskedROP3 != unmaskedROP3 )
		    || transparentBlt
			|| xShrinkStretch
		    || ( rop3 != 0xCC && ( rop3 != 0xF0 || pParms->pBrush ) ) )
		{
			DEBUGMSG(GPE_ZONE_BLT_LO,(TEXT("Complex Blt!\r\n")));
			DEBUGMSG(GPE_ZONE_BLT_LO,(TEXT("transparent:%d,masked:%2x,unmasked:%2x,rop3:%2x,mask.RowPtr:%08x\r\n"),
				transparentBlt,maskedROP3, unmaskedROP3, rop3, mask.RowPtr ));
			complexBlt = 1;
		}
	}
	brush.Value = src.Value;

	while( height-- )
	{
		// Handle y Shrinking or stretching
		if( yShrinkStretch )
		{
			if( yShrink )
			{
				while( yAccum < 0 )	// skip source line(s) if shrinking
				{
					DEBUGMSG(GPE_ZONE_BLT_LO,(TEXT("yShrink - skip line\r\n")));
					src.RowPtr += src.RowIncrement;
					mask.RowPtr += mask.RowIncrement;
					yAccum += yDMinor;
				}
				yAccum += yDMajor;
			}
			else
			{
				if( yAccum < 0 )
				{
					// repeat source line if stretching
					yAccum += yDMinor;		// yDMinor is a small +ve number
					if( yStretch )
					{
						DEBUGMSG(GPE_ZONE_BLT_LO,(TEXT("yStretch - Repeat. Accum=%d\r\n"), yAccum));
						src.RowIncrement = 0;
						mask.RowIncrement = 0;
					}
				}
				else
				{
					DEBUGMSG(GPE_ZONE_BLT_LO,(TEXT("yStretch - Switch to next after this Accum=%d\r\n"), yAccum));
					// Switch to next source line  (after this row)
					yAccum += yDMajor;		// yDMajor is a large -ve value (i.e. |yDMajor| > |yDMinor| )
					src.RowIncrement = originalSrcRowInc;
					mask.RowIncrement = originalMaskRowInc;
				}
			}
		}


		// Move all iterators to next line
		if( src.RowPtr )
		{
			src.Ptr = src.RowPtr;
			src.RowPtr += src.RowIncrement;
			if( ! src.Is24Bit )
			{
				src.Cache = *(unsigned long *)src.Ptr;
				src.CacheState = src.CacheStateNewRow;
			}
		}
		if( mask.RowPtr )
		{
//			DEBUGMSG(GPE_ZONE_BLT_LO,(TEXT("Incrementing mask.RowPtr\r\n")));
			mask.Ptr = mask.RowPtr;
			mask.RowPtr += mask.RowIncrement;
			mask.Cache = *(UNALIGNED unsigned long *)mask.Ptr;
			mask.CacheState = mask.CacheStateNewRow;
		}
		if( brush.RowPtr )
		{
//			DEBUGMSG(GPE_ZONE_BLT_LO,(TEXT("Incrementing brush.RowPtr\r\n")));
			brush.Ptr = brush.RowPtr;
			brush.LeftRowPtr = brush.Ptr - (brush.FirstRowPtr - (unsigned char *)(pParms->pBrush->Buffer()));
			if( --brush.RowsRemaining )
				brush.RowPtr += brush.RowIncrement;
			else
			{
				brush.RowPtr = brush.FirstRowPtr;
				brush.RowsRemaining = brush.Rows;
			}
			brush.Cache = *(unsigned long *)brush.Ptr;
			brush.CacheState = brush.CacheStateNewRow;
			brush.PixelsRemaining = brush.RowInitialPixelsRemaining;
//			DEBUGMSG(GPE_ZONE_BLT_LO,(TEXT("Brush Ptr = 0x%08x, next row:%08x\r\n"),
//			    brush.Ptr, brush.RowPtr));
		}
//		DEBUGMSG(GPE_ZONE_BLT_LO,(TEXT("Incrementing dst.RowPtr\r\n")));
		dst.Ptr = dst.RowPtr;
		dst.RowPtr += dst.RowIncrement;

		if( !quickWrite )
		{
			dst.Cache = *(unsigned long *)dst.Ptr;
			dst.CacheState = dst.CacheStateNewRow;
		}

		if( xStretch )
		{
			prevMaskPtr = mask.Ptr;
			prevMaskCache = mask.Cache;
			prevMaskCacheState = mask.CacheState;
			prevSrcPtr = src.Ptr;
			prevSrcCache = src.Cache;
			prevSrcCacheState = src.CacheState;
		}

		xAccum = rowXAccum;

		for( x=0; x<width; x++ )
		{
//			DEBUGMSG(GPE_ZONE_BLT_LO,(TEXT("x=%d\r\n"), x));

			if( src.Ptr )
			{


//				DEBUGMSG(GPE_ZONE_BLT_LO,(TEXT("Reading source pixel\r\n")));

				// Read the next pixel in and convert to same depth as destination

				if( src.Is24Bit )	// Since 24-bit packed pixels cross dword boundaries:
				{
					src.Value = ( *src.Ptr ) + ( *(src.Ptr+1) << 8 ) +	( *(src.Ptr+2) << 16 );
					src.Ptr += src.BytesPerAccess;
				}
				else
				{
					if( !(src.CacheState&0x00ff00) )	// Check bits remaining in src.Cache
					{
						src.Ptr += src.BytesPerAccess;			// increment src.Ptr
						src.CacheState = src.CacheStateNewDWord;
						src.Cache = *(unsigned long *)src.Ptr;	// reload src.Cache
					}
					src.Value = src.Cache >> ( (src.CacheState >> 16) ^ src.MaskShiftXor );
					src.CacheState += src.CacheStateIncrement;		 
				}
				originalSrc = ( src.Value &= src.Mask );
				if( pParms->pLookup )
					src.Value = (pParms->pLookup)[src.Value];
				if( pParms->pConvert )
				{
					src.Value = (pParms->pColorConverter->*(pParms->pConvert))( src.Value );
				}	
					// pParms->pConvert( pParms->pColorConverter, src.Value );
			}
			
			if( complexBlt )	// brushed, masked, transparent, unusual rop3 etc
			{
				if( brush.Ptr )
				{
//					DEBUGMSG(GPE_ZONE_BLT_LO,(TEXT("Reading brush pixel\r\n")));

					// Note that the brush will already have been converted to the same
					// depth as the destination (if they weren't the same).
					// The exception is if the dest is 24Bpp. In this case, the brush
					// will have been converted to 32Bpp for simplicity.
					// See DrvRealizeBrush for details

					if( !(brush.PixelsRemaining--) )	// Check for wrapping off right edge of pattern
					{
						brush.Ptr = brush.LeftRowPtr;
						brush.PixelsRemaining = brush.PixelsPerRow-1;
						brush.CacheState = brush.CacheStateNewDWord;
						brush.Cache = *(unsigned long *)brush.Ptr;	// reload brush.Cache
					}
					else if( !(brush.CacheState&0x00ff00) )	// Check bits remaining in brush.Cache
					{
						brush.Ptr += brush.BytesPerAccess;				// increment brush.Ptr
						brush.CacheState = brush.CacheStateNewDWord;
						brush.Cache = *(unsigned long *)brush.Ptr;	// reload brush.Cache
					}
					brush.Value = brush.Cache >> ( (brush.CacheState >> 16) ^ brush.MaskShiftXor );
					brush.CacheState += brush.CacheStateIncrement;		 
				}
				if( mask.Ptr ) // This selects between two rop3's
				{
//					DEBUGMSG(GPE_ZONE_BLT_LO,(TEXT("Reading mask pixel\r\n")));
					if( !(mask.CacheState&0x00ff00) )	// check bits remaining in mask cache
					{
						mask.Ptr += mask.BytesPerAccess;	//  == +/- 4
						mask.CacheState = mask.CacheStateNewDWord;
						mask.Cache = *(UNALIGNED unsigned long *)mask.Ptr;
					}
					rop3 = ( mask.Cache & ( 1 << ( (mask.CacheState >> 16) ^ mask.MaskShiftXor ) ) ) ? unmaskedROP3 : maskedROP3;
					mask.CacheState += mask.CacheStateIncrement;
				}

				if( xShrinkStretch )
				{
					// Handle x Stretching
					if( xStretch )
					{
						if( xAccum < 0 )	// Repeat this pixel
						{
							xAccum += xDMinor;
							mask.Ptr = prevMaskPtr;
							mask.Cache = prevMaskCache;
							mask.CacheState = prevMaskCacheState;
							src.Ptr = prevSrcPtr;
							src.Cache = prevSrcCache;
							src.CacheState = prevSrcCacheState;
						}
						else				// OK to move to next source pixel after this
						{
							xAccum += xDMajor;
							prevMaskPtr = mask.Ptr;
							prevMaskCache = mask.Cache;
							prevMaskCacheState = mask.CacheState;
							prevSrcPtr = src.Ptr;
							prevSrcCache = src.Cache;
							prevSrcCacheState = src.CacheState;
						}
					}
					else	// xShrink
					{
						while( xAccum < 0 )		// Skip pixel(s)
						{
							// Move to next src pixel
							if( src.Is24Bit )	// Since 24-bit packed pixels cross dword boundaries:
								src.Ptr += src.BytesPerAccess;
							else
							{
								if( !(src.CacheState&0x00ff00) )	// Check bits remaining in src.Cache
								{
									src.Ptr += src.BytesPerAccess;			// increment src.Ptr
									src.CacheState = src.CacheStateNewDWord;
                                    if (x != width - 1 )
									    src.Cache = *(unsigned long *)src.Ptr;	// reload src.Cache
								}
								src.CacheState += src.CacheStateIncrement;		 
							}
							// Move to next mask pixel
							if( pParms->pMask )
							{
								if( !(mask.CacheState&0x00ff00) )	// check bits remaining in mask cache
								{
									mask.Ptr += mask.BytesPerAccess;		//  == +/- 4
									mask.CacheState = mask.CacheStateNewDWord;
								    mask.Cache = *(UNALIGNED unsigned long *)mask.Ptr;
								}
								mask.CacheState += mask.CacheStateIncrement;
							}
							xAccum += xDMinor;
						}
						xAccum += xDMajor;
					}
				}
				if( dstMatters )		// The underlying rop4 utilizes the dst.
				{
//					DEBUGMSG(GPE_ZONE_BLT_LO,(TEXT("Reading dst pixel\r\n")));
					// We maintain dst.Cache valid at all times if not doing quickWrite
					if( quickWrite )
					{
						switch(dst.Bpp)
						{
						case 8:
								*(unsigned char *)&dst.Value = *(unsigned char *)dst.Ptr;
								break;
						case 16:
								*(unsigned short *)&dst.Value = *(unsigned short *)dst.Ptr;
								break;
						case 32:
								*(unsigned long *)&dst.Value = *(unsigned long *)dst.Ptr;
								break;
						case 24:
								dst.Value = ( *dst.Ptr ) + ( *(dst.Ptr+1) << 8 ) +	( *(dst.Ptr+2) << 16 );
						}
					}
					else
						dst.Value = dst.Cache >> ( ( dst.CacheState >> 16 ) ^ dst.MaskShiftXor );
				}

				switch( rop3 )
				{
					// The compiler sorts these and creates a binary search set of tests to
					// get to an entry quickly.  Branch "prediction" helps this a lot.
					// Remember that masked rop4s makes NOP and SRCCOPY quite a likely rop3
					case 0xAA: break;					  					   // NOP
					case 0xCC: break; 										   // SRCCOPY
					case 0x00: src.Value = 0;							break; // BLACKNESS
					case 0x22: src.Value = (~src.Value) & dst.Value;	break; // no-name
					case 0xB8: src.Value = (brush.Value & ~src.Value)|(src.Value & dst.Value);	break; // no-name
					case 0x11: src.Value = ~(src.Value | dst.Value);	break; // NOTSRCERASE
					case 0x33: src.Value = ~src.Value;					break; // NOTSRCCOPY
					case 0x44: src.Value &= ~dst.Value;					break; // SRCERASE
					case 0x55: src.Value = ~dst.Value;					break; // DSTINVERT
					case 0x5A: src.Value = brush.Value ^ dst.Value;		break; // PATINVERT
					case 0x66: src.Value ^= dst.Value;					break; // SRCINVERT
					case 0x88: src.Value &= dst.Value;					break; // SRCAND
					case 0xBB: src.Value = ~src.Value | dst.Value;		break; // MERGEPAINT
					case 0xC0: src.Value &= brush.Value;				break; // MERGECOPY
					case 0xEE: src.Value |= dst.Value;					break; // SRCPAINT
					case 0xF0: src.Value = brush.Value;					break; // PATCOPY
					case 0xFB: src.Value = brush.Value | ~src.Value | dst.Value; break; // PATPAINT
					case 0xFF: src.Value = 0xFFFFFFFF;					break; // WHITENESS
               case 0xE2: src.Value = ( dst.Value & ~src.Value ) | ( brush.Value & src.Value ); break;
               case 0xAC: src.Value = ((src.Value^dst.Value)&brush.Value)^src.Value; break;
					default:   src.Value = ProcessROP3(dst.Value,src.Value,brush.Value,rop3,dst.Bpp);
				}

				src.Value &= dst.Mask;

				if( rop3 == 0xAA || ( transparentBlt && ( originalSrc == transparentColor ) ) )
				{
//					DEBUGMSG(GPE_ZONE_BLT_LO,(TEXT("NOT writing pixel\r\n")));
					// we won't write this pixel
					if( quickWrite )
						dst.Ptr += dst.BytesPerAccess;
					else
					{
						dst.CacheState += dst.CacheStateIncrement; // leave dirty flag as-is
						if( !(dst.CacheState&0x00ff00) ) // Check bits remaining in dst cache
						{
							if( dst.CacheState & 0x0000ff )
								*(unsigned long *)dst.Ptr = dst.Cache;	// flush cache
							dst.CacheState = dst.CacheStateNewDWord;		// clears dirty flag
							dst.Ptr += dst.BytesPerAccess;					// +/- 4
							dst.Cache = *(unsigned long *)dst.Ptr;
						}
					}
					continue;
				}
			}

//			DEBUGMSG(GPE_ZONE_BLT_LO,(TEXT("Writing pixel\r\n")));
			// Now, we are ready to write the value from src.Value into the dst pixel
			if( quickWrite )
			{
				switch(dst.Bpp)
				{
				case 8:
						*(unsigned char *)dst.Ptr = (unsigned char)src.Value;
						break;
				case 16:
						*(unsigned short *)dst.Ptr = (unsigned short)src.Value;
						break;
				case 32:
						*(unsigned long *)dst.Ptr = (unsigned long)src.Value;
						break;
				case 24:
						*dst.Ptr = (unsigned char)(src.Value);
						*(dst.Ptr+1) = (unsigned char)(src.Value>>8);
						*(dst.Ptr+2) = (unsigned char)(src.Value>>16);
				}
				dst.Ptr += dst.BytesPerAccess;
			}
			else
			{
//				DEBUGMSG(GPE_ZONE_BLT_LO,(TEXT("Old Cache: 0x%08x, Cache State: 0x%08x, Ptr: 0x%08x\r\n"),
//					dst.Cache, dst.CacheState, dst.Ptr));
				unsigned char tmpShift = (unsigned char)(( dst.CacheState >> 16 ) ^ dst.MaskShiftXor);
				dst.Cache &= ~( dst.Mask << tmpShift);
				dst.Cache |= src.Value << tmpShift;
				dst.CacheState += dst.CacheStateIncrementDirty;  // sets dirty flag
				if( !(dst.CacheState&0x00ff00) ) // Check bits remaining in dst cache
				{
//					DEBUGMSG(GPE_ZONE_BLT_LO,(TEXT("Writing 0x%08x to Ptr: 0x%08x\r\n"),
//						dst.Cache, dst.Ptr ));
					
					*(unsigned long *)dst.Ptr = dst.Cache;	// flush cache (we know it was dirty)
					dst.CacheState = dst.CacheStateNewDWord;	// clears dirty flag
					dst.Ptr += dst.BytesPerAccess;				// +/- 4
                    if (x != width - 1 )
    					dst.Cache = *(unsigned long *)dst.Ptr;
				}
//				DEBUGMSG(GPE_ZONE_BLT_LO,(TEXT("New Cache: 0x%08x, Cache State: 0x%08x, Ptr: 0x%08x\r\n"),
//					dst.Cache, dst.CacheState, dst.Ptr));
			}

		} // next column
		if( dst.CacheState & 0x0000ff )
			*(unsigned long *)dst.Ptr = dst.Cache;	// flush cache
	} // next row

	return S_OK;
}

unsigned long ProcessROP3(
	unsigned long dstValue,
	unsigned long srcValue,
	unsigned long brushValue,
	unsigned char rop3,
	unsigned char dstBitsPerPixel)
{
	if( dstBitsPerPixel > 24 )
	{
		// Break into two halves, otherwise the brushValue<<=2 below will overflow
		return ProcessROP3(dstValue,srcValue,brushValue,rop3,16) |
			( ProcessROP3(dstValue>>16,srcValue>>16,brushValue>>16,rop3,dstBitsPerPixel-16) << 16);
	}

	DEBUGMSG(GPE_ZONE_BLT_LO,(TEXT("Process rop3=%02x, numbits=%d, pattern=%04x, source=%04x, dest=%04x, "),
		rop3,dstBitsPerPixel,brushValue,srcValue,dstValue ));


	unsigned long result = 0;
	unsigned long rsltBit = 1;

	brushValue <<= 2;
	srcValue <<= 1;

	while( dstBitsPerPixel-- )
	{
		if( ( 1<< ( brushValue&4 | srcValue&2 | dstValue&1 ) ) & rop3 )
			result |= rsltBit;
		brushValue >>= 1;
		srcValue >>= 1;
		dstValue >>= 1;
		rsltBit <<=1;
	}
	DEBUGMSG(GPE_ZONE_BLT_LO,(TEXT("resuult=%04x\r\n"), result));

	return result;
}
