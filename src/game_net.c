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
unsigned char gn_ingame = 0;
unsigned char gn_tryagain = 0;
struct game_state our_game_state;
struct game_state remote_game_state;

struct UiSprite *try_again_mainsprite;

PadObj *FakePadUnk8(PadObj * pad, short a1) {
	return pad;
}
int FakePadUnkC(PadObj *pad) {}
int FakePadGetButton(PadObj *pad, int button, int *analog_value) {
	*analog_value = 0;
	if(gn_state == GN_CONNECTED && gn_ingame) {
		if(gn_tryagain && button == 0x2000) {
			return (remote_game_state.in_game == 4 || remote_game_state.in_game == 0) ? 1 : 0;
		} else if(gn_tryagain && button == 0x8000) {
			return (remote_game_state.in_game == 5 || remote_game_state.in_game == 3) ? 1 : 0;
		} else if(gn_tryagain && button == 0x40) {
			return (remote_game_state.in_game < 4);
		} else if(!gn_tryagain && (button == 0x40 || button == 0x800) && (remote_game_state.in_game == 4 || remote_game_state.in_game == 5)) {
			return 1;
		} else if(button == 0x40000) {
			*analog_value = remote_game_state.stick_x;
			return 1;
		} else if(button == 0x80000) {
			*analog_value = remote_game_state.stick_y;
			return 1;
		} else if(button == 0x80) {
			return ((remote_game_state.flags & GN_FLAG_BUTTON) == 1) ? 1 : 0;
		} else if(button == 0x40) {
			return ((remote_game_state.flags & GN_FLAG_BUTTON) == 2) ? 1 : 0;
		} else if(button == 0x20) {
			return ((remote_game_state.flags & GN_FLAG_BUTTON) == 3) ? 1 : 0;
		} else if(button == 0x10) {
			return ((remote_game_state.flags & GN_FLAG_BUTTON) == 4) ? 1 : 0;
		}
	}
	return 0;
}
void FakePadUnk14(PadObj *pad, int a1, int a2) {}
void FakePadUnk18(PadObj *pad, int a1, int a2) {}
void FakePadUnk1C(PadObj *pad) {}

struct PadObj_vtable fake_pad_vtable = {
	0, 0,
	FakePadUnk8,
	FakePadUnkC,
	FakePadGetButton,
	FakePadUnk14,
	FakePadUnk18,
	FakePadUnk1C
};
PadObj fake_pad = {
	.vtable = &fake_pad_vtable
};

float gn_last_attack_accum = 0.0f;
unsigned char gn_last_miss_ctr = 0;
unsigned char gn_last_ok_ctr = 0;
unsigned char gn_last_good_ctr = 0;
unsigned char gn_last_great_ctr = 0;

//void HandleMultiPlayerInputs(void *a0, unsigned int time,int button_instant,int button, float stick_x,float stick_y,float health_left,float health_right, unsigned int player_num,void *t1, int is_computer, int t3, int st0)
static int oHandleMultiPlayerInputs_buf[4];
static int (*oHandleMultiPlayerInputs)(void *a0, unsigned int time,int button_instant,int button, float stick_x,float stick_y,float health_left,float health_right, unsigned int player_num,void *t1, int is_computer, int t3, int st0);
int hHandleMultiPlayerInputs(void *a0, unsigned int time,int button_instant,int button, float stick_x,float stick_y,float health_left,float health_right, unsigned int player_num,void *t1, int is_computer, int t3, int st0) {
	//if(player_num == 1) time -= 4800 + rand() % 9600; // an experiment

	if(player_num == gn_remote_playernum && gn_state == GN_CONNECTED) {
		time = remote_game_state.time;
	}

	int ret = oHandleMultiPlayerInputs(a0, time, button_instant, button, stick_x, stick_y, health_left, health_right, player_num, t1, is_computer, t3, st0);
	//printf("player: %d, time: %d\n", player_num, time);
	if(player_num == gn_our_playernum && gn_state == GN_CONNECTED) {
		our_game_state.stick_x = (int)stick_x;
		our_game_state.stick_y = (int)stick_y;
		our_game_state.flags = (our_game_state.flags & ~GN_FLAG_BUTTON) | (button & GN_FLAG_BUTTON);
		our_game_state.time = time;
	}
	return ret;
}

