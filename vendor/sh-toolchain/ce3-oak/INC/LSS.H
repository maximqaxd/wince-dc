// Copyright (c) 1999-2000 Microsoft Corporation.  All rights reserved.
/*
	Security subsystem, largely derived from ntseapi.h

*/

#ifndef _lss_H_
#define _lss_H_		1

#include <windows.h>
#if ! defined (UNDER_CE)
#include <stddef.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if ! defined (__cplusplus)
#define inline __inline
#endif

#if defined (UNDER_CE)
#define LSS(c) c
#define LookupAccountNameW LookupAccountName
#define LookupAccountSidW LookupAccountSid
#else
#define AllocateAndInitializeSid AllocateAndInitializeSidLSS
#define AllocateLocallyUniqueId AllocateLocallyUniqueIdLSS
#define CopySid CopySidLSS
#define DuplicateToken	DuplicateTokenLSS
#define EqualSid EqualSidLSS
#define FreeSid FreeSidLSS
#define GetLengthSid GetLengthSidLSS
#define GetSidIdentifierAuthority GetSidIdentifierAuthorityLSS
#define GetSidSubAuthority GetSidSubAuthorityLSS
#define GetSidSubAuthorityCount GetSidSubAuthorityCountLSS
#define GetTokenInformation GetTokenInformationLSS
#define IsValidSid IsValidSidLSS
#undef LookupAccountName
#define LookupAccountName LookupAccountNameLSS
#undef LookupAccountSid
#define LookupAccountSid LookupAccountSidLSS
#define OpenProcessToken OpenProcessTokenLSS
#define OpenThreadToken OpenThreadTokenLSS
#define SetThreadToken SetThreadTokenLSS
#define AccessCheck AccessCheckLSS
#undef LogonUser
#define LogonUser LogonUserLSS
#define RevertToSelf RevertToSelfLSS
#define AddAccessAllowedAce AddAccessAllowedAceLSS
#define GetAce GetAceLSS
#define AddAce AddAceLSS
#define GetSecurityDescriptorDacl GetSecurityDescriptorDaclLSS
#define InitializeAcl InitializeAclLSS
#define InitializeSecurityDescriptor InitializeSecurityDescriptorLSS
#define IsValidSecurityDescriptor IsValidSecurityDescriptorLSS
#define MakeSelfRelativeSD MakeSelfRelativeSDLSS
#define RegGetKeySecurity RegGetKeySecurityLSS
#define RegSetKeySecurity RegSetKeySecurityLSS
#define SetSecurityDescriptorDacl SetSecurityDescriptorDaclLSS
#define SetSecurityDescriptorSacl SetSecurityDescriptorSaclLSS
#define SetSecurityDescriptorOwner SetSecurityDescriptorOwnerLSS
#define SetSecurityDescriptorGroup SetSecurityDescriptorGroupLSS

#define LSS(c) c ## LSS
#endif

////////////////////////////////////////////////////////////////////////
//                                                                    //
//                             ACCESS MASK                            //
//                                                                    //
////////////////////////////////////////////////////////////////////////

//
//  Define the access mask as a longword sized structure divided up as
//  follows:
//
//       3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//       1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//      +---------------+---------------+-------------------------------+
//      |G|G|G|G|Res'd|A| StandardRights|         SpecificRights        |
//      |R|W|E|A|     |S|               |                               |
//      +-+-------------+---------------+-------------------------------+
//
//      typedef struct _ACCESS_MASK {
//          USHORT SpecificRights;
//          UCHAR StandardRights;
//          UCHAR AccessSystemAcl : 1;
//          UCHAR Reserved : 3;
//          UCHAR GenericAll : 1;
//          UCHAR GenericExecute : 1;
//          UCHAR GenericWrite : 1;
//          UCHAR GenericRead : 1;
//      } ACCESS_MASK;
//      typedef ACCESS_MASK *PACCESS_MASK;
//
//  but to make life simple for programmer's we'll allow them to specify
//  a desired access mask by simply OR'ing together mulitple single rights
//  and treat an access mask as a ULONG.  For example
//
//      DesiredAccess = DELETE | READ_CONTROL
//
//  So we'll declare ACCESS_MASK as ULONG
//
typedef ULONG ACCESS_MASK;
typedef ACCESS_MASK *PACCESS_MASK;


