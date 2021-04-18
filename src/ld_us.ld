OUTPUT(out_us.bin)

MEMORY
{
  ram      (rwx)  : ORIGIN = 0x311280, LENGTH = 0x800000
}

malloc = 0x1203f0;
free = 0x120350;
printf = 0x142588;
FlushCache = 0x12CAA0;
SifLoadModule = 0x131610;
SifBindRpc = 0x12f870;
SifCallRpc = 0x12fa40;
SifWriteBackDCache = 0x12f130;
SifSetDChain = 0x12cc10;
iWakeupThread = 0x12d790;
putchar = 0x12e128;
puts_internal = 0x12daf8;
CreateThread = 0x12c620;
StartThread = 0x12c640;
AddDmacHandler = 0x12c530;
EnableDmac = 0x12d3e0;
SleepThread = 0x12c740;
RemoveDmacHandler = 0x12c550;
TerminateThread = 0x12c670;
DeleteThread = 0x12c630;
DeleteSema = 0x12c830;
CreateSema = 0x12c820;
SignalSema = 0x12c840;
WaitSema = 0x12c860;
WakeupThread = 0x12c750;
SifSetDma = 0x12cbe0;
SifDmaStat = 0x12cbc0;
SifSendCmd = 0x12ef90;
iSifSetDChain = 0x12cc10;
_sif_rpc_data = 0x269f00;
SifRpcLoop = 0x1300c8;
memalign = 0x1400c8;
iReleaseWaitThread = 0x12c700;
SetAlarm = 0x12d448;
ReleaseAlarm = 0x12d4b8;
GetThreadId = 0x12c710;
strcpy = 0x1435c0;
PollSema = 0x12c870;
strlen = 0x1436d8;
AddIntcHandler = 0x12c500;
RemoveIntcHandler = 0x12c520;
EnableIntc = 0x12d310;
DisableIntc = 0x12d2a8;

FrameWait = 0x1c8a80;

INCLUDE ld_main.ld
