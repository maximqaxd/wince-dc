//
// dcwplay.c - DCWin music player (WAV + MP3). A Winamp-style window in the desktop: LCD time,
// spectrum analyzer, transport, seek + volume bars - clickable via the analog-stick pointer
// (DCWinGetPointer) plus keyboard shortcuts. Audio is streamed to a looping DirectSound buffer
// at the source sample rate (no resample). The whole file is loaded to RAM (the DC has 16 MB)
// and decoded on the fly: WAV by a small RIFF reader, MP3 by minimp3 (vendor/minimp3). The
// decoder also snapshots a mono window of recent samples for the FFT visualizer.
//
// Launched by Explorer with the file path as the command line (DCWinExec / CreateProcess).
//
#define CINTERFACE
#include <windows.h>
#include <dsound.h>
#include "dcwlib.h"
#include "minimp3.h"          // API only; the implementation is compiled as C++ in mp3impl.cpp

// ---- UI geometry --------------------------------------------------------------------
#define PW   300
#define PH   150
#define C_BG     RGB(28, 31, 26)
#define C_PANEL  RGB(13, 15, 11)
#define C_EDGE   RGB(49, 54, 41)
#define C_LCD    RGB(47, 224, 122)
#define C_LCDDIM RGB(31, 154, 85)
#define C_LCDHI  RGB(155, 232, 74)
#define C_BTN    RGB(20, 23, 17)

// ---- audio state --------------------------------------------------------------------
static LPDIRECTSOUND        g_ds;
static LPDIRECTSOUNDBUFFER  g_buf;
static HWND                 g_hwndHidden;
static DWORD                g_bufBytes;          // DSound ring size
static DWORD                g_writePos;          // next byte we'll write into the ring
static int                  g_rate = 44100, g_ch = 2;
static volatile int         g_playing, g_eof, g_stop;
static volatile DWORD       g_played;            // total stereo frames written to the card
static DWORD                g_totalFrames;       // est. total frames (for the seek bar)
static int                  g_vol = 80;          // 0..100

// loaded file + decoder
static BYTE                *g_data;
static DWORD                g_size;
static int                  g_isMp3;
static DWORD                g_wavOff, g_wavEnd, g_wavPos;   // WAV PCM byte window
static int                  g_wavBits = 16, g_wavCh = 2;
static mp3dec_t             g_mp3;
static DWORD                g_mp3pos;
static WCHAR                g_track[80] = L"(no file)";

// viz: mono window of the most recent samples (decoder writes, UI reads)
#define VIZN 256
static short                g_viz[VIZN];
static volatile int         g_vizW;

static CRITICAL_SECTION     g_cs;

// ---- helpers ------------------------------------------------------------------------
static void PushViz(const short *stereo, int frames)
{
    int i, w = g_vizW;
    for (i = 0; i < frames; i++) { g_viz[w & (VIZN - 1)] = (short)((stereo[i*2] + stereo[i*2+1]) >> 1); w++; }
    g_vizW = w;
}

// Decode up to maxFrames stereo 16-bit frames into out. Returns frames produced (0 = EOF).
static int Decode(short *out, int maxFrames)
{
    if (!g_data) return 0;
    if (g_isMp3)
    {
        mp3dec_frame_info_t fi;
        int got = 0;
        short tmp[MINIMP3_MAX_SAMPLES_PER_FRAME];
        while (got + 1152 <= maxFrames && g_mp3pos < g_size)
        {
            int n = mp3dec_decode_frame(&g_mp3, g_data + g_mp3pos, (int)(g_size - g_mp3pos), tmp, &fi);
            g_mp3pos += fi.frame_bytes;
            if (fi.frame_bytes == 0) break;            // no more frames
            if (n == 0) continue;                      // header/skip
            g_rate = fi.hz;
            { int i; for (i = 0; i < n; i++) { short l = tmp[i*fi.channels], r = (fi.channels>1)?tmp[i*fi.channels+1]:l;
                out[(got+i)*2] = l; out[(got+i)*2+1] = r; } }
            got += n;
        }
        if (got) PushViz(out, got);
        return got;
    }
    else
    {
        int frames = 0, bps = (g_wavBits/8) * g_wavCh, i;
        while (frames < maxFrames && g_wavPos + bps <= g_wavEnd)
        {
            const BYTE *s = g_data + g_wavPos;
            short l, r;
            if (g_wavBits == 16) { l = (short)(s[0]|(s[1]<<8)); r = (g_wavCh>1)?(short)(s[2]|(s[3]<<8)):l; }
            else                 { l = (short)((s[0]-128)<<8);  r = (g_wavCh>1)?(short)((s[1]-128)<<8):l; }
            out[frames*2] = l; out[frames*2+1] = r;
            g_wavPos += bps; frames++;
        }
        (void)i;
        if (frames) PushViz(out, frames);
        return frames;
    }
}

