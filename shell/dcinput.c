//
// dcinput.c - DirectInput keyboard + pointer (see dcinput.h).
//
// Keyboard: polled each frame, edge-detected (with auto-repeat for nav keys),
// DIK scan codes mapped to VK and queued for the shell's OnKey.
// Pointer (two sources, either drives the cursor):
//   - Mouse (DC Maple mouse / host mouse in Flycast): relative deltas, 1:1.
//   - Controller analog stick: deadzone + time-based speed + sub-pixel accumulate.
// Buttons (mouse L/M/R, or controller A/B/X/Y) -> click.
//
#define CINTERFACE
#define DIRECTINPUT_VERSION 0x0500
#include <windows.h>
#include <dinput.h>
#include "dcinput.h"

#define SCRW 640
#define SCRH 480

static LPDIRECTINPUT        g_di    = NULL;
static LPDIRECTINPUTDEVICE2 g_kbd   = NULL;
static LPDIRECTINPUTDEVICE2 g_joy   = NULL;
static LPDIRECTINPUTDEVICE2 g_mouse = NULL;
static HWND                 g_hwnd  = NULL;
static int                  g_wmPointer = 0;   // a GWES WM_MOUSE* arrived -> pointer exists

static BYTE  g_now[256], g_last[256];
static DWORD g_repeatAt[256];
static int   g_cx = SCRW / 2, g_cy = SCRH / 2;
static int   g_btnLast = 0, g_mbtnLast = 0, g_click = 0, g_activate = 0;
static DWORD g_lastTick = 0;
static long  g_accX = 0, g_accY = 0;   // sub-pixel motion accumulators (axis*ms)

// DC controller buttons live in the LOW 16 bits of the DInput button mask
// (observed on Flycast): D-pad U/D/L/R = bits 3/4/5/6, face A = bit 1. The HIGH
// bits (>=16) carry unstable analog trigger/stick data the driver leaks in (idle
// has e.g. 0x03110000) - they are NOT buttons, so mask them off.
#define DPAD_BITS  0x00000078u         // bits 3..6 (U,D,L,R)
#define FACE_BITS  0x0000FF87u         // low-16 digital buttons minus the D-pad bits
static const struct { DWORD bit; DWORD vk; } s_dpad[4] =
{
    { 0x00000008u, VK_UP }, { 0x00000010u, VK_DOWN },
    { 0x00000020u, VK_LEFT }, { 0x00000040u, VK_RIGHT },
};
static int   g_dpadHeld[4];
static DWORD g_dpadRepeatAt[4];
static int   g_joyPrimed = 0, g_mousePrimed = 0;   // skip edge events on first read

#define STICK_DZ   150     // analog deadzone (range is +-1000)
#define STICK_DIV  1250    // px = axis * dt_ms / DIV; full deflection ~= 800 px/sec

static DWORD g_q[32];
static int   g_qh = 0, g_qt = 0;

static void Push(DWORD vk) { int n = (g_qh + 1) & 31; if (n != g_qt) { g_q[g_qh] = vk; g_qh = n; } }

static const struct { int dik; DWORD vk; int nav; } g_map[] =
{
    { DIK_UP, VK_UP, 1 }, { DIK_DOWN, VK_DOWN, 1 }, { DIK_LEFT, VK_LEFT, 1 }, { DIK_RIGHT, VK_RIGHT, 1 },
    { DIK_RETURN, VK_RETURN, 0 }, { DIK_ESCAPE, VK_ESCAPE, 0 }, { DIK_TAB, VK_TAB, 0 }, { DIK_BACK, VK_BACK, 1 },
    { DIK_DELETE, VK_DELETE, 0 },
    { DIK_0, '0', 0 }, { DIK_1, '1', 0 }, { DIK_2, '2', 0 }, { DIK_3, '3', 0 }, { DIK_4, '4', 0 },
    { DIK_5, '5', 0 }, { DIK_6, '6', 0 }, { DIK_7, '7', 0 }, { DIK_8, '8', 0 }, { DIK_9, '9', 0 },
    { DIK_C, 'C', 0 },
};
#define NMAP (sizeof(g_map) / sizeof(g_map[0]))

