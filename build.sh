#!/bin/bash

set -e

cwd=$(pwd)

cd $(dirname $0)
cd src
ee-gcc -I$(echo -n $PS2SDK)/ee/include -I$(echo -n $PS2SDK)/common/include -O3 -G 0 -c *.c
ee-gcc -c *.S
ee-ld -Map out_us.map -L$(echo -n $PS2SDK)/ee/lib -T ld_us.ld

cd ..
mkdir -p final-patcher/bins
./utils/file-to-js.sh $PS2SDK/iop/irx/ps2dev9.irx dev9_irx > final-patcher/bins/dev9_irx.js
./utils/file-to-js.sh $PS2SDK/iop/irx/netman.irx netman_irx > final-patcher/bins/netman_irx.js
./utils/file-to-js.sh $PS2SDK/iop/irx/smap.irx smap_irx > final-patcher/bins/smap_irx.js

./utils/file-to-js.sh src/out_us.bin bin_us > final-patcher/bins/bin_us.js

cd $cwd