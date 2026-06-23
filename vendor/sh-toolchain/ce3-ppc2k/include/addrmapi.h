/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    addrmapi.h

Purpose:

    List of properties used in the address book database.

--*/

#ifndef _ADDRMAPI_
#define _ADDRMAPI_

#ifndef HHPR_TAG_ONLY

// Here are the MAPI property types, taken from MAPIDEFS.H.
#define MV_FLAG         0x1000                                                  // multi-value flag
#define PT_UNSPECIFIED  ((ULONG)  0)  // (reserved for interface use) type doesn't matter to caller
#define PT_NULL         ((ULONG)  1)                                         // NULL property value
#define PT_I2           ((ULONG)  2)                                         // signed 16-bit value
#define PT_LONG         ((ULONG)  3)                                         // signed 32-bit value
#define PT_R4           ((ULONG)  4)                                       // 4-byte floating point
#define PT_DOUBLE       ((ULONG)  5)                                       // floating point double
#define PT_CURRENCY     ((ULONG)  6) // signed 64-bit int (decimal w/ 4 digits right of decimal pt)
#define PT_APPTIME      ((ULONG)  7)                                            // application time
#define PT_ERROR        ((ULONG) 10)                                          // 32-bit error value
// our store doesn't understand PT_BOOLEAN
//#define PT_BOOLEAN      ((ULONG) 11)                              // 16-bit boolean (non-zero true)
#define PT_OBJECT       ((ULONG) 13)                               // embedded object in a property
#define PT_I8           ((ULONG) 20)                                       // 8-byte signed integer
#define PT_STRING8      ((ULONG) 30)                      // null terminated 8-bit character string
#define PT_UNICODE      ((ULONG) 31)                              // null terminated Unicode string
#define PT_SYSTIME      ((ULONG) 64) // FILETIME 64-bit int (num of 100ns periods since Jan 1,1601)
#define PT_CLSID        ((ULONG) 72)                                                    // OLE GUID
#define PT_BINARY       ((ULONG) 258)                         // uninterpreted (counted byte array)

// Alternate property type names for ease of use.
#define PT_BOOLEAN  PT_I2
#define PT_SHORT    PT_I2
#define PT_I4       PT_LONG
#define PT_FLOAT    PT_R4
#define PT_R8       PT_DOUBLE
#define PT_LONGLONG PT_I8

// Property Tags.  By convention, MAPI never uses 0 or FFFF as a property ID.  Use as null values,
// initializers, sentinels, or what have you.
#define PROP_TYPE_MASK        ((ULONG)0x0000FFFF)  /* Mask for Property type */
#define PROP_TYPE(ulPropTag)  (((ULONG)(ulPropTag))&PROP_TYPE_MASK)
#define PROP_ID(ulPropTag)    (((ULONG)(ulPropTag))>>16)
#define PROP_TAG(ulPropType,ulPropID)  ((((ULONG)(ulPropID))<<16)|((ULONG)(ulPropType)))
#define PROP_ID_NULL       0
#define PROP_ID_INVALID       0xFFFF
#define PR_NULL               PROP_TAG( PT_NULL, PROP_ID_NULL)
#define CHANGE_PROP_TYPE(ulPropTag, ulPropType) (((ULONG)0xFFFF0000 & ulPropTag) | ulPropType)

