/*

Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.

*/
#include "precomp.h"

#define NUMINTENSITYGROUPS	(31 + 15 + 7 + 1)	// (=54) Green,  Red,  Blue,  black
#define NUMRECENTENTRIES	16

class LRU
{
	unsigned long		*m_aiUniq;
	short				*m_aiNewer;
	short				*m_aiOlder;
	short				m_nLRU;
	short				m_nMRU;
	short				m_nListSize;
public:
						LRU();
						Init( short listSize );
	short				Find( unsigned long iUniq );	// -1 == not found
	short				Add( unsigned long iUniq );		// returns index chosen
	void				Fill( unsigned long value );
	void				Free();
//						~LRU();
};

class MaskInfo
{
public:
	unsigned long		m_aMask[3];			// entry #0:r, #1:g, #2:b
	unsigned char		m_anShift[3];		// To align top bit with bit#31
	unsigned short		m_iType;
	void				InitMaskInfo(
							XLATEOBJ *pxo,
							unsigned long ColorSource,	// XO_DESTPALETTE etc
							unsigned short ColorType );
};

// static unsigned long    (ColorConverter::*m_afpConvertFunction[])(unsigned long);

class MaskColorConverter : public ColorConverter
{
protected:
	MaskInfo			m_Src;			// Source bitmasks
	MaskInfo			m_Dst;			// Dest bitmasks
public:
	unsigned long		RGBToFromBGR( unsigned long rgbSrc );
	unsigned long		MaskedSrcToRGB( unsigned long maskedSrc );
	unsigned long		MaskedSrcToBGR( unsigned long maskedSrc );
	unsigned long		RGBSrcToMaskedDst( unsigned long rgbSrc );
	unsigned long		BGRSrcToMaskedDst( unsigned long bgrSrc );
	unsigned long		MaskedSrcToMaskedDst( unsigned long maskedSrc );
	void				Init( XLATEOBJ *pxo );
};

class GroupColorConverter : public ColorConverter
{
	unsigned long		m_aRecentResult[NUMRECENTENTRIES];
	LRU					m_LRURecent;
	short				m_piDstNext[256];	// Chain of entries of similar intensity
	short				m_aiIntensityHead[NUMINTENSITYGROUPS+1];	// last == -1
	unsigned short		m_nLowestGroup;	// Index of first valid entry in m_aiIntensityHead[]
	unsigned short		m_nHighestGroup;// Index of first -1 in m_aiIntensityHead[]
	unsigned long		m_pPalette[256];// Destination palette	 
	unsigned short		m_nPaletteSize;	// Destination palette size ( <= 256 )
	MaskInfo			m_Src;			// Source bitmasks
public:
	unsigned long		RGBToFromBGR( unsigned long rgbSrc );
	unsigned long		MaskedSrcToRGB( unsigned long maskedSrc );
	unsigned long		MaskedSrcToPaletteDst( unsigned long maskedSrc );
	int					InitLRU();
	void				FreeLRU();
	void				Init( XLATEOBJ *pxo );
};

class MonoColorConverter : public ColorConverter
{
	unsigned long		m_BackgroundColor;
public:
	unsigned long		AnyToMono( unsigned long srcValue );
	void				Init( unsigned long color )
						{
							m_BackgroundColor = color;
						}
};

class LookupColorConverter : public ColorConverter
{
	MaskInfo			m_Dst;			// Dest bitmasks
	unsigned long		m_aLookup[256];
public:
	unsigned long		RGBSrcToMaskedDst( unsigned long rgbSrc );
	void				Init( XLATEOBJ *pxo );
	unsigned long *		LookupTable() { return m_aLookup; }
};

#define MASK_LRU_SIZE 10
#define GROUP_LRU_SIZE 2
#define MONO_LRU_SIZE 10
#define LOOKUP_LRU_SIZE 4


LRU gMaskedLRU;
LRU gGroupLRU;
LRU gMonoLRU;
LRU gLookupLRU;

MaskColorConverter gMaskedConverter[MASK_LRU_SIZE];
GroupColorConverter gGroupConverter[GROUP_LRU_SIZE];
MonoColorConverter gMonoConverter[MONO_LRU_SIZE];
LookupColorConverter gLookupConverter[LOOKUP_LRU_SIZE];

