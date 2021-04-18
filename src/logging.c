#include <ps2ip.h>
#include <stdio.h>
#include <string.h>
#include "logging.h"
/*
static ip_addr_t logging_addr;
static unsigned short logging_port = 0;
static struct udp_pcb *logging_pcb = NULL;

static void
logging_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, unsigned short port)
{
	if(ip_addr_cmp(&logging_addr, addr) != 0 && logging_port != port) {
		ip_addr_set_ipaddr(&logging_addr, addr);
		logging_port = port;
		LogStr("Hello~!");
	}
}

void LogStr(const char* str) {
	if(logging_port != 0 && logging_pcb != NULL) {
		int len = strlen(str);
		struct pbuf *buf = pbuf_alloc(PBUF_TRANSPORT, len, PBUF_RAM);
		memcpy(buf->payload, str, len);
		udp_sendto(logging_pcb, buf, &logging_addr, logging_port);
		pbuf_free(buf);
	}
}

void InitLogging(void) {
	logging_pcb = udp_new();
	udp_bind(logging_pcb, IP_ADDR_ANY, LOGGING_PORT);
	udp_recv(logging_pcb, logging_recv, NULL);
}*/

static int curr_client = -1;

void LogStr(const char* str, int length) {
	if(curr_client >= 0) {
		lwip_write(curr_client, str, length);
	}
}

void InitLogging(void) {
	int sockfd;
	struct sockaddr_in address;
	int opt = 1;
	
	sockfd = lwip_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(sockfd < 0) {
		printf("Failed to create server socket.\n");
		return;
	}

	if(lwip_setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) < 0) {
		printf("setsockopt failed\n");
	}
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(LOGGING_PORT);

	if(lwip_bind(sockfd, (struct sockaddr*)&address, sizeof(address)) < 0) {
		printf("bind failed\n");
	}
	if(lwip_listen(sockfd, 2) < 0) {
		printf("listen failed\n");
	}
	while(1) {
		int client;
		struct sockaddr_in client_addr;
		int sockaddr_len = sizeof(client_addr);

		client = lwip_accept(sockfd, (struct sockaddr*) &client_addr, &sockaddr_len);
		if(client >= 0) {
			if(curr_client >= 0) {
				lwip_close(curr_client);
			}
			curr_client = client;
		}
	}
}
