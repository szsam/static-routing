#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>

static inline int Socket(int domain, int type, int proto)
{
	int rc;
    if ((rc = socket(domain, type, proto)) == -1) {
		perror("socket");
		exit(0);
	}
	return rc;
}


static inline
ssize_t Recvfrom(int sockfd, void *buf, size_t len, int flags,
		struct sockaddr *src_addr, socklen_t *addrlen)
{
	int rc;
    if ((rc = recvfrom(sockfd, buf, len, flags, src_addr, addrlen)) == -1) {
		perror("recvfrom");
		exit(0);
	}
	return rc;
}

static inline
int Ioctl(int fd, unsigned long request, void *argp)
{
	int rc;
	if ((rc = ioctl(fd, request, argp)) == -1) {
		perror("ioctl");
		exit(0);
	}
	return rc;
}

static inline 
ssize_t Sendto(int sockfd, const void *buf, size_t len, int flags,
			         const struct sockaddr *dest_addr, socklen_t addrlen)
{
	int rc;
	if ((rc = sendto(sockfd, buf, len, flags, dest_addr, addrlen)) == -1){
		perror("sendto");
		exit(0);
	}
	return rc;
}

