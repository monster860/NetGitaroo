#include <stdio.h>
#include <debug.h>
#include <kernel.h>
#include <netman.h>
#include <sbv_patches.h>
#include <stdlib.h>
#include <ps2sdkapi.h>
#include "net.h"

extern unsigned char DEV9_irx[];
extern unsigned int size_DEV9_irx;

extern unsigned char SMAP_irx[];
extern unsigned int size_SMAP_irx;

extern unsigned char NETMAN_irx[];
extern unsigned int size_NETMAN_irx;

hwaddr_t BROADCAST_HWADDR = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
hwaddr_t our_hwaddr = {0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc};
ipaddr_t IP_ANY = {0,0,0,0};
ipaddr_t IP_BROADCAST = {255,255,255,255};
ipaddr_t our_ip = {192, 168, 137, 25};
ipaddr_t gateway_ip = {192, 168, 137, 1};
ipaddr_t subnet_mask = {255,255,255,0};
ipaddr_t logging_client_ip = {0,0,0,0};
hwaddr_t logging_client_hwaddr = {0,0,0,0,0,0};

unsigned short ip_id_ctr = 0;

unsigned char frame_payload_recv_buffer[1500];
unsigned char frame_payload_send_buffer[1500];

struct packet_buffer *AllocTxPacket(unsigned char priority);
void AddTxPacket(struct packet_buffer *packet);
udp_datagram_t *MakeUdpPacket(struct packet_buffer **out_buf, ipaddr_t *dst, hwaddr_t *dst_hwaddr, unsigned short src_port, unsigned short dst_port, unsigned short payload_length, unsigned char priority);
void FinalizeUdpChecksum(udp_datagram_t *datagram);

static int ethGetNetIFLinkStatus(void)
{
	return(NetManIoctl(NETMAN_NETIF_IOCTL_GET_LINK_STATUS, NULL, 0, NULL, 0) == NETMAN_NETIF_ETH_LINK_STATE_UP);
}

static void EthStatusCheckCb(s32 alarm_id, u16 time, void *common)
{
	iWakeupThread(*(int*)common);
}

static int WaitValidNetState(int (*checkingFunction)(void))
{
	int ThreadID, retry_cycles;

	// Wait for a valid network status;
	ThreadID = GetThreadId();
	for(retry_cycles = 0; checkingFunction() == 0; retry_cycles++)
	{	//Sleep for 1000ms.
		SetAlarm(1000 * 16, &EthStatusCheckCb, &ThreadID);
		SleepThread();

		if(retry_cycles >= 10)	//10s = 10*1000ms
			return -1;
	}

	return 0;
}

static int ethWaitValidNetIFLinkState(void)
{
	return WaitValidNetState(&ethGetNetIFLinkStatus);
}

struct dhcp_state {
	ps2_clock_t start_time;
	ps2_clock_t last_req_time;
	int num_retries;
	ipaddr_t dhcp_server_ip;
	hwaddr_t dhcp_server_hwaddr;
	unsigned short xid_lower;
	unsigned short xid_upper;
	unsigned char state;
};

struct dhcp_state *dhcp_holder = NULL;
int dhcp_sema;
unsigned char link_up = 0;

unsigned char dhcp_cookie[] = {
	0x63, 0x82, 0x53, 0x63
};

unsigned char dhcp_discover_options[] = {
	0x35, 0x1, 0x1,
	0x39, 0x02, 0x05, 0xdc,
	0x37, 0x04, 0x1, 0x3, 0x1c, 0x6,
	0xff
};
unsigned char dhcp_request_options[] = {
	53, 1, 3,
	50, 4, 0, 0, 0, 0,
	54, 4, 0, 0, 0, 0,
	0xFF
};

