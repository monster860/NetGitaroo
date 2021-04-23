#ifndef IMPORTS_H
#define IMPORTS_H

void FrameWait(void);
void puts_internal(const char*);

struct UiSprite {
	int id;
	int enabled;
	int unk1[2];
	float x;
	float y;
	float unk2; // must be 1
	int unk3[9];
	void *texture_ptr;
	int unk4a;
	int blend_mode; // 2 for normal, 0 for white texture
	int unk4b[15];
	void *vtable;
	float tex_width;
	float tex_height;
	float tex_width_inv;
	float tex_height_inv;
	float uv_left;
	float uv_top;
	float uv_right;
	float uv_bottom;
	float unk6;
	float uv_pixel_x;
	float uv_pixel_y;
	float uv_pixel_width;
	float uv_pixel_height;
	float width;
	float height;
	float unk7[2];
	float color[4];
};

struct UiSprite *AddNewUiSprite(void* ui_holder, int texture_index, int unk, int id, float uv_x, float uv_y, float uv_width, float uv_height, float x, float y, float width, float height);

typedef struct {
	int num_pads;
	int unk[3];
	int pads[4];
} MultiMenuPads;

#define MM_PAD_X 0x1
#define MM_PAD_TRIANGLE 0x2
#define MM_PAD_UP_HELD 0x20
#define MM_PAD_DOWN_HELD 0x40
#define MM_PAD_UP 0x80
#define MM_PAD_DOWN 0x100
#define MM_PAD_LEFT 0x200
#define MM_PAD_RIGHT 0x400

void MultiMenuUpdatePads(MultiMenuPads *);

void MultiMenuInitUi(void *multi_menu, void *ui_holder, void *third_holder);
void MultiMenuHandleScreen(void *multi_menu);
void MultiMenuChangeScreen(void *multi_menu, int screen_id);

int MultiMenuLoop(int);

#endif
