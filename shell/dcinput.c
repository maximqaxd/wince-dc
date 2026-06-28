//
// dcinput.c - DirectInput keyboard + pointer (see dcinput.h).
//
// Keyboard: polled each frame, edge-detected (with auto-repeat for nav keys),
// DIK scan codes mapped to VK and queued for the shell's OnKey.
// Pointer (two sources, either drives the cursor):
//   - Mouse (DC Maple mouse / host-injected mouse): relative deltas, 1:1.
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
static HANDLE               g_newDev = NULL;   // maple "MAPLE_NEW_DEVICE" hotplug event
static DWORD                g_reEnumUntil = 0; // poll-rescan for devices until this tick

static BYTE  g_now[256], g_last[256];
static DWORD g_repeatAt[256];
static int   g_cx = SCRW / 2, g_cy = SCRH / 2;
static int   g_btnLast = 0, g_mbtnLast = 0, g_click = 0, g_activate = 0;
static int   g_mHeld = 0, g_jHeld = 0;   // pointer button currently HELD (mouse-L / controller-A) for drag
static DWORD g_lastTick = 0;
static long  g_accX = 0, g_accY = 0;   // sub-pixel motion accumulators (axis*ms)

// DC controller buttons are identified by Maple HID USAGE (inc\maplusag.h), NOT by a fixed
// rgbButtons[] bit position - the array index depends on enumeration order and varies. So we
// build a usage->index map at acquire time via EnumObjects, exactly like the SDK
// samples\dinput\Controller. (The old hardcoded FACE_BITS/DPAD_BITS were wrong-order
// guesses, which is why A never worked on a real pad.)  usage = 0xFF00 + index:
#define USG_FIRST   0xFF00
#define USG_A       0
#define USG_B       1
#define USG_START   3
#define USG_LA      4              // D-pad left
#define USG_RA      5              // D-pad right
#define USG_DA      6              // D-pad down
#define USG_UA      7              // D-pad up
#define USG_X       8
#define USG_Y       9
#define USG_N       24
static signed char g_btnIdx[USG_N];                 // Maple usage index -> rgbButtons[] index (-1 absent)
static int         g_btnEnum;                        // running button count while enumerating
static const DWORD s_dpadVk[4] = { VK_UP, VK_DOWN, VK_LEFT, VK_RIGHT };  // matches dn[] order below
// 1 if the button with Maple usage 'u' is pressed in joystate 'js'.
#define JBTN(js, u) ((g_btnIdx[u] >= 0 && ((js).rgbButtons[g_btnIdx[u]] & 0x80)) ? 1 : 0)
static int   g_dpadHeld[4];
static DWORD g_dpadRepeatAt[4];
static int   g_mousePrimed = 0;                    // skip edge events on first read
static DWORD g_joyPrimeUntil = 0;                  // joy: track baseline (no edges) until this tick
#define JOY_PRIME_MS 400                           // startup settle window for the controller

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

// EnumObjects callback: record each button's rgbButtons[] index (its ordinal among buttons)
// keyed by Maple HID usage, so we can read A/B/X/Y/Start/D-pad regardless of pad layout.
static BOOL CALLBACK JoyObjCb(LPCDIDEVICEOBJECTINSTANCE o, LPVOID ctx)
{
    (void)ctx;
    if (LOBYTE(LOWORD(o->dwType)) & DIDFT_BUTTON)
    {
        int u = (int)o->wUsage - USG_FIRST;
        if (u >= 0 && u < USG_N) g_btnIdx[u] = (signed char)g_btnEnum;
        g_btnEnum++;
    }
    return DIENUM_CONTINUE;
}

