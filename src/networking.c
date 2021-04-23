#include <stdio.h>
#include <netman.h>
#include <kernel.h>
#include <stdlib.h>
#include "networking.h"
#include "logging.h"

//static struct ip4_addr IP, NM, GW, DNS;
/*
static int ethApplyIPConfig(int use_dhcp, const struct ip4_addr *ip, const struct ip4_addr *netmask, const struct ip4_addr *gateway, const struct ip4_addr *dns)
{
	t_ip_info ip_info;
	const ip_addr_t *dns_curr;
	int result;

	//SMAP is registered as the "sm0" device to the TCP/IP stack.
	if ((result = ps2ip_getconfig("sm0", &ip_info)) >= 0)
	{
		//Obtain the current DNS server settings.
		dns_curr = dns_getserver(0);

		//Check if it's the same. Otherwise, apply the new configuration.
		if ((use_dhcp != ip_info.dhcp_enabled)
		    ||	(!use_dhcp &&
			 (!ip_addr_cmp(ip, (struct ip4_addr *)&ip_info.ipaddr) ||
			 !ip_addr_cmp(netmask, (struct ip4_addr *)&ip_info.netmask) ||
			 !ip_addr_cmp(gateway, (struct ip4_addr *)&ip_info.gw) ||
			 !ip_addr_cmp(dns, dns_curr))))
		{
			if (use_dhcp)
			{
				ip_info.dhcp_enabled = 1;
			}
			else
			{	//Copy over new settings if DHCP is not used.
				ip_addr_set((struct ip4_addr *)&ip_info.ipaddr, ip);
				ip_addr_set((struct ip4_addr *)&ip_info.netmask, netmask);
				ip_addr_set((struct ip4_addr *)&ip_info.gw, gateway);

				ip_info.dhcp_enabled = 0;
			}

			//Update settings.
			result = ps2ip_setconfig(&ip_info);
			if (!use_dhcp)
				dns_setserver(0, dns);
		}
		else
			result = 0;
	}

	return result;
}*/
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

static int ethGetNetIFLinkStatus(void)
{
	return(NetManIoctl(NETMAN_NETIF_IOCTL_GET_LINK_STATUS, NULL, 0, NULL, 0) == NETMAN_NETIF_ETH_LINK_STATE_UP);
}
/*
static int ethWaitValidNetIFLinkState(void)
{
	return WaitValidNetState(&ethGetNetIFLinkStatus);
}
*/
/*
static int ethGetDHCPStatus(void)
{
	t_ip_info ip_info;
	int result;

	if ((result = ps2ip_getconfig("sm0", &ip_info)) >= 0)
	{	//Check for a successful state if DHCP is enabled.
		if (ip_info.dhcp_enabled)
			result = (ip_info.dhcp_status == DHCP_STATE_BOUND || (ip_info.dhcp_status == DHCP_STATE_OFF));
		else
			result = -1;
	}

	return result;
}
*/
/*
static int ethWaitValidDHCPState(void)
{
	return WaitValidNetState(&ethGetDHCPStatus);
}
*/
static int connection_manager_tid = -1;

static void ConnectionManagerThread() {
	//Wait for the link to become ready.
	printf("NetManInit\n");
	NetManInit();
	
	printf("Setting link mode\n");
	int lmr = NetManSetLinkMode(NETMAN_NETIF_ETH_LINK_MODE_10M_FDX);
	printf("NetManSetLinkMode returned %d.\n", lmr);

	//ip4_addr_set_zero(&IP);
	//ip4_addr_set_zero(&NM);
	//ip4_addr_set_zero(&GW);
	//ip4_addr_set_zero(&DNS);

	//printf("Initializing ps2ip\n");
	/*ps2ipInit(&IP, &NM, &GW);
	ethApplyIPConfig(1, &IP, &NM, &GW, &DNS);
	printf("Waiting for connection...\n");
	if(ethWaitValidNetIFLinkState() != 0) {
		printf("Error: failed to get valid link status.\n");
		goto end;
	}

	printf("Waiting for DHCP lease...\n");
	//Wait for DHCP to initialize, if DHCP is enabled.
	if (ethWaitValidDHCPState() != 0)
	{
		printf("DHCP failed\n.");
		goto end;
	}
	printf("done!\n");

	printf("Initialized:\n");

	InitLogging();
*/
	SleepThread();

	return;
	end:
	//ps2ipDeinit();
	NetManDeinit();
	return;
}

int InitConnection(void) {
	ee_thread_t thp;
	int rv;

	if(connection_manager_tid >= 0) return 1;

	thp.attr = 0;
	thp.option = 0;
	thp.func = ConnectionManagerThread;
	thp.stack_size = 0x600;
	thp.stack = malloc(0x600);
	thp.initial_priority = 0x18;
	if((connection_manager_tid = CreateThread(&thp)) < 0) {
		printf("Failed to create connection thread!\n");
		return 0;
	}
	if((rv = StartThread(connection_manager_tid, NULL)) < 0) {
		printf("connection thread: StartThread failed, %d\n", rv);
		DeleteThread(connection_manager_tid);
		return 0;
	}
	printf("Started connection manager thread.\n");
	return 1;
}