static void DecoderReset(void)
{
    if (g_isMp3) { mp3dec_init(&g_mp3); g_mp3pos = 0; }
    else         { g_wavPos = g_wavOff; }
    g_played = 0; g_eof = 0; g_vizW = 0;
}

// ---- DirectSound --------------------------------------------------------------------
static int DsInit(void)
{
    WNDCLASSW wc;
    memset(&wc, 0, sizeof(wc)); wc.lpfnWndProc = DefWindowProcW; wc.hInstance = GetModuleHandleW(0);
    wc.lpszClassName = L"DCWPLAYDS"; RegisterClassW(&wc);
    g_hwndHidden = CreateWindowExW(0, L"DCWPLAYDS", L"", 0, 0, 0, 0, 0, NULL, NULL, wc.hInstance, NULL);
    if (DirectSoundCreate(NULL, &g_ds, NULL) != DS_OK) return 0;
    g_ds->lpVtbl->SetCooperativeLevel(g_ds, g_hwndHidden, DSSCL_NORMAL);
    return 1;
}

static void DsClose(void)
{
    if (g_buf) { g_buf->lpVtbl->Stop(g_buf); g_buf->lpVtbl->Release(g_buf); g_buf = NULL; }
}

// Create the streaming buffer for the current rate/channels and prime it.
static int DsOpenBuffer(void)
{
    DSBUFFERDESC d; WAVEFORMATEX wf;
    DsClose();
    memset(&wf, 0, sizeof(wf));
    wf.wFormatTag = WAVE_FORMAT_PCM; wf.nChannels = 2; wf.nSamplesPerSec = g_rate;
    wf.wBitsPerSample = 16; wf.nBlockAlign = 4; wf.nAvgBytesPerSec = g_rate * 4;
    g_bufBytes = (DWORD)(g_rate * 4);                  // 1 second ring
    memset(&d, 0, sizeof(d)); d.dwSize = sizeof(d);
    d.dwFlags = DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_GLOBALFOCUS | DSBCAPS_CTRLVOLUME;
    d.dwBufferBytes = g_bufBytes; d.lpwfxFormat = &wf;
    if (g_ds->lpVtbl->CreateSoundBuffer(g_ds, &d, &g_buf, NULL) != DS_OK) return 0;
    g_writePos = 0;
    return 1;
}

static void DsSetVolume(void)
{
    // DirectSound volume is hundredths of dB (-10000..0). vol 100 -> 0 dB, 0 -> ~ -45 dB (mute).
    long db = (g_vol >= 100) ? 0 : (g_vol <= 0) ? -10000 : (long)((g_vol - 100) * 45);
    if (g_buf) g_buf->lpVtbl->SetVolume(g_buf, db);
}

// Fill the part of the ring the card has already played with freshly decoded audio.
static void DsPump(void)
{
    DWORD play, write, freeb;
    void *p1, *p2; DWORD b1, b2;
    if (!g_buf) return;
    if (g_buf->lpVtbl->GetCurrentPosition(g_buf, &play, &write) != DS_OK) return;
    freeb = (play >= g_writePos) ? (play - g_writePos) : (g_bufBytes - g_writePos + play);
    if (freeb < 64) return;
    freeb &= ~3u;
    if (g_buf->lpVtbl->Lock(g_buf, g_writePos, freeb, &p1, &b1, &p2, &b2, 0) != DS_OK) return;
    {
        short *seg; DWORD segb, done;
        int part;
        for (part = 0; part < 2; part++)
        {
            seg = (short *)(part ? p2 : p1); segb = part ? b2 : b1;
            if (!seg || !segb) continue;
            done = 0;
            while (done < segb)
            {
                int want = (int)((segb - done) / 4), got;
                if (!g_playing) got = 0;
                else got = Decode(seg + done/2, want);
                if (got <= 0) { memset((BYTE*)seg + done, 0, segb - done); if (g_playing && !g_eof) g_eof = 1; break; }
                g_played += (DWORD)got;
                done += (DWORD)got * 4;
            }
        }
    }
    g_buf->lpVtbl->Unlock(g_buf, p1, b1, p2, b2);
    g_writePos = (g_writePos + freeb) % g_bufBytes;
}

