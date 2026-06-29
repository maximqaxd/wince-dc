//
// dcboot.c - see dcboot.h. Lazily maps the DCBOOT shared section; subsystems call DcBootSet()
// to publish a stage result, dcwboot reads state[]/result[]. Self-contained (just coredll).
//
#include "dcboot.h"

static DcBootShared *g_db;
static HANDLE        g_map;

DcBootShared *DcBootMap(int create)
{
    if (g_db) return g_db;
    g_map = CreateFileMappingW((HANDLE)-1, NULL, PAGE_READWRITE, 0, sizeof(DcBootShared), DCBOOT_SECTION);
    if (!g_map) return NULL;
    g_db = (DcBootShared *)MapViewOfFile(g_map, FILE_MAP_WRITE, 0, 0, sizeof(DcBootShared));
    if (g_db && create && g_db->magic != DCBOOT_MAGIC) g_db->magic = DCBOOT_MAGIC;
    return g_db;
}

void DcBootSet(int stage, int state, const WCHAR *result)
{
    DcBootShared *d = DcBootMap(1);
    int i;
    if (!d || stage < 0 || stage >= DCB_STAGES) return;
    if (result)
    {
        WCHAR *r = d->result[stage];
        for (i = 0; i < DCB_RESLEN - 1 && result[i]; i++) r[i] = result[i];
        r[i] = 0;
    }
    d->state[stage] = state;            // publish state last
}
