#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <arpa/inet.h>
#include <netinet/if_ether.h>
#include <net/if.h>
#include <netinet/ip.h>
#include <linux/if_packet.h>
#include <net/ethernet.h> /* the L2 protocols */
#include <netinet/ip_icmp.h>

#include "../wrapper.h"

const char *IFACE_NAME = "eth0";
const unsigned char NEXT_HOP_HWADDR[ETH_ALEN] = { 0x0 };
const unsigned char LOCAL_HWADDR[ETH_ALEN] = { 0x0 };
const unsigned char BROADCAST_HWADDR[ETH_ALEN] = {0xff,0xff,0xff,0xff,0xff,0xff};
const char *LOCAL_IP = "192.168.3.2"; 

#define BUFFSIZ 65536 
unsigned char buffer[BUFFSIZ];

int main()
{
	// Opening a raw socket
	int sock_r;
	sock_r = Socket(AF_PACKET, SOCK_RAW, htons(ETH_P_IP));
	

	/* Receive ping request, send ping reply */
	while (1) {
		//Receive a network packet and copy in to buffer
		ssize_t buflen = Recvfrom(sock_r,buffer,BUFFSIZ,0, NULL, NULL);

		// Extracting the Ethernet header
		struct ethhdr *eth = (struct ethhdr *)(buffer);
		if (strncmp((char *)eth->h_source, (char *)LOCAL_HWADDR, ETH_ALEN) ||
			strncmp((char *)eth->h_source, (char *)BROADCAST_HWADDR, ETH_ALEN))
			continue;

		// Extracting the IP header
		struct iphdr *ip = (struct iphdr*)(buffer + sizeof(struct ethhdr));
		struct in_addr local_in_addr, dst_in_addr;
		inet_aton(LOCAL_IP, &local_in_addr);
		dst_in_addr.s_addr = ip->daddr;
		if (local_in_addr.s_addr != dst_in_addr.s_addr || ip->protocol != 1)
			continue;

		// Extracting the IP header
		struct icmphdr *icmp_hdr = 
			(struct icmphdr *)(buffer + sizeof(struct ethhdr) + sizeof(struct iphdr));
		if (icmp_hdr->type  == ICMP_ECHO)
			continue;

		/* Send ping reply */
		// Change the source and destination MAC address
		memcpy(eth->h_source, eth->h_dest, ETH_ALEN);
		memcpy(eth->h_dest, NEXT_HOP_HWADDR, ETH_ALEN);

		// Change the source and destination IP address
		ip->daddr = ip->saddr;
		ip->saddr = inet_addr(LOCAL_IP);
		
		// Change ICMP header
		icmp_hdr->type = ICMP_ECHOREPLY;	

		// Getting the index of the interface to send a packet
		struct ifreq ifreq_i;
		memset(&ifreq_i,0,sizeof(ifreq_i));
		//giving name of Interface
		strncpy(ifreq_i.ifr_name, IFACE_NAME, IFNAMSIZ-1); 
		//getting Interface Index 
		Ioctl(sock_r, SIOCGIFINDEX, &ifreq_i); 

		struct sockaddr_ll sadr_ll;
		memset(&sadr_ll, 0, sizeof(sadr_ll));
		sadr_ll.sll_ifindex = ifreq_i.ifr_ifindex; // index of interface
		sadr_ll.sll_halen = ETH_ALEN; // length of destination mac address
		memcpy(sadr_ll.sll_addr, NEXT_HOP_HWADDR, ETH_ALEN);
		sadr_ll.sll_family = AF_PACKET;

		Sendto(sock_r, buffer, buflen, 0,
				(const struct sockaddr*)&sadr_ll,sizeof(struct sockaddr_ll));
		printf("Succeed in sending a ping reply\n");

	} // while(1)
}