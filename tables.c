#include <netinet/ether.h> /* for ETH_ALEN */
#include <net/if.h> /* for IFNAMSIZ */
#include <arpa/inet.h> /* for inet_ntoa() */
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>	

#include "tables.h"

#define MAX_ARPTBL_SIZ 8
static struct {
	struct in_addr ipaddr;
	unsigned char hwaddr[ETH_ALEN];
	//char iface[IFNAMSIZ];
}arp_table[MAX_ARPTBL_SIZ] = 
{
//	{0xc0a80202, {0x00,0x0c,0x29,0xca,0x14,0xb2} },
//	{0xc0a80101, {0x00,0x0c,0x29,0xdd,0x92,0xe4} }
};

int arptbl_len = 0;

#define MAX_ROUTBL_SIZ 8
static struct {
	struct in_addr dest;
	struct in_addr gw;
	struct in_addr genmask;
	char iface[IFNAMSIZ];
}route_table[MAX_ROUTBL_SIZ] = 
{
//	{0xc0a80100, 0x0, 0xffffff00, "eth0"},	
//	{0xc0a80200, 0x0, 0xffffff00, "eth1"}
};
int routbl_len = 0;

#define MAX_DEVTBL_SIZ 8
static struct {
	unsigned char hwaddr[ETH_ALEN];
	//char iface[IFNAMSIZ];
}device_items[MAX_DEVTBL_SIZ] =
{
	{ {0xff,0xff,0xff,0xff,0xff,0xff} }
//	{ {0x00,0x0c,0x29,0xf6,0x77,0x87} },
//	{ {0x00,0x0c,0x29,0xf6,0x77,0x91} }
};
int devtbl_len = 1;


/* Functions for searching */
int search_dev_tbl(uint8_t *macaddr)
{
	int ix = 0;
	while (ix < devtbl_len && 
			strncmp((char *)device_items[ix].hwaddr, (char *)macaddr, ETH_ALEN))
		++ix;

	if (ix == devtbl_len) return -1;
	else return ix;
}

int search_route_tbl(struct in_addr ipaddr)
{
	int ix = 0;
	while (ix < routbl_len && 
			(ipaddr.s_addr & route_table[ix].genmask.s_addr) != route_table[ix].dest.s_addr )
		++ix;

	if (ix == routbl_len) return -1;
	else return ix;
}

struct in_addr gateway(int ix) { return route_table[ix].gw; }
const char *iface(int ix) { return route_table[ix].iface; }

const uint8_t *ip2mac(struct in_addr ipaddr) 
{
	int ix;
	for (ix = 0; ix < arptbl_len; ++ix)
		if (ipaddr.s_addr == arp_table[ix].ipaddr.s_addr)
			return arp_table[ix].hwaddr;
	return NULL;
}


/* Functions for reading configuration files */

void read_arp_config(FILE *fp)
{
	char buf[32];
	while (arptbl_len < MAX_ARPTBL_SIZ && 
			fscanf(fp, "%s", buf) != EOF) {
		 inet_aton(buf, &arp_table[arptbl_len].ipaddr);
		 unsigned char *mac = arp_table[arptbl_len].hwaddr;
		 // "%hhx" matches an unsigned hex integer, argument type is unsigned char *
		 fscanf(fp, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", 
				 mac, mac+1, mac+2, mac+3, mac+4, mac+5);
		 ++ arptbl_len;

	}

}

void read_route_config(FILE *fp)
{
	char buf[32];
	while (routbl_len < MAX_ROUTBL_SIZ && 
			fscanf(fp, "%s", buf) != EOF) {
		inet_aton(buf, &route_table[routbl_len].dest);
		fscanf(fp, "%s", buf);
		inet_aton(buf, &route_table[routbl_len].gw);
		fscanf(fp, "%s", buf);
		inet_aton(buf, &route_table[routbl_len].genmask);
		fscanf(fp, "%s", buf);
		strncpy(route_table[routbl_len].iface, buf, IFNAMSIZ);
		++ routbl_len;
	}

}


void read_device_config(FILE *fp)
{
	unsigned char *mac = device_items[devtbl_len].hwaddr;
	while (devtbl_len < MAX_DEVTBL_SIZ && 
			fscanf(fp, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", 
				 mac, mac+1, mac+2, mac+3, mac+4, mac+5) != EOF) {
		++ devtbl_len;
		mac = device_items[devtbl_len].hwaddr;
	}

}

void init_config()
{
	FILE *fp= fopen("config/arp.conf", "r");
	if (!fp) { fprintf(stderr, "error in opening configuration file"); exit(-1); }
	read_arp_config(fp);
	fclose(fp);

	fp= fopen("config/route.conf", "r");
	if (!fp) { fprintf(stderr, "error in opening configuration file"); exit(-1); }
	read_route_config(fp);
	fclose(fp);

	fp= fopen("config/device.conf", "r");
	if (!fp) { fprintf(stderr, "error in opening configuration file"); exit(-1); }
	read_device_config(fp);
	fclose(fp);

}
