//
// dcwplay.c - DCWin music player (WAV + MP3). A Winamp-style window in the desktop: LCD time,
// spectrum analyzer, transport, seek + volume bars - clickable via the analog-stick pointer
// (DCWinGetPointer) plus keyboard shortcuts.
//
// Audio is STREAMED from the file through a small fixed buffer (g_sbuf, 32 KB) - NOT loaded
// whole - so an 8 MB MP3 costs ~32 KB of RAM, not 8 MB. The decoder pulls bytes on demand: WAV
// straight from the data chunk, MP3 via minimp3 (vendor/minimp3) frame-by-frame. Decoded PCM is
// pushed into a 1-second looping DirectSound ring (primed once, then refilled behind the play
// cursor). Lots of OutputDebugStringW logging (DCWPLAY:) so the SCIF console shows the DirectSound
// + decode path when there's no sound.
//
// Launched with the file path as the command line (DCWinExec / CreateProcess).
//
#define CINTERFACE
#include <windows.h>
#include <dsound.h>
#include <stdarg.h>
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

// ---- logging ------------------------------------------------------------------------
#define LOG(s) OutputDebugStringW(L"DCWPLAY: " s L"\r\n")
static void Logf(const WCHAR *fmt, ...)
{
    WCHAR b[160]; va_list ap;
    va_start(ap, fmt); wvsprintfW(b, fmt, ap); va_end(ap);
    OutputDebugStringW(b);
}

// ---- audio state --------------------------------------------------------------------
static LPDIRECTSOUND        g_ds;
static LPDIRECTSOUNDBUFFER  g_buf;
static HWND                 g_hwndHidden;
static DWORD                g_bufBytes;          // DSound ring size
static DWORD                g_writePos;          // next byte we'll write into the ring
static int                  g_rate = 44100, g_ch = 2;
static volatile int         g_playing, g_eof;
static volatile DWORD       g_played;            // total stereo frames written to the card
static DWORD                g_totalFrames;       // est. total frames (for the seek bar)
static int                  g_vol = 80;          // 0..100

// ---- streaming source (no whole-file load) ------------------------------------------
static HANDLE  g_file = INVALID_HANDLE_VALUE;
static DWORD   g_size;                           // total file size (for the seek bar)
static DWORD   g_streamPos;                      // file bytes consumed by the decoder (seek bar)
#define SBUF   (32 * 1024)
static BYTE    g_sbuf[SBUF];
static int     g_sN, g_sP;                       // valid bytes / consume cursor within g_sbuf
#define MP3_MAXFRAME 2881                         // worst-case MPEG frame; keep this much buffered

static int     g_isMp3;
static DWORD   g_wavOff, g_wavEnd;               // WAV PCM byte window (absolute file offsets)
static int     g_wavBits = 16, g_wavCh = 2;
static mp3dec_t g_mp3;
// One decoded MPEG frame held as stereo 16-bit PCM. mp3dec yields a fixed 1152-frame block, but
// the ring asks for arbitrary frame counts; we drain this across Decode() calls so the ring is
// filled CONTINUOUSLY (no silence padding for the maxFrames-mod-1152 tail -> no clicks/slowdown).
static short   g_pcm[1152 * 2];
static int     g_pcmN, g_pcmP;                   // valid frames / consume cursor within g_pcm

// Output resampler. The AICA's native rate is EXACTLY 44100 Hz (22.5792 MHz / 512), and only that
// rate is reliably pitch-accurate through the DC's mixerless DirectSound (a 48 kHz buffer plays a
// touch flat - the driver clamps to the 44100 mixer rate). So we always run the AICA buffer at
// 44100 and linearly resample the source into it: a 44100 source is a zero-cost 1:1 passthrough;
// 48000 downsamples; 22050 upsamples. Phase is 16.16 fixed-point; the interp weight is 8-bit so the
// (nxt-cur)*weight product can't overflow int32.
#define OUT_RATE 44100
static int      g_srcRate = OUT_RATE;            // decoded source sample rate
static unsigned g_rsStep;                        // source frames per output frame, 16.16
static unsigned g_rsPhase;                       // fractional source position [0,1), 16.16
static short    g_rsCurL, g_rsCurR, g_rsNxtL, g_rsNxtR;   // bracketing source frames
static int      g_rsPrimed, g_srcEof;            // resampler loaded? / source exhausted?