// If AllocConverters fails, this returns 0. No free's are done as the driver is going
// to exit anyway.
int AllocConverters()
{
	int i;

	if( !gMaskedLRU.Init(MASK_LRU_SIZE) ||
		!gGroupLRU.Init(GROUP_LRU_SIZE) ||
		!gMonoLRU.Init(MONO_LRU_SIZE) ||
		!gLookupLRU.Init(LOOKUP_LRU_SIZE) )
		return 0;
	
	for( i=0; i<GROUP_LRU_SIZE; i++ )
		if( !gGroupConverter[i].InitLRU() )
			return 0;

	return 1;
}

void FreeConverters()
{
	int i;

	gMaskedLRU.Free();
	gGroupLRU.Free();
	gMonoLRU.Free();
	gLookupLRU.Free();
	for( i=0; i<GROUP_LRU_SIZE; i++ )
		gGroupConverter[i].FreeLRU();
}

int GroupColorConverter::InitLRU()
{
	return m_LRURecent.Init(NUMRECENTENTRIES);
}

void GroupColorConverter::FreeLRU()
{
	m_LRURecent.Free();
}

LRU::LRU()
{
	m_aiUniq = (unsigned long *)NULL;
	m_aiNewer = (short *)NULL;
	m_aiOlder = (short *)NULL;
}

int LRU::Init( short listSize )
{
	int i;

	DEBUGMSG(GPE_ZONE_BLT_LO,(TEXT("LRU::LRU( list size = %d )\r\n"),listSize ));

	m_aiUniq = (unsigned long *)malloc( listSize * sizeof(unsigned long) );
	m_aiNewer = (short *)malloc( listSize * sizeof(unsigned short) );
	m_aiOlder = (short *)malloc( listSize * sizeof(unsigned short) );
	if( !m_aiUniq || !m_aiNewer || !m_aiOlder )
		return 0;	// see AllocConverters
	m_nListSize = listSize;
	for( i=0; i<m_nListSize; i++ )
	{
		m_aiUniq[i] = 0;
		m_aiNewer[i] = i-1;		// Note that m_aiNewer[m_nMRU] is "undefined"
		m_aiOlder[i] = i+1;		// Note that m_aiOlder[m_nLRU] is "undefined"
	}
	m_nMRU = 0;
	m_nLRU = m_nListSize-1;
	return 1;
}

short LRU::Find( unsigned long iUniq )	// -1 == not found
{
	int i;
	for( i=0; i<m_nListSize; i++ )
	{
		if( m_aiUniq[i] == iUniq )
		{
			DEBUGMSG(GPE_ZONE_BLT_LO,(TEXT("LRU 0x%08x Find 0x%08x: found at location %d\r\n"),
				this, iUniq, i ));
			// Now we pop this to the most recently used:
			if( i == m_nLRU )
			{
				m_nLRU = m_aiNewer[i];
				m_aiNewer[m_nMRU] = i;
				m_aiOlder[i] = m_nMRU;
				m_nMRU = i;
			}
			else if ( i != m_nMRU )
			{
				m_aiNewer[m_aiOlder[i]] = m_aiNewer[i];
				m_aiOlder[m_aiNewer[i]] = m_aiOlder[i];
				m_aiNewer[m_nMRU] = i;
				m_aiOlder[i] = m_nMRU;
				m_nMRU = i;
			}
			return i;
		}
	}
	DEBUGMSG(GPE_ZONE_BLT_LO,(TEXT("LRU 0x%08x Find 0x%08x: Not found!\r\n"),
		this, iUniq ));
	return -1;
}

short LRU::Add( unsigned long iUniq )		// returns index chosen
{
	m_aiNewer[m_nMRU] = m_nLRU;
	m_aiOlder[m_nLRU] = m_nMRU;
	m_nMRU = m_nLRU;
	m_nLRU = m_aiNewer[m_nLRU];
	m_aiUniq[m_nMRU] = iUniq;
	return m_nMRU;	
}

void LRU::Fill( unsigned long value )
{
	int i;
	for( i=0; i<m_nListSize; i++ )
		m_aiUniq[i] = value;
}

// Not needed so long as DrvDisablePDEV gets called when this driver gets unloaded.
//LRU::~LRU()
//{
//	Free();
//}

void LRU::Free()
{
	if( m_aiUniq )
	{
		free( (void *)m_aiUniq );
		m_aiUniq = (unsigned long *)NULL;
	}
	if( m_aiNewer )
	{
		free( (void *)m_aiNewer );
		m_aiNewer = (short *)NULL;
	}
	if( m_aiOlder )
	{
		free( (void *)m_aiOlder );
		m_aiOlder = (short *)NULL;
	}
}


