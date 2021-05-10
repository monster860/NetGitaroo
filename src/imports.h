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

typedef struct {
	int trace_state; // 0 means nothing, 1 means you can press button now, 2 means its too late
	int trace_ready;
	float unk_08;
	float trace_accuracy;
	float unk_10;
	float trace_dir;
	float unk_18;
	float unk_1c;
	float unk_20;
	float unk_24;
	float unk_28;
	float unk_2c;
} InGamePad;

typedef struct {
	int a;
	int b;
	int miss_flag;
	int hit_type;
} PlayerStatusResult;

void HandleInGameInputs(void* a0, unsigned int time, int a2, int a3, int t0, InGamePad* t1, int player_num, int t3, float stick_x, float stick_y);

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

void ShowToast(void*);
void UpdatePlayerStatus(PlayerStatusResult*, void*, void*, int, int, int);

// Some function that crashes the game sometimes for some IOP-related reason.
int CrashyFunc(void *a0, int a1, int a2, int a3);

// Some other function that causes the above crash when failing
int GetFile(void *a0, char *a1, int a2, int a3, int t0);

#endif