// Here are properties defined by MAPI, taken from MAPITAGS.H.
#define PR_BODY_W                             PROP_TAG( PT_UNICODE,   0x1000)
#define PR_BUSINESS_FAX_NUMBER_W              PROP_TAG( PT_UNICODE,   0x3A24)
#define PR_BUSINESS_TELEPHONE_NUMBER_W        PROP_TAG( PT_UNICODE,   0x3A08)
#define PR_CALLBACK_TELEPHONE_NUMBER_W        PROP_TAG( PT_UNICODE,   0x3A02)
#define PR_CAR_TELEPHONE_NUMBER_W             PROP_TAG( PT_UNICODE,   0x3A1E)
#define PR_COMPANY_NAME_W                     PROP_TAG( PT_UNICODE,   0x3A16)
#define PR_CONTENT_UNREAD                     PROP_TAG( PT_LONG,      0x3603)
#define PR_CREATION_TIME                      PROP_TAG( PT_SYSTIME,   0x3007)
#define PR_DEPARTMENT_NAME_W                  PROP_TAG( PT_UNICODE,   0x3A18)
#define PR_DISPLAY_BCC_W                      PROP_TAG( PT_UNICODE,   0x0E02)
#define PR_DISPLAY_CC_W                       PROP_TAG( PT_UNICODE,   0x0E03)
#define PR_DISPLAY_TO_W                       PROP_TAG( PT_UNICODE,   0x0E04)
#define PR_GENERATION_W                       PROP_TAG( PT_UNICODE,   0x3A05)
#define PR_GIVEN_NAME_W                       PROP_TAG( PT_UNICODE,   0x3A06)
#define PR_GOVERNMENT_ID_NUMBER_W             PROP_TAG( PT_UNICODE,   0x3A07)
#define PR_HOME2_TELEPHONE_NUMBER_W           PROP_TAG( PT_UNICODE,   0x3A2F)
#define PR_HOME_FAX_NUMBER_W                  PROP_TAG( PT_UNICODE,   0x3A25)
#define PR_HOME_TELEPHONE_NUMBER_W            PROP_TAG( PT_UNICODE,   0x3A09)
#define PR_IMPORTANCE                         PROP_TAG( PT_LONG,      0x0017)
#define PR_KEYWORD_W                          PROP_TAG( PT_UNICODE,   0x3A0B)
#define PR_LAST_MODIFICATION_TIME             PROP_TAG( PT_SYSTIME,   0x3008)
#define PR_MESSAGE_DELIVERY_TIME              PROP_TAG( PT_SYSTIME,   0x0E06)
#define PR_MESSAGE_FLAGS                      PROP_TAG( PT_LONG,      0x0E07)
#define PR_MESSAGE_SIZE                       PROP_TAG( PT_LONG,      0x0E08)
#define PR_MOBILE_TELEPHONE_NUMBER_W          PROP_TAG( PT_UNICODE,   0x3A1C)
#define PR_CELLULAR_TELEPHONE_NUMBER_W        PR_MOBILE_TELEPHONE_NUMBER_W
#define PR_RADIO_TELEPHONE_NUMBER_W           PROP_TAG( PT_UNICODE,   0x3A1D)
#define PR_OFFICE_LOCATION_W                  PROP_TAG( PT_UNICODE,   0x3A19)
#define PR_OFFICE_TELEPHONE_NUMBER_W          PR_BUSINESS_TELEPHONE_NUMBER_W
#define PR_OTHER_TELEPHONE_NUMBER_W           PROP_TAG( PT_UNICODE,   0x3A1F)
#define PR_PRIMARY_FAX_NUMBER_W               PROP_TAG( PT_UNICODE,   0x3A23)
#define PR_PRIMARY_TELEPHONE_NUMBER_W         PROP_TAG( PT_UNICODE,   0x3A1A)
#define PR_PRIORITY                           PROP_TAG( PT_LONG,      0x0026)
#define PR_REPLY_RECIPIENT_NAMES_W            PROP_TAG( PT_UNICODE,   0x0050)
#define PR_REPLY_REQUESTED                    PROP_TAG( PT_BOOLEAN,   0x0C17)
#define PR_REPLY_TIME                         PROP_TAG( PT_SYSTIME,   0x0030)
#define PR_RESPONSE_REQUESTED                 PROP_TAG( PT_BOOLEAN,   0x0063)
#define PR_SECURITY                           PROP_TAG( PT_LONG,      0x0034)
#define PR_SENSITIVITY                        PROP_TAG( PT_LONG,      0x0036)
#define PR_SUBJECT_W                          PROP_TAG( PT_UNICODE,   0x0037)
#define PR_SURNAME_W                          PROP_TAG( PT_UNICODE,   0x3A11)
#define PR_TELEX_NUMBER_W                     PROP_TAG( PT_UNICODE,   0x3A2C)
#define PR_TITLE_W                            PROP_TAG( PT_UNICODE,   0x3A17)

