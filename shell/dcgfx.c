//
// dcgfx.c - DirectDraw + GDI graphics layer for the Dreamcast CE shell.
//
// The DC primary surface is volatile (content not retained when idle), so we draw
// every frame into a persistent OFFSCREEN back buffer (created with an explicit
// 565 format that matches the primary, so COLORFILL colors are correct) and Blt
// it to the primary each loop iteration via GfxPresent.
//
#include "dcgfx.h"
#include <ddraw.h>

// Flip-chain model (the SDK-idiomatic DC present, see samples/ddraw): the screen
// is a page-flip chain (front+back); we composite the desktop into an offscreen
// "scene" surface (only when content changes), and present by Blt scene->back +
// cursor on top + Flip(DDFLIP_WAIT). Flip is vsync-paced and yields the CPU, and
// flip-chain surfaces retain content - so we present only on change, not in a
// 120 Hz busy Blt loop (which was starving the client processes).
static HWND                s_hwnd    = NULL;
static LPDIRECTDRAW        s_dd      = NULL;
static LPDIRECTDRAWSURFACE s_primary = NULL;   // flip-chain front
static LPDIRECTDRAWSURFACE s_back    = NULL;   // flip-chain back (attached to primary)
static LPDIRECTDRAWSURFACE s_scene   = NULL;   // offscreen composited desktop (no cursor)
static BOOL                s_useFlip = FALSE;  // TRUE if a flip chain was created
static LPDIRECTDRAWSURFACE s_icon[ICON_COUNT];      // 16x16
static LPDIRECTDRAWSURFACE s_iconBig[ICON_COUNT];   // 32x32 (pixel-doubled)

HFONT g_FontUI    = NULL;
HFONT g_FontBold  = NULL;
HFONT g_FontTitle = NULL;

// all drawing (fills/text/icons) composites into the offscreen scene
static LPDIRECTDRAWSURFACE Target(void)
{
    return s_scene ? s_scene : s_primary;
}

static WORD ToRgb565(COLORREF c)
{
    return (WORD)(((GetRValue(c) >> 3) << 11) | ((GetGValue(c) >> 2) << 5) | (GetBValue(c) >> 3));
}

static HFONT MakeFont(int height, int weight)
{
    LOGFONTW lf;

    memset(&lf, 0, sizeof(lf));
    lf.lfHeight = -height;
    lf.lfWeight = weight;
    lstrcpyW(lf.lfFaceName, L"Arial");
    return CreateFontIndirectW(&lf);
}