//typedef struct _XLATEOBJ {
//    ULONG  iUniq;
//    FLONG  flXlate;
//    USHORT iSrcType;
//    USHORT iDstType;
//    ULONG  cEntries;
//    ULONG  *pulXlate;
//} XLATEOBJ;


unsigned long SwapRedBlue( unsigned long srcValue )
{
	unsigned long dstValue = srcValue;
	((unsigned char *)(&dstValue))[0] = ((unsigned char *)(&srcValue))[2];
	((unsigned char *)(&dstValue))[2] = ((unsigned char *)(&srcValue))[0];
	return dstValue;
}



//#define XO_TRIVIAL 1
//#define XO_TABLE 2
//#define XO_TO_MONO 4
//
//#define PAL_INDEXED 1
//#define PAL_BITFIELDS 2
//#define PAL_RGB 4
//#define PAL_BGR 8
//
//#define XO_SRCPALETTE    1
//#define XO_DESTPALETTE   2
//#define XO_DESTDCPALETTE 3

	

void MaskInfo::InitMaskInfo(
	XLATEOBJ *pxo,
	unsigned long colorSource,	// XO_DESTPALETTE etc
	unsigned short colorType )	// PAL_INDEXED etc
{
	unsigned long mask;
	int topBit;

	m_iType = colorType;

	if( m_iType == PAL_BITFIELDS )
	{
		unsigned char component;
		XLATEOBJ_cGetPalette( pxo, colorSource, 3, m_aMask );
		for( component=0; component<3; component++ )
		{
			mask = m_aMask[component];
			for( topBit=0; mask; topBit++ )
				mask>>=1;
			m_anShift[component] = 32-topBit;
		}
	}
}

#define FNTYPE (unsigned long (ColorConverter::*)(unsigned long)) 

unsigned long (ColorConverter::* const m_afpConvertFunction[])(unsigned long) =
// unsigned long (ColorConverter::*ColorConverter::m_afpConvertFunction[])(unsigned long) =
{
//	(unsigned long (ColorConverter::*)(unsigned long)) NULL,	// Indexed -> indexed
//	(unsigned long (ColorConverter::*)(unsigned long)) NULL,	// Indexed -> bitfield
//	(unsigned long (ColorConverter::*)(unsigned long)) NULL,	// Indexed -> rgb
//	(unsigned long (ColorConverter::*)(unsigned long)) NULL,	// Indexed -> bgr

	FNTYPE GroupColorConverter::MaskedSrcToPaletteDst,			// bitfield -> indexed
	FNTYPE MaskColorConverter::MaskedSrcToMaskedDst,			// bitfield -> bitfield
	FNTYPE MaskColorConverter::MaskedSrcToRGB,					// bitfield -> rgb
	FNTYPE MaskColorConverter::MaskedSrcToBGR,					// bitfield -> bgr

	FNTYPE GroupColorConverter::MaskedSrcToPaletteDst,			// rgb -> indexed
	FNTYPE MaskColorConverter::RGBSrcToMaskedDst,				// rgb -> bitfield
	FNTYPE NULL,												// rgb -> rgb
	FNTYPE MaskColorConverter::RGBToFromBGR,					// rgb -> bgr

	FNTYPE GroupColorConverter::MaskedSrcToPaletteDst,			// bgr -> indexed
	FNTYPE MaskColorConverter::BGRSrcToMaskedDst,				// bgr -> bitfield
	FNTYPE MaskColorConverter::RGBToFromBGR,					// bgr -> rgb
	FNTYPE NULL													// bgr -> bgr
};


static unsigned char log2nOver2[] = { 0, 1, 2, 2, 3 };

unsigned long RGBError(
	unsigned long v1,
	unsigned long v2 )
{
	unsigned char *p1 = (unsigned char *)&v1;
	unsigned char *p2 = (unsigned char *)&v2;
	long accum = *p1++ - *p2++;
	accum *= accum;
	long diff = *p1++ - *p2++;
	accum += diff * diff;
	diff = *p1++ - *p2++;
	return accum + diff * diff;
}

short RGBToGroup( unsigned long rgbValue )
{
	// From 0x00bbggrr format
	return (short)(
		((rgbValue>>(24-3))&7)+		// 3 bits blue
		((rgbValue>>(16-5))&31)+	// 5 bits green
		((rgbValue>>(8-4))&15));		// 4 bits red
}



