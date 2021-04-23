#include <stdio.h>
#include <loadfile.h>
#include "networking.h"
#include "logging.h"
#include "hook.h"
#include "imports.h"
#include <string.h>
#include "multi_menu.h"
#include <time.h>

// Important: Keep the final out_us.bin file under 286 KB or the game will not have enough memory to run some stages.

void _ps2sdk_time_init(void);

static int oFrameWait_buf[4];
static void (*oFrameWait)(void);
static void hFrameWait(void) {
	oFrameWait();
	printf("clock: %i\n", ps2_clock());
}

static int o_puts_internal_buffer[4];
static void (*o_puts_internal)(const char*);
static void h_puts_internal(const char *str) {
	LogStr(str, strlen(str));
	o_puts_internal(str);
}

void load_mod(void) {
    _ps2sdk_time_init();
	printf("Loading networking IRX modules...\n");
	SifLoadModule("cdrom0:\\MODULES\\PS2DEV9.IRX;1", 0, "");
	SifLoadModule("cdrom0:\\MODULES\\NETMAN.IRX;1", 0, "");
	SifLoadModule("cdrom0:\\MODULES\\SMAP.IRX;1", 8, "-verbose");
	
	// Insert hooks
	oFrameWait = CreateHook(FrameWait, hFrameWait, oFrameWait_buf);
	o_puts_internal = CreateHook(puts_internal, h_puts_internal, o_puts_internal_buffer);
	init_multi_menu_hooks();

	//InitConnection();
}


//char pre_gp[32768] __attribute__ ((aligned (128)));
//char _gp[32768];
