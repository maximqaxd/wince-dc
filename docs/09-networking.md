# 09 — Networking: a universal link shim for the stock CE TCP/IP stack

**Status (2026-06-27): BBA path COMPLETE and verified end-to-end in Flycast** — DHCP → DNS →
TCP → HTTP all work on the *stock* CE 2.12 `microstk.exe` + `winsock.dll`, driven by our drop-in
`mppp.dll` replacement over the Broadband Adapter. W5500 (MACRAW over SPI) backend is written but
**hardware-only / untested** (no Flycast emulation yet). Modem (PPP) backend is **not started**.

## The idea

The shipped SDK has a full TCP/IP stack (`microstk.exe`) + BSD sockets (`winsock.dll`), but its
only link driver is `mppp.dll` (PPP over a dial-up modem). Instead of bringing our own stack
(the parallel lwIP port in `net/lwip-port`, see commit `08cb779`), we **replace `mppp.dll`** with a
universal link adapter and keep everything above it stock. Net result: every winsock title and the
stock resolver "just work" over Ethernet.

Source: `net/netif/` (the shim) + `drivers/dcspi/` (SPI transport for W5500). Build:
`net/netif/build-netif.bat debug` → `reference/net-obj/mppp.dll` (it builds `drivers/dcspi` first).

## The microstk ⇄ link-driver contract (reverse-engineered)

All offsets/behaviour decompiled from `microstk.exe` + `mppp.dll` in Ghidra (project `wce`).
Captured as a C ABI in `net/netif/microstk_if.h`.

- microstk `WinMain` does `LoadLibraryW("mppp.dll")` + `GetProcAddressW("InterfaceInitialize")`,
  then calls **`InterfaceInitialize("PPP", &ifnet)` TWICE** (two interface slots, by design) +
  a loopback. Each call must return a **distinct** `ifnet*`. It then back-fills the packet-pool
  callbacks at ifnet **+0x58/0x5c/0x60/0x64/0x68/0x6c** (AllocBuf/AllocPkt/FreePkt/QueueTimeout/
  FreeBuf/IPInterfaceConfigure) and calls `StackInitialize`, which `IPEnable`s each interface and
  installs **`ifn_IPInput` at +0x54** (NOT done by the WinMain loop — easy to miss).
- `ifnet` is 112 bytes. We set: name@0, `ifn_Ioctl`@0x4c, `ifn_IPOutput`@0x50, `ifn_dwMTU`@0x44,
  `ifn_dwFlags`@0x40 (start `IFF_BROADCAST`=0x2; set `IFF_UP`=0x1 only after IP config).
- **TX:** microstk calls `ifn_IPOutput(ifnet, NDIS_PACKET*, nextHop)`. We gather the NDIS buffer
  chain into one IP datagram, route it, frame it, send. **Must `ifn_FreePacket` and return 0.**