static BOOL CALLBACK EnumCb(LPCDIDEVICEINSTANCE di, LPVOID ctx)
{
    LPDIRECTINPUTDEVICE  d1 = NULL;
    LPDIRECTINPUTDEVICE2 d2 = NULL;
    DWORD                t;

    if (IDirectInput_CreateDevice(g_di, &di->guidInstance, &d1, NULL) != DI_OK)
        return DIENUM_CONTINUE;
    if (IDirectInputDevice_QueryInterface(d1, &IID_IDirectInputDevice2, (LPVOID *)&d2) != S_OK)
    {
        IDirectInputDevice_Release(d1);
        return DIENUM_CONTINUE;
    }
    IDirectInputDevice_Release(d1);

    t = GET_DIDEVICE_TYPE(di->dwDevType);
    if (t == DIDEVTYPE_KEYBOARD && !g_kbd)
    {
        IDirectInputDevice2_SetDataFormat(d2, &c_dfDIKeyboard);
        IDirectInputDevice2_Acquire(d2);
        g_kbd = d2;
        OutputDebugStringW(L"DCIN: keyboard acquired\r\n");
    }
    else if (t == DIDEVTYPE_MOUSE && !g_mouse)
    {
        HRESULT hr;
        WCHAR   b[64];
        IDirectInputDevice2_SetDataFormat(d2, &c_dfDIMouse);
        // a cooperative level is required for the mouse to deliver data (the
        // classic "acquired but GetDeviceState returns nothing" gotcha)
        IDirectInputDevice2_SetCooperativeLevel(d2, g_hwnd, DISCL_BACKGROUND | DISCL_NONEXCLUSIVE);
        hr = IDirectInputDevice2_Acquire(d2);
        g_mouse = d2;
        wsprintfW(b, L"DCIN: mouse acquired (hr=%08x)\r\n", (unsigned)hr);
        OutputDebugStringW(b);
    }
    else if (t == DIDEVTYPE_JOYSTICK && !g_joy)
    {
        DIPROPRANGE r;
        IDirectInputDevice2_SetDataFormat(d2, &c_dfDIJoystick);
        memset(&r, 0, sizeof(r));
        r.diph.dwSize       = sizeof(DIPROPRANGE);
        r.diph.dwHeaderSize = sizeof(DIPROPHEADER);
        r.diph.dwHow        = DIPH_DEVICE;
        r.lMin = -1000; r.lMax = 1000;
        IDirectInputDevice2_SetProperty(d2, DIPROP_RANGE, &r.diph);
        IDirectInputDevice2_Acquire(d2);
        g_joy = d2;
        OutputDebugStringW(L"DCIN: joystick acquired\r\n");
    }
    else
    {
        IDirectInputDevice2_Release(d2);
    }
    return DIENUM_CONTINUE;
}

BOOL DInInit(HWND hwnd)
{
    g_hwnd = hwnd;
    if (DirectInputCreate(GetModuleHandleW(NULL), DIRECTINPUT_VERSION, &g_di, NULL) != DI_OK)
    {
        OutputDebugStringW(L"DCIN: DirectInputCreate FAILED\r\n");
        return FALSE;
    }
    IDirectInput_EnumDevices(g_di, 0, EnumCb, NULL, DIEDFL_ATTACHEDONLY);
    return (g_kbd != NULL);
}

void DInShutdown(void)
{
    if (g_kbd)   { IDirectInputDevice2_Unacquire(g_kbd);   IDirectInputDevice2_Release(g_kbd);   g_kbd = NULL; }
    if (g_mouse) { IDirectInputDevice2_Unacquire(g_mouse); IDirectInputDevice2_Release(g_mouse); g_mouse = NULL; }
    if (g_joy)   { IDirectInputDevice2_Unacquire(g_joy);   IDirectInputDevice2_Release(g_joy);   g_joy = NULL; }
    if (g_di)    { IDirectInput_Release(g_di); g_di = NULL; }
}

