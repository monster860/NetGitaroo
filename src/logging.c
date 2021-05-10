#include <stdio.h>
#include <string.h>
#include "logging.h"
#include "networking.h"

static int curr_client = -1;

ipaddr_t logging_cip = {0,0,0,0};
hwaddr_t logging_chwaddr = {0,0,0,0,0,0};
unsigned short logging_cport;

void HandleLoggingPacket(udp_datagram_t *dg, ethernet_frame_t *eth, ip_header_t *ip) {
	IP_SET(logging_cip, ip->source_ip);
	HWADDR_SET(logging_chwaddr, eth->source);
	logging_cport = NTOHS(dg->src_port);
}

void LogStr(const char* str, int length) {
	/*if(curr_client >= 0) {
		lwip_write(curr_client, str, length);
	}*/
	if(link_up && !IP_EQ(logging_cip, IP_ANY)) {
		// udp_datagram_t *MakeUdpPacket(struct packet_buffer **out_buf, ipaddr_t *dst, hwaddr_t *dst_hwaddr, unsigned short src_port, unsigned short dst_port, unsigned short payload_length, unsigned char priority);
		struct packet_buffer *buf;
		int packet_len = (length + 3) & ~3;
		udp_datagram_t *dg = MakeUdpPacket(&buf, &logging_cip, &logging_chwaddr, LOGGING_PORT, logging_cport, packet_len, NET_PRIORITY_LOGGING);
		if(dg != NULL) {
			memset(dg->payload, 0, packet_len);
			memcpy(dg->payload, str, length);
			FinalizeUdpChecksum(dg);
			AddTxPacket(buf);
		}
	}
}