void ColorConverter::InitConverter(
	XLATEOBJ *pxo,
	ColorConverter **ppConvertObj,
	unsigned long (ColorConverter::**ppConvertFn)(unsigned long),
	unsigned long **ppLookup )
{
//	m_iUniq = pxo->iUniq;

	*ppConvertObj = (ColorConverter *)NULL;
	*ppLookup = (unsigned long *)NULL;
	*ppConvertFn = NULL;
	short converterIndex;

	if( pxo->flXlate == XO_TRIVIAL )
	{
		DEBUGMSG(GPE_ZONE_BLT_LO,(TEXT("XO_TRIVIAL\r\n")));
		;
	}
	else if( pxo->flXlate == XO_TABLE )
	{
		DEBUGMSG(GPE_ZONE_BLT_LO,(TEXT("XO_TABLE\r\n")));
		*ppLookup = pxo->pulXlate;
	}
	else if( pxo->flXlate == XO_TO_MONO )
	{
		DEBUGMSG(GPE_ZONE_BLT_LO,(TEXT("XO_TO_MONO\r\n")));
		// we could optionally create a lookup table if the source is PAL_INDEXED
		// for faster access
		unsigned long backgroundColor = pxo->pulXlate[0];
		if( ( converterIndex = gMonoLRU.Find( backgroundColor ) ) == -1 )
		{
			converterIndex = gMonoLRU.Add( backgroundColor );
		}
		gMonoConverter[converterIndex].Init(backgroundColor);
		*ppConvertObj = &gMonoConverter[converterIndex];
		*ppConvertFn = FNTYPE MonoColorConverter::AnyToMono;
	}
	else if( pxo->iSrcType == PAL_INDEXED )
	{
		DEBUGMSG(GPE_ZONE_BLT_LO,(TEXT("PAL_INDEXED->\r\n")));
		if( ( converterIndex = gLookupLRU.Find( pxo->iUniq ) ) == -1 )
		{
			// there is no cached converter for this iUniq
			converterIndex = gLookupLRU.Add( pxo->iUniq );
			gLookupConverter[converterIndex].Init( pxo );
		}
		*ppLookup = gLookupConverter[converterIndex].LookupTable();
	}
	else if( pxo->iDstType == PAL_INDEXED )
	{
		DEBUGMSG(GPE_ZONE_BLT_LO,(TEXT("->PAL_INDEXED\r\n")));
		if( ( converterIndex = gGroupLRU.Find( pxo->iUniq ) ) == -1 )
		{
			// there is no cached converter for this iUniq
			converterIndex = gGroupLRU.Add( pxo->iUniq );
			gGroupConverter[converterIndex].Init( pxo );
		}
		*ppConvertObj = &gGroupConverter[converterIndex];
		*ppConvertFn = FNTYPE GroupColorConverter::MaskedSrcToPaletteDst;
	}
	else
	{
		// It is masked,rgb, or bgr   -->  masked,rgb, or bgr
		// Look up the converter function based on the src & dst color types
		DEBUGMSG(GPE_ZONE_BLT_LO,(TEXT("mask/rgb <-> mask/rgb\r\n")));
		*ppConvertFn = m_afpConvertFunction[
			log2nOver2[pxo->iSrcType/2]*4+log2nOver2[pxo->iDstType/2] - 4];

		if( ( pxo->iSrcType | pxo->iDstType ) & PAL_BITFIELDS )
		{
			// for bitfields (i.e. masked), there is some storage required...
			if( ( converterIndex = gMaskedLRU.Find( pxo->iUniq ) ) == -1 )
			{
				// there is no cached converter for this iUniq
				converterIndex = gMaskedLRU.Add( pxo->iUniq );
				gMaskedConverter[converterIndex].Init( pxo );
			}
			*ppConvertObj = &gMaskedConverter[converterIndex];
		}
	}
}

void MaskColorConverter::Init( XLATEOBJ *pxo )
{
	DEBUGMSG(GPE_ZONE_BLT_LO,(TEXT("Mask Init\r\n")));
	
	m_Src.InitMaskInfo( pxo, XO_SRCPALETTE, pxo->iSrcType );
	m_Dst.InitMaskInfo( pxo, XO_DESTPALETTE, pxo->iDstType );
}

