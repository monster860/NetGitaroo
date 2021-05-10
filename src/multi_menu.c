#include "imports.h"
#include "hook.h"
#include "networking.h"
#include "game_net.h"
#include "multi_menu.h"
#include <stdlib.h>

#define UI_ID_BACKGROUND 310000
#define UI_ID_HOST 310001
#define UI_ID_JOIN 310002
#define UI_ID_COUCH 310003
#define UI_ID_HOSTIP 310100
#define UI_ID_CONNIP 310200
#define UI_ID_CONNECTING 310300
#define UI_ID_UNDERSCORE 319000
#define UI_ID_FADE_OVERLAY 319999

#define MENU_ID_EXITING 9
#define MENU_ID_MPTYPE 10
#define MENU_ID_HOST 11
#define MENU_ID_JOIN 12
#define MENU_ID_JOINING 13

typedef struct {
	struct UiSprite *dots[4]; // also includes a colon
	struct UiSprite *digits[12];
	struct UiSprite *port_digits[5];
	union {
		unsigned short shorts[2];
		unsigned char bytes[4];
	};
	unsigned short port;
	unsigned char include_port;
} ip_visual_t;

typedef struct {
	struct UiSprite *background;
	struct UiSprite *fade_overlay;
	struct UiSprite *type_menu[3];
	struct UiSprite *underscore;

	struct UiSprite *connecting_text;

	ip_visual_t hostip_visual;
	ip_visual_t connip_visual;
	char type_menu_item;
	char ip_digit;
} multi_menu_additions_t;

static float multi_menu_digit_x[] = {
	322, 334, 346, 358, 370, 382, 394, 406, 418, 430
};

static float mm_type_menu_uv_y[] = {
	297, 317, 337
};
#define MM_TYPE_MENU_UV_Y_OFFSET 61.0f

multi_menu_additions_t *multi_menu_additions;

void set_mp_type_menu_enabled(int enabled);
void set_hosting_menu_enabled(int enabled);
void set_join_menu_enabled(int enabled);
void set_connecting_scr_enabled(int enabled);

void initialize_ip_visual(void *ui_holder, ip_visual_t *visual, float x, float y, int id_base) {
	int i;
	for(i = 0; i < 4; i++) {
		int ip_digit = visual->bytes[i];
		int off = i*3;
		unsigned char digit_100 = (ip_digit / 100) % 10;
		unsigned char digit_10 = (ip_digit / 10) % 10;
		unsigned char digit_1 = (ip_digit) % 10;
		visual->digits[off+0] = AddNewUiSprite(ui_holder, 3, 1, id_base+i*4, multi_menu_digit_x[digit_100], 475, 10, 15, x+i*45, y, 10, 15);
		visual->digits[off+1] = AddNewUiSprite(ui_holder, 3, 1, id_base+i*4+1, multi_menu_digit_x[digit_10], 475, 10, 15, x+i*45+10, y, 10, 15);
		visual->digits[off+2] = AddNewUiSprite(ui_holder, 3, 1, id_base+i*4+2, multi_menu_digit_x[digit_1], 475, 10, 15, x+i*45+20, y, 10, 15);
		if(i < 3 || visual->include_port)
			visual->dots[i] = AddNewUiSprite(ui_holder, 3, 1, id_base+i*4+3, i == 3 ? 449 : 442, 475, 5, 15, x+i*45+35, y, 5, 15);
		if(digit_100 == 0) visual->digits[off+0]->enabled = 0;
		if(digit_10 == 0 && digit_100 == 0) visual->digits[off+1]->enabled = 0;
	}
	if(visual->include_port) {
		unsigned short shifted = visual->port;
		for(i = 4; i >= 0; i--) {
			unsigned char digit = shifted % 10;
			shifted = shifted / 10;
			visual->port_digits[i] = AddNewUiSprite(ui_holder, 3, 1, id_base+i+16, multi_menu_digit_x[digit], 475, 10, 15, x+180+i*10, y, 10, 15);	
			visual->port_digits[i]->enabled = (digit != 0);
		}
		unsigned char visible_yet = 0;
		for(i = 0; i < 5; i++) {
			struct UiSprite *d = visual->port_digits[i];
			if(d->enabled) visible_yet = 1;
			d->enabled = visible_yet;
		}
	}
}

