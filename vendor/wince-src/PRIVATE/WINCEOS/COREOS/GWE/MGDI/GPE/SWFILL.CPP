/*

Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.

*/
#include "precomp.h"
#include "swemul.h"

/* A polygon edge */
struct Edge
{
	int yTop;			// Smaller of yStart, yEnd of line
	Edge *pNextTop;		// Next edge (sorted by increasing yTop)
	int yBottom;		// Larger of yStart, yEnd of line
	Edge *pNextBottom;	// Next edge (sorted by increasing yBottom)
    int fx;				// x coordinate of edge's intersection with current scanline
	Edge *pNextActive;	// Next edge (sorted by x) which intersects current
	int direction;		// +1 if yEnd > yStart, -1 otherwise
	int fxTop;			// x coordinate at top of line

	/* For each increment of the y-coordinate, how many integral pixels do we 
	   move the fx-coordinate? */
	int fxIntegralAdvance; 

	/* For each increment of the y-coordinate, how many fractional units do we
	   move the fx-coordinate?  One fractional unit == 1 / EDGE.dyHeight */
	int fxNumeratorAdvance;

	 /* The sum of all the numerator advances we have done so far which have 
	    not yet added up to one integral advance.  Or think of it as the 
	    fraction you need to add to EDGE.x to get the real (non-integral) 
	    x-coordinate */
	int fxNumerator;
	int fxNumeratorStart;
	
	/* the height of the edge, i.e., yMax - yMin, i.e, the denominator */
	int fdyHeight;

};


EdgeList::EdgeList(int nAlloc)
{
	DEBUGMSG(GPE_ZONE_POLY,(TEXT("Enter EdgeList::EdgeList\r\n")));
	m_aEdge = (Edge *)LocalAlloc( LMEM_FIXED, nAlloc*sizeof(Edge) );
	m_nListSize = m_aEdge?nAlloc:0;	// this copes with alloc failure
	m_nNumEdges = 0;
	m_pFirstTop = m_pFirstBottom = (Edge *)NULL;	// indicate we haven't sorted edges yet
	DEBUGMSG(GPE_ZONE_POLY,(TEXT("Leave EdgeList::EdgeList\r\n")));
}

EdgeList::~EdgeList()
{
	DEBUGMSG(GPE_ZONE_POLY,(TEXT("Enter EdgeList::~EdgeList\r\n")));
	if( m_aEdge )
		LocalFree((HLOCAL)m_aEdge);
	DEBUGMSG(GPE_ZONE_POLY,(TEXT("Leave EdgeList::~EdgeList\r\n")));
}

#ifdef SWAP
#undef SWAP
#endif
#define SWAP(type,a,b) { type tmp = a; a = b; b = tmp; }

SCODE EdgeList::AddEdge(
	signed long fx0,
	signed long fy0,
	signed long fx1,
	signed long fy1 )
{
	DEBUGMSG(GPE_ZONE_POLY,(TEXT("Enter EdgeList::AddEdge\r\n")));


	int fdx, fdy;
	int fxIntegralAdvance, fxNumeratorAdvance;
	int fxNumerator;

	if( m_nListSize <= m_nNumEdges )
	{
		Edge *pNewList = (Edge *)LocalReAlloc(
			(HLOCAL)m_aEdge, (m_nListSize+200)*sizeof(Edge), LMEM_MOVEABLE);
		if( !pNewList )
        {
        	DEBUGMSG(GPE_ZONE_ERROR,(TEXT("EdgeList::AddEdge realloc failed\r\n")));
			return E_OUTOFMEMORY;	// The realloc failed, so return failure
        }
		m_nListSize += 200;
		m_aEdge = pNewList;
	}

	if( fy0 < fy1 )
	{
		m_aEdge[m_nNumEdges].direction			= 1;
	}
	else
	{
		m_aEdge[m_nNumEdges].direction			= -1;
		SWAP(long,fy0,fy1)
		SWAP(long,fx0,fx1)
	}

	if( ( (fy0+15) & ~15 ) >= ((fy1+15) & ~15) )
	{
		DEBUGMSG(GPE_ZONE_POLY,(TEXT("Abandoning edge with dY=0\r\n")));
		return S_OK;
	}

	fdx = fx1 - fx0;
	fdy = fy1 - fy0;

	fxIntegralAdvance	= fdx / fdy;
	fxNumeratorAdvance	= fdx % fdy;
	fxNumerator = 0;

	while( fy0 & 15 )
	{
		// Increase fy0 by 1/16 of a pixel and fx0 accordingly
		fy0++;
		fx0 += fxIntegralAdvance;
		fxNumerator += fxNumeratorAdvance;
		if( fxNumeratorAdvance >= 0 )
		{
			if( fxNumerator > 0 )
			{
				fxNumerator -= fdy;
				fx0++;
			}
		}
		else
		{
			if( fxNumerator <= -fdy )
			{
				fxNumerator += fdy;
				fx0--;
			}
		}
	}

	// Now fx0,fy0 are the x.16 coordinates of where the edge intersects a row

	m_aEdge[m_nNumEdges].yTop				= fy0>>4;
	m_aEdge[m_nNumEdges].yBottom			= (fy1+15)>>4;
	m_aEdge[m_nNumEdges].fxTop				= fx0;

	m_aEdge[m_nNumEdges].pNextTop			= (Edge *)NULL;
	m_aEdge[m_nNumEdges].pNextBottom		= (Edge *)NULL;
	m_aEdge[m_nNumEdges].fxIntegralAdvance	= ( fdx * 16 ) / fdy;
	m_aEdge[m_nNumEdges].fxNumeratorAdvance	= ( fdx * 16 ) % fdy;
	m_aEdge[m_nNumEdges].fdyHeight			= fdy;
	m_aEdge[m_nNumEdges].fxNumeratorStart	= fxNumerator;


	m_nNumEdges++;

	DEBUGMSG(GPE_ZONE_POLY,(TEXT("Leave EdgeList::AddEdge\r\n")));
	return S_OK;
}