- **RX:** `AllocatePacket` + `AllocateBufferWithMemory(len,&mem)` + memcpy the IP datagram +
  chain the buffer onto `pkt->Head`/`Tail` + `pkt->ValidCounts=0` + `ifn_IPInput(ifnet, pkt)`.
  (Exactly mppp's `Receive`.)
- **IP config:** `ifn_IPInterfaceConfigure(ifnet, ip, mask, mtu, peer)` — **arg4 = MTU (1500),
  arg5 = PPP peer (0 for ethernet)**. There is **NO gateway argument and no route table** in this
  stack — it is flat/single-subnet; off-subnet delivery is the link's problem.
- **Byte order: everything is network-order-as-little-endian** (i.e. raw wire bytes memcpy'd into a
  `ulong`). `IPInput` compares `ifn_ipAddr` *directly* against the raw packet dest field, and writes
  `ifn_ipAddr` as the wire source. Pass DHCP IP/mask to `IPInterfaceConfigure` **raw, do not
  byteswap.** (mppp byteswaps only because its IPCP source is host-order.)
- **DNS is registry-only.** microstk has no resolver; `winsock.dll` `DnsMakeQuery`→`RegEnumData`
  reads `HKLM\Comm` value **`DnsServers`** (REG_BINARY = `[hdr/count][ip…]`, IPs from index 1,
  size `n*4+4`, network order) fresh on every query. mppp normally writes it from IPCP; we write it
  from DHCP option 6.

### The three bugs that cost a round each (all fixed)
1. Passed the gateway where arg4 = MTU → `ifn_dwMTU` became ~2.9 GB → IP output dead (`connect`
   never reached the wire). Fix: pass `(ip, mask, 1500, 0)`.
2. Added a `swap32` on ip/mask (wrong) → byte-reversed wire **source** IP + every inbound unicast
   tossed as "not for us" → `connect` timed out (10060). Fix: pass raw network-order.
3. Never parsed DHCP option 6 nor wrote `HKLM\Comm\DnsServers` → resolver had no server →
   `gethostbyname` failed. Fix: parse option 6 + `RegSetValueExW`.

## Architecture (`net/netif/`)

- **`netif.c`** — the shim core + the `InterfaceInitialize`/`dllentry` exports. A `LinkOps`
  abstraction (`probe`/`init`/`tx`/`poll` of raw Ethernet frames) with a detect ladder
  **W5500 → BBA → modem**. Link-layer services live here (so every Ethernet backend reuses them):
  ARP cache + replies, Ethernet framing, a raw DHCP client (DISCOVER/REQUEST/ACK, options
  1/3/6/54), `NetifOnLease` (→ `IPInterfaceConfigure` + IFF_UP + `WriteDnsServers`), and
  `OurTransmit` (route by netmask; off-link → gateway → ARP). `InterfaceInitialize` is idempotent
  (HW + worker once; a fresh ifnet per call; first = the live RX/DHCP target).
- **`bba_hw.c`** — RTL8139/GAPS hardware (factored from `drivers/bba/bba.c`) behind
  `BbaProbe/Init/Tx/RxPoll`, `SetKMode`-wrapped, with a `CRITICAL_SECTION` guarding the G2 ring
  (microstk TX thread vs RX worker).
- **`ras.c`** — the 13 `AfdRas*` exports (ordinals 1-13). `mras.lib` thunks `RasDial`/etc. to
  these; stubbed so dial-based titles "connect" instantly (link is already up). `AfdRasDial` posts
  `WM_RASDIALEVENT`/`RASCS_Connected`.
- **`w5500.c`** — W5500 MACRAW backend (`W5500Probe/Init/Tx/RxPoll`). MACRAW = raw Ethernet, so it
  drops into `LinkOps` and reuses all of netif.c. Loads `dcspi.dll` on demand (no link-time dep).
- **`netif.def`** — 16 exports at the **exact** mppp ordinals (1-13 AfdRas*, 14 FCSTable, 15
  InterfaceInitialize, 16 dllentry). Verified with `dumpbin`.

## SPI transport (`drivers/dcspi/`) — reusable

`dcspi.dll` exports `SpiInit/SpiShutdown/SpiSetCS/SpiRwByte/SpiRwData` with a `bus` arg, ported
from KallistiOS (`c:/dev/dreamcast/kallistios` — `hardware/sci.c`, `hardware/scif-spi.c`):
- **`DCSPI_BUS_SCI`** — SH-4 SCI clocked-synchronous mode = **true hardware SPI** (CS on PA7 GPIO;
  SCI shifts LSB-first so we `bit_reverse8`).
- **`DCSPI_BUS_SCIF`** — software bit-bang on `SCSPTR2` (CS on RTS; **shares the debug console**).

Standalone DLL on purpose: the future **SD/CF block drivers + a FAT FSD** link the same exports.
SetKMode-wrapped (P4 control registers).

### W5500 safety gate
Probing bit-bangs SCIF/SCI + GPIO, which would disturb a stock board's console/pins. So the W5500
is **OFF unless explicitly wired** via registry `HKLM\Comm\Netif : "W5500Bus"` (REG_DWORD) =
`1` (SCI, CS=PA7 GPIO) or `2` (SCIF, CS=RTS); absent/0 = disabled. The value also encodes the CS
wiring (not autodetectable). With it absent, the proven BBA/Flycast path is untouched. (A future
non-destructive autodetect — save/restore SCIF+SCI+GPIO around a `VERSIONR==0x04` probe — can make
this automatic; still needs the emulator below to be testable.)

## SDK-side edits (NOT in this repo — reproduce on a fresh SDK)

These live under `C:\wcedreamcast\release\debug\` (and the `retail` tree if you boot retail):

1. **Deploy** built DLLs into `…\OS\`: copy `reference/net-obj/mppp.dll` over the SDK's `mppp.dll`
   (back up the original to `mppp.dll.sdkorig`), and copy `reference/driver-obj/dcspi.dll` in.
2. **`ce.bib` + `platform.bib`**: add `dcspi.dll` to MODULES; **remove** the standalone `bba.dll`
   MODULES line (it now conflicts — the shim owns the BBA).
3. **`reginit.ini` + `gemini.reg`**: remove the `[HKLM\WDMDrivers\BuiltIn\BBA]` block (stops
   `wdevice` from loading the old standalone driver alongside the shim).
4. Rebuild: `build-image debug` → `wrap-image.ps1` → `make-gdi.ps1` → `reference/disc/disc.gdi`.

The standalone `drivers/bba` WDM driver is **retired** in favour of `bba_hw.c` inside the shim
(kept in-tree as the RTL8139 hardware reference).

## Test apps
- `shell/dcwnet.c` — winsock probe (WSAStartup → gethostbyname → connect → HTTP GET → recv),
  launched from the desktop. Proved the full path: `dns ok www.sega.com=192.185.5.88, tcp connect
  ok, recv, HTTP/1.1 301`.
- Diagnostic logs in `netif.c` (`netif:`/`netif TX[]`/`netif RX[]`/`DNS write`) + `w5500.c` —
  strip when stable.

## Next
1. **Emulate the W5500 in Flycast** (sources at `c:/dev/pc/flycast`) so the W5500 + dcspi + an
   autodetect become testable. Plan: a virtual W5500 device (register file + socket-0 MACRAW ring)
   on the **SCI bus** (byte-level: hook `SCTDR1` write in `core/hw/sh4/modules/serial.cpp`, which
   today registers SCI as dumb RW cells; CS via `BSC_PDTRA` bit7 in `bsc.cpp`), bridging MACRAW
   frames through the **existing** host NAT: TX → `net::modbba::receiveEthFrame` (`netservice.cpp`),
   RX ← dispatch inside `bba_recv_frame` (`bba.cpp`, the single host→DC entry both
   `picoppp.cpp`/`dcnet.cpp` use). SCIF-SPI would need bit-level SCSPTR2 modeling — skip; use SCI.
2. **Modem (PPP) backend** — backport from `mppp.dll` (in Ghidra): open the modem serial, LCP/auth/
   IPCP, deliver IP directly (no ARP/DHCP), feed IPCP IP+DNS into `NetifOnLease`/`WriteDnsServers`.
   Doesn't fit the Ethernet `LinkOps`; needs its own branch. May be testable via Flycast's modem.