void update_ip_visual(ip_visual_t *visual) {
	int i,j;
	for(i = 0; i < 4; i++) {
		int ip_digit = visual->bytes[i];
		int off = i*3;
		unsigned char digit_100 = (ip_digit / 100) % 10;
		unsigned char digit_10 = (ip_digit / 10) % 10;
		unsigned char digit_1 = (ip_digit) % 10;
		visual->digits[off+0]->uv_pixel_x = multi_menu_digit_x[digit_100];
		visual->digits[off+1]->uv_pixel_x = multi_menu_digit_x[digit_10];
		visual->digits[off+2]->uv_pixel_x = multi_menu_digit_x[digit_1];
		for(j = 0; j < 3; j++) {
			struct UiSprite *d = visual->digits[off+j];
			d->uv_left = (d->uv_pixel_x) * d->tex_width_inv;
			d->uv_right = (d->uv_pixel_x+d->uv_pixel_width) * d->tex_width_inv;
		}
		if(digit_100 == 0) visual->digits[off+0]->enabled = 0;
		else visual->digits[off+0]->enabled = 1;
		if(digit_10 == 0 && digit_100 == 0) visual->digits[off+1]->enabled = 0;
		else visual->digits[off+1]->enabled = 1;
	}
	if(visual->include_port) {
		unsigned short shifted = visual->port;
		for(i = 4; i >= 0; i--) {
			unsigned char digit = shifted % 10;
			shifted = shifted / 10;
			struct UiSprite *d = visual->port_digits[i];
			d->uv_pixel_x = multi_menu_digit_x[digit];
			d->uv_left = (d->uv_pixel_x) * d->tex_width_inv;
			d->uv_right = (d->uv_pixel_x+d->uv_pixel_width) * d->tex_width_inv;
			d->enabled = (digit != 0);
		}
		unsigned char visible_yet = 0;
		for(i = 0; i < 5; i++) {
			struct UiSprite *d = visual->port_digits[i];
			if(d->enabled) visible_yet = 1;
			d->enabled = visible_yet;
		}
	}
}

void set_ip_visual_enabled(ip_visual_t *visual, int enabled) {
	int i;
	for(i = 0; i < 12; i++) {
		visual->digits[i]->enabled = enabled;
	}
	for(i = 0; i < 4; i++) {
		if(i < 3 || visual->include_port)
			visual->dots[i]->enabled = enabled;
	}
	if(visual->include_port) {
		for(i = 0; i < 5; i++) {
			visual->port_digits[i]->enabled = enabled;
		}
	}
	if(enabled) update_ip_visual(visual);
}

