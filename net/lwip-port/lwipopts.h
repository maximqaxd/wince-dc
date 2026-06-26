//
// lwipopts.h - lwIP 2.2.0 configuration for the Dreamcast WinCE net stack.
// First cut: NO_SYS (no OS threads; raw/callback API + a poll loop), one or more
// netifs (ethernet for BBA/W5500, PPP for the modem) feeding one IP core.
// Sockets/netconn (which need a sys_arch thread port) come later.
//
#ifndef LWIPOPTS_H
#define LWIPOPTS_H

#define NO_SYS                      1
#define SYS_LIGHTWEIGHT_PROT        0
#define LWIP_NETCONN                0
#define LWIP_SOCKET                 0
#define LWIP_NETCONN_SEM_PER_THREAD 0

// --- memory: lwIP's own heap + pools (no libc malloc) ---
#define MEM_LIBC_MALLOC             0
#define MEMP_MEM_MALLOC             0
#define MEM_ALIGNMENT               4
#define MEM_SIZE                    (64 * 1024)

// --- protocols ---
#define LWIP_ARP                    1
#define LWIP_ETHERNET               1
#define LWIP_IPV4                   1
#define LWIP_IPV6                   0
#define LWIP_DHCP                   1
#define LWIP_AUTOIP                 0
#define LWIP_ICMP                   1
#define LWIP_RAW                    1
#define LWIP_UDP                    1
#define LWIP_TCP                    1
#define LWIP_DNS                    1

// --- netif features ---
#define LWIP_NETIF_STATUS_CALLBACK  1
#define LWIP_NETIF_LINK_CALLBACK    1
#define LWIP_NETIF_HOSTNAME         1
#define LWIP_SINGLE_NETIF           0      // multiple links (modem/bba/w5500)

// --- TCP sizing (modest for the DC) ---
#define TCP_MSS                     1460
#define TCP_WND                     (8 * TCP_MSS)
#define TCP_SND_BUF                 (8 * TCP_MSS)
#define MEMP_NUM_TCP_SEG            32     // >= TCP_SND_QUEUELEN (lwIP sanity check)

// --- software checksums (no hardware offload) ---
#define CHECKSUM_GEN_IP             1
#define CHECKSUM_GEN_UDP            1
#define CHECKSUM_GEN_TCP            1
#define CHECKSUM_GEN_ICMP           1
#define CHECKSUM_CHECK_IP           1
#define CHECKSUM_CHECK_UDP          1
#define CHECKSUM_CHECK_TCP          1
#define CHECKSUM_CHECK_ICMP         1

// --- diagnostics / stats off for the first bring-up ---
#define LWIP_DEBUG                  0
#define LWIP_STATS                  0

#endif // LWIPOPTS_H