//
// 16x16 icons as ASCII art (' ' = transparent). Rows may be short - the converter
// pads the rest transparent. Built into color-keyed DDraw surfaces at init.
//
static const char *s_iconArt[ICON_COUNT][16] =
{
    {   // ICON_COMPUTER
        "", " KKKKKKKKKK", " KssssssssK", " KssssssssK", " KssssssssK",
        " KssssssssK", " KssssssssK", " KKKKKKKKKK", "    KKKK", "   KKKKKK",
        "  KLLLLLLK", "  KKKKKKKK", "", "", "", ""
    },
    {   // ICON_DRIVE
        "", "     KKKK", "   KKccccKK", "  KcccWWcccK", "  KccWKKWccK",
        " KccWK  KWccK", " KccWK  KWccK", "  KccWKKWccK", "  KcccWWcccK", "   KKccccKK",
        "     KKKK", "", "", "", "", ""
    },
    {   // ICON_FOLDER
        "", "", "  ooo", " oYYYo", "oYYYYYoooooo", "oYYYYYYYYYYo", "oYYYYYYYYYYo",
        "oYYYYYYYYYYo", "oYYYYYYYYYYo", "oYYYYYYYYYYo", "oooooooooooo", "", "", "", "", ""
    },
    {   // ICON_APP
        "", " KKKKKKKKKKK", " KBBBBBBBBBK", " KKKKKKKKKKK", " KWWWWWWWWWK",
        " KWLLLLLLLWK", " KWLLLLLLLWK", " KWLLLLLLLWK", " KWWWWWWWWWK", " KKKKKKKKKKK",
        "", "", "", "", "", ""
    },
    {   // ICON_CLOCK
        "", "     KKKK", "   KKWWWWKK", "  KWWWWWWWWK", " KWWWKWWWWWK",
        " KWWWKWWWWWK", " KWWWKKKWWWK", " KWWWWWWWWWK", " KWWWWWWWWWK", "  KWWWWWWWWK",
        "   KKWWWWKK", "     KKKK", "", "", "", ""
    },
    {   // ICON_FILE
        "", "  KKKKKKK", "  KWWWWKKK", "  KWWWWKWK", "  KWWWWKKK", "  KWGGGGWK",
        "  KWGGGGWK", "  KWGGGGWK", "  KWGGGGWK", "  KWGGGGWK", "  KWWWWWWK", "  KKKKKKKK",
        "", "", "", ""
    },
    {   // ICON_SWIRL (Dreamcast)
        "", "     OOOOO", "   OOOOOOOOO", "  OOO     OOO", " OOO   OO   OO",
        " OO  OOOOOO  O", " OO OO    OO O", " OO OO    O  O", " OO  OO     OO",
        " OOO  OOOOOOO", "  OOO      OO", "   OOOOOOOOO", "     OOOOO", "", "", ""
    },
    {   // ICON_CURSOR (arrow)
        "K", "KK", "KWK", "KWWK", "KWWWK", "KWWWWK", "KWWWWWK", "KWWWWWWK",
        "KWWWWWWWK", "KWWWWWKKK", "KWWKWWK", "KWK KWWK", "KK   KWWK", "      KWK",
        "       K", ""
    },
};

static COLORREF PalColor(char c)
{
    switch (c)
    {
    case 'K': return RGB(0, 0, 0);
    case 'W': return RGB(255, 255, 255);
    case 'D': return RGB(96, 96, 96);
    case 'G': return RGB(160, 160, 160);
    case 'L': return RGB(208, 208, 208);
    case 'Y': return RGB(255, 206, 90);
    case 'o': return RGB(200, 150, 70);
    case 'B': return RGB(40, 80, 200);
    case 'c': return RGB(80, 200, 220);
    case 's': return RGB(100, 160, 220);
    case 'g': return RGB(40, 200, 80);
    case 'r': return RGB(210, 60, 60);
    case 'O': return RGB(235, 125, 30);    // Dreamcast swirl orange
    default:  return RGB(255, 0, 255);    // transparent color key (565 0xF81F)
    }
}

// Build one color-keyed icon surface from 16x16 art, scaled scale x (pixel-doubled).
// SetPixel/CreateBitmap/FillRect are phantom here, so we write pixels via a DDraw Lock.
static LPDIRECTDRAWSURFACE MakeIcon(const char *const *art, int scale)
{
    LPDIRECTDRAWSURFACE s = NULL;
    DDSURFACEDESC       sd, ld;
    DDCOLORKEY          ck;
    int                 size = 16 * scale, x, y, n, sx, sy;

    memset(&sd, 0, sizeof(sd));
    sd.dwSize         = sizeof(sd);
    sd.dwFlags        = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
    sd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
    sd.dwWidth        = size;
    sd.dwHeight       = size;
    sd.ddpfPixelFormat.dwSize        = sizeof(DDPIXELFORMAT);
    sd.ddpfPixelFormat.dwFlags       = DDPF_RGB;
    sd.ddpfPixelFormat.dwRGBBitCount = 16;
    sd.ddpfPixelFormat.dwRBitMask    = 0xF800;
    sd.ddpfPixelFormat.dwGBitMask    = 0x07E0;
    sd.ddpfPixelFormat.dwBBitMask    = 0x001F;
    if (IDirectDraw_CreateSurface(s_dd, &sd, &s, NULL) != DD_OK)
        return NULL;

    memset(&ld, 0, sizeof(ld));
    ld.dwSize = sizeof(ld);
    if (IDirectDrawSurface_Lock(s, NULL, &ld, DDLOCK_WAIT, NULL) == DD_OK)
    {
        BYTE *base = (BYTE *)ld.lpSurface;
        for (y = 0; y < 16; y++)
        {
            const char *row = art[y];
            for (n = 0; row[n]; n++) ;
            for (x = 0; x < 16; x++)
            {
                WORD v = ToRgb565(PalColor((x < n) ? row[x] : ' '));
                for (sy = 0; sy < scale; sy++)
                {
                    WORD *px = (WORD *)(base + (y * scale + sy) * ld.lPitch);
                    for (sx = 0; sx < scale; sx++)
                        px[x * scale + sx] = v;
                }
            }
        }
        IDirectDrawSurface_Unlock(s, NULL);
    }
    ck.dwColorSpaceLowValue  = 0xF81F;
    ck.dwColorSpaceHighValue = 0xF81F;
    IDirectDrawSurface_SetColorKey(s, DDCKEY_SRCBLT, &ck);
    return s;
}