static WCHAR   g_track[80] = L"(no file)";

// viz: mono window of the most recent samples (decoder writes, UI reads)
#define VIZN 256
static short                g_viz[VIZN];
static volatile int         g_vizW;

// dst < src (we always slide toward the front), so a forward byte copy is safe (no memmove dep).
static void Slide(BYTE *d, const BYTE *s, int n) { int i; for (i = 0; i < n; i++) d[i] = s[i]; }

// Slide the unconsumed bytes to the front and read more from the file. After this, g_sbuf
// holds [0, g_sN) valid bytes and g_sP is reset toward 0.
static void StreamFill(void)
{
    DWORD got = 0;
    int   rem;
    if (g_file == INVALID_HANDLE_VALUE) return;
    rem = g_sN - g_sP;
    if (g_sP > 0) { if (rem > 0) Slide(g_sbuf, g_sbuf + g_sP, rem); g_sN = rem; g_sP = 0; }
    if (g_sN < SBUF) { ReadFile(g_file, g_sbuf + g_sN, (DWORD)(SBUF - g_sN), &got, 0); g_sN += (int)got; }
}

// ---- helpers ------------------------------------------------------------------------
static void PushViz(const short *stereo, int frames)
{
    int i, w = g_vizW;
    for (i = 0; i < frames; i++) { g_viz[w & (VIZN - 1)] = (short)((stereo[i*2] + stereo[i*2+1]) >> 1); w++; }
    g_vizW = w;
}

// Configure the resampler for a (newly discovered) source rate. Output is always OUT_RATE.
static void SetSrcRate(int srcRate)
{
    g_srcRate = srcRate > 0 ? srcRate : OUT_RATE;
    g_rate    = OUT_RATE;                                    // the AICA buffer always runs at 44100
    g_rsStep  = ((unsigned)g_srcRate << 16) / (unsigned)OUT_RATE;  // fits u32 for any sane rate
}

// Pull one decoded source frame (stereo 16-bit, at g_srcRate). Returns 0 at end of stream.
static int SrcNext(short *l, short *r)
{
    if (g_isMp3)
    {
        while (g_pcmN - g_pcmP <= 0)                         // need another MPEG frame
        {
            mp3dec_frame_info_t fi; short tmp[MINIMP3_MAX_SAMPLES_PER_FRAME]; int n, i;
            if (g_sN - g_sP < MP3_MAXFRAME) StreamFill();
            if (g_sN - g_sP <= 0) return 0;                 // end of file
            n = mp3dec_decode_frame(&g_mp3, g_sbuf + g_sP, g_sN - g_sP, tmp, &fi);
            if (fi.frame_bytes == 0) return 0;              // not enough data (EOF)
            g_sP += fi.frame_bytes; g_streamPos += (DWORD)fi.frame_bytes;
            if (n == 0) continue;                           // header/ID3 skip, no PCM
            for (i = 0; i < n; i++) { short L = tmp[i*fi.channels], R = (fi.channels>1)?tmp[i*fi.channels+1]:L;
                g_pcm[i*2] = L; g_pcm[i*2+1] = R; }
            g_pcmN = n; g_pcmP = 0;
        }
        *l = g_pcm[g_pcmP*2]; *r = g_pcm[g_pcmP*2+1]; g_pcmP++;
        return 1;
    }
    else
    {
        int bps = (g_wavBits/8) * (g_wavCh ? g_wavCh : 1);
        const BYTE *s;
        if (g_streamPos + (DWORD)bps > g_wavEnd) return 0;
        if (g_sN - g_sP < bps) StreamFill();
        if (g_sN - g_sP < bps) return 0;
        s = g_sbuf + g_sP;
        if (g_wavBits == 16) { *l = (short)(s[0]|(s[1]<<8)); *r = (g_wavCh>1)?(short)(s[2]|(s[3]<<8)):*l; }
        else                 { *l = (short)((s[0]-128)<<8); *r = (g_wavCh>1)?(short)((s[1]-128)<<8):*l; }
        g_sP += bps; g_streamPos += (DWORD)bps;
        return 1;
    }
}

