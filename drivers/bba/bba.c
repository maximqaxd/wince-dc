//
// bba.c - Dreamcast Broadband Adapter (HIT-0400, RTL8139C on the G2 bus) as a
// Dreamcast WinCE **WDM driver** - the model the DC actually uses (maple.dll,
// sh4ser.dll, seg_rock.dll): the driver provides an NT-style DriverEntry, links
// wdm.lib (which exports the InitWDMDriver entry that LoadWDMDriver calls), and is
// registered under HKLM\WDMDrivers\BuiltIn. NOT the HKLM\Drivers\BuiltIn stream
// model - the DC's wdevice.exe ignores that.
//
// Scaffold stage: probe the GAPS bridge, init it, reset the RTL8139C and read the
// MAC (logged at load). Polled. Raw TX/RX + lwIP next.
//
// Hardware sequence ported from KallistiOS (broadband_adapter.c, g2bus.c, fifo.h).
//
#include <windows.h>
#include <wdm.h>

DWORD SetKMode(DWORD fMode);    // coredll export, not in the SDK public headers

#ifndef CTL_CODE
#define CTL_CODE(t,f,m,a) (((t) << 16) | ((a) << 14) | ((f) << 2) | (m))
#define METHOD_BUFFERED   0
#define FILE_ANY_ACCESS   0
#endif
#define IOCTL_BBA_GET_MAC      CTL_CODE(FILE_DEVICE_NETWORK, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_BBA_GET_PRESENT  CTL_CODE(FILE_DEVICE_NETWORK, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)

// ---------------------------------------------------------------------------
// G2 bus access (P2 / uncached; drain the 32-byte G2 write FIFO before each).
// ---------------------------------------------------------------------------
#define G2_FIFO_STATUS  (*(volatile DWORD *)0xA05F688C)
#define FIFO_AICA       0x0001
#define FIFO_G2         0x0010

static void g2_fifo_wait(void)
{
    int i;
    for (i = 0; i < 0x1800; i++)
        if (!(G2_FIFO_STATUS & (FIFO_AICA | FIFO_G2)))
            break;
}

static BYTE  g2_read_8 (DWORD a) { g2_fifo_wait(); return *(volatile BYTE  *)a; }
static WORD  g2_read_16(DWORD a) { g2_fifo_wait(); return *(volatile WORD  *)a; }
static DWORD g2_read_32(DWORD a) { g2_fifo_wait(); return *(volatile DWORD *)a; }
static void  g2_write_8 (DWORD a, BYTE  v) { g2_fifo_wait(); *(volatile BYTE  *)a = v; }
static void  g2_write_16(DWORD a, WORD  v) { g2_fifo_wait(); *(volatile WORD  *)a = v; }
static void  g2_write_32(DWORD a, DWORD v) { g2_fifo_wait(); *(volatile DWORD *)a = v; }

static void g2_read_block_8(BYTE *out, DWORD addr, int amt)
{
    int i;
    for (i = 0; i < amt; i++)
    {
        if (!(i % 32)) g2_fifo_wait();
        out[i] = *(volatile BYTE *)(addr + i);
    }
}

// ---------------------------------------------------------------------------
// GAPS bridge + RTL8139C (from KOS broadband_adapter).
// ---------------------------------------------------------------------------
#define GAPS_BASE   0xA1000000
#define RTL_MEM     0xA1840000
#define NIC(reg)    (GAPS_BASE + 0x1700 + (reg))
#define RT_IDR0     0x00
#define RT_CHIPCMD  0x37
#define RT_CMD_RESET 0x10

static BYTE g_mac[6];
static BOOL g_present = FALSE;

static int gaps_init(void)
{
    char str[16];
    int  i;

    g2_read_block_8((BYTE *)str, GAPS_BASE + 0x1400, 16);
    if (memcmp(str, "GAPSPCI_BRIDGE_2", 16) != 0)
        return -1;

    g2_write_32(GAPS_BASE + 0x1418, 0x5a14a501);
    for (i = 10000; i > 0 && !(g2_read_32(GAPS_BASE + 0x1418) & 1); i--)
        ;
    if (!(g2_read_32(GAPS_BASE + 0x1418) & 1))
        return -2;

    g2_write_32(GAPS_BASE + 0x1420, 0x01000000);
    g2_write_32(GAPS_BASE + 0x1424, 0x01000000);
    g2_write_32(GAPS_BASE + 0x1428, RTL_MEM);
    g2_write_32(GAPS_BASE + 0x142c, RTL_MEM + 32 * 1024);
    g2_write_32(GAPS_BASE + 0x1414, 0x00000001);
    g2_write_32(GAPS_BASE + 0x1434, 0x00000001);
    g2_write_16(GAPS_BASE + 0x1606, 0xf900);
    g2_write_32(GAPS_BASE + 0x1630, 0x00000000);
    g2_write_8 (GAPS_BASE + 0x163c, 0x00);
    g2_write_8 (GAPS_BASE + 0x160d, 0xf0);
    g2_write_16(GAPS_BASE + 0x1604, g2_read_16(GAPS_BASE + 0x1604) | 0x6);
    g2_write_32(GAPS_BASE + 0x1614, 0x01000000);
    if (g2_read_8(GAPS_BASE + 0x1650) & 0x1)
        g2_write_16(GAPS_BASE + 0x1654, (g2_read_16(GAPS_BASE + 0x1654) & 0xfffc) | 0x8000);
    g2_write_32(GAPS_BASE + 0x1414, 0x00000001);
    return 0;
}

