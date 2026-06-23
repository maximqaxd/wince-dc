//*****************************************************************************
// WARNING: USE OF THIS SOURCE CODE IS RESTRICTED TO USE IN WINDOWS-CE BASED
//          SYSTEMS.  NO OTHER USE OF THIS CODE IS PERMITTED SUBJECT TO
//          LEGAL RESTRICTIONS. 
//*****************************************************************************


/************************************************************************/
/*                                                                      */
/*      FAT-FTL Lite Software Development Kit                              */
/*              Copyright (C) M-Systems Ltd. 1995-1998                    */
/*              Copyright (c) 1995-2000 Microsoft Corporation                */
/*                                                                      */
/************************************************************************/

#ifndef STDCOMP_H
#define STDCOMP_H

#include "flstatus.h"

#ifdef __cplusplus
extern "C" {
#endif

/*----------------------------------------------------------------------*/
/*    Registration routines for MTDs supplied with FLite		*/
/*----------------------------------------------------------------------*/

FLStatus    flRegisterI28F008(void);                  /* see I28F008.C  */
FLStatus    flRegisterI28F016(void);                  /* see I28F016.C  */
FLStatus    flRegisterAMDMTD(void);                   /* see AMDMTD.C   */
FLStatus    flRegisterWAMDMTD(void);                  /* see WAMDMTD.C  */
FLStatus    flRegisterCDSN(void);                     /* see NFDC2048.C */
FLStatus    flRegisterCFISCS(void);                   /* see CFISCS.C   */
FLStatus    flRegisterDOC2000(void); 	              /* see NFDC2148.C */


/*----------------------------------------------------------------------*/
/*    Registration routines for socket I/F supplied with FLite		*/
/*----------------------------------------------------------------------*/

FLStatus    flRegisterPCIC(unsigned int, unsigned int, unsigned char);
						      /* see PCIC.C     */
FLStatus    flRegisterElanPCIC(unsigned int, unsigned int, unsigned char);
                                                      /* see PCICELAN.C */
FLStatus    flRegisterLFDC(void);                     /* see LFDC.C     */
FLStatus    flRegisterDOCSOC(unsigned long, unsigned long);
                                                      /* see DOCSOC.C */
FLStatus    flRegisterElanRFASocket (int, int);       /* see ELRFASOC.C */
FLStatus    flRegisterElanDocSocket (long, long, int);/* see ELDOCSOC.C */
FLStatus    flRegisterVME177rfaSocket (unsigned long, unsigned long);
                                                      /* FLVME177.C */
FLStatus    flRegisterCobuxSocket (void);             /* see COBUXSOC.C */
FLStatus    flRegisterCEDOCSOC(void);                 /* see CEDOCSOC.C */
FLStatus    flRegisterCS(void);                       /* see CSwinCE.C */

/*----------------------------------------------------------------------*/
/*    Registration routines for translation layers supplied with FLite	*/
/*----------------------------------------------------------------------*/

FLStatus    flRegisterFTL(void);                      /* see FTLLITE.C  */
FLStatus    flRegisterNFTL(void);                     /* see NFTLLITE.C */
FLStatus    flRegisterSSFDC(void);                    /* see SSFDC.C    */
FLStatus    flRegisterATAtl(void);                    /* see atatl.c    */
FLStatus    flRegisterZIP(void);		      /* see ZIP.C	*/	
/*----------------------------------------------------------------------*/
/*    	    Component registration routine in CUSTOM.C			*/
/*----------------------------------------------------------------------*/

FLStatus    flRegisterComponents(void);

#ifdef __cplusplus
}
#endif

#endif /* STDCOMP_H */