// Produce up to maxFrames OUTPUT frames (at OUT_RATE), linearly resampling the source. 0 = EOF.
static int Decode(short *out, int maxFrames)
{
    int got = 0;
    if (g_file == INVALID_HANDLE_VALUE || g_srcEof) return 0;
    if (!g_rsPrimed)                                         // load the first two source frames
    {
        if (!SrcNext(&g_rsCurL, &g_rsCurR)) { g_srcEof = 1; return 0; }
        if (!SrcNext(&g_rsNxtL, &g_rsNxtR)) { g_rsNxtL = g_rsCurL; g_rsNxtR = g_rsCurR; }
        g_rsPhase = 0; g_rsPrimed = 1;
    }
    while (got < maxFrames)
    {
        int f = (int)((g_rsPhase >> 8) & 0xff);             // 8-bit interp weight (no int32 overflow)
        out[got*2]   = (short)(g_rsCurL + (((int)(g_rsNxtL - g_rsCurL) * f) >> 8));
        out[got*2+1] = (short)(g_rsCurR + (((int)(g_rsNxtR - g_rsCurR) * f) >> 8));
        got++;
        g_rsPhase += g_rsStep;
        while (g_rsPhase >= 0x10000)                         // step past whole source frames
        {
            g_rsPhase -= 0x10000;
            g_rsCurL = g_rsNxtL; g_rsCurR = g_rsNxtR;
            if (!SrcNext(&g_rsNxtL, &g_rsNxtR)) { g_srcEof = 1; g_rsNxtL = g_rsCurL; g_rsNxtR = g_rsCurR; }
        }
        if (g_srcEof) break;                                // source exhausted
    }
    if (got) PushViz(out, got);
    return got;
}

// Reset the decoder + stream to the start of audio (re-seek the file, drop the buffer).
static void DecoderReset(void)
{
    if (g_file == INVALID_HANDLE_VALUE) return;
    if (g_isMp3) { mp3dec_init(&g_mp3); SetFilePointer(g_file, 0, 0, FILE_BEGIN); g_streamPos = 0; }
    else         { SetFilePointer(g_file, (LONG)g_wavOff, 0, FILE_BEGIN); g_streamPos = g_wavOff; }
    g_sN = g_sP = 0;
    g_pcmN = g_pcmP = 0;
    g_rsPrimed = 0; g_rsPhase = 0; g_srcEof = 0;
    g_played = 0; g_eof = 0; g_vizW = 0;
}

