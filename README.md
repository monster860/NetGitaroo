# NetGitaroo

Download link / readme at: https://monster860.github.io/NetGitaroo/netgitaroo-patcher.html

### Modified netman

This repository includes a modified netman library that allocates less packet buffers. This is to reduce the IOP memory requirements, since this game has issues with running out of IOP memory when this value is set to 64. In addition, there is a patch on the IOP netman side to prevent hangs when the game receives too many packets when the netman Rx thread is prevented from executing