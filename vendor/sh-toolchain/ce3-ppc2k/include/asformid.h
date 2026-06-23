/*---------------------------------------------------------------------------*\
 *
 * (c) Copyright Microsoft Corp. 1994 All Rights Reserved
 *
 *  module: asformid.h
 *  date:
 *  author: tonykit
 *
 *  purpose: 
 *
\*---------------------------------------------------------------------------*/
#ifndef __ASFORMID_H__
#define __ASFORMID_H__

/////////////////////////////////////////////////////////////////////////////

#if !defined(__MKTYPLIB__) && !defined(__midl)

// {2ECBC940-FF44-11cf-B8A5-A8AE00C10000}
//DEFINE_GUID(ActiveSmallForm, 
//0x2ecbc940, 0xff44, 0x11cf, 0xb8, 0xa5, 0xa8, 0xae, 0x0, 0xc1, 0x0, 0x0);

// {2ECBC941-FF44-11cf-B8A5-A8AE00C10000}
//DEFINE_GUID(IASForm, 
//0x2ecbc941, 0xff44, 0x11cf, 0xb8, 0xa5, 0xa8, 0xae, 0x0, 0xc1, 0x0, 0x0);

// {2ECBC942-FF44-11cf-B8A5-A8AE00C10000}
//DEFINE_GUID(_DActiveSmallFormEvents, 
//0x2ecbc942, 0xff44, 0x11cf, 0xb8, 0xa5, 0xa8, 0xae, 0x0, 0xc1, 0x0, 0x0);

// {2ECBC943-FF44-11cf-B8A5-A8AE00C10000}
//DEFINE_GUID(CLSID_CActiveSmallForm, 
//0x2ecbc943, 0xff44, 0x11cf, 0xb8, 0xa5, 0xa8, 0xae, 0x0, 0xc1, 0x0, 0x0);

// {2ECBC954-FF44-11cf-B8A5-A8AE00C10000}
//DEFINE_GUID(_DActiveSmallXObject, 
//0x2ecbc954, 0xff44, 0x11cf, 0xb8, 0xa5, 0xa8, 0xae, 0x0, 0xc1, 0x0, 0x0);

// {2ECBC955-FF44-11cf-B8A5-A8AE00C10000}
//DEFINE_GUID(_DActiveSmallXObjectEvents, 
//0x2ecbc955, 0xff44, 0x11cf, 0xb8, 0xa5, 0xa8, 0xae, 0x0, 0xc1, 0x0, 0x0);

// {2ECBC956-FF44-11cf-B8A5-A8AE00C10000}
//DEFINE_GUID(CLSID_CActiveSmallXObject, 
//0x2ecbc956, 0xff44, 0x11cf, 0xb8, 0xa5, 0xa8, 0xae, 0x0, 0xc1, 0x0, 0x0);

// {2ECBC960-FF44-11cf-B8A5-A8AE00C10000}
//DEFINE_GUID(CLSID_CActiveAPISet, 
//0x2ecbc960, 0xff44, 0x11cf, 0xb8, 0xa5, 0xa8, 0xae, 0x0, 0xc1, 0x0, 0x0);

#endif // !(defined(__MKTYPLIB__) && !defined(__midl))

/////////////////////////////////////////////////////////////////////////////

#define DISPID_ACTIVATE                  (-650)
#define DISPID_DEACTIVATE                (-651)
#define DISPID_GOTFOCUS                  (-652)
#define DISPID_LOAD                      (-653)
#define DISPID_LOSTFOCUS                 (-654)
#define DISPID_PAINT                     (-655)
#define DISPID_RESIZE                    (-656)
#define DISPID_SETFOCUS                  (-657)
#define DISPID_UNLOAD                    (-658)
#define DISPID_ADDCONTROL                (-659)
#define DISPID_SELECTION                 (-660)

#define DISPID_LEFT                      (-661)
#define DISPID_TOP                       (-662)
#define DISPID_WIDTH                     (-663)
#define DISPID_HEIGHT                    (-664)
#define DISPID_USER                      (-690)
#define DISPID_VIEWPORTORGX              (-691)
#define DISPID_VIEWPORTORGY              (-692)
#define DISPID_WINDOWEXTX                (-693)
#define DISPID_WINDOWEXTY                (-694)
#define DISPID_HSCROLL                   (-696)
#define DISPID_VSCROLL                   (-697)
#define DISPID_SCALEWIDTH                (-702)
#define DISPID_SCALEHEIGHT               (-703)
#define DISPID_REDRAW                    (-704)
#define DISPID_BEFOREMESSAGE             (-705)
#define DISPID_FIREVIEWCHANGE            (-706)

#define DISPID_NAME                      (-669)
#define DISPID_PARENT                    (-670)
#define DISPID_TABINDEX                  (-671)
#define DISPID_TAG                       (-672)
#define DISPID_VISIBLE                   (-673)
#define DISPID_HDC                       (-674)
#define DISPID_HIDE                      (-675)
#define DISPID_SHOW                      (-676)
#define DISPID_SCROLLSTYLE               (-677)
#define DISPID_DISABLEDSCROLLSTYLE       (-678)
#define DISPID_SELECTED                  (-679)
#define DISPID_FOCUS					 (-680)

#define DISPID_OUTPUTDEBUGSTRING         (-800)

// HACKHACK: 4k of controls are possible, I don't know if this
//           will conflict with other DISPIDs.

#define DISPID_FIRST_CONTROL             (4096)
#define DISPID_LAST_CONTROL              (8192)

/////////////////////////////////////////////////////////////////////////////
	
#endif /* __ASFORMID_H__ */
