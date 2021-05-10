#include "networking.h"

#define LOGGING_PORT 22577


void LogStr(const char* str, int length);

void InitLogging(void);

void HandleLoggingPacket(udp_datagram_t *dg, ethernet_frame_t *eth, ip_header_t *ip);
