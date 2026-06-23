/*

Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.

*/
#include "precomp.h"
INSTANTIATE_GPE

GPE::GPE()
{
	m_pPrimarySurface = (GPESurf *)NULL;
	m_nScreenWidth = 0;
	m_nScreenHeight = 0;
	m_pMode = (GPEMode *)NULL;
}

GPE::~GPE(void)
{
	;
}

BOOL GPE::ContrastControl(
    ULONG cmd,
    ULONG *pValue)
{
    return TRUE;
}

VOID GPE::PowerHandler(
    BOOL bOff)
{
    return;
}

ULONG GPE::DrvEscape(
    SURFOBJ *pso,
    ULONG    iEsc,
    ULONG    cjIn,
    PVOID    pvIn,
    ULONG    cjOut,
    PVOID    pvOut)
{
    return 0;
}

ULONG GPE::GetGraphicsCaps(void)
{
    return 0;
}

// The following GPE methods are only used for DDHALs - the default does nothing 
void GPE::GetPhysicalVideoMemory(
	unsigned long *pPhysicalMemoryBase,
	unsigned long *pVideoMemorySize )
{
	;
}

void GPE::SetVisibleSurface( GPESurf *pSurf )
{
	;
}

int	 GPE::FlipInProgress()
{
	return 0;
}

void GPE::WaitForVBlank()
{
	;
}

int	GPE::SurfaceBusyFlipping( GPESurf *pSurf )
{
	return 0;
}

int GPE::IsBusy()
{
	return 0;
}

void GPE::WaitForNotBusy()
{
	;
}

unsigned long GPE::AvailableVideoMemory()
{
	return 0;
}

int GPE::ScanLine()
{
	return 0;
}

SCODE GPE::ProcessCommandBlock( unsigned char *pBlock )
{
	return E_NOTIMPL;
}

int GPE::IsPaletteSettable()
{
	// This should be overridden for settable palettes with !=8 Bpp, or for a fixed 8Bpp palette
	return ( m_pMode->Bpp == 8 );
}

GPESurf::GPESurf(
	int width,
	int height,
	EGPEFormat format )
{
	m_nWidth = width;
	m_nHeight = height;
	m_eFormat = format;
	m_nStrideBytes = ( (EGPEFormatToBpp[ format ] * width + 7 )/ 8 + 3 ) & ~3L;
	m_pVirtAddr = (ADDRESS)new unsigned char[ m_nStrideBytes * height ];
	m_fInVideoMemory = 0;
	m_nOffsetInVideoMemory = 0;
	m_fOwnsBuffer = 1;
}

void GPESurf::Init(
	int width,
	int height,
	void *pBits,
	int stride,
	EGPEFormat format )
{
	m_nWidth = width;
	m_nHeight = height;
	m_eFormat = format;
	m_nStrideBytes = stride;
	m_pVirtAddr = (ADDRESS)pBits;
	m_fInVideoMemory = 0;
	m_nOffsetInVideoMemory = 0;
	m_fOwnsBuffer = 0;
}

GPESurf::~GPESurf()
{
	if( m_fOwnsBuffer )
		if( m_pVirtAddr )
			delete (void *)m_pVirtAddr;
}