// ---- file load ----------------------------------------------------------------------
static DWORD rd32(const BYTE *p) { return p[0]|(p[1]<<8)|(p[2]<<16)|((DWORD)p[3]<<24); }

static int LoadFile(const WCHAR *path)
{
    HANDLE f; DWORD got = 0; const WCHAR *base;
    if (g_data) { LocalFree(g_data); g_data = NULL; }
    f = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if (f == INVALID_HANDLE_VALUE) return 0;
    g_size = GetFileSize(f, 0);
    g_data = (BYTE *)LocalAlloc(LPTR, g_size + 16);
    if (!g_data) { CloseHandle(f); return 0; }
    ReadFile(f, g_data, g_size, &got, 0); CloseHandle(f);
    g_size = got;

    base = path; { const WCHAR *q; for (q = path; *q; q++) if (*q==L'\\'||*q==L'/') base = q+1; }
    { int i; for (i = 0; i < 79 && base[i]; i++) g_track[i] = base[i]; g_track[i] = 0; }

    if (g_size > 12 && g_data[0]=='R' && g_data[1]=='I' && g_data[2]=='F' && g_data[3]=='F')
    {                                                    // WAV
        DWORD o = 12;
        g_isMp3 = 0; g_wavOff = 0; g_wavEnd = 0;
        while (o + 8 <= g_size)
        {
            DWORD csz = rd32(g_data + o + 4);
            if (!memcmp(g_data + o, "fmt ", 4))
            { g_wavCh = g_data[o+10]|(g_data[o+11]<<8); g_rate = (int)rd32(g_data+o+12); g_wavBits = g_data[o+22]|(g_data[o+23]<<8); }
            else if (!memcmp(g_data + o, "data", 4))
            { g_wavOff = o + 8; g_wavEnd = o + 8 + csz; if (g_wavEnd > g_size) g_wavEnd = g_size; break; }
            o += 8 + ((csz + 1) & ~1u);
        }
        if (!g_wavOff) return 0;
        g_totalFrames = (g_wavEnd - g_wavOff) / ((g_wavBits/8) * (g_wavCh ? g_wavCh : 1));
    }
    else
    {                                                    // assume MP3
        g_isMp3 = 1; g_rate = 44100; g_ch = 2;
        g_totalFrames = 0;                               // unknown until decoded; seek bar approximates by bytes
    }
    DecoderReset();
    if (!DsOpenBuffer()) return 0;
    DsSetVolume();
    return 1;
}

// ---- transport ----------------------------------------------------------------------
static void Play(void)  { if (!g_data) return; if (!g_playing) { g_playing = 1; if (g_buf) g_buf->lpVtbl->Play(g_buf, 0, 0, DSBPLAY_LOOPING); } }
static void Pause(void) { g_playing = 0; }
static void Stop(void)  { g_playing = 0; if (g_buf) g_buf->lpVtbl->Stop(g_buf); DecoderReset(); g_writePos = 0; if (g_buf) g_buf->lpVtbl->SetCurrentPosition(g_buf, 0); }

static void SeekFrac(float frac)
{
    if (!g_data) return;
    if (frac < 0) frac = 0; if (frac > 1) frac = 1;
    if (g_isMp3) { g_mp3pos = (DWORD)(frac * g_size) & ~1u; mp3dec_init(&g_mp3); }   // approx (CBR-ish)
    else         { DWORD bps = (g_wavBits/8)*(g_wavCh?g_wavCh:1); g_wavPos = g_wavOff + (DWORD)(frac*(g_wavEnd-g_wavOff))/bps*bps; }
    g_played = (DWORD)(frac * (g_totalFrames ? g_totalFrames : 1));
    g_eof = 0;
}