static void BuildIcons(void)
{
    int id;
    for (id = 0; id < ICON_COUNT; id++)
    {
        s_icon[id]    = MakeIcon(s_iconArt[id], 1);
        s_iconBig[id] = MakeIcon(s_iconArt[id], 2);
    }
}

static void ReleaseIcons(void)
{
    int id;
    for (id = 0; id < ICON_COUNT; id++)
    {
        if (s_icon[id])    { IDirectDrawSurface_Release(s_icon[id]);    s_icon[id] = NULL; }
        if (s_iconBig[id]) { IDirectDrawSurface_Release(s_iconBig[id]); s_iconBig[id] = NULL; }
    }
}

static BOOL CreateSurfaces(void)
{
    DDSURFACEDESC ddsd;
    HRESULT       hr = E_FAIL;
    int           tries;

    for (tries = 0; tries < 30; tries++)
    {
        hr = DirectDrawCreate(NULL, &s_dd, NULL);
        if (hr == DD_OK)
            break;
        s_dd = NULL;
        Sleep(100);
    }
    if (hr != DD_OK)
        return FALSE;

    IDirectDraw_SetCooperativeLevel(s_dd, s_hwnd, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    IDirectDraw_SetDisplayMode(s_dd, SCREEN_W, SCREEN_H, 16);

    // Primary as a page-flip chain (front + 1 back), the way the DC samples do it.
    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize            = sizeof(ddsd);
    ddsd.dwFlags           = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
    ddsd.ddsCaps.dwCaps    = DDSCAPS_PRIMARYSURFACE | DDSCAPS_FLIP | DDSCAPS_COMPLEX;
    ddsd.dwBackBufferCount = 1;
    hr = IDirectDraw_CreateSurface(s_dd, &ddsd, &s_primary, NULL);
    if (hr == DD_OK)
    {
        DDSCAPS caps;
        memset(&caps, 0, sizeof(caps));
        caps.dwCaps = DDSCAPS_BACKBUFFER;
        if (IDirectDrawSurface_GetAttachedSurface(s_primary, &caps, &s_back) == DD_OK)
            s_useFlip = TRUE;
    }
    if (!s_useFlip)
    {
        // Fallback: plain single primary (present = Blt scene->primary).
        if (s_primary) { IDirectDrawSurface_Release(s_primary); s_primary = NULL; }
        memset(&ddsd, 0, sizeof(ddsd));
        ddsd.dwSize         = sizeof(ddsd);
        ddsd.dwFlags        = DDSD_CAPS;
        ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
        if (IDirectDraw_CreateSurface(s_dd, &ddsd, &s_primary, NULL) != DD_OK)
            return FALSE;
    }

    // Offscreen composited "scene" (explicit 565 to match the primary).
    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize         = sizeof(ddsd);
    ddsd.dwFlags        = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY;
    ddsd.dwWidth        = SCREEN_W;
    ddsd.dwHeight       = SCREEN_H;
    ddsd.ddpfPixelFormat.dwSize        = sizeof(DDPIXELFORMAT);
    ddsd.ddpfPixelFormat.dwFlags       = DDPF_RGB;
    ddsd.ddpfPixelFormat.dwRGBBitCount = 16;
    ddsd.ddpfPixelFormat.dwRBitMask    = 0xF800;
    ddsd.ddpfPixelFormat.dwGBitMask    = 0x07E0;
    ddsd.ddpfPixelFormat.dwBBitMask    = 0x001F;
    if (IDirectDraw_CreateSurface(s_dd, &ddsd, &s_scene, NULL) != DD_OK)
        s_scene = NULL;
    {
        WCHAR b[80];
        wsprintfW(b, L"GfxInit: primary=%x back=%x scene=%x flip=%d\r\n",
                  (unsigned)s_primary, (unsigned)s_back, (unsigned)s_scene, s_useFlip);
        OutputDebugStringW(b);
    }
    __try { BuildIcons(); }
    __except (EXCEPTION_EXECUTE_HANDLER) { ReleaseIcons(); OutputDebugStringW(L"GfxInit: BuildIcons faulted (Lock unimplemented?)\r\n"); }
    return TRUE;
}

static void DestroySurfaces(void)
{
    ReleaseIcons();
    if (s_scene)   { IDirectDrawSurface_Release(s_scene);   s_scene = NULL; }
    // s_back is part of the complex flip chain - freed when the primary is released.
    s_back = NULL;
    s_useFlip = FALSE;
    if (s_primary) { IDirectDrawSurface_Release(s_primary); s_primary = NULL; }
    if (s_dd)      { IDirectDraw_RestoreDisplayMode(s_dd);  IDirectDraw_Release(s_dd); s_dd = NULL; }
}

BOOL GfxInit(HWND hwnd)
{
    s_hwnd = hwnd;
    if (!CreateSurfaces())
        return FALSE;
    g_FontUI    = MakeFont(12, FW_NORMAL);
    g_FontBold  = MakeFont(12, FW_BOLD);
    g_FontTitle = MakeFont(14, FW_BOLD);
    return TRUE;
}

void GfxShutdown(void)
{
    DestroySurfaces();
}

void GfxFill(int left, int top, int right, int bottom, COLORREF color)
{
    DDBLTFX fx;
    RECT    rc;
    LPDIRECTDRAWSURFACE t = Target();

    if (!t)
        return;
    SetRect(&rc, left, top, right, bottom);
    memset(&fx, 0, sizeof(fx));
    fx.dwSize      = sizeof(fx);
    fx.dwFillColor = ToRgb565(color);
    IDirectDrawSurface_Blt(t, &rc, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
}

void GfxIcon(int id, int x, int y)
{
    RECT                d;
    LPDIRECTDRAWSURFACE t = Target();

    if (id < 0 || id >= ICON_COUNT || !s_icon[id] || !t)
        return;
    SetRect(&d, x, y, x + 16, y + 16);
    IDirectDrawSurface_Blt(t, &d, s_icon[id], NULL, DDBLT_KEYSRC | DDBLT_WAIT, NULL);
}

void GfxIconBig(int id, int x, int y)
{
    RECT                d;
    LPDIRECTDRAWSURFACE t = Target();

    if (id < 0 || id >= ICON_COUNT || !s_iconBig[id] || !t)
        return;
    SetRect(&d, x, y, x + 32, y + 32);
    IDirectDrawSurface_Blt(t, &d, s_iconBig[id], NULL, DDBLT_KEYSRC | DDBLT_WAIT, NULL);
}

void GfxBevel(const RECT *rc, BOOL raised)
{
    COLORREF tl = raised ? RGB(255, 255, 255) : RGB(128, 128, 128);
    COLORREF br = raised ? RGB(128, 128, 128) : RGB(255, 255, 255);

    GfxFill(rc->left,      rc->top,        rc->right,    rc->top + 1, tl);  // top
    GfxFill(rc->left,      rc->top,        rc->left + 1, rc->bottom,  tl);  // left
    GfxFill(rc->left,      rc->bottom - 1, rc->right,    rc->bottom,  br);  // bottom
    GfxFill(rc->right - 1, rc->top,        rc->right,    rc->bottom,  br);  // right
}

HDC GfxLockDC(void)
{
    HDC                 hdc;
    LPDIRECTDRAWSURFACE t = Target();

    if (!t || IDirectDrawSurface_GetDC(t, &hdc) != DD_OK)
        return NULL;
    return hdc;
}

void GfxUnlockDC(HDC hdc)
{
    LPDIRECTDRAWSURFACE t = Target();
    if (t && hdc)
        IDirectDrawSurface_ReleaseDC(t, hdc);
}

void GfxText(HDC hdc, int x, int y, COLORREF fg, COLORREF bg, HFONT font, const WCHAR *text)
{
    if (font)
        SelectObject(hdc, font);
    SetTextColor(hdc, fg);
    SetBkColor(hdc, bg);
    SetBkMode(hdc, OPAQUE);
    ExtTextOutW(hdc, x, y, 0, NULL, text, lstrlenW(text), NULL);
}

// Color-key blit the pointer sprite onto a target surface (clamped to bounds).
static void BlitCursor(LPDIRECTDRAWSURFACE dst, int x, int y)
{
    RECT d, s;
    int  w = 16, h = 16;
    if (!dst || !s_icon[ICON_CURSOR])
        return;
    if (x < 0) x = 0;  if (y < 0) y = 0;
    if (x + w > SCREEN_W) w = SCREEN_W - x;
    if (y + h > SCREEN_H) h = SCREEN_H - y;
    if (w <= 0 || h <= 0)
        return;
    SetRect(&d, x, y, x + w, y + h);
    SetRect(&s, 0, 0, w, h);
    IDirectDrawSurface_Blt(dst, &d, s_icon[ICON_CURSOR], &s, DDBLT_KEYSRC | DDBLT_WAIT, NULL);
}

// Present the composited scene + pointer. Flip-chain path: Blt scene->back, draw
// the cursor on the back, then Flip (vsync-paced, CPU-yielding). Fallback path
// (no flip chain): Blt scene->primary + cursor. Call ONLY when something changed
// (content dirty or cursor moved) - flip-chain surfaces retain content, so there
// is no need to present every frame. Returns TRUE if a surface was lost (the
// caller must re-render the scene before the next present).
BOOL GfxPresent(int cursorX, int cursorY, BOOL showCursor)
{
    LPDIRECTDRAWSURFACE comp;

    if (!s_scene || !s_primary)
        return FALSE;

    if (IDirectDrawSurface_IsLost(s_scene) == DDERR_SURFACELOST)
    {
        IDirectDrawSurface_Restore(s_scene);
        IDirectDrawSurface_Restore(s_primary);
        return TRUE;                              // scene content lost -> re-render
    }

    comp = s_useFlip ? s_back : s_primary;        // where we assemble the frame
    if (IDirectDrawSurface_Blt(comp, NULL, s_scene, NULL, DDBLT_WAIT, NULL) == DDERR_SURFACELOST)
    {
        IDirectDrawSurface_Restore(s_primary);
        return TRUE;
    }
    if (showCursor)
        BlitCursor(comp, cursorX, cursorY);

    if (s_useFlip)
    {
        if (IDirectDrawSurface_Flip(s_primary, NULL, DDFLIP_WAIT) == DDERR_SURFACELOST)
        {
            IDirectDrawSurface_Restore(s_primary);
            return TRUE;
        }
    }
    return FALSE;
}

void GfxLaunch(const WCHAR *path)
{
    PROCESS_INFORMATION pi;

    DestroySurfaces();   // release the exclusive display so the child can own it
    if (CreateProcessW(path, NULL, NULL, NULL, FALSE, 0, NULL, NULL, NULL, &pi))
    {
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }
    CreateSurfaces();    // take it back
}