// Decode the first MP3 frame to learn the true sample rate/channel count BEFORE we size the AICA
// buffer. minimp3 carries the rate in the frame header, not the file header, so without this the
// buffer is built at a guessed rate and the song plays pitch-shifted (a 48 kHz file through a
// 44.1 kHz buffer is ~1.5 semitones flat). Caller must DecoderReset() afterward to rewind.
static void ProbeMp3Rate(void)
{
    short tmp[MINIMP3_MAX_SAMPLES_PER_FRAME];
    mp3dec_frame_info_t fi;
    int tries;
    for (tries = 0; tries < 16; tries++)
    {
        int n;
        if (g_sN - g_sP < MP3_MAXFRAME) StreamFill();
        if (g_sN - g_sP <= 0) break;
        n = mp3dec_decode_frame(&g_mp3, g_sbuf + g_sP, g_sN - g_sP, tmp, &fi);
        if (fi.frame_bytes == 0) break;
        g_sP += fi.frame_bytes;
        if (n > 0 && fi.hz > 0)
        {
            SetSrcRate(fi.hz); g_ch = fi.channels;          // output stays at OUT_RATE; resample the rest
            if (g_totalFrames == 0 && fi.bitrate_kbps > 0)  // estimate length (in OUTPUT frames) from CBR
            { DWORD secs = (g_size / 125) / (DWORD)fi.bitrate_kbps; g_totalFrames = secs * (DWORD)OUT_RATE; }
            Logf(L"DCWPLAY: MP3 src=%dHz ch=%d br=%dk -> out %dHz\r\n", fi.hz, fi.channels, fi.bitrate_kbps, OUT_RATE);
            break;
        }
    }
}

// ---- DirectSound --------------------------------------------------------------------
static int DsInit(void)
{
    WNDCLASSW wc;
    HRESULT hr;
    memset(&wc, 0, sizeof(wc)); wc.lpfnWndProc = DefWindowProcW; wc.hInstance = GetModuleHandleW(0);
    wc.lpszClassName = L"DCWPLAYDS"; RegisterClassW(&wc);
    g_hwndHidden = CreateWindowExW(0, L"DCWPLAYDS", L"", 0, 0, 0, 0, 0, NULL, NULL, wc.hInstance, NULL);
    hr = DirectSoundCreate(NULL, &g_ds, NULL);
    Logf(L"DCWPLAY: DirectSoundCreate hr=%08x ds=%08x\r\n", (unsigned)hr, (unsigned)(ULONG)g_ds);
    if (hr != DS_OK || !g_ds) return 0;
    hr = g_ds->lpVtbl->SetCooperativeLevel(g_ds, g_hwndHidden, DSSCL_NORMAL);
    Logf(L"DCWPLAY: SetCooperativeLevel hr=%08x\r\n", (unsigned)hr);
    return 1;
}

static void DsClose(void)
{
    if (g_buf) { g_buf->lpVtbl->Stop(g_buf); g_buf->lpVtbl->Release(g_buf); g_buf = NULL; }
}

// Create the streaming buffer for the current rate/channels.
static int DsOpenBuffer(void)
{
    DSBUFFERDESC d; WAVEFORMATEX wf; HRESULT hr;
    DsClose();
    memset(&wf, 0, sizeof(wf));
    wf.wFormatTag = WAVE_FORMAT_PCM; wf.nChannels = 2; wf.nSamplesPerSec = g_rate;
    wf.wBitsPerSample = 16; wf.nBlockAlign = 4; wf.nAvgBytesPerSec = g_rate * 4;
    // The DC AICA requires the buffer size AND every Lock offset/size to be 32-byte aligned
    // (else Play/Lock return DSERR_NOT32BYTEALIGNED = 0x887800ff). Round the 1-second ring down.
    g_bufBytes = (DWORD)(g_rate * 4) & ~31u;           // ~1 second ring, 32-byte aligned
    memset(&d, 0, sizeof(d)); d.dwSize = sizeof(d);
    // DSBCAPS_STATIC is REQUIRED on the DC: the AICA has no software mixer, so a playable buffer
    // must live in AICA sound RAM (STATIC). The ~1-second ring (176 KB) fits AICA RAM and we
    // still stream into it (Lock/refill behind the cursor).
    d.dwFlags = DSBCAPS_STATIC | DSBCAPS_CTRLVOLUME | DSBCAPS_GETCURRENTPOSITION2;
    d.dwBufferBytes = g_bufBytes; d.lpwfxFormat = &wf;
    hr = g_ds->lpVtbl->CreateSoundBuffer(g_ds, &d, &g_buf, NULL);
    Logf(L"DCWPLAY: CreateSoundBuffer hr=%08x rate=%d bytes=%u\r\n", (unsigned)hr, g_rate, g_bufBytes);
    if (hr != DS_OK) return 0;
    g_writePos = 0;
    return 1;
}

