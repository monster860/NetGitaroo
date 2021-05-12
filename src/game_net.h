#ifndef GAME_NET_H
#define GAME_NET_H

#include "networking.h"

#define GN_IDLE 0
#define GN_CONNECTING_ARP 1
#define GN_CONNECTING 2
#define GN_HOSTING 3
#define GN_CONNECTED 4

// modify this for different versions I guess
#define GN_PACKETID_CONNECT 33865

#define GN_PACKETID_ACK 8478
#define GN_PACKETID_GAMESTATE 48347
#define GN_PACKETID_FIN 17844

extern ipaddr_t gn_remote_ip;
extern ipaddr_t gn_remote_ip_or_gateway;
extern hwaddr_t gn_remote_hwaddr;
extern unsigned short gn_remote_port;
extern unsigned short gn_src_port;
extern unsigned char gn_state;
extern unsigned char gn_our_playernum;
extern unsigned char gn_remote_playernum;
extern unsigned char gn_ingame;

#define GN_FLAG_BUTTON 7
#define GN_FLAG_HANDOVER 8
#define GN_FLAG_HANDOVER_ACK 16

struct game_state {
	unsigned int seq_num;
	unsigned int score;
	float health;
	float angle_accuracy;
	unsigned int time;
	float attack_accum;
	char stick_x;
	char stick_y;
	unsigned char flags;
	unsigned char next_cue;
	unsigned char ctr_miss;
	unsigned char ctr_ok;
	unsigned char ctr_good;
	unsigned char ctr_great;
	
	unsigned char in_game; // 0 = multi menu, 1 = loading screen, 2 = loading screen, waiting for other player, 3 = in game, 4 = try again (back), 5 = try again (retry), 6 = in game+ack
	unsigned char mm_screen;
	unsigned char mm_song;
	unsigned char mm_character;
	unsigned char mm_ready;
};

extern struct game_state our_game_state;
extern struct game_state remote_game_state;

int HandleUdpGameNet(udp_datagram_t *dg, ethernet_frame_t *eth, ip_header_t *ip);

void init_game_net_hooks(void);
void send_game_state(void);

#endif