#ifdef JAPAN
#define PR_YOMI_FIRSTNAME_W                   PROP_TAG( PT_UNICODE,   0x4011)
#define PR_YOMI_LASTNAME_W                    PROP_TAG( PT_UNICODE,   0x4010)
#endif //JAPAN

#endif  // HHPR_TAG_ONLY

// Here are our property tags.  Note that we use the MAPI tags when they are defined, otherwise we
// use the MAPI dispid's, which are store-specific.  The numbers used here (8xxx) were obtained by
// examining the MAPI database entries created by Ren.  For these we rely on replication to do the
// appropriate conversions.  Note also that we use the _W form of their string properties, as we
// are a unicode system.  Your mileage may vary.

typedef ULONG HHPRTAG;                                                    // this is a property tag

// Please do NOT use PT_BOOLEAN, use PEGVT_I2 instead. because this file is shared by both
// pegasus and desktop (which uses MAPI with a different valued PT_BOOLEAN) replication.

#define HHPR_ANNIVERSARY                      PROP_TAG(PT_SYSTIME, 0x4003)
#define HHPR_ASSISTANT_NAME                   PROP_TAG(PT_UNICODE, 0x4002)
#define HHPR_ASSISTANT_TELEPHONE_NUMBER       PROP_TAG(PT_UNICODE, 0x4004)
#define HHPR_BIRTHDAY                         PROP_TAG(PT_SYSTIME, 0x4001)
#define HHPR_BUSINESS_FAX_NUMBER              PR_BUSINESS_FAX_NUMBER_W
#define HHPR_CAR_TELEPHONE_NUMBER             PR_CAR_TELEPHONE_NUMBER_W
#define HHPR_CATEGORY                         PROP_TAG(PT_UNICODE, 0x4005)
#define HHPR_CHILDREN_NAME                    PROP_TAG(PT_UNICODE, 0x4006)
#define HHPR_COMPANY_NAME                     PR_COMPANY_NAME_W
#define HHPR_CUSTOM_DISPLAY_FIELDS            PROP_TAG(PT_UNICODE, 0x4317)  //$$BUGBUG -- this should stay 4317, not 8317
// 10 so far
#define HHPR_DEPARTMENT_NAME                  PR_DEPARTMENT_NAME_W
#define HHPR_EMAIL1_EMAIL_ADDRESS             PROP_TAG(PT_UNICODE, 0x4083)
#define HHPR_EMAIL2_EMAIL_ADDRESS             PROP_TAG(PT_UNICODE, 0x4093)
#define HHPR_EMAIL3_EMAIL_ADDRESS             PROP_TAG(PT_UNICODE, 0x40A3)
#define HHPR_GENERATION                       PR_GENERATION_W
#define HHPR_GIVEN_NAME                       PR_GIVEN_NAME_W
#define HHPR_HOME2_TELEPHONE_NUMBER           PR_HOME2_TELEPHONE_NUMBER_W
#define HHPR_HOME_ADDRESS_CITY                PROP_TAG(PT_UNICODE, 0x4041)
#define HHPR_HOME_ADDRESS_COUNTRY             PROP_TAG(PT_UNICODE, 0x4044)
#define HHPR_HOME_ADDRESS_POSTAL_CODE         PROP_TAG(PT_UNICODE, 0x4043)
// 20 so far
#define HHPR_HOME_ADDRESS_STATE               PROP_TAG(PT_UNICODE, 0x4042)
#define HHPR_HOME_ADDRESS_STREET              PROP_TAG(PT_UNICODE, 0x4040)
#define HHPR_HOME_FAX_NUMBER                  PR_HOME_FAX_NUMBER_W
#define HHPR_HOME_TELEPHONE_NUMBER            PR_HOME_TELEPHONE_NUMBER_W
#define HHPR_MIDDLE_NAME                      PROP_TAG(PT_UNICODE, 0x4024)
#define HHPR_MOBILE_TELEPHONE_NUMBER          PR_MOBILE_TELEPHONE_NUMBER_W
#define HHPR_NAME_PREFIX                      PROP_TAG(PT_UNICODE, 0x4023)
#define HHPR_NOTES                            PR_BODY_W
#define HHPR_OFFICE_ADDRESS_CITY              PROP_TAG(PT_UNICODE, 0x4046)
#define HHPR_OFFICE_ADDRESS_COUNTRY           PROP_TAG(PT_UNICODE, 0x4049)
// 30 so far
#define HHPR_OFFICE_ADDRESS_POSTAL_CODE       PROP_TAG(PT_UNICODE, 0x4048)
#define HHPR_OFFICE_ADDRESS_STATE             PROP_TAG(PT_UNICODE, 0x4047)
#define HHPR_OFFICE_ADDRESS_STREET            PROP_TAG(PT_UNICODE, 0x4045)
#define HHPR_OFFICE_LOCATION                  PR_OFFICE_LOCATION_W
#define HHPR_OFFICE_TELEPHONE_NUMBER          PR_OFFICE_TELEPHONE_NUMBER_W
#define HHPR_OFFICE2_TELEPHONE_NUMBER         PROP_TAG(PT_UNICODE, 0x4007)
#define HHPR_OTHER_ADDRESS_CITY               PROP_TAG(PT_UNICODE, 0x404B)
#define HHPR_OTHER_ADDRESS_COUNTRY            PROP_TAG(PT_UNICODE, 0x404E)
#define HHPR_OTHER_ADDRESS_POSTAL_CODE        PROP_TAG(PT_UNICODE, 0x404D)
#define HHPR_OTHER_ADDRESS_STATE              PROP_TAG(PT_UNICODE, 0x404C)
// 40 so far
#define HHPR_OTHER_ADDRESS_STREET             PROP_TAG(PT_UNICODE, 0x404A)
#define HHPR_PAGER_NUMBER                     PROP_TAG(PT_UNICODE, 0x4009)
#define HHPR_SPOUSE_NAME                      PROP_TAG(PT_UNICODE, 0x400a)
#define HHPR_SURNAME                          PR_SURNAME_W
#define HHPR_TITLE                            PR_TITLE_W
#define HHPR_WEB_PAGE                         PROP_TAG(PT_UNICODE, 0x4008)
#define HHPR_RADIO_TELEPHONE_NUMBER           PR_RADIO_TELEPHONE_NUMBER_W
#define HHPR_FILEAS                           PROP_TAG(PT_UNICODE, 0x4013)

#ifdef JAPAN
#define HHPR_YOMI_FIRSTNAME                   PR_YOMI_FIRSTNAME_W
#define HHPR_YOMI_LASTNAME                    PR_YOMI_LASTNAME_W

#define HHPR_YOMI_NAME 						  PROP_TAG(PT_UNICODE, 0x8319)
#define HHPR_YOMI_COMPANY					  PROP_TAG(PT_UNICODE, 0x4012)
#define HHPR_YOMI_SORT						  PROP_TAG(PT_UNICODE, 0x8318)
#define HHPR_YOMICOMPANY_SORT				  PROP_TAG(PT_UNICODE, 0x8320)
#endif //JAPAN

// How many address card properties were there?
#ifdef JAPAN
#ifdef FEATURE_RAPIER_NEWPROPS     // New props
#define HHAB_CARD_NUMPROPS          57
#else  // FEATURE_RAPIER_NEWPROPS
#define HHAB_CARD_NUMPROPS          55
#endif // FEATURE_RAPIER_NEWPROPS
#else  //JAPAN
#define HHAB_CARD_NUMPROPS          50
#endif //JAPAN

#endif

