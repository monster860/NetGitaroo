

### Modified netman

This repository includes a modified netman library that has NETMAN_ RPC_BLOCK_SIZE set to 48 rather than 64. This is to reduce the IOP memory requirements, since this game has issues with running out of IOP memory when this value is set to 64.