void _process_dhcp() {
	if(dhcp_holder == NULL) return;
	if(!link_up) {
		scr_printf("Ending DHCP (link down)\n");
		free(dhcp_holder);
		dhcp_holder = NULL;
		return;
	}
	if((ps2_clock() - dhcp_holder->last_req_time) > PS2_CLOCKS_PER_SEC) {
		if(dhcp_holder->num_retries > 20) {
			// consider this a fail
			free(dhcp_holder);
			scr_printf("Too many attempts - DHCP failed\n");
			dhcp_holder = NULL;
			return;
		} else {
			// udp_datagram_t *MakeUdpPacket(struct packet_buffer **out_buf, ipaddr_t *dst, hwaddr_t *dst_hwaddr, unsigned short src_port, unsigned short dst_port, unsigned short payload_length, unsigned char priority);
			if(dhcp_holder->state == 0 || dhcp_holder->state == 2) {
				struct packet_buffer *buf;
				udp_datagram_t *dg;
				dg = MakeUdpPacket(&buf, &dhcp_holder->dhcp_server_ip, &dhcp_holder->dhcp_server_hwaddr, 68, 67, sizeof(dhcp_packet_t)+68, NET_PRIORITY_DHCP);
				if(!buf) return;
				scr_printf("Making DHCP packet\n");
				dhcp_packet_t *dhcp = (dhcp_packet_t*)dg->payload;
				memset(dhcp, 0, sizeof(dhcp_packet_t)+32);
				dhcp->op = DHCP_OP_BOOTREQUEST;
				dhcp->htype = 1;
				dhcp->hlen = 6;
				dhcp->hops = 0;
				dhcp->xid_lower = dhcp_holder->xid_lower;
				dhcp->xid_upper = dhcp_holder->xid_upper;
				unsigned short seconds = (ps2_clock() - dhcp_holder->start_time) / PS2_CLOCKS_PER_SEC;
				dhcp->secs = HTONS(seconds);
				dhcp->flags = 0;
				if(dhcp_holder->state == 0) {
					IP_SET(dhcp->ciaddr, IP_ANY);
					IP_SET(dhcp->yiaddr, IP_ANY);
					IP_SET(dhcp->siaddr, IP_ANY);
					IP_SET(dhcp->giaddr, IP_ANY);
				} else if(dhcp_holder->state == 2) {
					IP_SET(dhcp->ciaddr, our_ip);
					IP_SET(dhcp->yiaddr, IP_ANY);
					IP_SET(dhcp->siaddr, dhcp_holder->dhcp_server_ip);
					IP_SET(dhcp->giaddr, IP_ANY);
				}
				HWADDR_SET(dhcp->chaddr, our_hwaddr);
				
				memcpy(dhcp->cookie, dhcp_cookie, sizeof(dhcp_cookie));

				if(dhcp_holder->state == 0) {
					memcpy(dhcp->options, dhcp_discover_options, sizeof(dhcp_discover_options));
				} else if(dhcp_holder->state == 2) {
					dhcp_request_options[5+0] = our_ip.bytes[0];
					dhcp_request_options[5+1] = our_ip.bytes[1];
					dhcp_request_options[5+2] = our_ip.bytes[2];
					dhcp_request_options[5+3] = our_ip.bytes[3];

					dhcp_request_options[11+0] = dhcp_holder->dhcp_server_ip.bytes[0];
					dhcp_request_options[11+1] = dhcp_holder->dhcp_server_ip.bytes[1];
					dhcp_request_options[11+2] = dhcp_holder->dhcp_server_ip.bytes[2];
					dhcp_request_options[11+3] = dhcp_holder->dhcp_server_ip.bytes[3];

					memcpy(dhcp->options, dhcp_request_options, sizeof(dhcp_request_options));
				}
				
				FinalizeUdpChecksum(dg);

				AddTxPacket(buf);
				dhcp_holder->num_retries++;
				dhcp_holder->last_req_time = ps2_clock();
			}
		}
	}
}
void process_dhcp() {
	WaitSema(dhcp_sema);
	_process_dhcp();
	SignalSema(dhcp_sema);
}