static BOOL CALLBACK EnumCb(LPCDIDEVICEINSTANCE di, LPVOID ctx)
{
    LPDIRECTINPUTDEVICE  d1 = NULL;
    LPDIRECTINPUTDEVICE2 d2 = NULL;
    DWORD                t = GET_DIDEVICE_TYPE(di->dwDevType);
    (void)ctx;

    // Log each device DInput reports (before CreateDevice) - but only types we don't yet
    // have, so the 1s hotplug re-scan doesn't spam; we still see a mouse the moment it appears.
    {
        int have = (t == DIDEVTYPE_KEYBOARD && g_kbd) || (t == DIDEVTYPE_MOUSE && g_mouse) ||
                   (t == DIDEVTYPE_JOYSTICK && g_joy);
        if (!have) { WCHAR b[96]; wsprintfW(b, L"DCIN: enum type=%u (2=mouse 3=kbd 4=joy) dwDevType=%08x\r\n",
            (unsigned)t, (unsigned)di->dwDevType); OutputDebugStringW(b); }
    }

    if (IDirectInput_CreateDevice(g_di, &di->guidInstance, &d1, NULL) != DI_OK)
    { OutputDebugStringW(L"DCIN:   CreateDevice FAILED\r\n"); return DIENUM_CONTINUE; }
    if (IDirectInputDevice_QueryInterface(d1, &IID_IDirectInputDevice2, (LPVOID *)&d2) != S_OK)
    {
        OutputDebugStringW(L"DCIN:   QueryInterface(IDirectInputDevice2) FAILED\r\n");
        IDirectInputDevice_Release(d1);
        return DIENUM_CONTINUE;
    }
    IDirectInputDevice_Release(d1);
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
        // DC/CE mouse: SetDataFormat + Acquire ONLY. The SDK (samples\misc\DesktopCompat)
        // wraps SetCooperativeLevel in #ifndef UNDER_CE - it is NOT set on the Dreamcast.
        // Our previous SetCooperativeLevel(DISCL_BACKGROUND) made the mouse deliver no data
        // (c_dfDIMouse is relative-axis by default, which is what we want for a pointer).
        IDirectInputDevice2_SetDataFormat(d2, &c_dfDIMouse);
        hr = IDirectInputDevice2_Acquire(d2);
        g_mouse = d2;
        wsprintfW(b, L"DCIN: mouse acquired (hr=%08x)\r\n", (unsigned)hr);
        OutputDebugStringW(b);
    }
    else if (t == DIDEVTYPE_JOYSTICK && !g_joy)
    {
        DIPROPRANGE r;
        int i;
        // Build the Maple usage -> rgbButtons[] index map BEFORE SetDataFormat (SDK order):
        // EnumObjects visits each button; its ordinal among buttons is its rgbButtons[] index.
        for (i = 0; i < USG_N; i++) g_btnIdx[i] = -1;
        g_btnEnum = 0;
        IDirectInputDevice2_EnumObjects(d2, JoyObjCb, NULL, DIDFT_BUTTON);
        IDirectInputDevice2_SetDataFormat(d2, &c_dfDIJoystick);
        memset(&r, 0, sizeof(r));
        r.diph.dwSize       = sizeof(DIPROPRANGE);
        r.diph.dwHeaderSize = sizeof(DIPROPHEADER);
        r.diph.dwHow        = DIPH_DEVICE;
        r.lMin = -1000; r.lMax = 1000;
        IDirectInputDevice2_SetProperty(d2, DIPROP_RANGE, &r.diph);
        IDirectInputDevice2_Acquire(d2);
        g_joy = d2;
        { WCHAR b[96]; wsprintfW(b, L"DCIN: joystick acquired, %d buttons (A@%d B@%d X@%d Start@%d)\r\n",
            g_btnEnum, g_btnIdx[USG_A], g_btnIdx[USG_B], g_btnIdx[USG_X], g_btnIdx[USG_START]);
          OutputDebugStringW(b); }
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
    // Maple hotplug: a mouse/controller on a later port often registers with DInput a moment
    // AFTER boot, so the initial enum may only see the port-A keyboard. The SDK re-enumerates
    // when the driver signals "MAPLE_NEW_DEVICE"; we also poll-rescan for a few seconds.
    g_newDev = CreateEventW(NULL, FALSE, FALSE, L"MAPLE_NEW_DEVICE");
    g_reEnumUntil = GetTickCount() + 10000;
    // Flags = 0 (DIEDFL_ALLDEVICES), NOT DIEDFL_ATTACHEDONLY (the SDK uses 0; ATTACHEDONLY
    // dropped the mouse). The maple driver only reports actually-present devices.
    IDirectInput_EnumDevices(g_di, 0, EnumCb, NULL, 0);
    { WCHAR b[80]; wsprintfW(b, L"DCIN: found kbd=%d mouse=%d joy=%d\r\n",
        g_kbd ? 1 : 0, g_mouse ? 1 : 0, g_joy ? 1 : 0); OutputDebugStringW(b); }
    g_joyPrimeUntil = GetTickCount() + JOY_PRIME_MS;   // settle the controller before edge-detecting
    return (g_kbd != NULL);
}

void DInShutdown(void)
{
    if (g_kbd)   { IDirectInputDevice2_Unacquire(g_kbd);   IDirectInputDevice2_Release(g_kbd);   g_kbd = NULL; }
    if (g_mouse) { IDirectInputDevice2_Unacquire(g_mouse); IDirectInputDevice2_Release(g_mouse); g_mouse = NULL; }
    if (g_joy)   { IDirectInputDevice2_Unacquire(g_joy);   IDirectInputDevice2_Release(g_joy);   g_joy = NULL; }
    if (g_di)    { IDirectInput_Release(g_di); g_di = NULL; }
}