static void DsSetVolume(void)
{
    long db = (g_vol >= 100) ? 0 : (g_vol <= 0) ? -10000 : (long)((g_vol - 100) * 45);
    if (g_buf) g_buf->lpVtbl->SetVolume(g_buf, db);
}

// Prime the whole ring once with decoded audio so playback isn't silent for the first loop.
static void DsPrime(void)
{
    void *p1, *p2; DWORD b1, b2; HRESULT hr;
    int got;
    if (!g_buf) return;
    hr = g_buf->lpVtbl->Lock(g_buf, 0, g_bufBytes, &p1, &b1, &p2, &b2, DSBLOCK_ENTIREBUFFER);
    if (hr != DS_OK) { Logf(L"DCWPLAY: prime Lock hr=%08x\r\n", (unsigned)hr); return; }
    got = Decode((short *)p1, (int)(b1 / 4));
    if (got > 0) g_played += (DWORD)got;
    if ((DWORD)got * 4 < b1) memset((BYTE *)p1 + got * 4, 0, b1 - (DWORD)got * 4);
    if (p2 && b2) memset(p2, 0, b2);
    g_buf->lpVtbl->Unlock(g_buf, p1, b1, p2, b2);
    g_writePos = 0;
    Logf(L"DCWPLAY: primed %d frames\r\n", got);
}

// Fill the part of the ring the card has already played with freshly decoded audio.
static void DsPump(void)
{
    DWORD play, write, freeb; HRESULT hr;
    void *p1, *p2; DWORD b1, b2;
    static int s_logged;
    if (!g_buf) return;
    hr = g_buf->lpVtbl->GetCurrentPosition(g_buf, &play, &write);
    if (hr != DS_OK) { if (s_logged < 3) { Logf(L"DCWPLAY: GetCurrentPosition hr=%08x\r\n", (unsigned)hr); s_logged++; } return; }
    freeb = (play >= g_writePos) ? (play - g_writePos) : (g_bufBytes - g_writePos + play);
    if (s_logged < 6) { Logf(L"DCWPLAY: pump play=%u write=%u wp=%u freeb=%u\r\n", play, write, g_writePos, freeb); s_logged++; }
    if (freeb < 64) return;
    freeb &= ~31u;                                       // 32-byte align (AICA: DSERR_NOT32BYTEALIGNED)
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
                if (got <= 0) { memset((BYTE*)seg + done, 0, segb - done); if (g_playing && !g_eof) { g_eof = 1; LOG(L"EOF"); } break; }
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
    BYTE  hdr[512]; DWORD got = 0; const WCHAR *base;
    if (g_file != INVALID_HANDLE_VALUE) { CloseHandle(g_file); g_file = INVALID_HANDLE_VALUE; }
    g_file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if (g_file == INVALID_HANDLE_VALUE) { Logf(L"DCWPLAY: open FAILED err=%u\r\n", GetLastError()); return 0; }
    g_size = GetFileSize(g_file, 0);
    ReadFile(g_file, hdr, sizeof(hdr), &got, 0);            // peek the header (then we re-seek)
    Logf(L"DCWPLAY: opened size=%u hdr=%u\r\n", g_size, got);

    base = path; { const WCHAR *q; for (q = path; *q; q++) if (*q==L'\\'||*q==L'/') base = q+1; }
    { int i; for (i = 0; i < 79 && base[i]; i++) g_track[i] = base[i]; g_track[i] = 0; }

    if (got > 12 && hdr[0]=='R' && hdr[1]=='I' && hdr[2]=='F' && hdr[3]=='F')
    {                                                    // WAV
        DWORD o = 12; int wavRate = OUT_RATE, bps; DWORD srcFrames, secs;
        g_isMp3 = 0; g_wavOff = 0; g_wavEnd = 0;
        while (o + 8 <= got)
        {
            DWORD csz = rd32(hdr + o + 4);
            if (!memcmp(hdr + o, "fmt ", 4))
            { g_wavCh = hdr[o+10]|(hdr[o+11]<<8); wavRate = (int)rd32(hdr+o+12); g_wavBits = hdr[o+22]|(hdr[o+23]<<8); }
            else if (!memcmp(hdr + o, "data", 4))
            { g_wavOff = o + 8; g_wavEnd = o + 8 + csz; if (g_wavEnd > g_size) g_wavEnd = g_size; break; }
            o += 8 + ((csz + 1) & ~1u);
        }
        if (!g_wavOff) { LOG(L"WAV: no data chunk in first 512B"); return 0; }
        SetSrcRate(wavRate);                             // output stays at OUT_RATE; resample the source
        bps = (g_wavBits/8) * (g_wavCh ? g_wavCh : 1);
        srcFrames = (g_wavEnd - g_wavOff) / (DWORD)bps;  // length in OUTPUT frames (resampled)
        secs = wavRate ? srcFrames / (DWORD)wavRate : 0; g_totalFrames = secs * (DWORD)OUT_RATE;
        Logf(L"DCWPLAY: WAV src=%dHz ch=%d bits=%d -> out %dHz data=%u..%u\r\n", wavRate, g_wavCh, g_wavBits, OUT_RATE, g_wavOff, g_wavEnd);
    }
    else
    {                                                    // assume MP3
        g_isMp3 = 1; g_ch = 2; g_totalFrames = 0; SetSrcRate(OUT_RATE);
    }
    DecoderReset();
    if (g_isMp3) { ProbeMp3Rate(); DecoderReset(); }     // learn the real rate, then rewind to frame 0
    if (!DsOpenBuffer()) return 0;
    DsSetVolume();
    DsPrime();                                           // fill the ring before Play -> no silent gap
    return 1;
}