void _begin_dhcp() {
	if(dhcp_holder != NULL) {
		return;
	}
	scr_printf("Initializing DHCP\n");
	dhcp_holder = malloc(sizeof(struct dhcp_state));
	dhcp_holder->num_retries = 0;
	dhcp_holder->start_time = ps2_clock();
	dhcp_holder->last_req_time = 0;
	dhcp_holder->num_retries = 0;
	dhcp_holder->state = 0;
	dhcp_holder->xid_lower = rand();
	dhcp_holder->xid_upper = rand();
	IP_SET(dhcp_holder->dhcp_server_ip, IP_BROADCAST);
	HWADDR_SET(dhcp_holder->dhcp_server_hwaddr, BROADCAST_HWADDR);

	IP_SET(our_ip, IP_ANY);
	IP_SET(gateway_ip, IP_ANY);
	IP_SET(subnet_mask, IP_ANY);

	_process_dhcp();
}
void begin_dhcp() {
	WaitSema(dhcp_sema);
	_begin_dhcp();
	SignalSema(dhcp_sema);
}

void _HandleDhcp(dhcp_packet_t *dhcp, ethernet_frame_t *in_frame) {
	scr_printf("Received dhcp\n");
	if(dhcp_holder == NULL) {
		return;
	}
	if(dhcp->op != DHCP_OP_BOOTREPLY || dhcp->htype != 1 || dhcp->hlen != 6 || dhcp->xid_lower != dhcp_holder->xid_lower || dhcp->xid_upper != dhcp_holder->xid_upper)
		return;
	scr_printf("It be for us\n");
	dhcp_holder->dhcp_server_hwaddr = in_frame->source;
	int oi = 0;
	unsigned char dhcp_type = 0;
	ipaddr_t recv_subnet_mask = {0,0,0,0};
	ipaddr_t recv_router = {0,0,0,0};
	ipaddr_t recv_dhcp_server = {255,255,255,255};
	while(oi < 500 && dhcp->options[oi] != 0xFF) {
		unsigned char option_type = dhcp->options[oi++];
		unsigned char option_len = dhcp->options[oi++];

		if(option_type == 53) {
			dhcp_type = dhcp->options[oi];
		} else if(option_type == 1) {
			recv_subnet_mask.bytes[0] = dhcp->options[oi+0];
			recv_subnet_mask.bytes[1] = dhcp->options[oi+1];
			recv_subnet_mask.bytes[2] = dhcp->options[oi+2];
			recv_subnet_mask.bytes[3] = dhcp->options[oi+3];
		} else if(option_type == 3) {
			recv_router.bytes[0] = dhcp->options[oi+0];
			recv_router.bytes[1] = dhcp->options[oi+1];
			recv_router.bytes[2] = dhcp->options[oi+2];
			recv_router.bytes[3] = dhcp->options[oi+3];
		} else if(option_type == 54) {
			recv_dhcp_server.bytes[0] = dhcp->options[oi+0];
			recv_dhcp_server.bytes[1] = dhcp->options[oi+1];
			recv_dhcp_server.bytes[2] = dhcp->options[oi+2];
			recv_dhcp_server.bytes[3] = dhcp->options[oi+3];
		}

		oi += option_len;
	}
	scr_printf("type: %i\n", dhcp_type);
	if(dhcp_type == DHCPOFFER && dhcp_holder->state == 0) {
		IP_SET(our_ip, dhcp->yiaddr);
		IP_SET(gateway_ip, recv_router);
		IP_SET(subnet_mask, recv_subnet_mask);
		IP_SET(dhcp_holder->dhcp_server_ip, recv_dhcp_server);
		dhcp_holder->num_retries = 0;
		dhcp_holder->last_req_time = 0;
		dhcp_holder->state = 2;
	} else if(dhcp_type == DHCPACK && dhcp_holder->state == 2) {
		free(dhcp_holder);
		dhcp_holder = NULL;
		return; // We success~!
	} else if(dhcp_type == DHCPNACK && dhcp_holder->state == 2) {
		free(dhcp_holder);
		dhcp_holder = NULL;
		_begin_dhcp(); // we fail :(
	}
	_process_dhcp();
}