//void UpdatePlayerStatus(PlayerStatusResult*, void*, void*, int, int, int);
static int oUpdatePlayerStatus_buf[4];
static void (*oUpdatePlayerStatus)(PlayerStatusResult*, void*, void*, int, int, int);
void hUpdatePlayerStatus(PlayerStatusResult* result, void* player_struct, void* a2, int a3, int t0, int t1) {
	if(gn_state != GN_CONNECTED) {
		oUpdatePlayerStatus(result, player_struct, a2, a3, t0, t1);
		return;
	}
	void *health_struct = *(void**)(player_struct + 0x288);
	void *other_player_struct = *(void**)(player_struct + 0x28c);
	void *other_health_struct = *(void**)(other_player_struct + 0x288);
	int player_num = *(int*)(health_struct + 0xa0);
	if(player_num == gn_remote_playernum && gn_state == GN_CONNECTED) {
		void *toast_1 = *(void**)(player_struct + 0x278);
		void *toast_2 = *(void**)(player_struct + 0x27C);
		void *toast_3 = *(void**)(player_struct + 0x280);
		void *toast_4 = *(void**)(player_struct + 0x284);
		*(void**)(player_struct + 0x278) = 0;
		*(void**)(player_struct + 0x27C) = 0;
		*(void**)(player_struct + 0x280) = 0;
		*(void**)(player_struct + 0x284) = 0;
		float pre_health_adj = *(float*)(health_struct + 0x170);
		float pre_health_adj_2 = *(float*)(other_health_struct + 0x170);
		oUpdatePlayerStatus(result, player_struct, a2, a3, t0, t1);
		*(float*)(health_struct + 0x170) = pre_health_adj;
		*(float*)(other_health_struct + 0x170) = pre_health_adj_2;
		*(void**)(player_struct + 0x278) = toast_1;
		*(void**)(player_struct + 0x27C) = toast_2;
		*(void**)(player_struct + 0x280) = toast_3;
		*(void**)(player_struct + 0x284) = toast_4;
		result->a = 0;
		result->b = 0;
		result->hit_type = -1;
		result->miss_flag = 0;
		if(remote_game_state.ctr_miss != gn_last_miss_ctr) {
			gn_last_miss_ctr++;
			ShowToast(toast_1);
			result->hit_type = 0;
			result->miss_flag = 1;
		} else if(remote_game_state.ctr_ok != gn_last_ok_ctr) {
			gn_last_ok_ctr++;
			ShowToast(toast_2);
			result->hit_type = 1;
		} else if(remote_game_state.ctr_good != gn_last_good_ctr) {
			gn_last_good_ctr++;
			ShowToast(toast_3);
			result->hit_type = 2;
		} else if(remote_game_state.ctr_great != gn_last_great_ctr) {
			gn_last_great_ctr++;
			ShowToast(toast_4);
			result->hit_type = 3;
		}
		int prev_score = *(int*)(health_struct + 156);
		if(prev_score != remote_game_state.score) {
			AdjustScore(health_struct, remote_game_state.score - prev_score);
		}
		float health = *(float*)(health_struct + 0x90) + *(float*)(health_struct + 0x170);
		if(remote_game_state.health > health + 0.0001f) {
			AddHealth(health_struct, remote_game_state.health - health);
		} else if(remote_game_state.health < health - 0.0001f) {
			SubtractHealth(health_struct, health - remote_game_state.health);
		}

	} else if(player_num == gn_our_playernum && gn_state == GN_CONNECTED) {
		if(remote_game_state.attack_accum != gn_last_attack_accum) {
			SubtractHealth(health_struct, remote_game_state.attack_accum - gn_last_attack_accum);
			gn_last_attack_accum = remote_game_state.attack_accum;
		}

		float remote_health_addition = *(float*)(other_health_struct + 0x170);
		oUpdatePlayerStatus(result, player_struct, a2, a3, t0, t1);
		float our_health = *(float*)(health_struct + 0x90) + *(float*)(health_struct + 0x170);
		float remote_health_addition_new = *(float*)(other_health_struct + 0x170);
		if(remote_health_addition_new < remote_health_addition) {
			our_game_state.attack_accum += (remote_health_addition - remote_health_addition_new);
		}
		*(float*)(other_health_struct + 0x170) = remote_health_addition_new;
		our_game_state.score = *(int*)(health_struct + 156);
		our_game_state.health = our_health;
		if(result->hit_type == 0) our_game_state.ctr_miss++;
		else if(result->hit_type == 1) our_game_state.ctr_ok++;
		else if(result->hit_type == 2) our_game_state.ctr_good++;
		else if(result->hit_type == 3) our_game_state.ctr_great++;
	} else {
		oUpdatePlayerStatus(result, player_struct, a2, a3, t0, t1);	
	}
}