// ---- transport ----------------------------------------------------------------------
static void Play(void)  { if (g_file == INVALID_HANDLE_VALUE) return; if (!g_playing) { g_playing = 1; if (g_buf) { HRESULT hr = g_buf->lpVtbl->Play(g_buf, 0, 0, DSBPLAY_LOOPING); Logf(L"DCWPLAY: Play hr=%08x\r\n", (unsigned)hr); } } }
static void Pause(void) { g_playing = 0; LOG(L"Pause"); }
static void Stop(void)  { g_playing = 0; LOG(L"Stop"); if (g_buf) g_buf->lpVtbl->Stop(g_buf); DecoderReset(); g_writePos = 0; if (g_buf) g_buf->lpVtbl->SetCurrentPosition(g_buf, 0); }

static void SeekFrac(float frac)
{
    if (g_file == INVALID_HANDLE_VALUE) return;
    if (frac < 0) frac = 0; if (frac > 1) frac = 1;
    if (g_isMp3)
    {
        g_streamPos = (DWORD)(frac * g_size) & ~1u;       // approx (CBR-ish)
        SetFilePointer(g_file, (LONG)g_streamPos, 0, FILE_BEGIN);
        mp3dec_init(&g_mp3); g_sN = g_sP = 0;
    }
    else
    {
        DWORD bps = (g_wavBits/8)*(g_wavCh?g_wavCh:1);
        g_streamPos = g_wavOff + (DWORD)(frac*(g_wavEnd-g_wavOff))/bps*bps;
        SetFilePointer(g_file, (LONG)g_streamPos, 0, FILE_BEGIN); g_sN = g_sP = 0;
    }
    g_played = (DWORD)(frac * (g_totalFrames ? g_totalFrames : 1));
    g_eof = 0;
}