void DInUpdate(void)
{
    DWORD nowt = GetTickCount();
    int   i;

    if (g_kbd)
    {
        for (i = 0; i < 256; i++) g_last[i] = g_now[i];
        IDirectInputDevice2_Poll(g_kbd);
        if (IDirectInputDevice2_GetDeviceState(g_kbd, 256, g_now) != DI_OK)
            IDirectInputDevice2_Acquire(g_kbd);
        else
        {
            for (i = 0; i < (int)NMAP; i++)
            {
                int  dik  = g_map[i].dik;
                BOOL down = (g_now[dik] & 0x80) != 0;
                BOOL was  = (g_last[dik] & 0x80) != 0;
                if (down && !was)            { Push(g_map[i].vk); g_repeatAt[dik] = nowt + 350; }
                else if (down && was && g_map[i].nav && nowt >= g_repeatAt[dik])
                                             { Push(g_map[i].vk); g_repeatAt[dik] = nowt + 100; }
            }
        }
    }

    if (g_mouse)
    {
        DIMOUSESTATE ms;
        IDirectInputDevice2_Poll(g_mouse);
        if (IDirectInputDevice2_GetDeviceState(g_mouse, sizeof(ms), &ms) != DI_OK)
            IDirectInputDevice2_Acquire(g_mouse);
        else
        {
            int btn;
            g_cx += ms.lX;              // relative deltas, 1:1 (snappy)
            g_cy += ms.lY;
            if (g_cx < 0) g_cx = 0;  if (g_cx >= SCRW) g_cx = SCRW - 1;
            if (g_cy < 0) g_cy = 0;  if (g_cy >= SCRH) g_cy = SCRH - 1;
            btn = (ms.rgbButtons[0] | ms.rgbButtons[1] | ms.rgbButtons[2]) & 0x80;
            if (!g_mousePrimed) { g_mousePrimed = 1; g_mbtnLast = btn; }   // baseline, no edge
            else { if (btn && !g_mbtnLast) g_click = 1; g_mbtnLast = btn; }
        }
    }

    if (g_joy)
    {
        DIJOYSTATE js;
        IDirectInputDevice2_Poll(g_joy);
        if (IDirectInputDevice2_GetDeviceState(g_joy, sizeof(js), &js) != DI_OK)
            IDirectInputDevice2_Acquire(g_joy);
        else
        {
            DWORD dt = g_lastTick ? (nowt - g_lastTick) : 16;
            int   ax = js.lX < 0 ? -js.lX : js.lX;
            int   ay = js.lY < 0 ? -js.lY : js.lY;
            DWORD mask = 0, face;
            int   i, dx, dy;
            if (dt > 100) dt = 100;     // clamp after a stall so the cursor doesn't leap
            // analog stick -> cursor: time-based + sub-pixel accumulator (speed is
            // frame-rate independent AND small deflections still move)
            if (ax > STICK_DZ) { g_accX += (long)js.lX * (long)dt; dx = (int)(g_accX / STICK_DIV); g_accX -= (long)dx * STICK_DIV; g_cx += dx; }
            else g_accX = 0;
            if (ay > STICK_DZ) { g_accY += (long)js.lY * (long)dt; dy = (int)(g_accY / STICK_DIV); g_accY -= (long)dy * STICK_DIV; g_cy += dy; }
            else g_accY = 0;
            if (g_cx < 0) g_cx = 0;  if (g_cx >= SCRW) g_cx = SCRW - 1;
            if (g_cy < 0) g_cy = 0;  if (g_cy >= SCRH) g_cy = SCRH - 1;

            for (i = 0; i < 32; i++) if (js.rgbButtons[i] & 0x80) mask |= (1u << i);
            face = mask & FACE_BITS;    // low-16 digital face buttons only (no analog noise)
            if (!g_joyPrimed)
            {
                // first sample: record baseline only, generate NO edges - otherwise a
                // button/axis transient at boot fires a phantom "activate" (which was
                // auto-launching the selected desktop icon).
                g_joyPrimed = 1;
                for (i = 0; i < 4; i++) g_dpadHeld[i] = (mask & s_dpad[i].bit) ? 1 : 0;
                g_btnLast = face ? 1 : 0;
            }
            else
            {
                // D-pad (button bits) -> arrow keys, per-direction edge + auto-repeat
                for (i = 0; i < 4; i++)
                {
                    int dn = (mask & s_dpad[i].bit) ? 1 : 0;
                    if (dn && !g_dpadHeld[i])                  { Push(s_dpad[i].vk); g_dpadRepeatAt[i] = nowt + 350; }
                    else if (dn && nowt >= g_dpadRepeatAt[i])  { Push(s_dpad[i].vk); g_dpadRepeatAt[i] = nowt + 120; }
                    g_dpadHeld[i] = dn;
                }
                // face buttons (not D-pad / idle bit) -> "activate" (Enter): the
                // controller selection paradigm (D-pad selects, A opens selection).
                if (face && !g_btnLast) g_activate = 1;
                g_btnLast = face ? 1 : 0;
            }
        }
    }
    g_lastTick = nowt;
}

int DInNextKey(DWORD *vk)
{
    if (g_qt == g_qh) return 0;
    *vk = g_q[g_qt];
    g_qt = (g_qt + 1) & 31;
    return 1;
}

// GWES window-message fallback: the DC mouse may reach us via WM_MOUSE* instead
// of DirectInput. The shell forwards those here so all paths share one cursor.
void DInSetCursor(int x, int y)
{
    if (x < 0) x = 0;  if (x >= SCRW) x = SCRW - 1;
    if (y < 0) y = 0;  if (y >= SCRH) y = SCRH - 1;
    g_cx = x; g_cy = y;
    g_wmPointer = 1;
}
void DInPostClick(void) { g_click = 1; }

int  DInTookActivate(void)      { int a = g_activate; g_activate = 0; return a; }  // controller A -> Enter
int  DInHasPointer(void)        { return (g_mouse != NULL) || (g_joy != NULL) || g_wmPointer; }
void DInCursor(int *x, int *y)  { *x = g_cx; *y = g_cy; }
int  DInTookClick(void)         { int c = g_click; g_click = 0; return c; }