static int oMultiMenuInitUi_buf[4];
static void (*oMultiMenuInitUi)(void*, void*, void*);
void hMultiMenuInitUi(void *multi_menu, void *ui_holder, void *third_holder) {
	oMultiMenuInitUi(multi_menu, ui_holder, third_holder);

	// (ui_holder, texture_index, 1, id, uv_x, uv_y, uv_width, uv_height, x, y, width, height);
	multi_menu_additions->background = AddNewUiSprite(ui_holder, 3, 1, UI_ID_BACKGROUND, 0, 288, 320, 224, 0, 0, 640, 448);
	multi_menu_additions->type_menu[0] = AddNewUiSprite(ui_holder, 3, 1, UI_ID_HOST, 403, mm_type_menu_uv_y[0]+MM_TYPE_MENU_UV_Y_OFFSET, 87, 19, 320 - (87.0/2.0), 448.0/4.0 - 19.0/2.0, 87, 19);
	multi_menu_additions->type_menu[1] = AddNewUiSprite(ui_holder, 3, 1, UI_ID_JOIN, 407, mm_type_menu_uv_y[1], 77, 19, 320 - (77.0/2.0), 448.0/4.0*2.0 - 19.0/2.0, 77, 19);
	multi_menu_additions->type_menu[2] = AddNewUiSprite(ui_holder, 3, 1, UI_ID_COUCH, 392, mm_type_menu_uv_y[2], 107, 19, 320 - (107.0/2.0), 448.0/4.0*3.0 - 19.0/2.0, 107, 19);

	IP_SET(multi_menu_additions->hostip_visual, our_ip);
	multi_menu_additions->hostip_visual.port = GAME_PORT;
	multi_menu_additions->hostip_visual.include_port = 1;
	initialize_ip_visual(ui_holder, &multi_menu_additions->hostip_visual, 205, 240, UI_ID_HOSTIP);

	multi_menu_additions->connip_visual.shorts[0] = 0xFFFF;
	multi_menu_additions->connip_visual.shorts[1] = 0xFFFF;
	multi_menu_additions->connip_visual.port = GAME_PORT;
	multi_menu_additions->connip_visual.include_port = 1;
	initialize_ip_visual(ui_holder, &multi_menu_additions->connip_visual, 205, 240, UI_ID_CONNIP);

	multi_menu_additions->fade_overlay = AddNewUiSprite(ui_holder, 3, 1, UI_ID_FADE_OVERLAY, 34, 84, 1, 1, 0, 0, 640, 448);
	multi_menu_additions->fade_overlay->enabled = 0;
	multi_menu_additions->fade_overlay->color[3] = 0;

	multi_menu_additions->underscore = AddNewUiSprite(ui_holder, 3, 1, UI_ID_UNDERSCORE, 463, 475, 10, 18, 0, 0, 10, 18);

	multi_menu_additions->connecting_text = AddNewUiSprite(ui_holder, 3, 1, UI_ID_CONNECTING, 388, 437, 123, 19, 320 - (123/2.0), 224 - (19/2.0), 123, 19);

	set_mp_type_menu_enabled(0);
	set_hosting_menu_enabled(0);
	set_join_menu_enabled(0);
	set_connecting_scr_enabled(0);
}

static int oMultiMenuLoop_buf[4];
static int (*oMultiMenuLoop)(int);
int hMultiMenuLoop(int skip_multitap_screen) {
	multi_menu_additions = malloc(sizeof(multi_menu_additions_t));
	multi_menu_additions->type_menu_item = 0;
	
	int ret = oMultiMenuLoop(skip_multitap_screen);
	
	free(multi_menu_additions);
	multi_menu_additions = NULL;

	if(ret == 0) {
		gn_state = GN_IDLE; // make sure to disconnect;
	}

	return ret;
}

void set_mp_type_menu_enabled(int enabled) {
	int i;
	for(i = 0; i < 3; i++) {
		multi_menu_additions->type_menu[i]->enabled = enabled;
	}
	multi_menu_additions->background->enabled = enabled;
}

void set_hosting_menu_enabled(int enabled) {
	multi_menu_additions->background->enabled = enabled;
	set_ip_visual_enabled(&multi_menu_additions->hostip_visual, enabled);
}

void set_join_menu_enabled(int enabled) {
	multi_menu_additions->background->enabled = enabled;
	set_ip_visual_enabled(&multi_menu_additions->connip_visual, enabled);
	multi_menu_additions->underscore->enabled = enabled;
}

void set_connecting_scr_enabled(int enabled) {
	multi_menu_additions->background->enabled = enabled;
	multi_menu_additions->connecting_text->enabled = enabled;
}