// ---- UI -----------------------------------------------------------------------------
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
        int step = FFTN / (k << 1);
        for (j = 0; j < FFTN; j += k << 1)
            for (i = 0; i < k; i++)
            {
                int   idx = (i * step) & (FFTN - 1);
                float cr = COST[idx], ci = -SINT[idx];
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

    LOG(L"WinMain enter");
    win = DCWinOpen(70, 70, PW, PH, L"Media Player", ICON_APP);
    if (!win) return 1;
    if (DsInit() && lpCmd && lpCmd[0])
    {
        WCHAR p[260]; int i, j = 0;                      // strip surrounding quotes Explorer may add
        for (i = 0; lpCmd[i] && j < 259; i++) if (lpCmd[i] != L'"') p[j++] = lpCmd[i];
        p[j] = 0;
        Logf(L"DCWPLAY: cmdline '%s'\r\n", p);
        if (LoadFile(p)) Play();
    }
    else LOG(L"no file / DsInit failed");

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
            DCWinFill(win, 6, 6, cw - 90, 26, C_PANEL);
            DCWinText(win, 10, 9, C_LCD, C_PANEL, g_track);
            DCWinText(win, 10, 20, C_LCDDIM, C_PANEL, g_isMp3 ? L"MP3" : L"WAV");
            DCWinFill(win, cw - 80, 6, 74, 26, C_PANEL);
            FmtTime(tt, g_played); FmtTime(td, g_totalFrames);
            DCWinText(win, cw - 74, 9, C_LCDHI, C_PANEL, tt);
            wsprintfW(t, L"/ %s  %s", td, g_playing ? L"|>" : L"||");
            DCWinText(win, cw - 74, 20, C_LCDDIM, C_PANEL, t);
            DCWinFill(win, 6, 36, cw - 12, 28, C_PANEL);
            Bars(bars, 14);
            bw = (cw - 16) / 14;
            for (i = 0; i < 14; i++)
            {
                int h = bars[i] * 24 / 100; if (h < 1) h = 1;
                if (bars[i] > peak[i]) peak[i] = bars[i]; else if (peak[i] > 0) peak[i]--;
                x = 8 + i * bw;
                DCWinFill(win, x, 60 - h, bw - 2, h, (h > 18) ? C_LCDHI : C_LCD);
                DCWinFill(win, x, 60 - peak[i]*24/100 - 1, bw - 2, 1, C_LCDHI);
            }
            DCWinFill(win, 8, 70, cw - 16, 10, C_PANEL);
            { int sw = (g_isMp3 ? (int)((float)g_streamPos/(g_size?g_size:1)*(cw-18))
                                : (g_totalFrames ? (int)((float)g_played/g_totalFrames*(cw-18)) : 0));
              DCWinFill(win, 9, 71, sw, 8, C_LCD); }
            y = ch - 26; bw = (cw - 16) / 5;
            { const WCHAR *lbl[5] = { L"|<", L">", L"||", L"[]", L">|" };
              for (i = 0; i < 5; i++) { x = 8 + i*bw; DCWinFill(win, x, y, bw-3, 18, C_BTN); DCWinText(win, x + bw/2 - 6, y + 4, C_LCD, C_BTN, lbl[i]); } }
            DCWinText(win, cw/2 - 24, ch - 44, C_LCDDIM, C_BG, L"VOL");
            DCWinFill(win, cw/2, ch - 46, cw/2 - 26, 10, C_PANEL);
            DCWinFill(win, cw/2 + 1, ch - 45, (cw/2 - 28) * g_vol / 100, 8, C_LCD);
            DCWinText(win, cw - 24, ch - 44, C_LCDDIM, C_BG, L"O");
            DCWinEndFrame(win);
        }
        if (DCWinShouldClose(win)) break;
        Sleep(10);
    }

    Stop(); DsClose();
    if (g_ds) g_ds->lpVtbl->Release(g_ds);
    if (g_file != INVALID_HANDLE_VALUE) CloseHandle(g_file);
    DCWinClose(win);
    return 0;
}