//int MultiPlayerGame(int argv, char** argc);
static int oMultiPlayerGame_buf[4];
static int (*oMultiPlayerGame)(int argv, char** argc);
int hMultiPlayerGame(int argv, char** argc) {
	gn_ingame = 1;
	printf("setting gn_ingame = 1\n");
	if(gn_state == GN_CONNECTED) {
		printf("Networked multiplayer is on!\n");
		our_game_state.in_game = 1;
	}
	send_game_state();
	int ret = oMultiPlayerGame(argv, argc);
	our_game_state.in_game = 0;
	our_game_state.mm_screen = 2;
	our_game_state.mm_ready = 0;
	gn_ingame = 0;
	return ret;
}

void WakeupCB(int alarm_id, unsigned short time, void * thingy);

//void FinishLoading(int a0);
static int oFinishLoading_buf[4];
static void (*oFinishLoading)(int a0);
void hFinishLoading(int a0) {
	int tid;
	tid = GetThreadId();
	if(gn_ingame && gn_state == GN_CONNECTED) our_game_state.in_game = 2;
	send_game_state();
	if(gn_ingame && gn_state == GN_CONNECTED && remote_game_state.in_game < 2) {
		printf("Waiting for remote to finish loading...\n");
		int ctr = 0;
		while(remote_game_state.in_game < 2) {
			SetAlarm(160, WakeupCB, (void*)&tid);
			SleepThread();
			ctr++;
			if(ctr == 60) {
				send_game_state();
				ctr = 0;
			}
		}
		printf("Remote has finished loading. Starting game\n");
	}
	if(gn_ingame && gn_state == GN_CONNECTED) our_game_state.in_game = 3;
	send_game_state();
	oFinishLoading(a0);
}

//PadObj *GetPad(int index);
static int oGetPad_buf[4];
static PadObj *(*oGetPad)(int index);
PadObj *hGetPad(int index) {
	if(gn_state == GN_CONNECTED && gn_ingame) {
		if(index == gn_our_playernum) return oGetPad(0);
		else if(index == gn_remote_playernum) return &fake_pad;
		else return NULL;
	}
	return oGetPad(index);
}

static CueSetupBranch static_cue_branch = {0, 0.0f, 0, 0};
// int CueSetup(void* a0, int time, int left_avail, float left_health, int right_avail, float right_health);
static int oCueSetup_buf[4];
static int (*oCueSetup)(void* a0, int time, int left_avail, float left_health, int right_avail, float right_health);
int hCueSetup(void* a0, int time, int left_avail, float left_health, int right_avail, float right_health) {
	CueSetupBranch *old_branch = (a0 != NULL) ? *(CueSetupBranch**)(a0 + 0x180) : NULL;
	int did_thing = 0;
	int will_cue_setup = ((a0 != NULL) && (*(int*)(a0 + 0x214)) == 0) ? 1 : 0;
	if(a0 != NULL && gn_remote_playernum == 0 && gn_state == GN_CONNECTED && remote_game_state.next_cue > 0) {
		time = remote_game_state.time;
		static_cue_branch.if_true = remote_game_state.next_cue;
		*(CueSetupBranch**)(a0 + 0x180) = &static_cue_branch;
		did_thing = 1;
	}
	int ret = oCueSetup(a0, time, left_avail, left_health, right_avail, right_health);
	if(did_thing) {
		*(CueSetupBranch**)(a0 + 0x180) = old_branch;
	}
	if(ret && gn_state == GN_CONNECTED) {
		our_game_state.next_cue = *(int*)(a0 + 0x220);
	} else if(will_cue_setup && a0 != NULL && (*(int*)(a0 + 0x214)) != 0 && gn_state == GN_CONNECTED) {
		our_game_state.next_cue = 0;
	}
	return ret;
}

struct UiSprite *TryAgainMainSpriteHook(void* ui_holder, int texture_index, int unk, int id, float uv_x, float uv_y, float uv_width, float uv_height, float x, float y, float width, float height) {
	try_again_mainsprite = AddNewUiSprite(ui_holder, texture_index, unk, id, uv_x, uv_y, uv_width, uv_height, x, y, width, height);
	return try_again_mainsprite;
}

//int TryAgainScreen(int a0, int a1, int a2, int a3, int t0, float time)
static int oTryAgainScreen_buf[4];
static int (*oTryAgainScreen)(int a0, int a1, int a2, int a3, int t0, float time);
int hTryAgainScreen(int a0, int a1, int a2, int a3, int t0, float time) {
	gn_tryagain = 1;
	int ret = oTryAgainScreen(a0, a1, a2, a3, t0, time);
	try_again_mainsprite = NULL;
	gn_tryagain = 0;
	our_game_state.next_cue = 0;
	if(ret) {
		our_game_state.in_game = 3;
	} else {
		our_game_state.in_game = 0;
		our_game_state.mm_screen = 2;
	}
	return ret;
}