void GroupColorConverter::Init( XLATEOBJ *pxo )
{
	// Create an intensity-grouped table
	short group;
	short lastHead;
	short i;

	DEBUGMSG(GPE_ZONE_BLT_LO,(TEXT("Group Init\r\n")));

	m_Src.InitMaskInfo( pxo, XO_SRCPALETTE, pxo->iSrcType );

	// Read in destination palette
	unsigned short m_nPaletteSize = (unsigned short)XLATEOBJ_cGetPalette(pxo,XO_DESTPALETTE,0,(unsigned long *)NULL);
	if( m_nPaletteSize > 256 )
		m_nPaletteSize = 256;
	XLATEOBJ_cGetPalette(pxo,XO_DESTPALETTE,m_nPaletteSize,m_pPalette);

	m_nHighestGroup = 0;
	m_nLowestGroup = 0x0100;

	// Create chains of entries for each intensity group

	for( group=0; group<=NUMINTENSITYGROUPS; group++ )
		m_aiIntensityHead[group] = -1;

	for( i=0; i<m_nPaletteSize; i++ )
	{
		group = RGBToGroup( m_pPalette[i] );
		m_piDstNext[i] = m_aiIntensityHead[group];
		m_aiIntensityHead[group] = i;
		if( group > m_nHighestGroup )
			m_nHighestGroup = group;
		if( group < m_nLowestGroup )
			m_nLowestGroup = group;
	}
	
	lastHead = m_aiIntensityHead[m_nHighestGroup];
	m_nHighestGroup++;
	m_aiIntensityHead[m_nHighestGroup] = -1;

	for( group=m_nHighestGroup-2; group>=m_nLowestGroup; group-- )
	{
		if( ( i = m_aiIntensityHead[group] ) == -1 )
			m_aiIntensityHead[group] = lastHead;
		else
		{
			// Connect last entry in chain to first entry in next chain
			while( m_piDstNext[i] != -1 )
				i = m_piDstNext[i];
			m_piDstNext[i] = lastHead;
			lastHead = m_aiIntensityHead[group];
		}
	}
	// Invalidate the recent entry cache
	m_LRURecent.Fill(0xffffffff);
}

void LookupColorConverter::Init( XLATEOBJ *pxo )
{
	unsigned short srcInd;
	unsigned long dstPalette[256];

	// Used for palette -> palette, or palette -> masked

	// First, read in the source palette
	unsigned short srcPaletteSize = (unsigned short)XLATEOBJ_cGetPalette(pxo,XO_SRCPALETTE,0,(unsigned long *)NULL);
	if( srcPaletteSize > 256 )
		srcPaletteSize = 256;
	XLATEOBJ_cGetPalette(pxo,XO_SRCPALETTE,srcPaletteSize,m_aLookup);

	if( pxo->iDstType == PAL_INDEXED )
	{
		// Read in the dest palette
		DEBUGMSG(GPE_ZONE_BLT_LO,(TEXT("PAL_INDEXED->PAL_INDEXED\r\n")));
		unsigned short dstPaletteSize = (unsigned short)XLATEOBJ_cGetPalette(pxo,XO_DESTPALETTE,0,(unsigned long *)NULL);
		if( dstPaletteSize > 256 )
			dstPaletteSize = 256;
		XLATEOBJ_cGetPalette(pxo,XO_DESTPALETTE,dstPaletteSize,dstPalette);

		// Create a closest-match lookup table
		unsigned long *srcPtr;
		srcPtr = m_aLookup;

		for( srcInd=0; srcInd<srcPaletteSize; srcInd++ )
		{
			*srcPtr = DrvRealizeColor( PAL_INDEXED, dstPaletteSize, dstPalette, *srcPtr );
			srcPtr++;
		}
	}
	else
	{
		m_Dst.InitMaskInfo( pxo, XO_DESTPALETTE, pxo->iDstType );
		DEBUGMSG(GPE_ZONE_BLT_LO,(TEXT("PAL_INDEXED->mask/rgb\r\n")));
		// Populate a lookup table with an appropriately munged palette
		for( srcInd=0; srcInd < srcPaletteSize; srcInd++ )
		{
			if( pxo->iDstType == PAL_BITFIELDS )
				m_aLookup[srcInd] = RGBSrcToMaskedDst( m_aLookup[srcInd] );
			if( pxo->iDstType == PAL_BGR )
				m_aLookup[srcInd] = SwapRedBlue( m_aLookup[srcInd] );
		}
	}
}