void HandleDhcp(dhcp_packet_t *dhcp, ethernet_frame_t *in_frame) {
	WaitSema(dhcp_sema);
	_HandleDhcp(dhcp, in_frame);
	SignalSema(dhcp_sema);
}

void print_hwaddr(hwaddr_t *mac) {
	scr_printf("%02x:%02x:%02x:%02x:%02x:%02x\n", mac->bytes[0], mac->bytes[1], mac->bytes[2], mac->bytes[3], mac->bytes[4], mac->bytes[5]);
}

void LinkStateUp(void) {
	scr_printf("Link up~!\n");
	link_up = 1;
	begin_dhcp();
}
void LinkStateDown(void) {
	scr_printf("Link down~!\n");
	link_up = 0;
}
int has_failed_alloc = 0;
void *AllocRxPacket(unsigned int size, void **payload) {
	DI();
	struct packet_buffer *buf = free_packet_buffer;
	if(buf != NULL) {
		free_packet_buffer = buf->next;
		EI();
		buf->next = NULL;
		buf->size = size;
		*payload = buf->data;
	} else if(!has_failed_alloc) {
		EI();
		has_failed_alloc = 1;
		scr_printf("FAILED ALLOC!\n");
	}
	return buf;
}
void FreeRxPacket(void *packet) {
	struct packet_buffer *buf = packet;
	DI();
	buf->next = free_packet_buffer;
	free_packet_buffer = buf;
	EI();
}

unsigned short CalcHeaderChecksum(ip_header_t *header) {
	int i;
	int num_shorts = (header->version_ihl & 0xF) << 1;
	unsigned short *as_shorts = (unsigned short*) header;
	unsigned int running_total = 0;
	for(i = 0; i < num_shorts; i++) {
		if(i != 5)
			running_total += NTOHS(as_shorts[i]);
	}
	unsigned short checksum = (running_total & 0xFFFF) + ((running_total >> 16) & 0xFFFF);
	checksum = (checksum & 0xFFFF) + ((checksum >> 16) & 0xFFFF);
	return ~HTONS(checksum);
}

void *MakeIpPacket(struct packet_buffer **out_buf, ipaddr_t *dst, hwaddr_t *dst_hwaddr, unsigned char protocol, unsigned short payload_length, unsigned char priority) {
	struct packet_buffer *buf = AllocTxPacket(priority);
	if(buf == NULL) {
		return NULL;
	}
	*out_buf = buf;
	ethernet_frame_t *frame = (ethernet_frame_t *)buf->data;
	HWADDR_SET(frame->destination, *dst_hwaddr);
	HWADDR_SET(frame->source, our_hwaddr);
	frame->type = HTONS(0x0800);
	ip_header_t *header = (ip_header_t*)frame->payload;
	header->version_ihl = 0x45;
	header->dscp_ecn = 0;
	header->total_length = HTONS(20 + payload_length);
	header->identification = ip_id_ctr++;
	header->flags_fragment_offset = 0;
	header->ttl = 0x80;
	header->protocol = protocol;
	IP_SET(header->source_ip, our_ip);
	IP_SET(header->destination_ip, *dst);
	header->header_checksum = CalcHeaderChecksum(header);
	buf->size = sizeof(ethernet_frame_t) + 20 + payload_length;
	return (void*)((int)header + 20);
}

