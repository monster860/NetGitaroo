#!/bin/bash

bin2c $(echo $PS2SDK)/iop/irx/ps2dev9.irx DEV9_irx.c DEV9_irx
bin2c $(echo $PS2SDK)/iop/irx/netman.irx NETMAN_irx.c NETMAN_irx
bin2c $(echo $PS2SDK)/iop/irx/smap.irx SMAP_irx.c SMAP_irx

ee-gcc *.c -I$(echo -n $PS2SDK)/ee/include -I$(echo -n $PS2SDK)/common/include -O3 -G 0 -L$(echo -n $PS2SDK)/ee/lib -lnetman -ldebug -lpatches -o nettest.elf