void send_game_state(void) {
	ResetPadButtonCache(&fake_pad);
	PadObj *pad = oGetPad(0);
	int meh;
	if(gn_state == GN_CONNECTED) {
		if(remote_game_state.flags & GN_FLAG_HANDOVER_ACK) our_game_state.flags &= ~GN_FLAG_HANDOVER;
		if(!(remote_game_state.flags & GN_FLAG_HANDOVER)) our_game_state.flags &= ~GN_FLAG_HANDOVER_ACK;
		if(!gn_ingame && our_game_state.mm_screen == 2) {
			if(pad->vtable->GetButton(pad, 0x20, &meh) && pad->vtable->GetButton(pad, 0x80, &meh) && !(remote_game_state.flags & GN_FLAG_HANDOVER) && gn_our_playernum == 0) {
				if(GetPadButtonInstant(pad, 0x20) || GetPadButtonInstant(pad, 0x80)) {
					our_game_state.flags |= GN_FLAG_HANDOVER;
					gn_our_playernum = 1;
					gn_remote_playernum = 0;
				}
			}
			if(remote_game_state.flags & GN_FLAG_HANDOVER) {
				our_game_state.flags |= GN_FLAG_HANDOVER_ACK;
				gn_our_playernum = 0;
				gn_remote_playernum = 1;
			}
			ResetPadButtonCache(pad);
		}
		if(gn_ingame && gn_tryagain && try_again_mainsprite != NULL) {
			our_game_state.in_game = (try_again_mainsprite->enabled ? 5 : 4);
		}
		if(gn_ingame && our_game_state.in_game == 3 && (remote_game_state.in_game == 3 || remote_game_state.in_game == 6)) {
			our_game_state.in_game = 6;
		}
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
	oHandleMultiPlayerInputs = CreateHook(HandleMultiPlayerInputs, hHandleMultiPlayerInputs, oHandleMultiPlayerInputs_buf);
	oUpdatePlayerStatus = CreateHook(UpdatePlayerStatus, hUpdatePlayerStatus, oUpdatePlayerStatus_buf);
	oMultiPlayerGame = CreateHook(MultiPlayerGame, hMultiPlayerGame, oMultiPlayerGame_buf);
	oFinishLoading = CreateHook(FinishLoading, hFinishLoading, oFinishLoading_buf);
	oGetPad = CreateHook(GetPad, hGetPad, oGetPad_buf);
	oCueSetup = CreateHook(CueSetup, hCueSetup, oCueSetup_buf);
	try_again_mainsprite_hook_point = 0x0C000000 | ((int)TryAgainMainSpriteHook >> 2);
	oTryAgainScreen = CreateHook(TryAgainScreen, hTryAgainScreen, oTryAgainScreen_buf);
}

void reset_game_net(void) {
	our_game_state.seq_num = 0;
	remote_game_state.seq_num = 0;
	our_game_state.in_game = 0;
	remote_game_state.in_game = 0;
	our_game_state.mm_screen = 2;
	remote_game_state.mm_screen = 2;
	our_game_state.attack_accum = 0;
	remote_game_state.attack_accum = 0.0f;
	our_game_state.attack_accum = 0.0f;
	gn_last_attack_accum = 0.0f;
	our_game_state.ctr_good = 0;
	our_game_state.ctr_great = 0;
	our_game_state.ctr_ok = 0;
	our_game_state.ctr_miss = 0;
	remote_game_state.ctr_good = 0;
	remote_game_state.ctr_great = 0;
	remote_game_state.ctr_ok = 0;
	remote_game_state.ctr_miss = 0;
	gn_last_miss_ctr = 0;
	gn_last_ok_ctr = 0;
	gn_last_good_ctr = 0;
	gn_last_great_ctr = 0;
	our_game_state.flags = 0;
	remote_game_state.flags = 0;
}

int HandleUdpGameNet(udp_datagram_t *dg, ethernet_frame_t *eth, ip_header_t *ip) {
	unsigned short ptype = *(short*)dg->payload;
	if(gn_state == GN_HOSTING && dg->dst_port == HTONS(GAME_PORT)) {
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
		&& IP_EQ(ip->source_ip, gn_remote_ip)) {
			if(ptype == GN_PACKETID_GAMESTATE) {
				struct game_state *incoming_gs = (struct game_state *)(dg->payload + 2);
				if(incoming_gs->seq_num > remote_game_state.seq_num) {
					remote_game_state = *incoming_gs;
				}
			} else if(ptype == GN_PACKETID_FIN) {
				gn_state = GN_IDLE;
			}
		}
	}
	return 0;
}
