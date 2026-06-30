//
// dcgfx.c - Direct3D / PVR2 hardware compositor for the Dreamcast CE shell.
//
// d3dim.dll is the real DX6 immediate-mode HAL driving the PVR2 tile renderer
// (TA/ISP/TSP) - confirmed by the Step-1 test quad. So the whole UI is rendered as
// hardware quads instead of software rasterization:
//   - GfxFill / GfxBevel  -> flat-colored D3DTLVERTEX quads (no texture)
//   - GfxIcon / GfxText    -> textured quads from ONE ARGB4444 VRAM atlas
//   - GfxPresent           -> BeginScene -> DrawIndexedPrimitive per same-texture run
//                             (emit order = painter's Z) -> EndScene -> Flip
// No CPU framebuffer, no per-frame memcpy, no per-layer Lock drain. Glyphs + icons
// live in PVR VRAM (the atlas), off the SH-4 bus. Constraints: DP1 path only (DP2 is
// a trap stub), NO Z buffer (submit-order Z), color-key -> alpha=0 at atlas upload.
//
#define CINTERFACE          // C-style DDraw/D3D COM (dcgfx.h no longer defines this)
#include "dcgfx.h"
#include <ddraw.h>
#include <d3d.h>
#include <wdm.h>            // MmMapIoSpace / PHYSICAL_ADDRESS (page-layer framebuffer map)

// GDI framebuffer info from the display driver (ExtEscape GETGDIINFO), as in the SDK htmlsamp.
typedef struct { int width, height, stride; unsigned long physicalAddr; } GDISurfaceInfo;
#define GETGDIINFO 6500

#define KEY       0xF81F           // 565 magenta = transparent key (atlas -> alpha 0)
#define ATLAS_DIM 512
#define GFIRST    32
#define GLAST     126
#define GN        (GLAST - GFIRST + 1)
#define GH        16               // glyph cell height
// Quad storage is DYNAMIC: the scene + desktop-cache arrays grow on demand (PushQuad doubles them)
// and shrink back after the scene stays small, so RAM tracks the busiest recent frame instead of a
// worst-case BSS reservation. Text renders one quad per character, so a couple of text-heavy
// windows can need a few thousand quads; with on-demand growth nothing is ever dropped (no
// taskbar/cursor loss) up to a ceiling no real UI reaches. The WORD index buffer caps a single
// same-texture batch at 16384 quads (index 16384*4 = 65536 would overflow), which is the ceiling.
#define QUAD_INIT  256               // initial capacity (one modest window)
#define QUAD_MAX   16384             // hard ceiling (WORD index limit); far above any real scene

static HWND                s_hwnd    = NULL;
static LPDIRECTDRAW        s_dd      = NULL;
static LPDIRECTDRAWSURFACE s_primary = NULL;
static LPDIRECTDRAWSURFACE s_back    = NULL;
static BOOL                s_useFlip = FALSE;
static LPDIRECT3D2         s_d3d     = NULL;
static LPDIRECT3DDEVICE2   s_dev     = NULL;
static LPDIRECT3DVIEWPORT2 s_vp      = NULL;
static BOOL                s_d3dOk   = FALSE;

static LPDIRECTDRAWSURFACE s_atlasSurf = NULL;   // VRAM atlas (kept alive while bound)
static LPDIRECT3DTEXTURE2  s_atlasTex  = NULL;
static D3DTEXTUREHANDLE    s_hAtlas    = 0;

// Page layer (browser): GWES framebuffer wrap -> VRAM texture quad. See GfxInitPageLayer.
static LPDIRECTDRAWSURFACE s_gdiSurf  = NULL;    // system-mem surface aliasing the GDI framebuffer
static LPDIRECTDRAWSURFACE s_pageSurf = NULL;    // VRAM texture the page is blitted into
static LPDIRECT3DTEXTURE2  s_pageTex  = NULL;
static D3DTEXTUREHANDLE    s_hPage    = 0;
#define PAGE_TW 1024                              // page texture is pow2 and >= 640x480
#define PAGE_TH 512

// Retained quad list (the scene), replayed every frame; grown on demand. Indices are base-relative.
static D3DTLVERTEX *s_vb;   static WORD *s_ib;   static BYTE *s_qtex;   // tex: 0=solid 1=atlas 2=page
static int          s_cap, s_nQuad;

// Desktop cache: a cached vertex SUB-LIST (not pixels) prepended each frame; also grown on demand.
static D3DTLVERTEX *s_dvb;  static WORD *s_dib;  static BYTE *s_dtex;
static int          s_dcap, s_dQuad;
static BOOL         s_recDesk = FALSE;

typedef struct { float u0, v0, u1, v1; BYTE adv; } GlyphUV;
static GlyphUV s_glyph[3][GN];
static BOOL    s_glyphReady = FALSE;
typedef struct { float u0, v0, u1, v1; } RectUV;
static RectUV  s_iconUV[ICON_COUNT][2];          // [0]=16px, [1]=32px

HFONT g_FontUI    = NULL;
HFONT g_FontBold  = NULL;
HFONT g_FontTitle = NULL;

static WORD ToRgb565(COLORREF c)
{
    return (WORD)(((GetRValue(c) >> 3) << 11) | ((GetGValue(c) >> 2) << 5) | (GetBValue(c) >> 3));
}
static D3DCOLOR ToArgb(COLORREF c)
{
    return (D3DCOLOR)(0xFF000000u | ((DWORD)GetRValue(c) << 16) | ((DWORD)GetGValue(c) << 8) | GetBValue(c));
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

// 16x16 icons as ASCII art (' ' = transparent).
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
    {   // ICON_SWIRL
        "", "     OOOOO", "   OOOOOOOOO", "  OOO     OOO", " OOO   OO   OO",
        " OO  OOOOOO  O", " OO OO    OO O", " OO OO    O  O", " OO  OO     OO",
        " OOO  OOOOOOO", "  OOO      OO", "   OOOOOOOOO", "     OOOOO", "", "", ""
    },
    {   // ICON_CURSOR
        "K", "KK", "KWK", "KWWK", "KWWWK", "KWWWWK", "KWWWWWK", "KWWWWWWK",
        "KWWWWWWWK", "KWWWWWKKK", "KWWKWWK", "KWK KWWK", "KK   KWWK", "      KWK",
        "       K", ""
    },
};

