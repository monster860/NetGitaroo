#include "game_net.h"
#include "imports.h"
#include "hook.h"
#include <stdlib.h>	
#include <ps2sdkapi.h>

ipaddr_t gn_remote_ip;
ipaddr_t gn_remote_ip_or_gateway;
hwaddr_t gn_remote_hwaddr;
unsigned short gn_remote_port;
unsigned short gn_src_port = 13438;
unsigned char gn_state = GN_IDLE;
ps2_clock_t gn_last_send = 0;
unsigned char gn_our_playernum = 0;
unsigned char gn_remote_playernum = 1;

//void HandleInGameInputs(void* a0, unsigned int time, int a2, int a3, int t0, InGamePad* t1, int player_num, int t3, float stick_x, float stick_y);
static int oHandleInGameInputs_buf[4];
static void (*oHandleInGameInputs)(void* a0, unsigned int time, int a2, int a3, int t0, InGamePad* t1, int player_num, int t3, float stick_x, float stick_y);
void hHandleInGameInputs(void* a0, unsigned int time, int a2, int a3, int t0, InGamePad* t1, int player_num, int t3, float stick_x, float stick_y) {
	if(player_num == 1) time -= 4800 + rand() % 9600; // an experiment
	oHandleInGameInputs(a0, time, a2, a3, t0, t1, player_num, t3, stick_x, stick_y);
}

//void UpdatePlayerStatus(PlayerStatusResult*, void*, void*, int, int, int);
static int oUpdatePlayerStatus_buf[4];
static void (*oUpdatePlayerStatus)(PlayerStatusResult*, void*, void*, int, int, int);
void hUpdatePlayerStatus(PlayerStatusResult* result, void* player_struct, void* a2, int a3, int t0, int t1) {
	oUpdatePlayerStatus(result, player_struct, a2, a3, t0, t1);
}

void send_game_state(void) {
	if(gn_state == GN_CONNECTED) {
		struct packet_buffer *out_buf;
		udp_datagram_t *dg = MakeUdpPacket(&out_buf, &gn_remote_ip, &gn_remote_hwaddr, gn_src_port, gn_remote_port, (sizeof(struct game_state) + 3) & ~1, NET_PRIORITY_GAME);
		if(dg != NULL) {
			our_game_state.seq_num++;
			*(short*)dg->payload = GN_PACKETID_GAMESTATE;
			*(struct game_state *)(dg->payload + 2) = our_game_state;
			FinalizeUdpChecksum(dg);
			AddTxPacket(out_buf);
		}
	} else if(gn_state == GN_CONNECTING_ARP) {
		ps2_clock_t curr_clock = ps2_clock();
		if((curr_clock - gn_last_send) > PS2_CLOCKS_PER_SEC) {
			struct packet_buffer *arp_buf = AllocTxPacket(NET_PRIORITY_GAME);
			if(arp_buf != NULL) {
				gn_last_send = curr_clock;
				ethernet_frame_t *frame = (ethernet_frame_t*)arp_buf->data;
				HWADDR_SET(frame->destination, BROADCAST_HWADDR);
				HWADDR_SET(frame->source, our_hwaddr);
				frame->type = HTONS(0x0806);
				arp_packet_t *arp = (arp_packet_t*)frame->payload;
				arp->htype = HTONS(1);
				arp->ptype = HTONS(0x0800);
				arp->hlen = 6;
				arp->plen = 4;
				arp->oper = HTONS(1);
				IP_SET(arp->sender_ip, our_ip);
				HWADDR_SET(arp->sender_hw, our_hwaddr);
				IP_SET(arp->target_ip, gn_remote_ip_or_gateway);
				HWADDR_SET(arp->target_hw, NULL_HWADDR);
				arp_buf->size = sizeof(*frame) + sizeof(*arp);
				AddTxPacket(arp_buf);
			}
		}
	} else if(gn_state == GN_CONNECTING) {
		ps2_clock_t curr_clock = ps2_clock();
		if((curr_clock - gn_last_send) > PS2_CLOCKS_PER_SEC) {
			struct packet_buffer *out_buf;
			udp_datagram_t *dg = MakeUdpPacket(&out_buf, &gn_remote_ip, &gn_remote_hwaddr, gn_src_port, gn_remote_port, 2, NET_PRIORITY_GAME);
			if(dg != NULL) {
				gn_last_send = curr_clock;
				*(short*)dg->payload = GN_PACKETID_CONNECT;
				FinalizeUdpChecksum(dg);
				AddTxPacket(out_buf);
			}
		}
	}
}

