/*

Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.

*/
#ifndef __SWEMUL_H__
#define __SWEMUL_H__

//
// SMEMUL.h
//
// This file contains public class definitions and prototypes for the software graphics
// primitives provided in swblt.cpp, swline.cpp, swfill.cpp, and swconvert.cpp
//


////////////////////////////////////////////////////////////////////////////////
// Software Blt emulation
//
// swblt.cpp

// The non stretch emulated blt can handle any combination of brushes, ROP4s,
// color conversion.
// The stretch emulated blt is a simple SRCCOPY with no masking, brushes, or color
// conversion.
// For stretch blts requiring more functionality than EmulatedStretchBlt provides,
// a temporary surface must be created. The source is stretched onto the temporary
// surface and then EmulatedBlt is called using the temporary surface
// as the source argument.

SCODE GPE::EmulatedBlt( GPEBltParms *pParms );

////////////////////////////////////////////////////////////////////////////////
// Software line drawing emulation
//
// swline.cpp

SCODE GPE::EmulatedLine( GPELineParms *pParms );

////////////////////////////////////////////////////////////////////////////////
// Software polygon fill emulation
//
// swfill.cpp

// For software filling of a polygon, construct a GPEBltParms as if preparing
// to Blt to the screen using a solid color or a patterned brush, and call
// GPE::Blt( &GPEBltParms, gpePrepare ).
// Then call EdgeList::AddEdge for each edge in the polygon, passing in the edges using
// coordinates in 28.4 fixed point format.
// Finally call EdgeList::Fill for each cliprect in the clip region.

/* A polygon edge */
struct Edge;

class EdgeList
{
	Edge			*m_aEdge;
	Edge			*m_pFirstTop;
	Edge			*m_pFirstBottom;
	Edge			*m_pFirstActive;
	int				m_nListSize;
	int				m_nNumEdges;
	int				m_nyMin;
	int				m_nyMax;

public:
					EdgeList(int nAlloc);
					~EdgeList();
	SCODE			Fill( GPEBltParms *pParms, RECTL *prclClip, GPE *pGPE );
	SCODE 			AddEdge(
						signed long fx0,
						signed long fy0,
						signed long fx1,
						signed long fy1 );
};




#endif // __SWEMUL_H__