////////////////////////////////////////////////////////////////////////
//                                                                    //
//              Security Id     (SID)                                 //
//                                                                    //
////////////////////////////////////////////////////////////////////////
//
//
// Pictorially the structure of an SID is as follows:
//
//         1   1   1   1   1   1
//         5   4   3   2   1   0   9   8   7   6   5   4   3   2   1   0
//      +---------------------------------------------------------------+
//      |      SubAuthorityCount        |Reserved1 (SBZ)|   Revision    |
//      +---------------------------------------------------------------+
//      |                   IdentifierAuthority[0]                      |
//      +---------------------------------------------------------------+
//      |                   IdentifierAuthority[1]                      |
//      +---------------------------------------------------------------+
//      |                   IdentifierAuthority[2]                      |
//      +---------------------------------------------------------------+
//      |                                                               |
//      +- -  -  -  -  -  -  -  SubAuthority[]  -  -  -  -  -  -  -  - -+
//      |                                                               |
//      +---------------------------------------------------------------+
//
//
#ifndef SID_IDENTIFIER_AUTHORITY_DEFINED
#define SID_IDENTIFIER_AUTHORITY_DEFINED
typedef struct _SID_IDENTIFIER_AUTHORITY {
    UCHAR Value[6];
} SID_IDENTIFIER_AUTHORITY, *PSID_IDENTIFIER_AUTHORITY;
#endif
                                                // in a future release.
#ifndef SID_DEFINED
#define SID_DEFINED
typedef struct _SID {
   UCHAR Revision;
   UCHAR SubAuthorityCount;
   SID_IDENTIFIER_AUTHORITY IdentifierAuthority;
   ULONG SubAuthority[ANYSIZE_ARRAY];
} SID, *PISID;
#endif

#define SID_REVISION                     (1)    // Current revision level
#define SID_MAX_SUB_AUTHORITIES          (15)
#define SID_RECOMMENDED_SUB_AUTHORITIES  (1)    // Will change to around 6

typedef SID_AND_ATTRIBUTES SID_AND_ATTRIBUTES_ARRAY[ANYSIZE_ARRAY];
typedef SID_AND_ATTRIBUTES_ARRAY *PSID_AND_ATTRIBUTES_ARRAY;

//
//	Function decls and inline defs
//
#define SID_LENGTH_REQ(n) (offsetof (SID, SubAuthority) + sizeof (ULONG) * n)
#define SID_LENGTH(pSid)  (SID_LENGTH_REQ(((SID *)pSid)->SubAuthorityCount))

BOOL LSS(AllocateAndInitializeSid) (
    PSID_IDENTIFIER_AUTHORITY pIdentifierAuthority,
    BYTE nSubAuthorityCount,
    DWORD nSubAuthority0,
    DWORD nSubAuthority1,
    DWORD nSubAuthority2,
    DWORD nSubAuthority3,
    DWORD nSubAuthority4,
    DWORD nSubAuthority5,
    DWORD nSubAuthority6,
    DWORD nSubAuthority7,
    PSID *pSid
    );

BOOL LSS(AllocateLocallyUniqueId) (
    PLUID Luid
    );

inline BOOL LSS(CopySid) (
    DWORD nDestinationSidLength,
    PSID pDestinationSid,
    PSID pSourceSid
    ) {
	size_t cSourceSidLen = SID_LENGTH(pSourceSid);
	if (nDestinationSidLength < cSourceSidLen)
		return FALSE;

	memcpy (pDestinationSid, pSourceSid, cSourceSidLen);
	return TRUE;

}

inline BOOL LSS(DuplicateToken) (
    HANDLE ExistingTokenHandle,
    SECURITY_IMPERSONATION_LEVEL ImpersonationLevel,
    PHANDLE DuplicateTokenHandle
    ) {
	*DuplicateTokenHandle = ExistingTokenHandle;
	return TRUE;
}

inline BOOL LSS(EqualSid) (
    PSID pSid1,
    PSID pSid2
    ) {
	size_t cLen1 = SID_LENGTH(pSid1);
	size_t cLen2 = SID_LENGTH(pSid2);
	return (cLen1 == cLen2) && (memcmp (pSid1, pSid2, cLen1) == 0);
}

inline PVOID LSS(FreeSid) (
    PSID pSid
    ) {
	LocalFree (pSid);
	return NULL;
}

inline DWORD LSS(GetLengthSid) (
    PSID pSid
    ) {
	return SID_LENGTH(pSid);
}

inline PSID_IDENTIFIER_AUTHORITY LSS(GetSidIdentifierAuthority) (
    PSID pSid
    ) {
	return &(((SID *)pSid)->IdentifierAuthority);
}

inline PDWORD LSS(GetSidSubAuthority) (
    PSID pSid,
    DWORD nSubAuthority
    ) {
	SID *pSID = (SID *)pSid;
	if (pSID->SubAuthorityCount >= nSubAuthority)
		return NULL;
	return &pSID->SubAuthority[nSubAuthority];
}

