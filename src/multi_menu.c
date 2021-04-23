#include "imports.h"
#include "hook.h"
#include <stdlib.h>

#define UI_ID_BACKGROUND 310000
#define UI_ID_HOST 310001
#define UI_ID_JOIN 310002
#define UI_ID_COUCH 310003
#define UI_ID_TESTIP 310100

#define MENU_ID_MPTYPE 10

#define PLAY_MENU_SOUND(a,b,c) (*(void(**)(void*,int,int))(*(void**)(a+0x18) + 0xC))(a,b,c)

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
	struct UiSprite *type_menu[3];
	ip_visual_t test_visual;
	int type_menu_item;
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

static void initialize_ip_visual(void *ui_holder, ip_visual_t *visual, float x, float y, int id_base) {
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
		
	}
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
	set_mp_type_menu_enabled(0);

	multi_menu_additions->test_visual.bytes[0] = 192;
	multi_menu_additions->test_visual.bytes[1] = 168;
	multi_menu_additions->test_visual.bytes[2] = 137;
	multi_menu_additions->test_visual.bytes[3] = 25;
	initialize_ip_visual(ui_holder, &multi_menu_additions->test_visual, 20, 20, UI_ID_TESTIP);
}

static int oMultiMenuLoop_buf[4];
static int (*oMultiMenuLoop)(int);
int hMultiMenuLoop(int skip_multitap_screen) {
	multi_menu_additions = malloc(sizeof(multi_menu_additions_t));
	multi_menu_additions->type_menu_item = 0;
	
	int ret = oMultiMenuLoop(skip_multitap_screen);
	
	free(multi_menu_additions);
	multi_menu_additions = NULL;

	return ret;
}

void set_mp_type_menu_enabled(int enabled) {
	int i;
	for(i = 0; i < 3; i++) {
		multi_menu_additions->type_menu[i]->enabled = enabled;
	}
	multi_menu_additions->background->enabled = enabled;
}

static int oMultiMenuHandleScreen_buf[4];
static void (*oMultiMenuHandleScreen)(void*);
void hMultiMenuHandleScreen(void *multi_menu) {
	int i;
	int menu_id = *(int*)(multi_menu + 0x9c4);
	int skip_multitap_screen = *(int*)(multi_menu + 0xa04);
	if(menu_id == 0 && !skip_multitap_screen) {
		menu_id = *(int*)(multi_menu + 0x9c4) = MENU_ID_MPTYPE;
		MultiMenuChangeScreen(multi_menu, menu_id);
		set_mp_type_menu_enabled(1);
	}
	oMultiMenuHandleScreen(multi_menu);
	MultiMenuPads *pads = multi_menu + 0xa10;
	void** menu_sounds = *(void***)(multi_menu+0x50);
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
				menu_id = *(int*)(multi_menu + 0x9c4) = 1;
				MultiMenuChangeScreen(multi_menu, menu_id);
			}
		} else if(pads->pads[0] & MM_PAD_TRIANGLE) {
			PLAY_MENU_SOUND(menu_sounds[3], 0, 0);
			*(int*)(multi_menu+0xa00) = 1; // return to main menu
		}
	}
}

void init_multi_menu_hooks(void) {
	oMultiMenuInitUi = CreateHook(MultiMenuInitUi, hMultiMenuInitUi, oMultiMenuInitUi_buf);
	oMultiMenuLoop = CreateHook(MultiMenuLoop, hMultiMenuLoop, oMultiMenuLoop_buf);
	oMultiMenuHandleScreen = CreateHook(MultiMenuHandleScreen, hMultiMenuHandleScreen, oMultiMenuHandleScreen_buf);
}
