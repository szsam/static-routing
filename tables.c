#include <netinet/in.h> /* for in_addr */
#include <netinet/ether.h> /* for ETH_ALEN */
#include <net/if.h> /* IFNAMSIZ */
#include <string.h>
#include <assert.h>

#define MAX_ARPTBL_SIZ 8
static struct {
	uint32_t ipaddr;
	unsigned char hwaddr[ETH_ALEN];
	//char iface[IFNAMSIZ];
}arp_table[MAX_ARPTBL_SIZ] = 
{
	{0xc0a80202, {0x00,0x0c,0x29,0xca,0x14,0xb2} },
	{0xc0a80101, {0x00,0x0c,0x29,0xdd,0x92,0xe4} }
};

int arptbl_len = 2;

#define MAX_ROUTBL_SIZ 8
static struct {
	uint32_t dest;
	uint32_t gw;
	uint32_t genmask;
	char iface[IFNAMSIZ];
}route_table[MAX_ROUTBL_SIZ] = 
{
	{0xc0a80100, 0x0, 0xffffff00, "eth0"},	
	{0xc0a80200, 0x0, 0xffffff00, "eth1"}
};
int routbl_len = 2;

#define MAX_DEVTBL_SIZ 8
static struct {
	unsigned char hwaddr[ETH_ALEN];
	//char iface[IFNAMSIZ];
}device_items[MAX_DEVTBL_SIZ] =
{
	{ {0x00,0x0c,0x29,0xf6,0x77,0x87} },
	{ {0x00,0x0c,0x29,0xf6,0x77,0x91} },
	{ {0xff,0xff,0xff,0xff,0xff,0xff} }
};
int devtbl_len = 3;


int search_dev_tbl(uint8_t *macaddr)
{
	int ix = 0;
	while (ix < devtbl_len && 
			strncmp((char *)device_items[ix].hwaddr, (char *)macaddr, ETH_ALEN))
		++ix;

	if (ix == devtbl_len) return -1;
	else return ix;
}

int search_route_tbl(uint32_t ipaddr)
{
	int ix = 0;
	while (ix < routbl_len && (ipaddr & route_table[ix].genmask) != route_table[ix].dest )
		++ix;

	if (ix == routbl_len) return -1;
	else return ix;
}

uint32_t gateway(int ix) { return route_table[ix].gw; }
const char *iface(int ix) { return route_table[ix].iface; }

const uint8_t *ip2mac(uint32_t ipaddr) 
{
	int ix;
	for (ix = 0; ix < arptbl_len; ++ix)
		if (ipaddr == arp_table[ix].ipaddr)
			return arp_table[ix].hwaddr;
	return NULL;
}