static COLORREF PalColor(char c)
{
    switch (c)
    {
    case 'K': return RGB(0, 0, 0);        case 'W': return RGB(255, 255, 255);
    case 'D': return RGB(96, 96, 96);     case 'G': return RGB(160, 160, 160);
    case 'L': return RGB(208, 208, 208);  case 'Y': return RGB(255, 206, 90);
    case 'o': return RGB(200, 150, 70);   case 'B': return RGB(40, 80, 200);
    case 'c': return RGB(80, 200, 220);   case 's': return RGB(100, 160, 220);
    case 'g': return RGB(40, 200, 80);    case 'r': return RGB(210, 60, 60);
    case 'O': return RGB(235, 125, 30);   default:  return RGB(255, 0, 255);   // -> KEY
    }
}

// 565 -> ARGB4444; the color key becomes fully transparent.
static WORD Conv4444(WORD px565)
{
    int r5, g6, b5;
    if (px565 == KEY) return 0x0000;
    r5 = (px565 >> 11) & 0x1F;  g6 = (px565 >> 5) & 0x3F;  b5 = px565 & 0x1F;
    return (WORD)(0xF000 | ((r5 >> 1) << 8) | ((g6 >> 2) << 4) | (b5 >> 1));
}

// --- clip rect (compositor-side window clipping) --------------------------------
static float s_clipX0, s_clipY0, s_clipX1, s_clipY1;
static int   s_clipOn = 0;
void GfxSetClip(int x0, int y0, int x1, int y1)
{ s_clipX0 = (float)x0; s_clipY0 = (float)y0; s_clipX1 = (float)x1; s_clipY1 = (float)y1; s_clipOn = 1; }
void GfxClearClip(void) { s_clipOn = 0; }

// --- dynamic quad arrays --------------------------------------------------------
// Grow the three parallel arrays (verts / indices / tex-flags) to hold >= need quads, preserving
// the first `keep`. Doubling from QUAD_INIT -> amortized O(1) across a frame. Returns 0 only at the
// QUAD_MAX ceiling or on OOM (PushQuad then drops, which no real scene reaches).
static int GrowQuads(D3DTLVERTEX **vb, WORD **ib, BYTE **qt, int *cap, int need, int keep)
{
    int nc; D3DTLVERTEX *nvb; WORD *nib; BYTE *nqt;
    if (need <= *cap) return 1;
    if (need > QUAD_MAX) return 0;
    nc = *cap ? *cap : QUAD_INIT;
    while (nc < need) nc <<= 1;
    if (nc > QUAD_MAX) nc = QUAD_MAX;
    nvb = (D3DTLVERTEX *)LocalAlloc(LMEM_FIXED, (DWORD)nc * 4 * sizeof(D3DTLVERTEX));
    nib = (WORD *)       LocalAlloc(LMEM_FIXED, (DWORD)nc * 6 * sizeof(WORD));
    nqt = (BYTE *)       LocalAlloc(LMEM_FIXED, (DWORD)nc);
    if (!nvb || !nib || !nqt) { if (nvb) LocalFree(nvb); if (nib) LocalFree(nib); if (nqt) LocalFree(nqt); return 0; }
    if (keep > 0 && *vb)
    {
        memcpy(nvb, *vb, (DWORD)keep * 4 * sizeof(D3DTLVERTEX));
        memcpy(nib, *ib, (DWORD)keep * 6 * sizeof(WORD));
        memcpy(nqt, *qt, (DWORD)keep);
    }
    if (*vb) LocalFree(*vb);
    if (*ib) LocalFree(*ib);
    if (*qt) LocalFree(*qt);
    *vb = nvb; *ib = nib; *qt = nqt; *cap = nc;
    return 1;
}

// Reclaim the live scene after it has stayed small: realloc DOWN to fit `want` (no copy - the frame
// is drawn and the next rebuilds from scratch). No-op if it wouldn't shrink or if the alloc fails.
static void ShrinkScene(int want)
{
    int nc = QUAD_INIT; D3DTLVERTEX *nvb; WORD *nib; BYTE *nqt;
    while (nc < want) nc <<= 1;
    if (nc >= s_cap) return;
    nvb = (D3DTLVERTEX *)LocalAlloc(LMEM_FIXED, (DWORD)nc * 4 * sizeof(D3DTLVERTEX));
    nib = (WORD *)       LocalAlloc(LMEM_FIXED, (DWORD)nc * 6 * sizeof(WORD));
    nqt = (BYTE *)       LocalAlloc(LMEM_FIXED, (DWORD)nc);
    if (!nvb || !nib || !nqt) { if (nvb) LocalFree(nvb); if (nib) LocalFree(nib); if (nqt) LocalFree(nqt); return; }
    if (s_vb) LocalFree(s_vb);
    if (s_ib) LocalFree(s_ib);
    if (s_qtex) LocalFree(s_qtex);
    s_vb = nvb; s_ib = nib; s_qtex = nqt; s_cap = nc;
}