udp_datagram_t *MakeUdpPacket(struct packet_buffer **out_buf, ipaddr_t *dst, hwaddr_t *dst_hwaddr, unsigned short src_port, unsigned short dst_port, unsigned short payload_length, unsigned char priority) {
	struct packet_buffer *buf = NULL;
	udp_datagram_t *datagram = MakeIpPacket(&buf, dst, dst_hwaddr, 17, payload_length + 8, priority);
	*out_buf = buf;
	if(datagram == NULL) {
		return NULL;
	}
	unsigned int checksum = 0;
	checksum += NTOHS(dst->shorts[0]);
	checksum += NTOHS(dst->shorts[1]);
	checksum += NTOHS(our_ip.shorts[0]);
	checksum += NTOHS(our_ip.shorts[1]);
	checksum += 17;
	checksum += payload_length + 8;

	checksum = (checksum & 0xFFFF) + ((checksum >> 16) & 0xFFFF);
	checksum = (checksum & 0xFFFF) + ((checksum >> 16) & 0xFFFF);

	unsigned short short_checksum = checksum;

	datagram->src_port = HTONS(src_port);
	datagram->dst_port = HTONS(dst_port);
	datagram->length = HTONS(payload_length + 8);
	datagram->checksum = ~HTONS(short_checksum);
	return datagram;
}

void FinalizeUdpChecksum(udp_datagram_t *datagram) {
	unsigned short *as_shorts = (unsigned short*)datagram;
	int amt_shorts = NTOHS(datagram->length) >> 1;
	unsigned short short_checksum = ~NTOHS(datagram->checksum);
	unsigned int checksum = short_checksum;
	int i;
	for(i = 0; i < amt_shorts; i++) {
		if(i != 3)
			checksum += NTOHS(as_shorts[i]);
	}
	
	checksum = (checksum & 0xFFFF) + ((checksum >> 16) & 0xFFFF);
	checksum = (checksum & 0xFFFF) + ((checksum >> 16) & 0xFFFF);
	short_checksum = checksum;
	datagram->checksum = ~HTONS(short_checksum);
}

void HandleIp(ip_header_t *header, ethernet_frame_t *in_frame) {
	void* payload = (void*)((int)header + ((header->version_ihl & 0xF) << 2));
	if((header->version_ihl & 0xF0) != 0x40 || (NTOHS(header->flags_fragment_offset) & 0x4fff) != 0)
		return;
	unsigned short checksum = CalcHeaderChecksum(header);
	if(checksum != header->header_checksum) {
		scr_printf("CHECKSUMS DONT MATCH (%i, %i)\n", NTOHS(checksum), NTOHS(header->header_checksum));
		return;
	}
	if(header->protocol == 17) {
		udp_datagram_t *dg = payload;
		if(dg->src_port == HTONS(67) && dg->dst_port == HTONS(68)) {
			// Handle DHCP stuff before checking the IP
			HandleDhcp((dhcp_packet_t*)dg->payload, in_frame);
			return;
		}
	}
	if(!IP_EQ(header->destination_ip, our_ip) && !IP_EQ(header->destination_ip, IP_BROADCAST)) {
		return;
	}
	
	scr_printf("Received packet from %i.%i.%i.%i\n", header->source_ip.bytes[0], header->source_ip.bytes[1], header->source_ip.bytes[2], header->source_ip.bytes[3]);
	if(header->protocol == 17) {
		udp_datagram_t *dg = payload;
		if(dg->dst_port == HTONS(LOGGING_PORT)) {
			
		}
	}
}

void HandleArp(arp_packet_t *arp, ethernet_frame_t *in_frame) {
	//scr_printf("htype:%i\nptype:%i\nhlen:%i\nplen:%i\n", NTOHS(arp->htype),NTOHS(arp->ptype),arp->hlen,arp->plen);
	if(arp->htype != HTONS(1) || arp->ptype != HTONS(0x0800) || arp->hlen != 6 || arp->plen != 4) {
		return;
	}
	//scr_printf("oper:%i\ntarget ip:%i.%i.%i.%i\n", NTOHS(arp->htype),arp->target_ip.bytes[0],arp->target_ip.bytes[1],arp->target_ip.bytes[2],arp->target_ip.bytes[3]);
	if(arp->oper == HTONS(1) && IP_EQ(arp->target_ip, our_ip)) {
		scr_printf("ARP at us! Sending ARP reply...\n");
		struct packet_buffer *reply_buf = AllocTxPacket(NET_PRIORITY_ARP);
		if(reply_buf != NULL) {
			scr_printf("Got buffer for ARP reply\n");
			ethernet_frame_t *frame = (ethernet_frame_t*)reply_buf->data;
			HWADDR_SET(frame->destination, in_frame->source);
			HWADDR_SET(frame->source, our_hwaddr);
			frame->type = HTONS(0x0806);
			arp_packet_t *reply_arp = (arp_packet_t*)frame->payload;
			reply_arp->htype = HTONS(1);
			reply_arp->ptype = HTONS(0x0800);
			reply_arp->hlen = 6;
			reply_arp->plen = 4;
			reply_arp->oper = HTONS(2);
			IP_SET(reply_arp->sender_ip, our_ip);
			HWADDR_SET(reply_arp->sender_hw, our_hwaddr);
			IP_SET(reply_arp->target_ip, arp->sender_ip);
			HWADDR_SET(reply_arp->target_hw, arp->sender_hw);
			reply_buf->size = sizeof(*frame) + sizeof(*reply_arp);
			AddTxPacket(reply_buf);
		} else {
			scr_printf("FAILED to allocate ARP reply packet\n");
		}
	}
}

