/*

Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.

*/
#include "precomp.h"

SCODE GPE::EmulatedLine( GPELineParms *pParms )
{
	int Bpp = EGPEFormatToBpp[pParms->pDst->Format()];
	unsigned long baseMask = (2 << (Bpp-1) )-1;
	int remainingPels;
	unsigned char *Ptr;
	unsigned char *newPtr;
	int i;
	long accum = (long)(pParms->dN) + pParms->llGamma;
	long axstp = (long)(pParms->dN);
	long dgstp = (long)(pParms->dN) - (long)(pParms->dM);
	unsigned char rop2Mark = (unsigned char)(pParms->mix);
	unsigned char rop2Space = (unsigned char)(pParms->mix >> 8);
	unsigned char rop2;
	unsigned long mask;
	long stride = pParms->pDst->Stride();
	unsigned long style = pParms->style;
	int styleState = pParms->styleState;

	DEBUGMSG(GPE_ZONE_LINE,(TEXT("GPE::EmulatedLine\r\n")));
	DEBUGMSG(GPE_ZONE_LINE,(TEXT("Solid color 0x%08x\r\n"),pParms->solidColor));
	DEBUGMSG(GPE_ZONE_LINE,(TEXT("Start X=%d,Y=%d\r\n"),pParms->xStart,pParms->yStart));
	DEBUGMSG(GPE_ZONE_LINE,(TEXT("iDir:%d accum %d axstp %d dgstp %d\r\n"),
		pParms->iDir,accum,axstp,dgstp));
	DEBUGMSG(GPE_ZONE_LINE,(TEXT("Mix: %04x Bpp: %d\r\n"),pParms->mix,Bpp));



	int MajorDPtr = 0;
	int MinorDPtr = 0;
	int MajorDPixel = 0;
	int MinorDPixel = 0;

	if(	pParms->pDst->InVideoMemory() )
	{
		// If we have a pending blt and now attempt a software operation using
		// video memory, the pipeline must be flushed.
		WaitForNotBusy();
	}



	if( Bpp != 24 )
	{
		unsigned long pen[32];
		unsigned long penMask[32];
		int pixelsPerDWord = 32 / Bpp;
		int subPixel;
		unsigned long cache;
		unsigned long D;
		unsigned long P;

		for( i=0; i<pixelsPerDWord; i++ )
		{
			int shift = i*Bpp;			// Pixel 0 is in least significant byte
			if( Bpp < 8 )			// Pixel 0 is in most significant bits of byte
				shift^=(8-Bpp);
			pen[i] = (pParms->solidColor & baseMask)<<shift;
			penMask[i] = baseMask<<shift;
			DEBUGMSG(GPE_ZONE_LINE,(TEXT("DWord Pixel %d pen: %08x penMask: %08x\r\n"),
				i,pen[i],penMask[i]));
		}

		switch( pParms->iDir )
		{
			case 0: MajorDPixel =  1; MinorDPtr =  stride; break;
			case 1: MinorDPixel =  1; MajorDPtr =  stride; break;
			case 2: MinorDPixel = -1; MajorDPtr =  stride; break;
			case 3: MajorDPixel = -1; MinorDPtr =  stride; break;
			case 4: MajorDPixel = -1; MinorDPtr = -stride; break;
			case 5: MinorDPixel = -1; MajorDPtr = -stride; break;
			case 6: MinorDPixel =  1; MajorDPtr = -stride; break;
			case 7: MajorDPixel =  1; MinorDPtr = -stride; break;
			default:
				DEBUGMSG(GPE_ZONE_ERROR,(TEXT("Invalid direction: %d\r\n"),pParms->iDir));
				return E_INVALIDARG;
		}


		Ptr = ((unsigned char *)pParms->pDst->Buffer())
			  + pParms->yStart * stride
			  + (((pParms->xStart * Bpp) >> 5 ) << 2 );

		subPixel = pParms->xStart % pixelsPerDWord;
		
		cache = *(unsigned long *)Ptr;

		for( remainingPels = pParms->cPels; remainingPels; remainingPels-- )
		{
			DEBUGMSG(GPE_ZONE_LINE,(TEXT("Remain:%d Ptr:%08x SubPix:%d cache:%08x\r\n"),
				remainingPels,Ptr,subPixel,cache ));
			
			D = cache;
			P = pen[subPixel];

			if( ( style >> ((styleState++) & 31 ) ) & 1 )
				rop2 = rop2Space;
			else
				rop2 = rop2Mark;

			switch( rop2 )
			{
				case 1:	 D = 0;				break;
				case 2:  D = ~( P | D );	break;
				case 3:	 D = ~P & D;		break;
				case 4:	 D = ~P;			break;
				case 5:	 D = P & ~D;		break;
				case 6:	 D = ~D;			break;
				case 7:	 D = P ^ D;			break;
				case 8:	 D = ~( P & D );	break;
				case 9:	 D = P & D;			break;
				case 10: D = ~( P ^ D );	break;
				case 11: D = D;				break;
				case 12: D = ~P | D;		break;
				case 13: D = P;				break;
				case 14: D = P | ~D;		break;
				case 15: D = P | D;			break;
				case 16: D = 0xffffffff;	break;
				default:
					DEBUGMSG(GPE_ZONE_ERROR,(TEXT("Invalid rop2: %d\r\n"),rop2));
					return E_INVALIDARG;
			}

			mask = penMask[subPixel];

			cache = ( cache & ~mask ) | ( D & mask );

			DEBUGMSG(GPE_ZONE_LINE,(TEXT("Result: P %08x, D %08x, mask %08x, cache %08x\r\n"),
				P, D, mask, cache ));
			

			if( remainingPels == 1 )
			{
				*(unsigned long *)Ptr = cache;
				break;
			}

			newPtr = Ptr;
			newPtr += MajorDPtr;
			subPixel += MajorDPixel;
			if( axstp )		// check for vertical/horizontal
			{
				if( accum < 0 )
				{
					DEBUGMSG(GPE_ZONE_LINE,(TEXT("Straight accum=%d axstp=%d\r\n"),accum,axstp));

					accum += axstp;
				}
				else
				{
					DEBUGMSG(GPE_ZONE_LINE,(TEXT("Diagonal accum=%d dgstp=%d\r\n"),accum,dgstp));
					newPtr += MinorDPtr;
					subPixel += MinorDPixel;
					accum += dgstp;
				}
			}
			if( subPixel < 0 )
			{
				newPtr-=4;
				subPixel += pixelsPerDWord;
			}
			else if( subPixel >= pixelsPerDWord )
			{
				newPtr+=4;
				subPixel -= pixelsPerDWord;
			}

			if( newPtr != Ptr )
			{
				*(unsigned long *)Ptr = cache;
				Ptr = newPtr;
				cache = *(unsigned long *)Ptr;
			}
		}

	}
	else
	{
		unsigned char P0,P1,P2;	// The pen (i.e. solid color), split into three bytes

		// For 24bpp, we do brute force byte-by-byte accesses for each pixel
		switch( pParms->iDir )
		{
			case 0: MajorDPtr =  3; MinorDPtr =  stride; break;
			case 1: MinorDPtr =  3; MajorDPtr =  stride; break;
			case 2: MinorDPtr = -3; MajorDPtr =  stride; break;
			case 3: MajorDPtr = -3; MinorDPtr =  stride; break;
			case 4: MajorDPtr = -3; MinorDPtr = -stride; break;
			case 5: MinorDPtr = -3; MajorDPtr = -stride; break;
			case 6: MinorDPtr =  3; MajorDPtr = -stride; break;
			case 7: MajorDPtr =  3; MinorDPtr = -stride; break;
			default:
				DEBUGMSG(GPE_ZONE_ERROR,(TEXT("Invalid direction: %d\r\n"),pParms->iDir));
				return E_INVALIDARG;
		}

		Ptr = ((unsigned char *)pParms->pDst->Buffer())
			  + pParms->yStart * stride
			  + pParms->xStart * 3;

		P0 = ((unsigned char *)&(pParms->solidColor))[0];
		P1 = ((unsigned char *)&(pParms->solidColor))[1];
		P2 = ((unsigned char *)&(pParms->solidColor))[2];

		for( remainingPels = pParms->cPels; remainingPels; remainingPels-- )
		{
			
			if( ( style >> ((styleState++) & 31 ) ) & 1 )
				rop2 = rop2Space;
			else
				rop2 = rop2Mark;

			switch( rop2 )
			{
				case 1:	 Ptr[0] = 0;				Ptr[1] = 0;				   Ptr[2] = 0;				   break;
				case 2:  Ptr[0] = ~( P0 | Ptr[0] );	Ptr[1] = ~( P1 | Ptr[1] ); Ptr[2] = ~( P2 | Ptr[2] );  break;	
				case 3:	 Ptr[0] = ~P0 & Ptr[0];		Ptr[1] = ~P1 & Ptr[1];	   Ptr[2] = ~P2 & Ptr[2];	   break;	
				case 4:	 Ptr[0] = ~P0;				Ptr[1] = ~P1;			   Ptr[2] = ~P2;			   break;
				case 5:	 Ptr[0] = P0 & ~Ptr[0];		Ptr[1] = P1 & ~Ptr[1];	   Ptr[2] = P2 & ~Ptr[2];	   break;	
				case 6:	 Ptr[0] = ~Ptr[0];			Ptr[1] = ~Ptr[1];		   Ptr[2] = ~Ptr[2];		   break;	
				case 7:	 Ptr[0] = P0 ^ Ptr[0];		Ptr[1] = P1 ^ Ptr[1];	   Ptr[2] = P2 ^ Ptr[2];	   break;		
				case 8:	 Ptr[0] = ~( P0 & Ptr[0] );	Ptr[1] = ~( P1 & Ptr[1] ); Ptr[2] = ~( P2 & Ptr[2] );  break;	
				case 9:	 Ptr[0] = P0 & Ptr[0];		Ptr[1] = P1 & Ptr[1];	   Ptr[2] = P2 & Ptr[2];	   break;		
				case 10: Ptr[0] = ~( P0 ^ Ptr[0] );	Ptr[1] = ~( P1 ^ Ptr[1] ); Ptr[2] = ~( P2 ^ Ptr[2] );  break;	
				case 11: Ptr[0] = Ptr[0];			Ptr[1] = Ptr[1];		   Ptr[2] = Ptr[2];			   break;	
				case 12: Ptr[0] = ~P0 | Ptr[0];		Ptr[1] = ~P1 | Ptr[1];	   Ptr[2] = ~P2 | Ptr[2];	   break;	
				case 13: Ptr[0] = P0;				Ptr[1] = P1;			   Ptr[2] = P2;				   break;
				case 14: Ptr[0] = P0 | ~Ptr[0];		Ptr[1] = P1 | ~Ptr[1];	   Ptr[2] = P2 | ~Ptr[2];	   break;	
				case 15: Ptr[0] = P0 | Ptr[0];		Ptr[1] = P1 | Ptr[1];	   Ptr[2] = P2 | Ptr[2];	   break;		
				case 16: Ptr[0] = 0xff;				Ptr[1] = 0xff;			   Ptr[2] = 0xff;			   break;
				default:
					DEBUGMSG(GPE_ZONE_ERROR,(TEXT("Invalid rop2: %d\r\n"),rop2));
					return E_INVALIDARG;
			}


			Ptr += MajorDPtr;

			if( axstp )		// check for vertical/horizontal
			{
				if( accum < 0 )
				{
					DEBUGMSG(GPE_ZONE_LINE,(TEXT("Straight accum=%d axstp=%d\r\n"),accum,axstp));

					accum += axstp;
				}
				else
				{
					DEBUGMSG(GPE_ZONE_LINE,(TEXT("Diagonal accum=%d dgstp=%d\r\n"),accum,dgstp));
					Ptr += MinorDPtr;
					accum += dgstp;
				}
			}
		}
	}
	
	return S_OK;
}