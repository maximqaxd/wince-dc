//
// dcboot.h - a tiny shared "boot status" section, written by subsystems as they come up
// (the netif shim publishes the network adapter + DHCP result) and read by dcwboot.exe to
// drive the boot screen's live checklist. Same named-shared-section trick as syslog.h, so a
// WDM driver / microstk-side shim and the boot app all see one global struct.
//
#ifndef DCBOOT_H
#define DCBOOT_H

#include <windows.h>

#define DCBOOT_SECTION  L"DCBOOT"
#define DCBOOT_MAGIC    0x54424344u     // 'DCBT'

// Per-stage state. dcwboot shows ACTIVE (spinner) itself while waiting; subsystems publish OK/FAIL.
#define DCB_PENDING 0
#define DCB_ACTIVE  1
#define DCB_OK      2
#define DCB_FAIL    3

// Stage ids that subsystems publish (dcwboot adds its own implicit + storage stages).
#define DCB_NET     0                   // network adapter detected (BBA / W5500)
#define DCB_ADDR    1                   // DHCP address acquired
#define DCB_STORE   2                   // external storage (dcwboot also probes this live)
#define DCB_STAGES  3

#define DCB_RESLEN  32

typedef struct
{
    DWORD        magic;
    volatile LONG state[DCB_STAGES];            // DCB_*
    WCHAR        result[DCB_STAGES][DCB_RESLEN]; // short human result ("W5500 (SCIF)", "192.168.0.137")
} DcBootShared;

DcBootShared *DcBootMap(int create);            // map the section (reader passes create=0)
void          DcBootSet(int stage, int state, const WCHAR *result);  // result may be NULL

#endif // DCBOOT_H