void init_game_net_hooks(void) {
	oHandleInGameInputs = CreateHook(HandleInGameInputs, hHandleInGameInputs, oHandleInGameInputs_buf);
}

void reset_game_net(void) {
	our_game_state.seq_num = 0;
	remote_game_state.seq_num = 0;
	our_game_state.in_game = 0;
	remote_game_state.in_game = 0;
	our_game_state.mm_screen = 2;
	remote_game_state.mm_screen = 2;
}

int HandleUdpGameNet(udp_datagram_t *dg, ethernet_frame_t *eth, ip_header_t *ip) {
	unsigned short ptype = *(short*)dg->payload;
	if(gn_state == GN_HOSTING && dg->dst_port == GAME_PORT) {
		if(ptype == GN_PACKETID_CONNECT) {
			IP_SET(gn_remote_ip, ip->source_ip);
			HWADDR_SET(gn_remote_hwaddr, eth->source);
			gn_remote_port = NTOHS(dg->src_port);
			gn_src_port = NTOHS(dg->dst_port);

			struct packet_buffer *out_buf;
			udp_datagram_t *reply_dg = MakeUdpPacket(&out_buf, &gn_remote_ip, &gn_remote_hwaddr, gn_src_port, gn_remote_port, 2, NET_PRIORITY_GAME);
			if(reply_dg != NULL) {
				our_game_state.seq_num++;
				*(short*)reply_dg->payload = GN_PACKETID_ACK;
				FinalizeUdpChecksum(reply_dg);
				AddTxPacket(out_buf);
			}
		} else if(ptype == GN_PACKETID_GAMESTATE && IP_EQ(ip->source_ip, gn_remote_ip)) {
			HWADDR_SET(gn_remote_hwaddr, eth->source);
			gn_remote_port = NTOHS(dg->src_port);
			gn_src_port = NTOHS(dg->dst_port);
			gn_state = GN_CONNECTED;
			gn_our_playernum = 0;
			gn_remote_playernum = 1;
			reset_game_net();
		}
	} else if(gn_state == GN_CONNECTING) {
		if(NTOHS(dg->src_port) == gn_remote_port && NTOHS(dg->dst_port) == gn_src_port
		&& (IP_EQ(gn_remote_ip, IP_BROADCAST) || IP_EQ(ip->source_ip, gn_remote_ip))
		&& ptype == GN_PACKETID_ACK) {
			IP_SET(gn_remote_ip, ip->source_ip);
			HWADDR_SET(gn_remote_hwaddr, eth->source);
			gn_state = GN_CONNECTED;
			gn_our_playernum = 1;
			gn_remote_playernum = 0;
			reset_game_net();
		}
	}
	if(gn_state == GN_CONNECTED) {
		if(NTOHS(dg->src_port) == gn_remote_port && NTOHS(dg->dst_port) == gn_src_port
		&& IP_EQ(ip->destination_ip, gn_remote_ip)) {
			if(ptype == GN_PACKETID_GAMESTATE) {
				struct game_state *incoming_gs = (struct game_state *)(dg->payload + 2);
				if(incoming_gs->seq_num > remote_game_state.seq_num) {
					remote_game_state = *incoming_gs;
				}
			}
		}
	}
	return 0;
}