// ---- UI -----------------------------------------------------------------------------
// Spectrum bars via a self-contained radix-2 FFT (N=128). No libm: twiddles come from a cos
// table built once with a 6-term Taylor cos, and magnitude uses |re|+|im| (good enough for a
// VU display). Keeps the player free of any math-library link.
#define FFTN 128
static float COST[FFTN], SINT[FFTN];
static int   g_twInit;
static float cosa(float x)
{
    float x2;
    while (x >  3.14159265f) x -= 6.28318531f;
    while (x < -3.14159265f) x += 6.28318531f;
    x2 = x * x;
    return 1.f - x2*(0.5f - x2*(1.f/24 - x2*(1.f/720 - x2*(1.f/40320))));
}
static void Bars(int *bars, int nbars)
{
    static float re[FFTN], im[FFTN];
    int i, j, k, b, w = g_vizW;
    if (!g_twInit) { for (i = 0; i < FFTN; i++) { COST[i] = cosa(6.28318531f*i/FFTN); SINT[i] = cosa(6.28318531f*i/FFTN - 1.57079633f); } g_twInit = 1; }
    for (i = 0; i < FFTN; i++)
    {
        short s = g_viz[(w - FFTN + i) & (VIZN - 1)];
        float win = 0.5f - 0.5f * COST[i];                 // Hann
        re[i] = (float)s * win * (1.f/32768); im[i] = 0;
    }
    for (i = 1, j = 0; i < FFTN; i++) { int bit = FFTN >> 1; for (; j & bit; bit >>= 1) j ^= bit; j ^= bit;
        if (i < j) { float t = re[i]; re[i] = re[j]; re[j] = t; t = im[i]; im[i] = im[j]; im[j] = t; } }
    for (k = 1; k < FFTN; k <<= 1)
    {
        int step = FFTN / (k << 1);                        // twiddle table stride for this stage
        for (j = 0; j < FFTN; j += k << 1)
            for (i = 0; i < k; i++)
            {
                int   idx = (i * step) & (FFTN - 1);
                float cr = COST[idx], ci = -SINT[idx];     // w = cos - j*sin
                float ar = re[j+i+k], ai = im[j+i+k];
                float ur = ar*cr - ai*ci, ui = ar*ci + ai*cr;
                re[j+i+k] = re[j+i] - ur; im[j+i+k] = im[j+i] - ui;
                re[j+i] += ur; im[j+i] += ui;
            }
    }
    for (b = 0; b < nbars; b++)
    {
        int lo = 1 + b * (FFTN/2 - 1) / nbars, hi = 1 + (b+1) * (FFTN/2 - 1) / nbars;
        float mag = 0;
        for (i = lo; i < hi; i++) { float m = (re[i]<0?-re[i]:re[i]) + (im[i]<0?-im[i]:im[i]); if (m > mag) mag = m; }
        { int v = (int)(mag * 300.f); if (v > 100) v = 100; bars[b] = v; }
    }
}

