---
name: microstk-mppp-shim
description: Universal link shim replacing SDK mppp.dll so STOCK microstk+winsock run over BBA/W5500 (no lwIP)
metadata:
  type: project
---

Networking via the **stock** CE 2.12 microstk.exe + winsock.dll — by replacing the SDK's
mppp.dll with our own link adapter (`net/netif/`). Alternative to (and simpler than) the lwIP
route (`net/lwip-port`): keep the shipped TCP/IP stack, only supply the link driver. Full writeup:
`docs/09-networking.md`.

**Contract** (decompiled from mppp.dll + microstk.exe in Ghidra project `wce`; captured in
`net/netif/microstk_if.h`): microstk `LoadLibraryW("mppp.dll")` + `GetProcAddress(
"InterfaceInitialize")`, called TWICE (distinct ifnet each) + a loopback. `ifnet` = 112B. TX =
`ifn_IPOutput(ifn, NDIS_PACKET*, nextHop)` (must `ifn_FreePacket`, ret 0). RX = AllocatePacket +
AllocateBufferWithMemory + memcpy + chain Head/Tail + `ifn_IPInput`@+0x54 (installed by IPEnable in
StackInitialize, NOT the WinMain callback loop). microstk speaks raw IP only — the link owns ARP +
ethernet + DHCP. **No route table** (flat single-subnet).

**Three gotchas (each cost a debug round; all fixed):**
1. `ifn_IPInterfaceConfigure(ifn, ip, mask, MTU, peer)` — arg4 = **MTU (1500)**, arg5 = PPP peer
   (0 for ethernet). NO gateway arg. (Passing gw → ~2.9GB MTU → TX dead.)
2. IPs are **network-order-as-LE everywhere** — pass IP/mask RAW, do NOT byteswap. IPInput@0x178c8
   compares ifn_ipAddr raw vs the wire dest + writes it as the wire source. (swap32 → garbage source
   + RX tossed → connect timeout 10060.)
3. **DNS = registry only**: winsock DnsMakeQuery→RegEnumData@0x10004a64 reads `HKLM\Comm:
   "DnsServers"` (REG_BINARY `[hdr/count][ips]`, IPs from index 1, size n*4+4, net order) per query.
   Parse DHCP option 6 + RegSetValueExW it.

**Files** (`net/netif/`, `build-netif.bat debug` → `reference/net-obj/mppp.dll`):
- `netif.c` — shim core; LinkOps ladder W5500→BBA→modem; ARP/DHCP/DNS/routing; idempotent init
  (HW+worker once, fresh ifnet/call, first = live g_ifn). `dllentry` = entry AND export @16.
- `bba_hw.c` — RTL8139/GAPS factored from `drivers/bba`, SetKMode-wrapped, CRITICAL_SECTION on G2.
- `ras.c` — 13 `AfdRas*` stubs (dial titles "connect" instantly).
- `w5500.c` — W5500 MACRAW backend; loads dcspi.dll on demand.
- `netif.def` — 16 exports at the EXACT mppp ordinals (verified dumpbin).

**dcspi** (`drivers/dcspi/`) = reusable SPI transport DLL (SpiInit/SetCS/RwData…, bus arg
DCSPI_BUS_SCI = SH-4 hardware sync-SPI / DCSPI_BUS_SCIF = SCSPTR2 bit-bang). Ported from KOS
`c:/dev/dreamcast/kallistios`. For W5500 now + SD/CF/FAT later.

**BBA PATH COMPLETE + VERIFIED in Flycast (2026-06-27)**: DHCP → DNS → TCP → HTTP
(`dns ok www.sega.com=192.185.5.88, tcp connect ok, recv, HTTP/1.1 301`). W5500 backend written but
**hardware-only / untested** — gated by `HKLM\Comm\Netif:"W5500Bus"` (1=SCI/PA7-CS, 2=SCIF/RTS-CS;
absent=off so BBA/Flycast untouched). Modem (PPP) backend NOT started.

**SDK-side edits (NOT in repo)**: deploy mppp.dll+dcspi.dll into `…\OS\`; ce.bib/platform.bib add
dcspi.dll + remove standalone bba.dll; reginit.ini/gemini.reg remove `[HKLM\WDMDrivers\BuiltIn\BBA]`.
Diagnostic OutputDebugStrings in netif.c/w5500.c — strip when stable.

**NEXT**: (1) emulate W5500 in Flycast (`c:/dev/pc/flycast`) to make it testable — virtual W5500 on
the SCI bus (byte-level hook in core/hw/sh4/modules/serial.cpp; CS via BSC_PDTRA in bsc.cpp) reusing
the host NAT bridge (net::modbba / bba_recv_frame). (2) Modem (PPP) backport from mppp.dll.
