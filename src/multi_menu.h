void init_multi_menu_hooks(void);

#define PLAY_MENU_SOUND(a,b,c) (*(void(**)(void*,int,int))(*(void**)(a+0x18) + 0xC))(a,b,c)

#define MM_MENUID(a) (*(int*)(a + 0x9c4))
#define MM_MENUID_PREV(a) (*(int*)(a + 0x9c8))
#define MM_RETURNVAL(a) (*(int*)(a + 0xa00))
#define MM_SKIP_ARG(a) (*(int*)(a + 0xa04))
#define MM_SONG(a) (*(int*)(a + 0x9e0))
#define MM_PLAYER_CHARACTER(a, b) (*(int*)(a + 0x85c + b*80))
#define MM_PLAYER_MAXCHAR(a, b) (*(int*)(a + 0x88c + b*80))
#define MM_PLAYER_READY(a, b) (*(int*)(a + 0x858 + b*80))
#define MM_PLAYER_READY2(a, b) (*(int*)(a + 0x884 + b*80))
#define MM_PLAYER_STARTHEALTH(a, b) (*(float*)(a + 0x888 + b*80))