static void FmtTime(WCHAR *out, DWORD frames)
{
    DWORD s = g_rate ? frames / g_rate : 0;
    wsprintfW(out, L"%u:%02u", s / 60, s % 60);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPWSTR lpCmd, int nShow)
{
    DCWin *win;
    DWORD  key, last = 0;
    int    pbtnWas = 0;
    static int bars[14], peak[14];

    InitializeCriticalSection(&g_cs);
    win = DCWinOpen(70, 70, PW, PH, L"Media Player", ICON_APP);
    if (!win) return 1;
    if (DsInit() && lpCmd && lpCmd[0])
    {
        WCHAR p[260]; int i, j = 0;                      // strip surrounding quotes Explorer may add
        for (i = 0; lpCmd[i] && j < 259; i++) if (lpCmd[i] != L'"') p[j++] = lpCmd[i];
        p[j] = 0;
        if (LoadFile(p)) Play();
    }

    for (;;)
    {
        int cw = PW, ch = PH, px, py, pbtn = 0, over, changed = 0;
        int i, x, y, bw;

        DsPump();
        DCWinClientSize(win, &cw, &ch);
        over = DCWinGetPointer(win, &px, &py, &pbtn);

        while (DCWinPollKey(win, &key))
        {
            changed = 1;
            if (key == L' ') { if (g_playing) Pause(); else Play(); }
            else if (key == 'S') Stop();
            else if (key == VK_LEFT)  SeekFrac((float)g_played/(g_totalFrames?g_totalFrames:1) - 0.05f);
            else if (key == VK_RIGHT) SeekFrac((float)g_played/(g_totalFrames?g_totalFrames:1) + 0.05f);
            else if (key == VK_UP)   { if (g_vol < 100) g_vol += 5; DsSetVolume(); }
            else if (key == VK_DOWN) { if (g_vol > 0)   g_vol -= 5; DsSetVolume(); }
        }

        // clickable controls: transport row buttons, seek bar, volume bar
        bw = (cw - 16) / 5;
        if (over && pbtn && !pbtnWas)                    // click edge
        {
            int ty = ch - 26;
            if (py >= ty && py < ty + 18)                // transport buttons row
            {
                int b = (px - 8) / bw;
                if      (b == 0) SeekFrac(0);
                else if (b == 1) Play();
                else if (b == 2) Pause();
                else if (b == 3) Stop();
                else if (b == 4) SeekFrac(1);
            }
            else if (py >= 70 && py < 80)                // seek bar
                SeekFrac((float)(px - 8) / (cw - 16));
            changed = 1;
        }
        if (over && pbtn && py >= ch - 46 && py < ch - 36 && px > cw/2)   // volume bar (right half)
        { int v = (px - cw/2 - 4) * 100 / (cw/2 - 30); if (v<0) v=0; if (v>100) v=100; g_vol = v; DsSetVolume(); changed = 1; }
        pbtnWas = over ? pbtn : 0;

        if (GetTickCount() - last > 33) { last = GetTickCount(); changed = 1; }   // ~30fps for the viz
        if (DCWinResized(win)) changed = 1;

        if (changed)
        {
            WCHAR t[40], tt[16], td[16];
            DCWinBeginFrame(win);
            DCWinFillBg(win, C_BG);
            // track display panel
            DCWinFill(win, 6, 6, cw - 90, 26, C_PANEL);
            DCWinText(win, 10, 9, C_LCD, C_PANEL, g_track);
            DCWinText(win, 10, 20, C_LCDDIM, C_PANEL, g_isMp3 ? L"MP3" : L"WAV");
            // LCD time
            DCWinFill(win, cw - 80, 6, 74, 26, C_PANEL);
            FmtTime(tt, g_played); FmtTime(td, g_totalFrames);
            DCWinText(win, cw - 74, 9, C_LCDHI, C_PANEL, tt);
            wsprintfW(t, L"/ %s  %s", td, g_playing ? L"|>" : L"||");
            DCWinText(win, cw - 74, 20, C_LCDDIM, C_PANEL, t);
            // spectrum
            DCWinFill(win, 6, 36, cw - 12, 28, C_PANEL);
            Bars(bars, 14);
            bw = (cw - 16) / 14;
            for (i = 0; i < 14; i++)
            {
                int h = bars[i] * 24 / 100; if (h < 1) h = 1;
                if (bars[i] > peak[i]) peak[i] = bars[i]; else if (peak[i] > 0) peak[i]--;
                x = 8 + i * bw;
                DCWinFill(win, x, 60 - h, bw - 2, h, (h > 18) ? C_LCDHI : C_LCD);
                DCWinFill(win, x, 60 - peak[i]*24/100 - 1, bw - 2, 1, C_LCDHI);   // peak cap
            }
            // seek bar
            DCWinFill(win, 8, 70, cw - 16, 10, C_PANEL);
            { int sw = (g_totalFrames ? (int)((float)g_played/g_totalFrames*(cw-18)) : (int)((float)g_mp3pos/(g_size?g_size:1)*(cw-18)));
              DCWinFill(win, 9, 71, sw, 8, C_LCD); }
            // transport row + volume
            y = ch - 26; bw = (cw - 16) / 5;
            { const WCHAR *lbl[5] = { L"|<", L">", L"||", L"[]", L">|" };
              for (i = 0; i < 5; i++) { x = 8 + i*bw; DCWinFill(win, x, y, bw-3, 18, C_BTN); DCWinText(win, x + bw/2 - 6, y + 4, C_LCD, C_BTN, lbl[i]); } }
            // volume
            DCWinText(win, cw/2 - 24, ch - 44, C_LCDDIM, C_BG, L"VOL");
            DCWinFill(win, cw/2, ch - 46, cw/2 - 26, 10, C_PANEL);
            DCWinFill(win, cw/2 + 1, ch - 45, (cw/2 - 28) * g_vol / 100, 8, C_LCD);
            DCWinText(win, cw - 24, ch - 44, C_LCDDIM, C_BG, L"O");   // open hint
            DCWinEndFrame(win);
        }
        if (DCWinShouldClose(win)) break;
        Sleep(10);
    }

    Stop(); DsClose();
    if (g_ds) g_ds->lpVtbl->Release(g_ds);
    if (g_data) LocalFree(g_data);
    DCWinClose(win);
    return 0;
}
