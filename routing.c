#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <arpa/inet.h>
#include <netinet/if_ether.h>
//#include <sockios.h>
//#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/ip.h>
//#include <netinet/udp.h>
//#include <netpacket/packet.h>
#include <linux/if_packet.h>
#include <net/ethernet.h> /* the L2 protocols */

#include "wrapper.h"
#include "tables.h"

void parse_ethhdr(struct ethhdr *eth)
{
	printf("\nEthernet Header\n");
	printf("\t|-Source Address : %.2X-%.2X-%.2X-%.2X-%.2X-%.2X\n",eth->h_source[0],eth->h_source[1],eth->h_source[2],eth->h_source[3],eth->h_source[4],eth->h_source[5]);
	printf("\t|-Destination Address : %.2X-%.2X-%.2X-%.2X-%.2X-%.2X\n",eth->h_dest[0],eth->h_dest[1],eth->h_dest[2],eth->h_dest[3],eth->h_dest[4],eth->h_dest[5]);
	printf("\t|-Protocol : %d\n", ntohs(eth->h_proto));	
}

void parse_iphdr(struct iphdr *ip)
{
	struct sockaddr_in source, dest;
	memset(&source, 0, sizeof(source));
	source.sin_addr.s_addr = ip->saddr;
	memset(&dest, 0, sizeof(dest));
	dest.sin_addr.s_addr = ip->daddr;

	printf("IP Header\n");
//	printf("\t|-Version : %d\n", (unsigned int)ip->version);
//	printf("\t|-Internet Header Length : %d DWORDS or %d Bytes\n", (unsigned int)ip->ihl,((unsigned int)(ip->ihl))*4);
//	printf("\t|-Type Of Service : %d\n", (unsigned int)ip->tos);
//	printf("\t|-Total Length : %d Bytes\n", ntohs(ip->tot_len));
//	printf("\t|-Identification : %d\n", ntohs(ip->id));
//	printf("\t|-Time To Live : %d\n", (unsigned int)ip->ttl);
//	printf("\t|-Protocol : %d\n", (unsigned int)ip->protocol);
//	printf("\t|-Header Checksum : %d\n", ntohs(ip->check));
	printf("\t|-Source IP : %s\n", inet_ntoa(source.sin_addr));
	printf("\t|-Destination IP : %s\n", inet_ntoa(dest.sin_addr));

}

#define BUFFSIZ 65536

int main()
{
	/* Read configuration files */
	init_config();

	// Opening a raw socket
	int sock_r;
	sock_r = Socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	//sock_r = Socket(AF_PACKET, SOCK_RAW, IPPROTO_RAW);
	
	// Reception of the network packet
	unsigned char *buffer = (unsigned char *) malloc(BUFFSIZ); //to receive data

	while (1) {
		memset(buffer, 0, BUFFSIZ);
		struct sockaddr saddr;
		int saddr_len = sizeof (saddr);

		//Receive a network packet and copy in to buffer
		ssize_t buflen = Recvfrom(sock_r,buffer,BUFFSIZ,0,&saddr,(socklen_t *)&saddr_len);

		// Extracting the Ethernet header
		struct ethhdr *eth = (struct ethhdr *)(buffer);
		parse_ethhdr(eth);

		if (search_dev_tbl(eth->h_dest) == -1) {
			printf("MAC address does not belong to any device\n");
			continue;
		}

		if (eth->h_proto != htons(ETH_P_IP)) {
			printf("Ethernet protocol is not IP\n");
			continue;
		}

		// Extracting the IP header
		struct iphdr *ip = (struct iphdr*)(buffer + sizeof(struct ethhdr));
		parse_iphdr(ip);

		// get the IP address of next hop
		struct in_addr dst_addr;
		dst_addr.s_addr = ip->daddr;
		int rou_ix = search_route_tbl(dst_addr);
		if (rou_ix == -1) {
			printf("No route rule for the destination IP\n");
			continue;
		}
		struct in_addr next_hop_ipaddr = gateway(rou_ix);
		// if Gateway is 0.0.0.0, then the destination and 
		// (one of the interfaces of) the router is in the same subnet
		if (next_hop_ipaddr.s_addr == 0)
			next_hop_ipaddr = dst_addr;
		
		// get the MAC address corresponding to next hop's IP address
		const uint8_t *next_hop_hwaddr = ip2mac(next_hop_ipaddr);
		if (!next_hop_hwaddr) {
			fprintf(stderr, "ARP cache NOT found\n");
			continue;
		}

		// Change the source and destination MAC address
		memcpy(eth->h_source, eth->h_dest, ETH_ALEN);
		memcpy(eth->h_dest, next_hop_hwaddr, ETH_ALEN);

		// Getting the index of the interface to send a packet
		struct ifreq ifreq_i;
		memset(&ifreq_i,0,sizeof(ifreq_i));
		//giving name of Interface
		strncpy(ifreq_i.ifr_name, iface(rou_ix), IFNAMSIZ-1); 
		//getting Interface Index 
		Ioctl(sock_r, SIOCGIFINDEX, &ifreq_i); 
		printf("interface name: %s, index=%d\n", iface(rou_ix), ifreq_i.ifr_ifindex);

		struct sockaddr_ll sadr_ll;
		memset(&sadr_ll, 0, sizeof(sadr_ll));
		sadr_ll.sll_ifindex = ifreq_i.ifr_ifindex; // index of interface
		sadr_ll.sll_halen = ETH_ALEN; // length of destination mac address
		memcpy(sadr_ll.sll_addr, next_hop_hwaddr, ETH_ALEN);
		sadr_ll.sll_family = AF_PACKET;

		Sendto(sock_r, buffer, buflen, 0,
				(const struct sockaddr*)&sadr_ll,sizeof(struct sockaddr_ll));
		printf("Succeed in forwarding a packet\n");

	} // while(1)
}