// --- quad list ------------------------------------------------------------------
static void PushQuad(float x0, float y0, float x1, float y1,
                     float u0, float v0, float u1, float v1, D3DCOLOR col, BYTE tex)
{
    D3DTLVERTEX *v;
    WORD        *ix;
    int          b, q;
    if (!GrowQuads(&s_vb, &s_ib, &s_qtex, &s_cap, s_nQuad + 1, s_nQuad)) return;  // only at the ceiling
    // Software clip to the active clip rect, adjusting UVs so textured quads (glyphs/icons)
    // clip cleanly instead of spilling outside a window's client area.
    if (s_clipOn)
    {
        float cx0 = x0, cy0 = y0, cx1 = x1, cy1 = y1;
        if (cx0 < s_clipX0) cx0 = s_clipX0;  if (cy0 < s_clipY0) cy0 = s_clipY0;
        if (cx1 > s_clipX1) cx1 = s_clipX1;  if (cy1 > s_clipY1) cy1 = s_clipY1;
        if (cx1 <= cx0 || cy1 <= cy0) return;                // fully outside
        if (x1 > x0 && y1 > y0)                               // re-map UVs to the clipped extent
        {
            float du = (u1 - u0), dv = (v1 - v0);
            float nu0 = u0 + du * (cx0 - x0) / (x1 - x0), nu1 = u0 + du * (cx1 - x0) / (x1 - x0);
            float nv0 = v0 + dv * (cy0 - y0) / (y1 - y0), nv1 = v0 + dv * (cy1 - y0) / (y1 - y0);
            u0 = nu0; u1 = nu1; v0 = nv0; v1 = nv1;
        }
        x0 = cx0; y0 = cy0; x1 = cx1; y1 = cy1;
    }
    b = s_nQuad * 4;
    v = &s_vb[b];
    v[0].sx=x0; v[0].sy=y0; v[0].sz=0; v[0].rhw=1; v[0].color=col; v[0].specular=0; v[0].tu=u0; v[0].tv=v0;
    v[1].sx=x1; v[1].sy=y0; v[1].sz=0; v[1].rhw=1; v[1].color=col; v[1].specular=0; v[1].tu=u1; v[1].tv=v0;
    v[2].sx=x0; v[2].sy=y1; v[2].sz=0; v[2].rhw=1; v[2].color=col; v[2].specular=0; v[2].tu=u0; v[2].tv=v1;
    v[3].sx=x1; v[3].sy=y1; v[3].sz=0; v[3].rhw=1; v[3].color=col; v[3].specular=0; v[3].tu=u1; v[3].tv=v1;
    q = s_nQuad * 6;
    ix = &s_ib[q];
    ix[0]=(WORD)b; ix[1]=(WORD)(b+1); ix[2]=(WORD)(b+2);
    ix[3]=(WORD)(b+1); ix[4]=(WORD)(b+3); ix[5]=(WORD)(b+2);
    s_qtex[s_nQuad] = tex;
    s_nQuad++;
}

