#include <kernel.h>
#include <stdlib.h>
#include <stdio.h>
#include <loadfile.h>
#include "networking.h"
#include "logging.h"
#include "hook.h"
#include "imports.h"
#include <string.h>
#include "multi_menu.h"
#include "game_net.h"
#include <time.h>

// Important: Keep the final out_us.bin file under 286 KB or the game will not have enough memory to run some stages.

void _ps2sdk_time_init(void);

int frame_waiting_tid = -1;
int frame_wait_tid = -1;
int frame_wait_status = 0;
static int oFrameWait_buf[4];
static void (*oFrameWait)(void);
static void hFrameWait(void) {
	process_dhcp();
	if(frame_wait_tid >= 0) {
		frame_waiting_tid = GetThreadId();
		frame_wait_status = 1;
		WakeupThread(frame_wait_tid);
		while(frame_wait_status == 1) {
			SleepThread();
		}
		frame_wait_status = 0;
	} else {
		oFrameWait();
	}
}

static int o_puts_internal_buffer[4];
static void (*o_puts_internal)(const char*);
static void h_puts_internal(const char *str) {
	LogStr(str, strlen(str));
	o_puts_internal(str);
}

// Crash happens at 197300
//int CrashyFunc(void *a0, int a1, int a2, int a3);
static int oCrashyFunc_buffer[4];
static int (*oCrashyFunc)(void *a0, int a1, int a2, int a3);
static int hCrashyFunc(void *a0, int a1, int a2, int a3) {
	if(a0 == NULL) {
		printf("PREVENTED CRASH!\n");
		return 1;
	}
	return oCrashyFunc(a0, a1, a2, a3);
}

static void GetFileAlarmCB(int alarm_id, unsigned short time, void * thingy) {
	iWakeupThread(*(int*)thingy);
}

//int GetFile(void *a0, char *a1, int a2, int a3, int t0);
static int oGetFile_buffer[4];
static int (*oGetFile)(void *a0, char *a1, int a2, int a3, int t0);
static int hGetFile(void *a0, char *a1, int a2, int a3, int t0) {
	int i;
	int tid;
	tid = GetThreadId();
	for(i = 0; i < 10; i++) {
		int result = oGetFile(a0, a1, a2, a3, t0);
		if(result) {
			return result;
		}
		printf("Sleeping\n");

		SetAlarm(8000, GetFileAlarmCB, (void*)&tid);
		SleepThread();

		printf("Trying again\n");
	}
	printf("Failed...\n");
	SleepThread(); // so that we at least get networking
	return 0;
}


void FrameWaitThread() {
	while(1) {
		if(frame_wait_status != 1) {
			SleepThread();
		}
		if(frame_wait_status == 1) {
			oFrameWait();
			frame_wait_status = 2;
		}
		iWakeupThread(frame_waiting_tid);
	}
}

int init_frame_wait_thread(void) {
	ee_thread_t thp;
	int rv;

	thp.attr = 0;
	thp.option = 0;
	thp.func = FrameWaitThread;
	thp.stack_size = 0x600;
	thp.stack = malloc(0x600);
	thp.initial_priority = 0x78;
	thp.gp_reg = &_gp;
	if((frame_wait_tid = CreateThread(&thp)) < 0) {
		printf("Failed to create framewait thread!\n");
		return 0;
	}
	if((rv = StartThread(frame_wait_tid, NULL)) < 0) {
		printf("framewait thread: StartThread failed, %d\n", rv);
		DeleteThread(frame_wait_tid);
		return 0;
	}
	printf("Started framewait thread.\n");
}

void load_mod(void) {
    _ps2sdk_time_init();
	printf("Loading networking IRX modules...\n");
	SifLoadModule("cdrom0:\\MODULES\\PS2DEV9.IRX;1", 0, "");
	SifLoadModule("cdrom0:\\MODULES\\NETMAN.IRX;1", 0, "");
	SifLoadModule("cdrom0:\\MODULES\\SMAP.IRX;1", 0, "");
	
	// Insert hooks
	oFrameWait = CreateHook(FrameWait, hFrameWait, oFrameWait_buf);
	o_puts_internal = CreateHook(puts_internal, h_puts_internal, o_puts_internal_buffer);
	//oCrashyFunc = CreateHook(CrashyFunc, hCrashyFunc, oCrashyFunc_buffer);
	//oGetFile = CreateHook(GetFile, hGetFile, oGetFile_buffer);
	init_multi_menu_hooks();
	init_game_net_hooks();

	InitConnection();
	init_frame_wait_thread();
}

/* A rough look at how much memory each stage uses:

Twisted Reality:      26098584
Flying to your heart: 23384120
Bee Jam Blues:        26748200
Nuff Respect:         25755512
Born to be bone:      27827640
Tainted Lovers:       25333320
Resurrection:         16717240


MASTER:
1: 25580552 (Twisted reality)
2: 22618376 (flying to your heart)
3: 24639896 (Bee Jam Blues)
4: 13060264
5: 24354728 (Nuff Respect)          24673464
6: 15771160                         16109096
7: 27482520 (Born to bone)          27997032


214 KB (0x30000): OK
406 KB (0x60000): Nope
310 KB (0x48000): Nope
262 KB (0x3C000): Yes
286 KB (0x42000): Yes
298 KB (0x45000): Nope

*/

// MSTREAM offset is 0x85530

//char pre_gp[32768] __attribute__ ((aligned (128)));
//char _gp[32768];