// Hand the input devices to a launched full-screen app (e.g. a DirectInput game):
// drop OUR acquisition so the app's Acquire (often EXCLUSIVE) succeeds. We keep the
// device objects so we can re-Acquire when the app exits (DInReacquire).
void DInRelease(void)
{
    if (g_kbd)   IDirectInputDevice2_Unacquire(g_kbd);
    if (g_mouse) IDirectInputDevice2_Unacquire(g_mouse);
    if (g_joy)   IDirectInputDevice2_Unacquire(g_joy);
    OutputDebugStringW(L"DCIN: released input to app\r\n");
}

// App exited: take input back. Re-prime edge detection + clear the key snapshot so a
// key/button still held at hand-back doesn't fire a phantom press into the shell.
void DInReacquire(void)
{
    if (g_kbd)   IDirectInputDevice2_Acquire(g_kbd);
    if (g_mouse) IDirectInputDevice2_Acquire(g_mouse);
    if (g_joy)   IDirectInputDevice2_Acquire(g_joy);
    memset(g_now, 0, sizeof(g_now));
    memset(g_last, 0, sizeof(g_last));
    g_joyPrimeUntil = GetTickCount() + JOY_PRIME_MS; g_mousePrimed = 0;
    g_qh = g_qt = 0;                 // drop any queued keys from before the hand-off
    OutputDebugStringW(L"DCIN: reacquired input\r\n");
}

void DInUpdate(void)
{
    DWORD nowt = GetTickCount();
    int   i;

    // Hotplug re-scan: pick up a mouse/controller that registered after the initial enum.
    // Trigger on the maple MAPLE_NEW_DEVICE event, and poll-rescan every 1s for the first 10s.
    {
        static DWORD lastScan = 0;
        int evt = (g_newDev && WaitForSingleObject(g_newDev, 0) != WAIT_TIMEOUT);
        if (g_di && (evt || (nowt < g_reEnumUntil && nowt - lastScan >= 1000)))
        {
            lastScan = nowt;
            IDirectInput_EnumDevices(g_di, 0, EnumCb, NULL, 0);
        }
    }

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
            // DI mouse drives the pointer button as a HELD state -> the shell's press/drag/
            // release model handles click vs drag. (g_click stays for the GWES WM-tap path.)
            g_mHeld = btn ? 1 : 0;
            (void)g_mbtnLast; (void)g_mousePrimed;
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
            int   i, dx, dy, face, dn[4];
            if (dt > 100) dt = 100;     // clamp after a stall so the cursor doesn't leap
            // analog stick -> cursor: time-based + sub-pixel accumulator (speed is
            // frame-rate independent AND small deflections still move)
            if (ax > STICK_DZ) { g_accX += (long)js.lX * (long)dt; dx = (int)(g_accX / STICK_DIV); g_accX -= (long)dx * STICK_DIV; g_cx += dx; }
            else g_accX = 0;
            if (ay > STICK_DZ) { g_accY += (long)js.lY * (long)dt; dy = (int)(g_accY / STICK_DIV); g_accY -= (long)dy * STICK_DIV; g_cy += dy; }
            else g_accY = 0;
            if (g_cx < 0) g_cx = 0;  if (g_cx >= SCRW) g_cx = SCRW - 1;
            if (g_cy < 0) g_cy = 0;  if (g_cy >= SCRH) g_cy = SCRH - 1;

            // Buttons by Maple usage (the proper, layout-independent way). D-pad U/D/L/R,
            // and "activate" = A or Start.
            dn[0] = JBTN(js, USG_UA); dn[1] = JBTN(js, USG_DA);
            dn[2] = JBTN(js, USG_LA); dn[3] = JBTN(js, USG_RA);
            face  = (JBTN(js, USG_A) || JBTN(js, USG_START)) ? 1 : 0;
            g_jHeld = (nowt < g_joyPrimeUntil) ? 0 : JBTN(js, USG_A);   // A held (drag), post-settle
            if (nowt < g_joyPrimeUntil)
            {
                // STARTUP SETTLE WINDOW: just track the baseline, generate NO edges (lets the
                // device's transient first reads settle so they can't fire a phantom activate).
                for (i = 0; i < 4; i++) g_dpadHeld[i] = dn[i];
                g_btnLast = face;
            }
            else
            {
                // D-pad -> arrow keys, per-direction edge + auto-repeat
                for (i = 0; i < 4; i++)
                {
                    if (dn[i] && !g_dpadHeld[i])               { Push(s_dpadVk[i]); g_dpadRepeatAt[i] = nowt + 350; }
                    else if (dn[i] && nowt >= g_dpadRepeatAt[i]) { Push(s_dpadVk[i]); g_dpadRepeatAt[i] = nowt + 120; }
                    g_dpadHeld[i] = dn[i];
                }
                // A / Start -> "activate" (D-pad selects, A opens the selection)
                if (face && !g_btnLast) g_activate = 1;
                g_btnLast = face;
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
int  DInPointerDown(void)       { return g_mHeld || g_jHeld; }   // pointer button currently HELD (drag)