void EnQRxPacket(void *packet) {
	struct packet_buffer *buf = packet;
	//scr_printf("Received packet of length %i\n", buf->size);
	ethernet_frame_t *frame = (ethernet_frame_t*)&buf->data;
	//print_hwaddr(&frame->destination);
	//print_hwaddr(&frame->source);
	if(HWADDR_EQ(frame->destination, our_hwaddr) || HWADDR_EQ(frame->destination, BROADCAST_HWADDR)) {
		if(frame->type == HTONS(0x0806)) { // arp
			HandleArp((arp_packet_t*)frame->payload, frame);
		} else if(frame->type == HTONS(0x0800)) {
			HandleIp((ip_header_t*)frame->payload, frame);
		} else if(frame->type == HTONS(0x86DD)) {
			//scr_printf("It's an IPv6!\n");
		}
	} else {
		//scr_printf("Not at us or broadcast...\n");
	}
	
	// We MUST free the packet before returning or the RX thread will hang in an infinite loop until this is freed.
	FreeRxPacket(packet);

	process_dhcp();
}

// Gets the next packet to transmit
int NextTxPacket(void **payload) {
	DI();
	if(next_tx_packet_buffer != NULL) {
		next_tx_packet_buffer->transmitting = 1;
		*payload = next_tx_packet_buffer->data;
		EI();
		print_hwaddr((hwaddr_t *)*payload);
		return next_tx_packet_buffer->size;
	}
	EI();
	return 0;
}

// the last packet has been transmitted
void DeQTxPacket(void) {
	if(next_tx_packet_buffer != NULL) {
		DI();
		struct packet_buffer* next = next_tx_packet_buffer->next;
		next_tx_packet_buffer->next = free_tx_packet_buffer;
		free_tx_packet_buffer = next_tx_packet_buffer;
		next_tx_packet_buffer = next;
		EI();
	}
}

struct packet_buffer *AllocTxPacket(unsigned char priority) {
	struct packet_buffer *buf;
	DI();
	if(free_tx_packet_buffer != NULL) {
		buf = free_tx_packet_buffer;
		free_tx_packet_buffer = buf->next;
		EI();
		buf->priority = priority;
		buf->transmitting = 0;
		return buf;
	} else {
		struct packet_buffer *next_yoink = next_tx_packet_buffer;
		while(next_yoink != NULL) {
			if(!next_yoink->transmitting && priority <= next_yoink->priority) {
				buf = next_yoink;
			}
			next_yoink = next_yoink->next;
		}
		if(buf != NULL) {
			if(buf == next_tx_packet_buffer) {
				next_tx_packet_buffer = buf->next;
			} else {
				next_yoink = next_tx_packet_buffer;
				while(next_yoink != NULL) {
					if(next_yoink->next == buf) {
						next_yoink->next = buf->next;
						break;
					}
					next_yoink = next_yoink->next;
				}
			}
		}
		EI();
		buf->priority = priority;
		buf->transmitting = 0;
		return buf;
	}
	EI();
	return NULL;
}

