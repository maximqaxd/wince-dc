//
// For shell-reserved GUID
//
//  The Win95 Shell has been allocated a block of 256 GUIDs,
// which follow the general format:
//
//  000214xx-0000-0000-C000-000000000046
//
//  Copyright (c) 1995, Microsoft Corp. All rights reserved.
//
#define DEFINE_SHLGUID(name, l, w1, w2) DEFINE_GUID(name, l, w1, w2, 0xC0,0,0,0,0,0,0,0x46)

//
// Class IDs        xx=00-DF
//
DEFINE_SHLGUID(CLSID_ShellDesktop,      0x00021400L, 0, 0);
//DEFINE_SHLGUID(CLSID_ShellLink, 	0x00021401L, 0, 0);

//
// Interface IDs    xx=E0-FF
//
//DEFINE_SHLGUID(IID_IPropSheetPage,  	0x000214E1L, 0, 0);
DEFINE_SHLGUID(IID_IContextMenu,    	0x000214E4L, 0, 0);
DEFINE_SHLGUID(IID_IShellFolder,    	0x000214E6L, 0, 0);
//DEFINE_SHLGUID(IID_IShellExtInit,   	0x000214E8L, 0, 0);
//DEFINE_SHLGUID(IID_IShellPropSheetExt,  0x000214E9L, 0, 0);
//DEFINE_SHLGUID(IID_IExtractIcon,   	0x000214EBL, 0, 0);
//DEFINE_SHLGUID(IID_IShellLink,		0x000214EEL, 0, 0);
//DEFINE_SHLGUID(IID_IShellCopyHook,	0x000214EFL, 0, 0);
//DEFINE_SHLGUID(IID_IFileViewer,		0x000214F0L, 0, 0);
DEFINE_SHLGUID(IID_IEnumIDList,     	0x000214F2L, 0, 0);
//DEFINE_SHLGUID(IID_IFileViewerSite, 	0x000214F3L, 0, 0);

DEFINE_SHLGUID(IID_IShellCommandUI,    	0x000214FEL, 0, 0);
DEFINE_SHLGUID(IID_IShellListView,    	0x000214FFL, 0, 0);

// {42650BC0-41C1-11d2-88E3-0000F87A49DB}
DEFINE_GUID(IID_INewMenuItemServer, 0x42650bc0, 0x41c1, 0x11d2, 0x88, 0xe3, 0x0, 0x0, 0xf8, 0x7a, 0x49, 0xdb);