inline PUCHAR LSS(GetSidSubAuthorityCount) (
    PSID pSid
    ) {
	SID *pSID = (SID *)pSid;
	return &pSID->SubAuthorityCount;
}

BOOL LSS(GetTokenInformation) (
    HANDLE TokenHandle,
    TOKEN_INFORMATION_CLASS TokenInformationClass,
    LPVOID TokenInformation,
    DWORD TokenInformationLength,
    PDWORD ReturnLength
    );

inline BOOL LSS(IsValidSid) (
    PSID pSid
    ) {
	SID *pSID = (SID *)pSid;
	return (pSID->Revision == SID_REVISION) && (pSID->SubAuthorityCount <= SID_MAX_SUB_AUTHORITIES);
}

BOOL LSS(LookupAccountName) (
    LPCTSTR lpSystemName,
    LPCTSTR lpAccountName,
    PSID Sid,
    LPDWORD cbSid,
    LPTSTR ReferencedDomainName,
    LPDWORD cbReferencedDomainName,
    PSID_NAME_USE peUse
    );

BOOL LSS(LookupAccountSid) (
    LPCTSTR lpSystemName,
    PSID Sid,
    LPTSTR Name,
    LPDWORD cbName,
    LPTSTR ReferencedDomainName,
    LPDWORD cbReferencedDomainName,
    PSID_NAME_USE peUse
    );

inline BOOL LSS(OpenProcessToken) (
    HANDLE ProcessHandle,
    DWORD DesiredAccess,
    PHANDLE TokenHandle
    ) {
	*TokenHandle = (HANDLE)1; // GetCurrentProcess ();
	return TRUE;
}

inline BOOL LSS(OpenThreadToken) (
    HANDLE ThreadHandle,
    DWORD DesiredAccess,
    BOOL OpenAsSelf,
    PHANDLE TokenHandle
    ) {
	*TokenHandle = (HANDLE)1; // GetCurrentThread ();
	return TRUE;
}

inline BOOL LSS(SetThreadToken) (
    PHANDLE Thread,
    HANDLE Token
    ) {
	return TRUE;
}

inline BOOL LSS(AccessCheck) (
    PSECURITY_DESCRIPTOR pSecurityDescriptor,
    HANDLE ClientToken,
    DWORD DesiredAccess,
    PGENERIC_MAPPING GenericMapping,
    PPRIVILEGE_SET PrivilegeSet,
    LPDWORD PrivilegeSetLength,
    LPDWORD GrantedAccess,
    LPBOOL AccessStatus
    ) {
	if (*PrivilegeSetLength < offsetof (PRIVILEGE_SET, Privilege)) {
		*PrivilegeSetLength = offsetof (PRIVILEGE_SET, Privilege);
		SetLastError (ERROR_INSUFFICIENT_BUFFER);
		return FALSE;
	}

	PrivilegeSet->PrivilegeCount = 0;
	PrivilegeSet->Control        = 0;
	*GrantedAccess = DesiredAccess;
	*AccessStatus = TRUE;

	return TRUE;
}

inline BOOL LSS(LogonUser) (
    LPWSTR lpszUsername,
    LPWSTR lpszDomain,
    LPWSTR lpszPassword,
    DWORD dwLogonType,
    DWORD dwLogonProvider,
    PHANDLE phToken
    ) {
	*phToken = (HANDLE)1; // GetCurrentProcess ();
	return TRUE;
}

inline BOOL LSS(RevertToSelf) (
    VOID
    ) {
	return TRUE;
}

BOOL LSS(AddAccessAllowedAce) (
    PACL pAcl,
    DWORD dwAceRevision,
    DWORD AccessMask,
    PSID pSid
    );

BOOL LSS(AddAce) (
  PACL pAcl,
  DWORD dwAceRevision,
  DWORD dwStartingAceIndex,
  LPVOID pAceList,
  DWORD nAceListLength
  );
  
BOOL LSS(GetAce) (PACL pAcl,
  DWORD dwAceIndex,
  LPVOID *pAce
  );

inline BOOL LSS(GetSecurityDescriptorDacl) (
    PSECURITY_DESCRIPTOR pSecurityDescriptor,
    LPBOOL lpbDaclPresent,
    PACL *pDacl,
    LPBOOL lpbDaclDefaulted
    ) {
	SECURITY_DESCRIPTOR *pSD = (SECURITY_DESCRIPTOR *)pSecurityDescriptor;
	if (pSD->Dacl) {
		*lpbDaclPresent = TRUE;
		*pDacl = pSD->Dacl;
	} else {
		*lpbDaclPresent = FALSE;
		*pDacl = NULL;
	}

	*lpbDaclDefaulted = pSD->Control & SE_OWNER_DEFAULTED;
	return TRUE;
}

