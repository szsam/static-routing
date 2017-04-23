#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <unistd.h>
#include <signal.h>

#include <sys/socket.h>
#include <linux/if_packet.h>
#include <net/ethernet.h> /* the L2 protocols */
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/ip_icmp.h>
#include <sys/ioctl.h>
#include <net/if.h>

#include <sys/time.h>
#include <assert.h>

#include "../wrapper.h"

#define ICMP_HDR_LEN sizeof(struct icmphdr)
#define ICMP_PACKET_LEN (ICMP_HDR_LEN+sizeof(struct timeval))

int SEC_SLEEP_PER_ECHO =1;

struct sockaddr_ll sadr_ll;
const char *IFACE_NAME = "eth0";
const unsigned char NEXT_HOP_HWADDR[ETH_ALEN] = { 0x00,0x0c,0x29,0x8c,0xf6,0xc9 };
const unsigned char LOCAL_HWADDR[ETH_ALEN] = { 0x00,0x0c,0x29,0x37,0xf5,0xba };
const char *LOCAL_IP = "192.168.1.1"; 

struct timeval begin_tv;

#define BUF_SIZE 1024
char sendbuf[BUF_SIZE];
char recvbuf[BUF_SIZE];
char *DST_IP;

int nr_received   = 0;
int nr_transmited = 0;

struct 
{
	bool d;
	bool D;

}option;

//sum 16bit /2Byte
unsigned short checkSum(unsigned short *addr, int size){
	unsigned int sum      =0;
	unsigned short *w     =addr;

	while(size>1)
	{
		sum+=*w++;
		size-=2;
	}

	if(size==1)  
		sum+=*w;  

	/*
	   On a 2's complement machine, the 1's complement sum must be
	   computed by means of an "end around carry", i.e., any overflows
	   from the most significant bits are added into the least
	   significant bits. See the examples below.
	   */
	sum=(sum>>16)+(sum&0xffff); 
	sum=(sum>>16)+(sum&0xffff); 
	// sum+=(sum>>16);  
	return (unsigned short)(~sum); 
}

void icmpRequest(int sockfd,int sequence){
	memset(sendbuf, 0, BUF_SIZE);

	/* Construct Ethernet header */
	struct ethhdr *eth = (struct ethhdr *)(sendbuf);
	memcpy(eth->h_source, LOCAL_HWADDR, ETH_ALEN);
	memcpy(eth->h_dest, NEXT_HOP_HWADDR, ETH_ALEN);
	eth->h_proto = htons(ETH_P_IP);

	/* Construct IP header */
	struct iphdr *iph = (struct iphdr*)(sendbuf + sizeof(struct ethhdr));
	iph->version = 4;
	iph->ihl = 5;
	iph->ttl = 64;
	iph->protocol = 1;	// ICMP
	iph->saddr = inet_addr(LOCAL_IP);
	iph->daddr = inet_addr(DST_IP);
	iph->tot_len = htons(sizeof(struct iphdr) + ICMP_PACKET_LEN);
	iph->check = checkSum((unsigned short *)iph, sizeof(struct iphdr));
	
	/* Construct ICMP header */
	struct icmphdr *icmp_hdr = 
		(struct icmphdr *)(sendbuf + sizeof(struct ethhdr) + sizeof(struct iphdr));
	icmp_hdr->type  = ICMP_ECHO; //8 – Echo Request
	icmp_hdr->code  = 0;
	// icmp_hdr->checksum = 0;
	icmp_hdr->un.echo.id = htons(getpid());
	icmp_hdr->un.echo.sequence = htons(sequence);
	//set data
	gettimeofday((struct timeval *)((char *)icmp_hdr+sizeof(struct icmphdr)),NULL);
	icmp_hdr->checksum =checkSum((unsigned short*)icmp_hdr, ICMP_PACKET_LEN);

	/* Send Ethernet frame */
	int total_len = sizeof(struct ethhdr) + sizeof(struct iphdr) + ICMP_PACKET_LEN;
	Sendto(sockfd, sendbuf, total_len, 0,
			(const struct sockaddr*)&sadr_ll,sizeof(struct sockaddr_ll));
}