static int oMultiMenuHandleScreen_buf[4];
static void (*oMultiMenuHandleScreen)(void*);
void hMultiMenuHandleScreen(void *multi_menu) {
	int i;
	int menu_id = MM_MENUID(multi_menu);
	int prev_menu_id = MM_MENUID_PREV(multi_menu);
	int skip_multitap_screen = MM_SKIP_ARG(multi_menu);
	if(menu_id == 0 && !skip_multitap_screen) {
		menu_id = MM_MENUID(multi_menu) = MENU_ID_MPTYPE;
		MultiMenuChangeScreen(multi_menu, menu_id);
		set_mp_type_menu_enabled(1);
	}
	MultiMenuPads *pads = multi_menu + 0xa10;
	void** menu_sounds = *(void***)(multi_menu+0x50);

	if(gn_state == GN_CONNECTED && menu_id >= 2 && menu_id <= 4) {
		pads->num_pads = 2;
		pads->pads[gn_our_playernum] = pads->pads[0];
		pads->pads[gn_remote_playernum] = 0;
		if(remote_game_state.mm_screen >= 2 && remote_game_state.mm_screen <= 4) {
			int do_update = 1;
			int target_screen = remote_game_state.mm_screen;
			int curr_song = MM_SONG(multi_menu);
			if(gn_remote_playernum == 0 && curr_song != remote_game_state.mm_song && remote_game_state.mm_song < 7) {
				if(menu_id == 2) {
					MM_SONG(multi_menu) = remote_game_state.mm_song + 1;
					pads->pads[0] |= MM_PAD_UP;
					do_update = 0;
				} else {
					target_screen = 2;
				}
			}
			int remote_character = MM_PLAYER_CHARACTER(multi_menu, gn_remote_playernum);
			if(remote_game_state.mm_character != remote_character && curr_song == remote_game_state.mm_song) {
				if(menu_id == 3) {
					if(remote_game_state.mm_character > remote_character) {
						pads->pads[gn_remote_playernum] |= MM_PAD_LEFT;
					} else if(remote_game_state.mm_character < remote_character) {
						pads->pads[gn_remote_playernum] |= MM_PAD_RIGHT;
					}
					do_update = 0;
				} else if(menu_id == 4 && gn_remote_playernum == 0) {
					target_screen = 3;
				} else {
					if(remote_game_state.mm_character <= MM_PLAYER_MAXCHAR(multi_menu, gn_remote_playernum))
						MM_PLAYER_CHARACTER(multi_menu, gn_remote_playernum) = remote_game_state.mm_character;
				}
			}
			if(target_screen != menu_id && do_update && gn_remote_playernum == 0) {
				for(i = 0; i < pads->num_pads; i++) {
					pads->pads[i] |= (target_screen >= menu_id ? MM_PAD_X : MM_PAD_TRIANGLE);
				}
			}
		}
	}

	oMultiMenuHandleScreen(multi_menu);
	if(menu_id == MENU_ID_MPTYPE) {
		int menu_item_changed = 0;
		if((pads->pads[0] & MM_PAD_DOWN) && multi_menu_additions->type_menu_item+1 < 3) {
			menu_item_changed = 1;
			multi_menu_additions->type_menu_item++;
		} else if((pads->pads[0] & MM_PAD_UP) && multi_menu_additions->type_menu_item-1 >= 0) {
			menu_item_changed = 1;
			multi_menu_additions->type_menu_item--;
		}
		if(menu_item_changed) {
			PLAY_MENU_SOUND(menu_sounds[1], 0, 0);
			for(i = 0; i < 3; i++) {
				struct UiSprite *sprite = multi_menu_additions->type_menu[i];
				sprite->uv_pixel_y = mm_type_menu_uv_y[i] + (i == multi_menu_additions->type_menu_item ? MM_TYPE_MENU_UV_Y_OFFSET : 0);
				sprite->uv_top = sprite->uv_pixel_y * sprite->tex_height_inv;
				sprite->uv_bottom = (sprite->uv_pixel_y+sprite->uv_pixel_height) * sprite->tex_height_inv;
			}
		}
		if(pads->pads[0] & MM_PAD_X) {
			if(multi_menu_additions->type_menu_item == 2) {
				set_mp_type_menu_enabled(0);
				menu_id = MM_MENUID(multi_menu) = 1;
				MultiMenuChangeScreen(multi_menu, menu_id);
			} else if(multi_menu_additions->type_menu_item == 0) {
				PLAY_MENU_SOUND(menu_sounds[2], 0, 0);
				set_mp_type_menu_enabled(0);
				set_hosting_menu_enabled(1);
				menu_id = MM_MENUID(multi_menu) = MENU_ID_HOST;
				MultiMenuChangeScreen(multi_menu, menu_id);
				gn_src_port = GAME_PORT;
				gn_state = GN_HOSTING;
			} else if(multi_menu_additions->type_menu_item == 1) {
				PLAY_MENU_SOUND(menu_sounds[2], 0, 0);
				set_mp_type_menu_enabled(0);
				set_join_menu_enabled(1);
				menu_id = MM_MENUID(multi_menu) = MENU_ID_JOIN;
				MultiMenuChangeScreen(multi_menu, menu_id);
			}
		} else if(pads->pads[0] & MM_PAD_TRIANGLE) {
			PLAY_MENU_SOUND(menu_sounds[3], 0, 0);
			menu_id = MM_MENUID(multi_menu) = MENU_ID_EXITING;
			multi_menu_additions->fade_overlay->enabled = 1;
		}
	} else if(menu_id == MENU_ID_HOST) {
		if(gn_state == GN_CONNECTED) {
			set_hosting_menu_enabled(0);
			PLAY_MENU_SOUND(menu_sounds[0], 0, 0);
			menu_id = MM_MENUID(multi_menu) = 2;
			MultiMenuChangeScreen(multi_menu, menu_id);
		} else if(pads->pads[0] & MM_PAD_TRIANGLE) {
			set_hosting_menu_enabled(0);
			set_mp_type_menu_enabled(1);
			PLAY_MENU_SOUND(menu_sounds[3], 0, 0);
			menu_id = MM_MENUID(multi_menu) = MENU_ID_MPTYPE;
			multi_menu_additions->fade_overlay->enabled = 1;
			gn_state = GN_IDLE;
		}
	} else if(menu_id == MENU_ID_JOIN) {
		int menu_item_changed = 0;
		if(pads->pads[0] & MM_PAD_LEFT) {
			multi_menu_additions->ip_digit--;
			if(multi_menu_additions->ip_digit < 0) multi_menu_additions->ip_digit = 0;
			menu_item_changed = 1;
		}
		if(pads->pads[0] & MM_PAD_RIGHT) {
			multi_menu_additions->ip_digit++;
			if(multi_menu_additions->ip_digit > 16) multi_menu_additions->ip_digit = 16;
			menu_item_changed = 1;
		}
		int change = 0;
		if(pads->pads[0] & MM_PAD_DOWN) {
			change--;
			menu_item_changed = 1;
		}
		if(pads->pads[0] & MM_PAD_UP) {
			change++;
			menu_item_changed = 1;
		}
		if(change != 0) {
			if(multi_menu_additions->ip_digit > 12) {
				int digit = multi_menu_additions->ip_digit - 12;
				int mult = 10000;
				for(i = 0; i < digit; i++) {
					mult /= 10;
				}
				int port_change = mult*change;
				multi_menu_additions->connip_visual.port += port_change;
			} else {
				int octet_num = multi_menu_additions->ip_digit / 3;
				int digit = multi_menu_additions->ip_digit % 3;
				int mult = digit == 0 ? 100 : (digit == 1 ? 10 : 1);
				multi_menu_additions->connip_visual.bytes[octet_num] += mult*change;
			}
		}
		if(menu_item_changed || menu_id != prev_menu_id) {
			if(menu_item_changed) PLAY_MENU_SOUND(menu_sounds[1], 0, 0);
			update_ip_visual(&multi_menu_additions->connip_visual);
			if(multi_menu_additions->ip_digit > 16) multi_menu_additions->ip_digit = 16;
			if(multi_menu_additions->ip_digit < 0) multi_menu_additions->ip_digit = 0;
			// Yes I'm overflowing digits into port_digits.
			multi_menu_additions->underscore->x = multi_menu_additions->connip_visual.digits[multi_menu_additions->ip_digit]->x;
			multi_menu_additions->underscore->y = multi_menu_additions->connip_visual.digits[multi_menu_additions->ip_digit]->y;
		}
		if(pads->pads[0] & MM_PAD_TRIANGLE) {
			set_join_menu_enabled(0);
			set_mp_type_menu_enabled(1);
			PLAY_MENU_SOUND(menu_sounds[3], 0, 0);
			menu_id = MM_MENUID(multi_menu) = MENU_ID_MPTYPE;
		} else if(pads->pads[0] & MM_PAD_X) {
			set_join_menu_enabled(0);
			set_connecting_scr_enabled(1);
			PLAY_MENU_SOUND(menu_sounds[3], 0, 0);
			menu_id = MM_MENUID(multi_menu) = MENU_ID_JOINING;

			gn_src_port = (rand() & 0x3FFF) | 0xC000;
			IP_SET(gn_remote_ip, multi_menu_additions->connip_visual);
			IP_SET(gn_remote_ip_or_gateway, gn_remote_ip);
			gn_remote_port = multi_menu_additions->connip_visual.port;

			if(IP_EQ(gn_remote_ip, IP_BROADCAST)) {
				printf("Using broadcast adddress\n");
				HWADDR_SET(gn_remote_hwaddr, BROADCAST_HWADDR);
				gn_state = GN_CONNECTING;
			} else {
				ipaddr_t masked_remote_ip;
				ipaddr_t masked_gateway;
				IP_SET(masked_remote_ip, gn_remote_ip);
				IP_MASK(masked_remote_ip, subnet_mask);
				IP_SET(masked_gateway, gateway_ip);
				IP_MASK(masked_gateway, subnet_mask);
				printf("Subnet mask: %i.%i.%i.%i", subnet_mask.bytes[0], subnet_mask.bytes[1], subnet_mask.bytes[2], subnet_mask.bytes[3]);
				printf("Masked remote IP: %i.%i.%i.%i\n", masked_remote_ip.bytes[0], masked_remote_ip.bytes[1], masked_remote_ip.bytes[2], masked_remote_ip.bytes[3]);
				printf("Masked gateway: %i.%i.%i.%i\n", masked_gateway.bytes[0], masked_gateway.bytes[1], masked_gateway.bytes[2], masked_gateway.bytes[3]);
				if(!IP_EQ(masked_gateway, masked_remote_ip)) {
					printf("Masked IPs don't match, using gateway\n");
					IP_SET(gn_remote_ip_or_gateway, gn_remote_ip);
				}
				gn_state = GN_CONNECTING_ARP;
			}
		}
	} else if(menu_id == MENU_ID_JOINING) {
		if(gn_state == GN_CONNECTED) {
			set_connecting_scr_enabled(0);
			PLAY_MENU_SOUND(menu_sounds[0], 0, 0);
			menu_id = MM_MENUID(multi_menu) = 2;
			MultiMenuChangeScreen(multi_menu, menu_id);
		} else if(pads->pads[0] & MM_PAD_TRIANGLE) {
			set_connecting_scr_enabled(0);
			set_join_menu_enabled(1);
			PLAY_MENU_SOUND(menu_sounds[3], 0, 0);
			menu_id = MM_MENUID(multi_menu) = MENU_ID_JOIN;
			gn_state = GN_IDLE;
		}
	} else if(menu_id == MENU_ID_EXITING) {
		multi_menu_additions->fade_overlay->enabled = 1;
		if(multi_menu_additions->fade_overlay->color[3] >= 1) {
			MM_RETURNVAL(multi_menu) = 1; // return to main menu
			return;
		} else {
			multi_menu_additions->fade_overlay->color[3] += 0.01666666666f;
			if(multi_menu_additions->fade_overlay->color[3] >= 1)
				multi_menu_additions->fade_overlay->color[3] = 1;
		}
	}

	if(menu_id <= 4 && menu_id >= 2 && gn_state == GN_CONNECTED) {
		our_game_state.mm_screen = menu_id;
		our_game_state.mm_song = *(int*)(multi_menu+0x9e0);
		our_game_state.mm_character = MM_PLAYER_CHARACTER(multi_menu, gn_our_playernum);
		if(menu_id == 3 && MM_PLAYER_READY(multi_menu, gn_our_playernum)) {
			our_game_state.mm_ready = 3;
		} else {
			our_game_state.mm_ready = 0;
		}
	}

	send_game_state();
}

void init_multi_menu_hooks(void) {
	oMultiMenuInitUi = CreateHook(MultiMenuInitUi, hMultiMenuInitUi, oMultiMenuInitUi_buf);
	oMultiMenuLoop = CreateHook(MultiMenuLoop, hMultiMenuLoop, oMultiMenuLoop_buf);
	oMultiMenuHandleScreen = CreateHook(MultiMenuHandleScreen, hMultiMenuHandleScreen, oMultiMenuHandleScreen_buf);
}
