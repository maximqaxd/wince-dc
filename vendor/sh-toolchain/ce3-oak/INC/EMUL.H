/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Copyright (c) 1996-2000 Microsoft Corporation.  All rights reserved.

Module Name:

    emul.h

Abstract:

    Prototypes for special-case fast Blt functions for display drivers to use.


-------------------------------------------------------------------*/

#ifndef __EMUL_H__
#define __EMUL_H__

#include <gpe.h>

#ifdef __cplusplus
extern "C" {
#endif


/////////////////////////////////////////////
//                                         //
//      Helper functions to select         //
//      from available blit routines       //
//                                         //
//      Please update these functions      //
//      whenever you add a new             //
//      accelerated blit function          //
//                                         //
/////////////////////////////////////////////

SCODE EmulatedBltSelect02( GPEBltParms * );
SCODE EmulatedBltSelect08( GPEBltParms * );
SCODE EmulatedBltSelect16( GPEBltParms * );

class Emulator : public GPE
{

public:

	/////////////////////////////////////////////
    //                                         //
    //              2bpp Methods               //
    //                                         //
    /////////////////////////////////////////////

    //
    // Sourceless blts
    //
    SCODE EmulatedBltFill02     ( GPEBltParms * );
    SCODE EmulatedBltDstInvert02( GPEBltParms * );
    SCODE EmulatedBltPatInvert02( GPEBltParms * );

    //
    // Sourced blts
    //
    SCODE EmulatedBltSrcCopy0202  ( GPEBltParms * );
    SCODE EmulatedBltSrcAnd0202   ( GPEBltParms * );
    SCODE EmulatedBltSrcPaint0202 ( GPEBltParms * );
    SCODE EmulatedBltSrcInvert0202( GPEBltParms * );

    //
    // Text expansion
    //
    SCODE EmulatedBltText02     ( GPEBltParms * );

    /////////////////////////////////////////////
    //                                         //
    //              8bpp Methods               //
    //                                         //
    /////////////////////////////////////////////

    //
    // Sourceless blts
    //
    SCODE EmulatedBltFill08     ( GPEBltParms * );
    SCODE EmulatedBltDstInvert08( GPEBltParms * );
    SCODE EmulatedBltPatInvert08( GPEBltParms * );

    //
    // Sourced blts
    //
    SCODE EmulatedBltSrcCopy0808( GPEBltParms * );
    SCODE EmulatedBltSrcAnd0808( GPEBltParms * );
    SCODE EmulatedBltSrcPaint0808( GPEBltParms * );
    SCODE EmulatedBltSrcInvert0808( GPEBltParms * );
	SCODE EmulatedBltSrcCopy0408( GPEBltParms * );
	SCODE EmulatedBltSrcCopy0108( GPEBltParms * );

	//
	// Pattern blts
	//
	SCODE EmulatedBltPatternCopy08( GPEBltParms * );
	SCODE EmulatedBltPatternInvert08( GPEBltParms * );

    //
    // Text expansion
    //
    SCODE EmulatedBltText08                   ( GPEBltParms * );
    SCODE EmulatedBltCleartypeHalftoneText08  ( GPEBltParms * );
    SCODE EmulatedBltCleartype332Text08       ( GPEBltParms * );

    /////////////////////////////////////////////
    //                                         //
    //              16bpp Methods              //
    //                                         //
    /////////////////////////////////////////////

    //
    // Sourceless blts
    //
    SCODE EmulatedBltFill16     ( GPEBltParms * );

    //
    // Sourced blts
    //
    SCODE EmulatedBltSrcCopy1616( GPEBltParms * );
    SCODE EmulatedBltSrcAnd1616( GPEBltParms * );
    SCODE EmulatedBltSrcPaint1616( GPEBltParms * );
    SCODE EmulatedBltSrcInvert1616( GPEBltParms * );
	SCODE EmulatedBltSrcCopy0416( GPEBltParms * );
	SCODE EmulatedBltSrcCopy0116( GPEBltParms * );

    //
    // Text expansion
    //
    SCODE EmulatedBltText16         ( GPEBltParms * );
    SCODE EmulatedBltAlphaText16    ( GPEBltParms * );
    SCODE EmulatedBltAlphaText08    ( GPEBltParms * );
    SCODE EmulatedBltAlphaText02    ( GPEBltParms * );
    SCODE EmulatedBltCleartype565Text16       ( GPEBltParms * );

};

#ifdef __cplusplus
}
#endif

#endif __EMUL_H__

