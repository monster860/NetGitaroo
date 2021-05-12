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

void HandleInGameInputs(void* a0, unsigned int time, int button_instant, int button, int t0, InGamePad* t1, int player_num, int t3, float stick_x, float stick_y);
int HandleMultiPlayerInputs(void *a0, unsigned int time,int button_instant, int button, float stick_x,float stick_y,float health_left,float health_right, unsigned int player_num, void *t1, int is_computer, int t3, int st0);

#define MM_PAD_X 0x1
#define MM_PAD_TRIANGLE 0x2
#define MM_PAD_UP_HELD 0x20
#define MM_PAD_DOWN_HELD 0x40
#define MM_PAD_UP 0x80
#define MM_PAD_DOWN 0x100
#define MM_PAD_LEFT 0x200
#define MM_PAD_RIGHT 0x400

#define INGAME_PAD_TRIANGLE 4
#define INGAME_PAD_X 2
#define INGAME_PAD_O 3
#define INGAME_PAD_SQUARE 1

void MultiMenuUpdatePads(MultiMenuPads *);

void MultiMenuInitUi(void *multi_menu, void *ui_holder, void *third_holder);
void MultiMenuHandleScreen(void *multi_menu);
void MultiMenuChangeScreen(void *multi_menu, int screen_id);

int MultiMenuLoop(int);

int MultiPlayerGame(int argv, char** argc);

void ShowToast(void*);
void UpdatePlayerStatus(PlayerStatusResult*, void*, void*, int, int, int);
void AddHealth(void*, float);
void SubtractHealth(void*, float);
void AdjustScore(void*, int);

void FinishLoading(int a0);

int TryAgainScreen(int a0, int a1, int a2, int a3, int t0, float time);

// Some function that crashes the game sometimes for some IOP-related reason.
int CrashyFunc(void *a0, int a1, int a2, int a3);

// Some other function that causes the above crash when failing
int GetFile(void *a0, char *a1, int a2, int a3, int t0);

struct PadObj_vtable;

struct PadObj {
	int buttons;
	int unk_4;
	char analog_rx;
	char analog_ry;
	char analog_lx;
	char analog_ly;
	int unk_c;
	int unk_10;
	int unk_14;
	int unk_18;
	int unk_1c;
	int unk_20;
	unsigned int button_cache;
	int unk_28;
	int unk_2c;
	int unk_30;
	int unk_34;
	int unk_38;
	int unk_3c;
	int unk_40;
	int unk_44;
	int unk_48;
	int unk_4c;
	struct PadObj_vtable *vtable;
};
typedef struct PadObj PadObj;

struct PadObj_vtable {
	int unk_0;
	int unk_4;
	PadObj *(*unk_8)(PadObj *, short);
	int (*unk_c)(PadObj *);
	int (*GetButton)(PadObj *, int button, int*);
	void (*unk_14)(PadObj *, int, int);
	void (*unk_18)(PadObj *, int, int);
	void (*unk_1c)(PadObj *);
};

void ResetPadButtonCache(PadObj *pad);
PadObj *GetPad(int index);
int GetPadButtonInstant(PadObj *pad, int button);


typedef struct {
	int type;
	float param;
	int if_true;
	int if_false;
} CueSetupBranch;

int CueSetup(void* a0, int time, int left_avail, float left_health, int right_avail, float right_health);

extern int try_again_mainsprite_hook_point;

#endif
