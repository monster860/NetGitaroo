OUTPUT_FORMAT(binary)
ENTRY(entry)
STARTUP(entry.o)
INPUT(
	loader.o
	util.o
	util_asm.o
	networking.o
	logging.o
	time.o
	hook.o
  multi_menu.o
  game_net.o
)
INPUT(libnetman.a)
/*INPUT(libps2ip.a)*/

SECTIONS {
  .text : { *(.text) }
  .bss :  { *(.bss)  *(COMMON) } 
  .data : { *(.data) } 
  .scommon : { *(.scommon) }
  .reginfo : { *(.reginfo) }
  .mdebug : { *(.mdebug) }
  .comment : { *(.comment) }
  .rodata : { *(.rodata) }
}