unsigned long GroupColorConverter::MaskedSrcToPaletteDst( unsigned long maskedSrc )
{

	short lruIndex;
	unsigned long dstValue;
	unsigned long srcRGB;
	short possibleDst;
	short lowGroup, highGroup;
	unsigned long error;
	unsigned long smallestError;
	
	if( ( lruIndex = m_LRURecent.Find(maskedSrc) ) == -1 )
	{
		
		if( m_Src.m_iType == PAL_RGB )
			srcRGB = maskedSrc;
		else if( m_Src.m_iType == PAL_BGR )
			srcRGB = RGBToFromBGR( maskedSrc );
		else
			srcRGB = MaskedSrcToRGB( maskedSrc );
           
		// We need to find the closest lookup
		// Determine range of intensity groups to check
		highGroup = lowGroup = RGBToGroup( srcRGB );

		lowGroup = (lowGroup<m_nLowestGroup+4)?m_nLowestGroup:lowGroup-4;
		highGroup = (highGroup>m_nHighestGroup-5)?m_nHighestGroup:highGroup+5;
		if( m_aiIntensityHead[lowGroup] == m_aiIntensityHead[highGroup] )
		{
			// No palette entries in range (i.e. inept palette) - scan all entries
			lowGroup = m_nLowestGroup;
			highGroup = m_nHighestGroup;
		}

		smallestError=0x10000000;

		for( possibleDst = m_aiIntensityHead[lowGroup];
			possibleDst != m_aiIntensityHead[highGroup];
			possibleDst = m_piDstNext[possibleDst] )
		{
			error = RGBError( srcRGB, m_pPalette[possibleDst] );
			if( error < smallestError )
			{
				smallestError = error;
				dstValue = (unsigned long)possibleDst;
			}
		}

		// add this conversion to the LRU
		lruIndex = m_LRURecent.Add(maskedSrc);
		m_aRecentResult[lruIndex] = dstValue;
	}
	else								 
	{
		// we've recently done the same conversion so we can look it up
		dstValue = m_aRecentResult[lruIndex];
	}

	return dstValue;
}
	
unsigned long MaskColorConverter::RGBToFromBGR( unsigned long rgbSrc )
{
	return SwapRedBlue( rgbSrc );
}
unsigned long GroupColorConverter::RGBToFromBGR( unsigned long rgbSrc )
{
	return SwapRedBlue( rgbSrc );
}
unsigned long MaskColorConverter::MaskedSrcToRGB( unsigned long srcValue )
{
	return
		( ( ( srcValue & m_Src.m_aMask[0] ) << m_Src.m_anShift[0] ) >> 24 ) |
		( ( ( srcValue & m_Src.m_aMask[1] ) << m_Src.m_anShift[1] ) >> 16 ) |
		( ( ( srcValue & m_Src.m_aMask[2] ) << m_Src.m_anShift[2] ) >> 8 );
}
unsigned long GroupColorConverter::MaskedSrcToRGB( unsigned long srcValue )
{
	return
		( ( ( srcValue & m_Src.m_aMask[0] ) << m_Src.m_anShift[0] ) >> 24 ) |
		( ( ( srcValue & m_Src.m_aMask[1] ) << m_Src.m_anShift[1] ) >> 16 ) |
		( ( ( srcValue & m_Src.m_aMask[2] ) << m_Src.m_anShift[2] ) >> 8 );
}
unsigned long MaskColorConverter::MaskedSrcToBGR( unsigned long srcValue )
{
	return
		( ( ( srcValue & m_Src.m_aMask[0] ) << m_Src.m_anShift[0] ) >> 8 ) |
		( ( ( srcValue & m_Src.m_aMask[1] ) << m_Src.m_anShift[1] ) >> 16 ) |
		( ( ( srcValue & m_Src.m_aMask[2] ) << m_Src.m_anShift[2] ) >> 24 );
}
unsigned long MaskColorConverter::RGBSrcToMaskedDst( unsigned long srcValue )
{
	return
		((( srcValue << 24 ) >> m_Dst.m_anShift[0] ) & m_Dst.m_aMask[0] ) |
		((( srcValue << 16 ) >> m_Dst.m_anShift[1] ) & m_Dst.m_aMask[1] ) |
		((( srcValue << 8  ) >> m_Dst.m_anShift[2] ) & m_Dst.m_aMask[2] );
}
unsigned long LookupColorConverter::RGBSrcToMaskedDst( unsigned long srcValue )
{
	return
		((( srcValue << 24 ) >> m_Dst.m_anShift[0] ) & m_Dst.m_aMask[0] ) |
		((( srcValue << 16 ) >> m_Dst.m_anShift[1] ) & m_Dst.m_aMask[1] ) |
		((( srcValue << 8  ) >> m_Dst.m_anShift[2] ) & m_Dst.m_aMask[2] );
}
unsigned long MaskColorConverter::BGRSrcToMaskedDst( unsigned long srcValue )
{
	return
		((( srcValue << 8  ) >> m_Dst.m_anShift[0] ) & m_Dst.m_aMask[0] ) |
		((( srcValue << 16 ) >> m_Dst.m_anShift[1] ) & m_Dst.m_aMask[1] ) |
		((( srcValue << 24 ) >> m_Dst.m_anShift[2] ) & m_Dst.m_aMask[2] );
}
unsigned long MaskColorConverter::MaskedSrcToMaskedDst( unsigned long maskedSrc )
{
	return
		( ( ( ( maskedSrc & m_Src.m_aMask[0] ) << m_Src.m_anShift[0] )
			>> m_Dst.m_anShift[0] ) & m_Dst.m_aMask[0] ) |
		( ( ( ( maskedSrc & m_Src.m_aMask[1] ) << m_Src.m_anShift[1] )
			>> m_Dst.m_anShift[1] ) & m_Dst.m_aMask[1] ) |
		( ( ( ( maskedSrc & m_Src.m_aMask[2] ) << m_Src.m_anShift[2] )
			>> m_Dst.m_anShift[2] ) & m_Dst.m_aMask[2] );
}