void icmpReply(int sockfd)
{
	struct ethhdr * eth = (struct ethhdr *)recvbuf;
	struct iphdr * ip_hdr    = (struct iphdr *)(recvbuf + sizeof(struct ethhdr));
	struct icmphdr *icmp_hdr = (struct icmphdr *)(ip_hdr + 1);
	struct timeval *send_tv  = (struct timeval *)(icmp_hdr + 1);

	while (1) {
		Recvfrom(sockfd, recvbuf, BUF_SIZE, 0, NULL,NULL);
		if (strncmp((const char *)eth->h_dest, (const char *)LOCAL_HWADDR, ETH_ALEN) 
				|| eth->h_proto != htons(ETH_P_IP))
			continue;
		struct in_addr local_in_addr, dst_in_addr;
		inet_aton(LOCAL_IP, &local_in_addr);
		dst_in_addr.s_addr = ip_hdr->daddr;
		if (local_in_addr.s_addr != dst_in_addr.s_addr || ip_hdr->protocol != 1)
			continue;
		if (icmp_hdr->type != ICMP_ECHOREPLY || icmp_hdr->un.echo.id != htons(getpid()))
			continue;
		break;
	}

	struct timeval now_tv;
	gettimeofday(&now_tv,NULL);
	if(option.D)
	{
		printf("[%ld.%ld]", now_tv.tv_sec,now_tv.tv_usec);
	}
	printf("%d bytes from %s:\ticmp_seq=%d\tttl=%d\ttime=%.1fms\n",
			ICMP_PACKET_LEN,	
			DST_IP,
			ntohs(icmp_hdr->un.echo.sequence ),
			ip_hdr->ttl,
			1000*(now_tv.tv_sec-send_tv->tv_sec)+(now_tv.tv_usec-send_tv->tv_usec)/1000.0
		  );
}

int initSocket()
{
	int sockfd;

	sockfd = Socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));

	/*
	   set timeout
	   */
	struct timeval ti;   
	ti.tv_sec  =3;  
	ti.tv_usec =0;  
	Setsockopt(sockfd,SOL_SOCKET,SO_RCVTIMEO,&ti,sizeof(ti));
	Setsockopt(sockfd,SOL_SOCKET,SO_SNDTIMEO,&ti,sizeof(ti));
	if(option.d)
	{
		Setsockopt(sockfd,SOL_SOCKET,SO_DEBUG,&ti,sizeof(ti));
	}

	// Getting the index of the interface to send a packet
	struct ifreq ifreq_i;
	memset(&ifreq_i,0,sizeof(ifreq_i));
	//giving name of Interface
	strncpy(ifreq_i.ifr_name, IFACE_NAME, IFNAMSIZ-1); 
	//getting Interface Index 
	Ioctl(sockfd, SIOCGIFINDEX, &ifreq_i); 
	sadr_ll.sll_ifindex = ifreq_i.ifr_ifindex; // index of interface

	sadr_ll.sll_halen = ETH_ALEN;
	sadr_ll.sll_family = AF_PACKET;
	memcpy(sadr_ll.sll_addr, NEXT_HOP_HWADDR, ETH_ALEN);

	return sockfd;
}

void closeSocket(int sockfd)
{
	close(sockfd);
}

void my_handler(int s)
{  
	struct timeval now_tv;
	gettimeofday(&now_tv,NULL);
	printf("\n--- %s ping statistics ---\n",DST_IP); 
	printf("%d packets transmitted, %d received, %d%% packet loss, time %.1fms\n",
			nr_transmited,
			nr_received,
			100-100*nr_received/nr_transmited,
			1000*(now_tv.tv_sec-begin_tv.tv_sec)+(now_tv.tv_usec-begin_tv.tv_usec)/1000.0 );
	(void) signal(SIGINT, SIG_DFL); 
	exit(1);
}  



int main(int argc, char* argv[]){
	if(argc<2)
	{
		printf("usage:%s -h to show help\n",argv[0]); exit(1);
	}
	assert(signal(SIGINT, my_handler)!=SIG_ERR);
	unsigned int count =-1;
	int sequence       =1;
	int ch;
	extern char *optarg;

	while((ch = getopt(argc,argv,"Ddhc:i:"))!= -1)
	{
		switch(ch)
		{
			case 'c': count             =atoi(optarg); break;
			case 'd':option.d           =true;break;
			case 'D':option.D           =true;break;
			case 'i':SEC_SLEEP_PER_ECHO =atoi(optarg);break;
			case 'h':system("less ./src/help"); exit(1);
			case '?':printf("arguments error!\n");break;
			default: assert(0);
		}
	}

	int socket_fd=initSocket();
	DST_IP=argv[argc-1];

	gettimeofday(&begin_tv,NULL);

	/* Send ping request, receive ping reply */
	while(count--)
	{
		icmpRequest(socket_fd,sequence++);nr_transmited++;
		icmpReply(socket_fd);
		nr_received++;
		sleep(SEC_SLEEP_PER_ECHO);
	}
	closeSocket(socket_fd);
	return 0;
}