// --- atlas build (one-time): glyphs via GDI raster + icons, upload to VRAM -------
static void BuildAtlas(void)
{
    DDSURFACEDESC sd, ld, lt;
    LPDIRECTDRAWSURFACE stg = NULL, tmp = NULL, dst = NULL;
    LPDIRECT3DTEXTURE2  stgTex = NULL;
    HFONT  fonts[3];
    HDC    hdc;
    BYTE  *ab;
    int    ap, id, x, y, sx, sy, scale, f, c, cols, n;

    s_glyphReady = FALSE;
    s_hAtlas = 0;
    if (!s_dd || !s_dev) return;

    memset(&sd, 0, sizeof(sd)); sd.dwSize = sizeof(sd);
    sd.dwFlags        = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
    sd.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY;
    sd.dwWidth = ATLAS_DIM; sd.dwHeight = ATLAS_DIM;
    sd.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
    sd.ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
    sd.ddpfPixelFormat.dwRGBBitCount = 16;
    sd.ddpfPixelFormat.dwRBitMask = 0x0F00; sd.ddpfPixelFormat.dwGBitMask = 0x00F0;
    sd.ddpfPixelFormat.dwBBitMask = 0x000F; sd.ddpfPixelFormat.dwRGBAlphaBitMask = 0xF000;
    if (IDirectDraw_CreateSurface(s_dd, &sd, &stg, NULL) != DD_OK || !stg)
    { OutputDebugStringW(L"atlas: staging create FAIL\r\n"); return; }

    // icons -> atlas (16px row at y=144, 32px row at y=160)
    memset(&ld, 0, sizeof(ld)); ld.dwSize = sizeof(ld);
    if (IDirectDrawSurface_Lock(stg, NULL, &ld, DDLOCK_WAIT, NULL) != DD_OK)
    { IDirectDrawSurface_Release(stg); return; }
    ab = (BYTE *)ld.lpSurface; ap = ld.lPitch;
    for (y = 0; y < ATLAS_DIM; y++) memset(ab + y * ap, 0, ATLAS_DIM * 2);
    for (id = 0; id < ICON_COUNT; id++)
        for (scale = 1; scale <= 2; scale++)
        {
            int dim = 16 * scale, cx = (scale == 1) ? id * 16 : id * 32, cy = (scale == 1) ? 144 : 160;
            RectUV *u = &s_iconUV[id][scale - 1];
            u->u0 = cx / (float)ATLAS_DIM;        u->v0 = cy / (float)ATLAS_DIM;
            u->u1 = (cx + dim) / (float)ATLAS_DIM; u->v1 = (cy + dim) / (float)ATLAS_DIM;
            for (y = 0; y < 16; y++)
            {
                const char *row = s_iconArt[id][y];
                for (n = 0; row[n]; n++) ;
                for (x = 0; x < 16; x++)
                {
                    WORD t = Conv4444(ToRgb565(PalColor((x < n) ? row[x] : ' ')));
                    for (sy = 0; sy < scale; sy++)
                        for (sx = 0; sx < scale; sx++)
                            *((WORD *)(ab + (cy + y * scale + sy) * ap) + (cx + x * scale + sx)) = t;
                }
            }
        }
    IDirectDrawSurface_Unlock(stg, NULL);

    // glyphs: render the 3 fonts to a temp 565 surface, read coverage -> atlas alpha
    cols = ATLAS_DIM / 16;
    fonts[0] = g_FontUI; fonts[1] = g_FontBold; fonts[2] = g_FontTitle;
    memset(&sd, 0, sizeof(sd)); sd.dwSize = sizeof(sd);
    sd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
    sd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
    sd.dwWidth = ATLAS_DIM; sd.dwHeight = 160;
    sd.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
    sd.ddpfPixelFormat.dwFlags = DDPF_RGB; sd.ddpfPixelFormat.dwRGBBitCount = 16;
    sd.ddpfPixelFormat.dwRBitMask = 0xF800; sd.ddpfPixelFormat.dwGBitMask = 0x07E0; sd.ddpfPixelFormat.dwBBitMask = 0x001F;
    if (IDirectDraw_CreateSurface(s_dd, &sd, &tmp, NULL) == DD_OK && tmp)
    {
        memset(&lt, 0, sizeof(lt)); lt.dwSize = sizeof(lt);
        if (IDirectDrawSurface_Lock(tmp, NULL, &lt, DDLOCK_WAIT, NULL) == DD_OK)
        { for (y = 0; y < 160; y++) memset((BYTE *)lt.lpSurface + y * lt.lPitch, 0, ATLAS_DIM * 2);
          IDirectDrawSurface_Unlock(tmp, NULL); }
        if (IDirectDrawSurface_GetDC(tmp, &hdc) == DD_OK && hdc)
        {
            SetBkColor(hdc, RGB(0, 0, 0)); SetTextColor(hdc, RGB(255, 255, 255)); SetBkMode(hdc, OPAQUE);
            for (f = 0; f < 3; f++)
            {
                SelectObject(hdc, fonts[f]);
                for (c = 0; c < GN; c++)
                { int idx = f * GN + c; WCHAR ch = (WCHAR)(GFIRST + c);
                  ExtTextOutW(hdc, (idx % cols) * 16, (idx / cols) * 16, 0, NULL, &ch, 1, NULL); }
            }
            IDirectDrawSurface_ReleaseDC(tmp, hdc);
        }
        memset(&ld, 0, sizeof(ld)); ld.dwSize = sizeof(ld);
        memset(&lt, 0, sizeof(lt)); lt.dwSize = sizeof(lt);
        if (IDirectDrawSurface_Lock(stg, NULL, &ld, DDLOCK_WAIT, NULL) == DD_OK)
        {
            ab = (BYTE *)ld.lpSurface; ap = ld.lPitch;
            if (IDirectDrawSurface_Lock(tmp, NULL, &lt, DDLOCK_WAIT, NULL) == DD_OK)
            {
                BYTE *tb = (BYTE *)lt.lpSurface; int tp = lt.lPitch;
                for (f = 0; f < 3; f++)
                    for (c = 0; c < GN; c++)
                    {
                        int idx = f * GN + c, gx = (idx % cols) * 16, gy = (idx / cols) * 16, w = 0;
                        GlyphUV *g = &s_glyph[f][c];
                        for (y = 0; y < 16; y++)
                        {
                            WORD *tr = (WORD *)(tb + (gy + y) * tp);
                            WORD *ar = (WORD *)(ab + (gy + y) * ap);
                            for (x = 0; x < 16; x++)
                            {
                                int a4 = ((tr[gx + x] >> 11) & 0x1F) >> 1;    // luminance -> 4-bit alpha
                                if (a4) { ar[gx + x] = (WORD)((a4 << 12) | 0x0FFF); if (x + 1 > w) w = x + 1; }
                            }
                        }
                        g->u0 = gx / (float)ATLAS_DIM;        g->v0 = gy / (float)ATLAS_DIM;
                        g->u1 = (gx + 16) / (float)ATLAS_DIM; g->v1 = (gy + 16) / (float)ATLAS_DIM;
                        g->adv = (BYTE)(w ? w + 1 : 4);
                    }
                IDirectDrawSurface_Unlock(tmp, NULL);
                s_glyphReady = TRUE;
            }
            IDirectDrawSurface_Unlock(stg, NULL);
        }
        IDirectDrawSurface_Release(tmp);
    }

    // upload staging -> VRAM texture
    memset(&sd, 0, sizeof(sd)); sd.dwSize = sizeof(sd);
    sd.dwFlags        = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
    sd.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_VIDEOMEMORY | DDSCAPS_ALLOCONLOAD;
    sd.dwWidth = ATLAS_DIM; sd.dwHeight = ATLAS_DIM;
    sd.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
    sd.ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
    sd.ddpfPixelFormat.dwRGBBitCount = 16;
    sd.ddpfPixelFormat.dwRBitMask = 0x0F00; sd.ddpfPixelFormat.dwGBitMask = 0x00F0;
    sd.ddpfPixelFormat.dwBBitMask = 0x000F; sd.ddpfPixelFormat.dwRGBAlphaBitMask = 0xF000;
    if (IDirectDraw_CreateSurface(s_dd, &sd, &dst, NULL) == DD_OK && dst)
    {
        if (IDirectDrawSurface_QueryInterface(stg, &IID_IDirect3DTexture2, (void **)&stgTex) == DD_OK &&
            IDirectDrawSurface_QueryInterface(dst, &IID_IDirect3DTexture2, (void **)&s_atlasTex) == DD_OK &&
            IDirect3DTexture2_Load(s_atlasTex, stgTex) == DD_OK &&
            IDirect3DTexture2_GetHandle(s_atlasTex, s_dev, &s_hAtlas) == DD_OK)
        {
            s_atlasSurf = dst;
            OutputDebugStringW(L"atlas: VRAM upload OK, handle bound\r\n");
        }
        else
        {
            OutputDebugStringW(L"atlas: QI/Load/GetHandle FAIL\r\n");
            if (s_atlasTex) { IDirect3DTexture2_Release(s_atlasTex); s_atlasTex = NULL; }
            IDirectDrawSurface_Release(dst);
        }
        if (stgTex) IDirect3DTexture2_Release(stgTex);
    }
    else OutputDebugStringW(L"atlas: VRAM dest create FAIL\r\n");
    IDirectDrawSurface_Release(stg);
}