static void rtl_reset_and_mac(void)
{
    DWORD lo, hi;
    int   i;
    g2_write_8(NIC(RT_CHIPCMD), RT_CMD_RESET);
    for (i = 1000; i > 0 && (g2_read_8(NIC(RT_CHIPCMD)) & RT_CMD_RESET); i--)
        ;
    lo = g2_read_32(NIC(RT_IDR0));
    hi = g2_read_32(NIC(RT_IDR0 + 4));
    g_mac[0] = (BYTE)lo;        g_mac[1] = (BYTE)(lo >> 8);
    g_mac[2] = (BYTE)(lo >> 16); g_mac[3] = (BYTE)(lo >> 24);
    g_mac[4] = (BYTE)hi;        g_mac[5] = (BYTE)(hi >> 8);
}

static BOOL bba_bringup(void)
{
    DWORD prev = SetKMode(TRUE);     // P2 / G2 access needs kernel mode
    BOOL  ok = FALSE;
    WCHAR b[80];

    __try
    {
        if (gaps_init() == 0)
        {
            rtl_reset_and_mac();
            ok = TRUE;
            wsprintfW(b, L"BBA: up, MAC %02x:%02x:%02x:%02x:%02x:%02x\r\n",
                      g_mac[0], g_mac[1], g_mac[2], g_mac[3], g_mac[4], g_mac[5]);
            OutputDebugStringW(b);
        }
        else
            OutputDebugStringW(L"BBA: no GAPS bridge / init failed (adapter present?)\r\n");
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        OutputDebugStringW(L"BBA: probe faulted (G2 access) - skipping\r\n");
    }

    SetKMode(prev);
    return ok;
}

// ---------------------------------------------------------------------------
// WDM dispatch. One handler for all major functions; IOCTLs use buffered I/O
// (input+output share Irp->AssociatedIrp.SystemBuffer).
// ---------------------------------------------------------------------------
static NTSTATUS BbaDispatch(PDEVICE_OBJECT dev, PIRP irp)
{
    PIO_STACK_LOCATION sp = IoGetCurrentIrpStackLocation(irp);
    NTSTATUS st = STATUS_SUCCESS;
    ULONG    info = 0;

    if (sp->MajorFunction == IRP_MJ_DEVICE_CONTROL)
    {
        ULONG code   = sp->Parameters.DeviceIoControl.IoControlCode;
        PVOID buf    = irp->AssociatedIrp.SystemBuffer;
        ULONG outlen = sp->Parameters.DeviceIoControl.OutputBufferLength;
        if (code == IOCTL_BBA_GET_MAC && buf && outlen >= 6)
        { memcpy(buf, g_mac, 6); info = 6; }
        else if (code == IOCTL_BBA_GET_PRESENT && buf && outlen >= 4)
        { *(DWORD *)buf = g_present ? 1 : 0; info = 4; }
        else
            st = STATUS_NOT_IMPLEMENTED;
    }
    // IRP_MJ_CREATE / CLOSE / READ / WRITE just complete (success / 0 bytes).

    irp->IoStatus.Status = st;
    irp->IoStatus.Information = info;
    IoCompleteRequest(irp, 0);
    return st;
}

// Provided to wdm.lib's InitWDMDriver (the exported entry LoadWDMDriver calls).
NTSTATUS DriverEntry(PDRIVER_OBJECT drv, PUNICODE_STRING regpath)
{
    UNICODE_STRING  name;
    PDEVICE_OBJECT  dev = NULL;
    NTSTATUS        st;
    int             i;

    OutputDebugStringW(L"BBA DriverEntry\r\n");
    g_present = bba_bringup();        // probe + log MAC at load

    RtlInitUnicodeString(&name, L"\\Device\\BBA1");
    st = IoCreateDevice(drv, 0, &name, FILE_DEVICE_NETWORK, 0, FALSE, &dev);
    if (!NT_SUCCESS(st))
    {
        OutputDebugStringW(L"BBA: IoCreateDevice failed\r\n");
        return st;
    }

    for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)
        drv->MajorFunction[i] = BbaDispatch;

    OutputDebugStringW(L"BBA: device created (DriverEntry ok)\r\n");
    return STATUS_SUCCESS;
}

BOOL WINAPI DllMain(HANDLE hInst, DWORD reason, LPVOID reserved) { return TRUE; }