unsigned long MonoColorConverter::AnyToMono( unsigned long srcValue )
{
	return ( srcValue == m_BackgroundColor ) ? 1 : 0;
}




ULONG APIENTRY DrvRealizeColor(
    USHORT     iDstType,
    ULONG      cEntries,
    ULONG     *pPalette,
    ULONG      rgbColor)
{
	unsigned short palInd;
	unsigned long *palPtr;
	unsigned long dstValue;
	unsigned long smallestError;
	unsigned long error;
	unsigned long mask;
	char shiftRight;
	char shiftLeft;

	switch( iDstType )
	{
	case PAL_INDEXED:
		smallestError = 0x7fffffff;	// Set large value
		palPtr = pPalette;
		for( palInd=0; palInd < cEntries; palInd++ )
		{
			error = RGBError( rgbColor, *palPtr++ );
			if( error > smallestError )
				continue;
			smallestError = error;
			dstValue = palInd;
			if( error == 0 )	// perfect match, no need to keep looking
				break;
		}
		return dstValue;
	case PAL_BITFIELDS:
		dstValue = 0;
		for( shiftLeft=24; (shiftLeft>=0)&&(cEntries--); shiftLeft-=8 )
		{
			mask = *pPalette;
			for( shiftRight=32; mask; shiftRight-- )
				mask>>=1;
			dstValue |= ( ( rgbColor  << shiftLeft ) >> shiftRight ) & *pPalette++;
		}
		return dstValue;
	case PAL_BGR:
		return SwapRedBlue( rgbColor );
	case PAL_RGB:
		return rgbColor;
	}
	return 0;	// actually an illegal type was encountered
}

ULONG APIENTRY DrvUnrealizeColor(
    USHORT     iSrcType,
    ULONG      cEntries,
    ULONG     *pPalette,
    ULONG      iRealizedColor)
{
	unsigned long dstValue;
	unsigned long mask;
	char shiftRight;
	char shiftLeft;

	switch( iSrcType )
	{
	case PAL_INDEXED:
		return pPalette[iRealizedColor];
	case PAL_BITFIELDS:
		dstValue = 0;
		for( shiftRight=24; (shiftRight>=0)&&(cEntries--); shiftRight-=8 )
		{
			mask = *pPalette;
			for( shiftLeft=32; mask; shiftLeft-- )
				mask>>=1;
			dstValue |= ( ( iRealizedColor & *pPalette++ ) << shiftLeft ) >> shiftRight;
		}
		return dstValue;
	case PAL_BGR:
		return SwapRedBlue(iRealizedColor);
	case PAL_RGB:
		return iRealizedColor;
	}
	return 0;	// actually an illegal type was encountered
}