inline BOOL LSS(InitializeAcl) (
    PACL pAcl,
    DWORD nAclLength,
    DWORD dwAclRevision
    ) {
	if ((dwAclRevision != ACL_REVISION) || (nAclLength < sizeof(ACL)))
		return FALSE;

	pAcl->AclRevision = ACL_REVISION;
	pAcl->Sbz1        = 0;
	pAcl->AclSize     = (WORD)nAclLength;
	pAcl->AceCount    = 0;
	pAcl->Sbz2        = 0;

	return TRUE;
}

inline BOOL LSS(InitializeSecurityDescriptor) (
    PSECURITY_DESCRIPTOR pSecurityDescriptor,
    DWORD dwRevision
    ) {
	SECURITY_DESCRIPTOR *pSD = (SECURITY_DESCRIPTOR *)pSecurityDescriptor;
	memset (pSD, 0, sizeof(*pSD));
	pSD->Revision = (BYTE)dwRevision;

	return TRUE;
}

inline BOOL LSS(IsValidSecurityDescriptor) (
    PSECURITY_DESCRIPTOR pSecurityDescriptor
    ) {
	return TRUE;
}

BOOL LSS(MakeSelfRelativeSD) (
    PSECURITY_DESCRIPTOR pAbsoluteSecurityDescriptor,
    PSECURITY_DESCRIPTOR pSelfRelativeSecurityDescriptor,
    LPDWORD lpdwBufferLength
    );


LONG LSS(RegGetKeySecurity) (HKEY hKey,
  SECURITY_INFORMATION SecurityInformation,
  PSECURITY_DESCRIPTOR pSecurityDescriptor,
  LPDWORD lpcbSecurityDescriptor);

inline LONG LSS(RegSetKeySecurity) (
  HKEY hKey,
  SECURITY_INFORMATION SecurityInformation,
  PSECURITY_DESCRIPTOR pSecurityDescriptor 
  ) {
	return ERROR_SUCCESS;
}

inline BOOL LSS(SetSecurityDescriptorDacl) (
    PSECURITY_DESCRIPTOR pSecurityDescriptor,
    BOOL bDaclPresent,
    PACL pDacl,
    BOOL bDaclDefaulted
    ) {
	if (bDaclPresent) {
		SECURITY_DESCRIPTOR *pSD = (SECURITY_DESCRIPTOR *)pSecurityDescriptor;
		pSD->Dacl = pDacl;
		if (bDaclDefaulted)
			pSD->Control |= SE_DACL_DEFAULTED;
		else
			pSD->Control &= ~SE_DACL_DEFAULTED;
	}

	return TRUE;
}

inline BOOL LSS(SetSecurityDescriptorSacl) (
    PSECURITY_DESCRIPTOR pSecurityDescriptor,
    BOOL bSaclPresent,
    PACL pSacl,
    BOOL bSaclDefaulted
    ) {
	if (bSaclPresent) {
		SECURITY_DESCRIPTOR *pSD = (SECURITY_DESCRIPTOR *)pSecurityDescriptor;
		pSD->Sacl = pSacl;
		if (bSaclDefaulted)
			pSD->Control |= SE_SACL_DEFAULTED;
		else
			pSD->Control &= ~SE_SACL_DEFAULTED;
	}

	return TRUE;
}

inline BOOL LSS(SetSecurityDescriptorOwner) (
    PSECURITY_DESCRIPTOR pSecurityDescriptor,
    PSID pOwner,
    BOOL bOwnerDefaulted
    ) {
	SECURITY_DESCRIPTOR *pSD = (SECURITY_DESCRIPTOR *)pSecurityDescriptor;
	pSD->Owner = pOwner;
	if (bOwnerDefaulted)
		pSD->Control |= SE_OWNER_DEFAULTED;
	else
		pSD->Control &= ~SE_OWNER_DEFAULTED;

	return TRUE;
}

inline BOOL LSS(SetSecurityDescriptorGroup) (
    PSECURITY_DESCRIPTOR pSecurityDescriptor,
    PSID pGroup,
    BOOL bGroupDefaulted
    ) {
	SECURITY_DESCRIPTOR *pSD = (SECURITY_DESCRIPTOR *)pSecurityDescriptor;
	pSD->Group = pGroup;
	if (bGroupDefaulted)
		pSD->Control |= SE_GROUP_DEFAULTED;
	else
		pSD->Control &= ~SE_GROUP_DEFAULTED;

	return TRUE;
}


#ifdef __cplusplus
}
#endif

#endif // __lss_H__