SCODE EdgeList::Fill( GPEBltParms *pParms, RECTL *prclClip, GPE *pGPE )
{
	int newEdgeNo;
	Edge **ppEdge;
	Edge *pEdge;
	SCODE sc;
	RECTL rclDst;
	int edgeCount;
	pParms->prclDst = &rclDst;
	int xLeft, xRight;
	int y;
	int fSwapped;
	
	DEBUGMSG(GPE_ZONE_POLY,(TEXT("Enter EdgeList::Fill\r\n")));

	if( m_nNumEdges < 1 )
		return S_OK;	// filling a null polygon!

	if( !m_pFirstTop )
	{
		// This is the first fill for this edgelist so sort by yTop & yBottom:
	
		for(	pEdge = m_aEdge, newEdgeNo=0;
				newEdgeNo<m_nNumEdges;
				newEdgeNo++, pEdge++ )
		{
			DEBUGMSG(GPE_ZONE_POLY,(TEXT("Inserting edge %d (yTop=%d,yBottom=%d) into Top/Bottom list\r\n"),
				newEdgeNo, pEdge->yTop, pEdge->yBottom ));
			// Insert into list sorted by yTop
			for(	ppEdge = &m_pFirstTop;
					( *ppEdge != (Edge *)NULL ) && ( pEdge->yTop > (*ppEdge)->yTop );
					ppEdge = & ( (*ppEdge)->pNextTop ) );
			pEdge->pNextTop = *ppEdge;
			*ppEdge = pEdge;

			// Insert into list sorted by yBottom
			for(	ppEdge = &m_pFirstBottom;
					( *ppEdge != (Edge *)NULL ) && ( pEdge->yBottom > (*ppEdge)->yBottom );
					ppEdge = & ( (*ppEdge)->pNextBottom ) );
			if( *ppEdge == (Edge *)NULL )
				m_nyMax = pEdge->yBottom;
			pEdge->pNextBottom = *ppEdge;
			*ppEdge = pEdge;
		}
		m_nyMin = m_pFirstTop->yTop;
	}

	DEBUGMSG(GPE_ZONE_POLY,(TEXT("yMin = %d, yMax = %d\r\n"), m_nyMin, m_nyMax ));

	if( prclClip && ( m_nyMin >= prclClip->bottom || m_nyMax < prclClip->top ) )
		return S_OK;	// completely clipped vertically

	Edge *pTopList = m_pFirstTop;
	Edge *pBottomList = m_pFirstBottom;
	m_pFirstActive = (Edge *)NULL;

	for( y=m_nyMin; y< m_nyMax; y++ )
	{

		DEBUGMSG(GPE_ZONE_POLY,(TEXT("Process row y=%d\r\n"), y ));

		if( prclClip && ( y>= prclClip->bottom ) )
			break;									// Hit bottom of clip rect

		pParms->prclDst->top = y;
		pParms->prclDst->bottom = y + 1;

		// Now add any new edges into active list
		for( ; pTopList && (pTopList->yTop <= y); pTopList = pTopList->pNextTop )
		{
			DEBUGMSG(GPE_ZONE_POLY,(TEXT("Add edge to active list\r\n")));
			// Reset x & error accumulator for line
			pTopList->fxNumerator = pTopList->fxNumeratorStart;
			pTopList->fx = pTopList->fxTop;

			// Insert into list sorted by x
			for(	ppEdge = &m_pFirstActive;
					( *ppEdge != (Edge *)NULL ) && ( pTopList->fx > (*ppEdge)->fx );
					ppEdge = & ( (*ppEdge)->pNextActive ) );
			pTopList->pNextActive = *ppEdge;
			*ppEdge = pTopList;
		}

		// Remove any edges from active list that we are now below the bottom of
		for( ; pBottomList && (pBottomList->yBottom <=y); pBottomList = pBottomList->pNextBottom )
		{
			DEBUGMSG(GPE_ZONE_POLY,(TEXT("Remove edge from active list\r\n")));
			for( 	ppEdge = &m_pFirstActive;
					( *ppEdge != (Edge *)NULL ) && (*ppEdge != pBottomList );
					ppEdge = & ( (*ppEdge)->pNextActive ) );
			if( *ppEdge )
				*ppEdge = (*ppEdge)->pNextActive;
		}

		// Bubble sort the active edge list by x
		DEBUGMSG(GPE_ZONE_POLY,(TEXT("Bubble sort active list\r\n")));
		do
		{
			fSwapped = 0;
			for(	ppEdge = &m_pFirstActive;
					( *ppEdge != (Edge *)NULL ) && ((*ppEdge)->pNextActive != (Edge *)NULL);
					ppEdge = &((*ppEdge)->pNextActive) )
			{
				pEdge = *ppEdge;
				if( pEdge->fx > pEdge->pNextActive->fx )
				{
					fSwapped = 1;
					*ppEdge = pEdge->pNextActive;
					pEdge->pNextActive = pEdge->pNextActive->pNextActive;
					(*ppEdge)->pNextActive = pEdge;
				}
			}
		} while ( fSwapped );

		edgeCount = 0;

		/* Draw each span for scan line y */
		if( !prclClip || ( y >= prclClip->top ) )
		{
			DEBUGMSG(GPE_ZONE_POLY,(TEXT("Drawing row y=%d\r\n"), y ));
			for( pEdge = m_pFirstActive; pEdge; pEdge = pEdge->pNextActive )
			{
				xLeft = (pEdge->fx+15)>>4;
				if( 0 ) // ( i.e. alternateMode )
					pEdge=pEdge->pNextActive;
				else
				{
					for( edgeCount = pEdge->direction; edgeCount; )
                    {
						pEdge = pEdge->pNextActive;
						if( !pEdge )   // Unlikely to occur but this is for safety
                            break;
					    edgeCount += pEdge->direction;
                    }
				}
				if( !pEdge )
				{
					DEBUGMSG(GPE_ZONE_ERROR,(TEXT("Unmatched line segment in polygon fill!\r\n")));
					return E_INVALIDARG;
				}
				xRight = (pEdge->fx+15)>>4;
				DEBUGMSG(GPE_ZONE_POLY,(TEXT("xLeft=%d, xRight=%d\r\n"), xLeft, xRight));
				if( prclClip )
				{
					if( xLeft < prclClip->left )
						xLeft = prclClip->left;
					if( xRight > prclClip->right )
						xRight = prclClip->right;
				}
				if( xLeft < xRight )
				{
					pParms->prclDst->left = xLeft;
					pParms->prclDst->right = xRight;
					// Perform actual Blt
					if( FAILED( sc = (pGPE->*(pParms->pBlt))( pParms ) ) )
						return sc;
				}
			}
		}

		// Now increment each edge
		DEBUGMSG(GPE_ZONE_POLY,(TEXT("Incrementing edges\r\n")));

		for( pEdge = m_pFirstActive; pEdge; pEdge = pEdge->pNextActive )
		{
			pEdge->fx += pEdge->fxIntegralAdvance;
			pEdge->fxNumerator += pEdge->fxNumeratorAdvance;

			// Round up
			if( pEdge->fxNumeratorAdvance >= 0 )
			{
				if( pEdge->fxNumerator > 0 )
				{
					pEdge->fxNumerator -= pEdge->fdyHeight;
					pEdge->fx++;
				}
			}
			else
			{
				if( pEdge->fxNumerator <= - pEdge->fdyHeight )
				{
					pEdge->fxNumerator += pEdge->fdyHeight;
					pEdge->fx--;
				}
			}
		}
	}

	DEBUGMSG(GPE_ZONE_POLY,(TEXT("Leave EdgeList::Fill\r\n")));

	return S_OK;
}

