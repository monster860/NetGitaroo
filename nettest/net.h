#include <netman.h>
#include <netman_rpc.h>

#ifndef NET_H
#define NET_H

#define HTONS(x) ((((x) & 0xff) << 8) | (((x) & 0xff00) >> 8))
#define NTOHS(x) HTONS(x)
#define HTONL(x) ((((x) & 0xff) << 24) | \
                     (((x) & 0xff00) << 8) | \
                     (((x) & 0xff0000UL) >> 8) | \
                     (((x) & 0xff000000UL) >> 24))
#define NTOHL(x) HTONL(x)

#define HWADDR_EQ(a, b) (((a).shorts[0] == (b).shorts[0]) && ((a).shorts[1] == (b).shorts[1]) && ((a).shorts[2] == (b).shorts[2]))
#define IP_EQ(a, b) (((a).shorts[0] == (b).shorts[0]) && ((a).shorts[1] == (b).shorts[1]))

#define HWADDR_SET(a, b) do {(a).shorts[0] = (b).shorts[0];(a).shorts[1] = (b).shorts[1];(a).shorts[2] = (b).shorts[2];} while(0)
#define IP_SET(a, b) do {(a).shorts[0] = (b).shorts[0];(a).shorts[1] = (b).shorts[1];} while(0)

#define DHCP_OP_BOOTREQUEST 1
#define DHCP_OP_BOOTREPLY 2

#define DHCPDISCOVER 1
#define DHCPOFFER 2
#define DHCPREQUEST 3
#define DHCPDECLINE 4
#define DHCPACK 5
#define DHCPNACK 6


typedef struct {
	union {
		unsigned char bytes[6];
		unsigned short shorts[3];
	};
} hwaddr_t;
typedef struct {
	union {
		unsigned char bytes[4];
		unsigned short shorts[2];
	};
} ipaddr_t;

typedef struct {
	hwaddr_t destination;
	hwaddr_t source;
	unsigned short type;
	unsigned char payload[];
} ethernet_frame_t;

typedef struct {
	unsigned char version_ihl;
	unsigned char dscp_ecn;
	unsigned short total_length;
	unsigned short identification;
	unsigned short flags_fragment_offset;
	unsigned char ttl;
	unsigned char protocol;
	unsigned short header_checksum;
	ipaddr_t source_ip;
	ipaddr_t destination_ip;
} ip_header_t;

typedef struct {
	unsigned short src_port;
	unsigned short dst_port;
	unsigned short length;
	unsigned short checksum;
	unsigned char payload[];
} udp_datagram_t;

typedef struct {
	unsigned char op;
	unsigned char htype;
	unsigned char hlen;
	unsigned char hops;
	unsigned short xid_lower;
	unsigned short xid_upper;
	unsigned short secs;
	unsigned short flags;
	ipaddr_t ciaddr;
	ipaddr_t yiaddr;
	ipaddr_t siaddr;
	ipaddr_t giaddr;
	hwaddr_t chaddr;
	unsigned char chaddr_padding[10];
	char server[64];
	char file[128];
	unsigned char cookie[4];
	unsigned char options[];
} dhcp_packet_t;

typedef struct {
	unsigned short htype;
	unsigned short ptype;
	unsigned char hlen;
	unsigned char plen;
	unsigned short oper;
	hwaddr_t sender_hw;
	ipaddr_t sender_ip;
	hwaddr_t target_hw;
	ipaddr_t target_ip;
} arp_packet_t;

struct packet_buffer {
	unsigned char data[NETMAN_NETIF_FRAME_SIZE];
	struct packet_buffer* next;
	int size;
	unsigned char priority;
	unsigned char transmitting;
} ALIGNED(64);

#define NUM_PACKET_BUFFERS (NETMAN_RPC_BLOCK_SIZE_LESSER+1)
#define NUM_TX_PACKET_BUFFERS (16)
extern struct packet_buffer packet_buffers[NUM_PACKET_BUFFERS];
extern struct packet_buffer tx_packet_buffers[NUM_TX_PACKET_BUFFERS];
extern struct packet_buffer *free_packet_buffer;
extern struct packet_buffer *free_tx_packet_buffer;
extern struct packet_buffer *next_tx_packet_buffer;

#define NET_PRIORITY_ARP 2
#define NET_PRIORITY_DHCP 0
#define NET_PRIORITY_GAME 1
#define NET_PRIORITY_LOGGING 3

#define LOGGING_PORT 22577

#endif