void AddTxPacket(struct packet_buffer *packet) {
	DI();
	packet->next = NULL;
	packet->transmitting = 0;
	if(!next_tx_packet_buffer) {
		next_tx_packet_buffer = packet;
	} else {
		struct packet_buffer *last = next_tx_packet_buffer;
		while(last->next != NULL) {
			last = last->next;
		}
		last->next = packet;
	}
	EI();
	NetManNetIFXmit();
}

// Gets the next next packet to transmit
int AfterTxPacket(void **payload) {
	DI();
	if(next_tx_packet_buffer != NULL && next_tx_packet_buffer->next != NULL) {
		*payload = next_tx_packet_buffer->next->data;
		next_tx_packet_buffer->next->transmitting = 1;
		EI();
		return next_tx_packet_buffer->next->size;
	}
	EI();
	return 0;
}
void ReallocRxPacket(void *packet, unsigned int size) {
	((struct packet_buffer *)packet)->size = size;
}

struct packet_buffer packet_buffers[NUM_PACKET_BUFFERS];
struct packet_buffer tx_packet_buffers[NUM_TX_PACKET_BUFFERS];
struct packet_buffer *free_packet_buffer = NULL;
struct packet_buffer *free_tx_packet_buffer = NULL;
struct packet_buffer *next_tx_packet_buffer = NULL;

int main(int argc, char *argv[])
{
	ee_sema_t sema;
	int i;
	SifInitRpc(0);
	while(!SifIopReset("", 0)){};
	while(!SifIopSync()){};

	SifInitRpc(0);
	SifLoadFileInit();
	SifInitIopHeap();
	sbv_patch_enable_lmb();

	sio_printf("Loading IRXs\n");

	SifExecModuleBuffer(DEV9_irx, size_DEV9_irx, 0, NULL, NULL);
	SifExecModuleBuffer(NETMAN_irx, size_NETMAN_irx, 0, NULL, NULL);
	SifExecModuleBuffer(SMAP_irx, size_SMAP_irx, 0, NULL, NULL);

	sio_printf("Nya~!\n");

	sio_printf("Doing init_scr()\n");

	init_scr();

	sema.init_count = 1;
	sema.max_count = 1;
	sema.option = 0;
	dhcp_sema = CreateSema(&sema);

	for(i = 0; i < NUM_PACKET_BUFFERS-1; i++) {
		packet_buffers[i].next = &packet_buffers[i+1];
	}
	packet_buffers[NUM_PACKET_BUFFERS-1].next = NULL;
	free_packet_buffer = &packet_buffers[0];

	for(i = 0; i < NUM_TX_PACKET_BUFFERS-1; i++) {
		tx_packet_buffers[i].next = &tx_packet_buffers[i+1];
	}
	tx_packet_buffers[NUM_TX_PACKET_BUFFERS-1].next = NULL;
	free_tx_packet_buffer = &tx_packet_buffers[0];

	static struct NetManNetProtStack stack={
		&LinkStateUp,
		&LinkStateDown,
		&AllocRxPacket,
		&FreeRxPacket,
		&EnQRxPacket,
		&NextTxPacket,
		&DeQTxPacket,
		&AfterTxPacket,
		&ReallocRxPacket
	};

	scr_printf("NetManInit()\n");

	NetManInit();
	
	NetManIoctl(NETMAN_NETIF_IOCTL_ETH_GET_MAC, NULL, 0, &our_hwaddr, sizeof(our_hwaddr));
	scr_printf("HW addr:");
	print_hwaddr(&our_hwaddr);

	scr_printf("Registering network stack\n");

	NetManRegisterNetworkStack(&stack);

	scr_printf("Waiting for connection...\n");
	if(ethWaitValidNetIFLinkState() != 0) {
		scr_printf("Failed\n");
		goto end;
	}

	SleepThread();

	end:
	NetManDeinit();
	SifExitRpc();
	return 0;
}