static void D3DLog(const WCHAR *what, HRESULT hr)
{
    WCHAR b[96];
    wsprintfW(b, L"D3D: %s hr=%08x\r\n", what, (unsigned)hr);
    OutputDebugStringW(b);
}

static void InitD3D(void)
{
    HRESULT      hr;
    D3DVIEWPORT2 vp;

    s_d3dOk = FALSE;
    if (!s_dd || !s_back) { OutputDebugStringW(L"D3D: no flip back buffer\r\n"); return; }

    hr = IDirectDraw_QueryInterface(s_dd, &IID_IDirect3D2, (void **)&s_d3d);
    D3DLog(L"QI IDirect3D2", hr);
    if (hr != DD_OK || !s_d3d) return;
    hr = IDirect3D2_CreateDevice(s_d3d, &IID_IDirect3DHALDevice, s_back, &s_dev);
    D3DLog(L"CreateDevice(HAL)", hr);
    if (hr != DD_OK || !s_dev) return;
    hr = IDirect3D2_CreateViewport(s_d3d, &s_vp, NULL);
    D3DLog(L"CreateViewport", hr);
    if (hr != DD_OK || !s_vp) return;

    IDirect3DDevice2_AddViewport(s_dev, s_vp);
    memset(&vp, 0, sizeof(vp));
    vp.dwSize = sizeof(vp);
    vp.dwWidth = SCREEN_W; vp.dwHeight = SCREEN_H;
    vp.dvClipX = -1.0f; vp.dvClipY = 1.0f; vp.dvClipWidth = 2.0f; vp.dvClipHeight = 2.0f;
    vp.dvMinZ = 0.0f; vp.dvMaxZ = 1.0f;
    IDirect3DViewport2_SetViewport2(s_vp, &vp);
    IDirect3DDevice2_SetCurrentViewport(s_dev, s_vp);

    IDirect3DDevice2_SetRenderState(s_dev, D3DRENDERSTATE_ZENABLE,          FALSE);
    IDirect3DDevice2_SetRenderState(s_dev, D3DRENDERSTATE_CULLMODE,         D3DCULL_NONE);
    IDirect3DDevice2_SetRenderState(s_dev, D3DRENDERSTATE_SHADEMODE,        D3DSHADE_FLAT);
    IDirect3DDevice2_SetRenderState(s_dev, D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
    IDirect3DDevice2_SetRenderState(s_dev, D3DRENDERSTATE_SRCBLEND,         D3DBLEND_SRCALPHA);
    IDirect3DDevice2_SetRenderState(s_dev, D3DRENDERSTATE_DESTBLEND,        D3DBLEND_INVSRCALPHA);
    IDirect3DDevice2_SetRenderState(s_dev, D3DRENDERSTATE_TEXTUREMAPBLEND,  D3DTBLEND_MODULATEALPHA);
    IDirect3DDevice2_SetRenderState(s_dev, D3DRENDERSTATE_TEXTUREMIN,       D3DFILTER_NEAREST);
    IDirect3DDevice2_SetRenderState(s_dev, D3DRENDERSTATE_TEXTUREMAG,       D3DFILTER_NEAREST);
    IDirect3DDevice2_SetRenderState(s_dev, D3DRENDERSTATE_TEXTUREHANDLE,    0);

    s_d3dOk = TRUE;
    OutputDebugStringW(L"D3D: device UP - PVR2 HW compositor\r\n");
}

static void DestroyD3D(void)
{
    if (s_atlasTex)  { IDirect3DTexture2_Release(s_atlasTex);  s_atlasTex  = NULL; }
    if (s_atlasSurf) { IDirectDrawSurface_Release(s_atlasSurf); s_atlasSurf = NULL; }
    s_hAtlas = 0;
    if (s_dev && s_vp) IDirect3DDevice2_DeleteViewport(s_dev, s_vp);
    if (s_vp)  { IDirect3DViewport2_Release(s_vp); s_vp  = NULL; }
    if (s_dev) { IDirect3DDevice2_Release(s_dev);  s_dev = NULL; }
    if (s_d3d) { IDirect3D2_Release(s_d3d);        s_d3d = NULL; }
    s_d3dOk = FALSE;
}

static BOOL CreateSurfaces(void)
{
    DDSURFACEDESC ddsd;
    HRESULT       hr = E_FAIL;
    int           tries;

    for (tries = 0; tries < 30; tries++)
    {
        hr = DirectDrawCreate(NULL, &s_dd, NULL);
        if (hr == DD_OK) break;
        s_dd = NULL;
        Sleep(100);
    }
    if (hr != DD_OK) return FALSE;

    IDirectDraw_SetCooperativeLevel(s_dd, s_hwnd, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    IDirectDraw_SetDisplayMode(s_dd, SCREEN_W, SCREEN_H, 16);

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize            = sizeof(ddsd);
    ddsd.dwFlags           = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
    ddsd.ddsCaps.dwCaps    = DDSCAPS_PRIMARYSURFACE | DDSCAPS_FLIP | DDSCAPS_COMPLEX | DDSCAPS_3DDEVICE;
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
        WCHAR b[64];
        wsprintfW(b, L"GfxInit: flip+3DDEVICE primary FAIL hr=%08x\r\n", (unsigned)hr);
        OutputDebugStringW(b);
        return FALSE;
    }
    return TRUE;
}

static void DestroySurfaces(void)
{
    s_hPage = 0;                                  // page layer (browser only; NULL elsewhere)
    if (s_pageTex)  { IDirect3DTexture2_Release(s_pageTex);   s_pageTex  = NULL; }
    if (s_pageSurf) { IDirectDrawSurface_Release(s_pageSurf); s_pageSurf = NULL; }
    if (s_gdiSurf)  { IDirectDrawSurface_Release(s_gdiSurf);  s_gdiSurf  = NULL; }
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
    InitD3D();
    if (!s_d3dOk)
        return FALSE;                       // hard requirement now (no CPU fallback path)
    g_FontUI    = MakeFont(12, FW_NORMAL);
    g_FontBold  = MakeFont(12, FW_BOLD);
    g_FontTitle = MakeFont(14, FW_BOLD);
    __try { BuildAtlas(); }
    __except (EXCEPTION_EXECUTE_HANDLER) { s_glyphReady = FALSE; OutputDebugStringW(L"GfxInit: BuildAtlas faulted\r\n"); }
    return TRUE;
}

void GfxShutdown(void)
{
    DestroyD3D();
    DestroySurfaces();
    if (s_vb)  LocalFree(s_vb);   if (s_ib)  LocalFree(s_ib);   if (s_qtex) LocalFree(s_qtex);
    if (s_dvb) LocalFree(s_dvb);  if (s_dib) LocalFree(s_dib);  if (s_dtex) LocalFree(s_dtex);
    s_vb = s_dvb = NULL; s_ib = s_dib = NULL; s_qtex = s_dtex = NULL;
    s_cap = s_dcap = s_nQuad = s_dQuad = 0;
}

// --- public draw API: enqueue quads (no pixels touched) -------------------------
void GfxFill(int left, int top, int right, int bottom, COLORREF color)
{
    if (right <= left || bottom <= top) return;
    PushQuad((float)left, (float)top, (float)right, (float)bottom, 0,0,0,0, ToArgb(color), 0);
}

void GfxBevel(const RECT *rc, BOOL raised)
{
    COLORREF tl = raised ? RGB(255, 255, 255) : RGB(128, 128, 128);
    COLORREF br = raised ? RGB(128, 128, 128) : RGB(255, 255, 255);
    GfxFill(rc->left,      rc->top,        rc->right,    rc->top + 1, tl);
    GfxFill(rc->left,      rc->top,        rc->left + 1, rc->bottom,  tl);
    GfxFill(rc->left,      rc->bottom - 1, rc->right,    rc->bottom,  br);
    GfxFill(rc->right - 1, rc->top,        rc->right,    rc->bottom,  br);
}

void GfxIcon(int id, int x, int y)
{
    RectUV *u;
    if (id < 0 || id >= ICON_COUNT) return;
    u = &s_iconUV[id][0];
    PushQuad((float)x, (float)y, (float)(x + 16), (float)(y + 16), u->u0, u->v0, u->u1, u->v1, 0xFFFFFFFF, 1);
}

void GfxIconBig(int id, int x, int y)
{
    RectUV *u;
    if (id < 0 || id >= ICON_COUNT) return;
    u = &s_iconUV[id][1];
    PushQuad((float)x, (float)y, (float)(x + 32), (float)(y + 32), u->u0, u->v0, u->u1, u->v1, 0xFFFFFFFF, 1);
}

HDC  GfxLockDC(void)         { return (HDC)1; }   // text targets the quad list now
void GfxUnlockDC(HDC hdc)    { (void)hdc; }

void GfxText(HDC hdc, int x, int y, COLORREF fg, COLORREF bg, HFONT font, const WCHAR *text)
{
    int      fi = (font == g_FontBold) ? 1 : (font == g_FontTitle) ? 2 : 0;
    D3DCOLOR fgc = ToArgb(fg), bgc = ToArgb(bg);
    const WCHAR *p;
    int      runW = 0, cx;

    (void)hdc;
    if (!s_glyphReady) return;
    for (p = text; *p; p++)
    { WCHAR ch = (*p < GFIRST || *p > GLAST) ? '?' : *p; runW += s_glyph[fi][ch - GFIRST].adv; }
    if (runW > 0)                                   // one opaque bg quad for the whole run
        PushQuad((float)x, (float)y, (float)(x + runW), (float)(y + GH), 0,0,0,0, bgc, 0);
    cx = x;
    for (p = text; *p; p++)
    {
        WCHAR    ch = (*p < GFIRST || *p > GLAST) ? '?' : *p;
        GlyphUV *g  = &s_glyph[fi][ch - GFIRST];
        PushQuad((float)cx, (float)y, (float)(cx + 16), (float)(y + GH), g->u0, g->v0, g->u1, g->v1, fgc, 1);
        cx += g->adv;
    }
}

int GfxTextWidth(HFONT font, const WCHAR *text)
{
    int fi = (font == g_FontBold) ? 1 : (font == g_FontTitle) ? 2 : 0, w = 0;
    const WCHAR *p;
    if (!s_glyphReady) return 0;
    for (p = text; *p; p++)
    { WCHAR ch = (*p < GFIRST || *p > GLAST) ? '?' : *p; w += s_glyph[fi][ch - GFIRST].adv; }
    return w;
}

// --- desktop cache as a vertex sub-list -----------------------------------------
void GfxBeginDesktopCache(void)
{
    s_recDesk = TRUE;
    s_nQuad = 0;                                     // record desktop quads from index 0
}

void GfxEndDesktopCache(void)
{
    s_dQuad = s_nQuad;
    if (!GrowQuads(&s_dvb, &s_dib, &s_dtex, &s_dcap, s_dQuad, 0)) s_dQuad = s_dcap;  // fit the desktop
    if (s_dQuad > 0)
    {
        memcpy(s_dvb, s_vb, s_dQuad * 4 * sizeof(D3DTLVERTEX));
        memcpy(s_dib, s_ib, s_dQuad * 6 * sizeof(WORD));
        memcpy(s_dtex, s_qtex, s_dQuad);
    }
    s_recDesk = FALSE;
    s_nQuad = 0;
}

void GfxBlitDesktopCache(void)
{
    if (s_dQuad <= 0) { s_nQuad = 0; return; }
    if (!GrowQuads(&s_vb, &s_ib, &s_qtex, &s_cap, s_dQuad, 0)) { s_nQuad = 0; return; }  // room for it
    memcpy(s_vb, s_dvb, s_dQuad * 4 * sizeof(D3DTLVERTEX));
    memcpy(s_ib, s_dib, s_dQuad * 6 * sizeof(WORD));   // base-relative indices stay valid at slot 0
    memcpy(s_qtex, s_dtex, s_dQuad);
    s_nQuad = s_dQuad;
}

// --- present: the only D3D-talking path -----------------------------------------
static int s_ghostIcon = -1;
void GfxSetDragGhost(int iconId) { s_ghostIcon = iconId; }

BOOL GfxPresent(int cursorX, int cursorY, BOOL showCursor)
{
    int i;
    if (!s_d3dOk || !s_dev || !s_primary) return FALSE;

    if (s_ghostIcon >= 0 && s_ghostIcon < ICON_COUNT)  // drag ghost: translucent 32x32 under cursor
    {
        RectUV *u = &s_iconUV[s_ghostIcon][1];
        int gx = cursorX - 6, gy = cursorY - 4;        // carried just below-right of the pointer tip
        PushQuad((float)gx, (float)gy, (float)(gx + 32), (float)(gy + 32),
                 u->u0, u->v0, u->u1, u->v1, 0xB0FFFFFF, 1);   // ~0.69 alpha (blend is SRCALPHA)
    }

    if (showCursor)                                  // cursor = ICON_CURSOR quad, drawn last (top)
    {
        RectUV *u = &s_iconUV[ICON_CURSOR][0];
        float x1, y1;
        // Clamp the hotspot on-screen and clip the 16x16 sprite to the screen so the pointer
        // never draws past the edges (the arrow's tip is the top-left hotspot, so it can sit
        // right at an edge; we just don't extend the quad beyond the framebuffer).
        if (cursorX < 0) cursorX = 0; else if (cursorX > SCREEN_W - 1) cursorX = SCREEN_W - 1;
        if (cursorY < 0) cursorY = 0; else if (cursorY > SCREEN_H - 1) cursorY = SCREEN_H - 1;
        x1 = (float)(cursorX + 16); if (x1 > SCREEN_W) x1 = SCREEN_W;
        y1 = (float)(cursorY + 16); if (y1 > SCREEN_H) y1 = SCREEN_H;
        PushQuad((float)cursorX, (float)cursorY, x1, y1,
                 u->u0, u->v0,
                 u->u0 + (u->u1 - u->u0) * (x1 - cursorX) / 16.0f,   // scale UV to the clipped extent
                 u->v0 + (u->v1 - u->v0) * (y1 - cursorY) / 16.0f, 0xFFFFFFFF, 1);
    }

    if (IDirect3DDevice2_BeginScene(s_dev) == D3D_OK)
    {
        i = 0;
        while (i < s_nQuad)                          // emit-order batches: run per texture state
        {
            BYTE tex = s_qtex[i];
            int  j = i;
            D3DTEXTUREHANDLE th;
            while (j < s_nQuad && s_qtex[j] == tex) j++;
            th = (tex == 2) ? s_hPage : (tex == 1) ? s_hAtlas : 0;   // 2 = page, 1 = atlas, 0 = solid
            IDirect3DDevice2_SetRenderState(s_dev, D3DRENDERSTATE_TEXTUREHANDLE, th);
            // Pass ONLY this run's vertices (&s_vb[i*4], (j-i)*4) with the base-relative
            // index template s_ib[0..]. Passing the whole array each batch made the TA
            // re-transform every vertex per batch -> O(verts x batches) -> the 56ms spike.
            IDirect3DDevice2_DrawIndexedPrimitive(s_dev, D3DPT_TRIANGLELIST, D3DVT_TLVERTEX,
                                                  &s_vb[i * 4], (j - i) * 4, s_ib, (j - i) * 6,
                                                  D3DDP_DONOTCLIP | D3DDP_DONOTLIGHT);
            i = j;
        }
        IDirect3DDevice2_EndScene(s_dev);
    }

    {   // reclaim RAM when the scene has stayed well under capacity (hysteresis: a ~120-frame window
        // so a brief burst of windows doesn't thrash grow<->shrink). want = recent peak + ~50% slack.
        static int peak, frames;
        if (s_nQuad > peak) peak = s_nQuad;
        if (++frames >= 120)
        {
            int want = peak + (peak >> 1) + 32;
            if (s_cap > QUAD_INIT && want < s_cap / 2) ShrinkScene(want);
            peak = 0; frames = 0;
        }
    }
    s_nQuad = 0;

    if (IDirectDrawSurface_Flip(s_primary, NULL, DDFLIP_WAIT) == DDERR_SURFACELOST)
    {
        IDirectDrawSurface_Restore(s_primary);
        return TRUE;
    }
    return FALSE;
}

// Block until the PVR vertical blank (~60Hz). Used to pace the shell loop instead
// of Sleep (which rounds up to the ~50ms CE system tick = the 20fps cap).
HRESULT GfxWaitVBlank(void)
{
    if (!s_dd) return E_FAIL;
    return IDirectDraw_WaitForVerticalBlank(s_dd, DDWAITVB_BLOCKBEGIN, NULL);
}

//
// Page layer: alias the GWES GDI framebuffer as a system-mem DDraw surface, and create a VRAM
// texture the page region is BltFast'd into each frame. Mirrors the SDK htmlsamp draw.cpp trick
// (GETGDIINFO -> MmMapIoSpace -> IDirectDrawSurface3::SetSurfaceDesc(lpSurface=...)).
//
BOOL GfxInitPageLayer(void)
{
    GDISurfaceInfo        info;
    DDSURFACEDESC         sd;
    LPDIRECTDRAWSURFACE   sys = NULL;
    LPDIRECTDRAWSURFACE3  sys3 = NULL;
    PHYSICAL_ADDRESS      pa;
    void                 *bits;
    HDC                   dc;

    if (!s_dd || !s_dev) return FALSE;

    dc = GetDC(NULL);
    memset(&info, 0, sizeof(info));
    if (!dc || ExtEscape(dc, GETGDIINFO, 0, 0, sizeof(info), (LPSTR)&info) <= 0)
    { if (dc) ReleaseDC(NULL, dc); OutputDebugStringW(L"page: GETGDIINFO FAIL\r\n"); return FALSE; }
    ReleaseDC(NULL, dc);

    { WCHAR b[128]; wsprintfW(b, L"page: GETGDIINFO w=%d h=%d stride=%d phys=%08x\r\n",
        info.width, info.height, info.stride, (unsigned)info.physicalAddr); OutputDebugStringW(b); }

    pa.HighPart = 0; pa.LowPart = info.physicalAddr;
    bits = MmMapIoSpace(pa, (ULONG)(info.height * info.stride), TRUE);
    if (!bits) { OutputDebugStringW(L"page: MmMapIoSpace FAIL\r\n"); return FALSE; }

    // system-mem surface, then re-point it at the GDI framebuffer via IDirectDrawSurface3.
    // Pin the format to 565 so BltFast to the 565 page texture is a straight copy (a format
    // mismatch shows up as vertical RGB striping over the recognizable image).
    memset(&sd, 0, sizeof(sd)); sd.dwSize = sizeof(sd);
    sd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
    sd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
    sd.dwWidth = info.width; sd.dwHeight = info.height;
    if (IDirectDraw_CreateSurface(s_dd, &sd, &sys, NULL) != DD_OK || !sys)
    { OutputDebugStringW(L"page: gdi surface create FAIL\r\n"); return FALSE; }
    if (IDirectDrawSurface_QueryInterface(sys, &IID_IDirectDrawSurface3, (void **)&sys3) == DD_OK && sys3)
    {
        memset(&sd, 0, sizeof(sd)); sd.dwSize = sizeof(sd);
        sd.dwFlags    = DDSD_HEIGHT | DDSD_WIDTH | DDSD_LPSURFACE | DDSD_PITCH | DDSD_PIXELFORMAT;
        sd.dwWidth    = info.width; sd.dwHeight = info.height;
        sd.lPitch     = info.stride; sd.lpSurface = bits;
        sd.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
        sd.ddpfPixelFormat.dwFlags = DDPF_RGB; sd.ddpfPixelFormat.dwRGBBitCount = 16;
        sd.ddpfPixelFormat.dwRBitMask = 0xF800; sd.ddpfPixelFormat.dwGBitMask = 0x07E0; sd.ddpfPixelFormat.dwBBitMask = 0x001F;
        if (IDirectDrawSurface3_SetSurfaceDesc(sys3, &sd, 0) != DD_OK)
            OutputDebugStringW(L"page: SetSurfaceDesc FAIL\r\n");
        IDirectDrawSurface3_Release(sys3);
    }
    s_gdiSurf = sys;

    // VRAM texture (565) the page is blitted into; bound as a normal compositor texture
    memset(&sd, 0, sizeof(sd)); sd.dwSize = sizeof(sd);
    sd.dwFlags        = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
    sd.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_VIDEOMEMORY;
    sd.dwWidth = PAGE_TW; sd.dwHeight = PAGE_TH;
    sd.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
    sd.ddpfPixelFormat.dwFlags = DDPF_RGB; sd.ddpfPixelFormat.dwRGBBitCount = 16;
    sd.ddpfPixelFormat.dwRBitMask = 0xF800; sd.ddpfPixelFormat.dwGBitMask = 0x07E0; sd.ddpfPixelFormat.dwBBitMask = 0x001F;
    if (IDirectDraw_CreateSurface(s_dd, &sd, &s_pageSurf, NULL) != DD_OK || !s_pageSurf)
    { OutputDebugStringW(L"page: vram texture create FAIL\r\n"); return FALSE; }
    if (IDirectDrawSurface_QueryInterface(s_pageSurf, &IID_IDirect3DTexture2, (void **)&s_pageTex) != DD_OK ||
        IDirect3DTexture2_GetHandle(s_pageTex, s_dev, &s_hPage) != DD_OK)
    { OutputDebugStringW(L"page: texture handle FAIL\r\n"); return FALSE; }

    OutputDebugStringW(L"page: layer ready\r\n");
    return TRUE;
}

// Blit a GDI-framebuffer rect into the page texture and queue it as a compositor quad.
void GfxBlitPage(int sx, int sy, int sw, int sh, int dx, int dy, int dw, int dh)
{
    RECT src;
    if (!s_gdiSurf || !s_pageSurf || !s_hPage) return;
    if (sw > PAGE_TW) sw = PAGE_TW;
    if (sh > PAGE_TH) sh = PAGE_TH;
    src.left = sx; src.top = sy; src.right = sx + sw; src.bottom = sy + sh;
    IDirectDrawSurface_BltFast(s_pageSurf, 0, 0, s_gdiSurf, &src, DDBLTFAST_WAIT);
    PushQuad((float)dx, (float)dy, (float)(dx + dw), (float)(dy + dh),
             0.0f, 0.0f, (float)sw / PAGE_TW, (float)sh / PAGE_TH, 0xFFFFFFFF, 2);
}

void GfxLaunch(const WCHAR *path)
{
    PROCESS_INFORMATION pi;

    DestroyD3D();
    DestroySurfaces();
    if (CreateProcessW(path, NULL, NULL, NULL, FALSE, 0, NULL, NULL, NULL, &pi))
    {
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }
    if (CreateSurfaces()) { InitD3D(); BuildAtlas(); }
}
