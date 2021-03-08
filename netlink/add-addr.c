/* netlink imports */
#include <asm/types.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>

/* memset */
#include <string.h>

/* printf */
#include <stdio.h>

/* if_nametoindex() */
#include <net/if.h>

/* ntohl() */
#include <arpa/inet.h>

/* interface name, ip address, and prefix length */
#define IF_NAME "veth0"
#define IF_ADDR 0xC0A80117
#define IF_PREFIXLEN 24

/* create netlink socket and return socket fd */
int create_socket() {
	/* create socket address */
	struct sockaddr_nl sa;
	memset(&sa, 0, sizeof(sa));
	sa.nl_family = AF_NETLINK;

	/* create and bind socket */
	int fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
	if (fd < 0) {
		return fd;
	}
	if (bind(fd, (struct sockaddr *) &sa, sizeof(sa))) {
		return -1;
	}
	return fd;
}

/* send request to get all links */
int send_request(int fd) {
	/* create socket address */
	struct sockaddr_nl sa;
	memset(&sa, 0, sizeof(sa));
	sa.nl_family = AF_NETLINK;

	/* create request message */
	struct {
		struct nlmsghdr hdr;
		struct ifaddrmsg ifa;
		char attrbuf[512];
	} req;
	memset(&req, 0, sizeof(req));

	/* fill header */
	req.hdr.nlmsg_len = NLMSG_LENGTH(sizeof(req.ifa));
	req.hdr.nlmsg_pid = 0;
	req.hdr.nlmsg_seq = 1;
	req.hdr.nlmsg_type = RTM_NEWADDR;
	req.hdr.nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE;

	/* fill interface address message */
	req.ifa.ifa_family = AF_INET;
	req.ifa.ifa_prefixlen = IF_PREFIXLEN;
	req.ifa.ifa_flags = IFA_F_PERMANENT;
	req.ifa.ifa_index = if_nametoindex(IF_NAME);

	/* add address attribute */
	__u32 addr = ntohl(IF_ADDR);
	struct rtattr *addr_rta;
	addr_rta = (struct rtattr *)(((char *) &req) +
				     NLMSG_ALIGN(req.hdr.nlmsg_len));
	addr_rta->rta_type = IFA_LOCAL;
	addr_rta->rta_len = RTA_LENGTH(sizeof(addr));
	memcpy(RTA_DATA(addr_rta), &addr, sizeof(addr));

	/* update message length */
	req.hdr.nlmsg_len = NLMSG_ALIGN(req.hdr.nlmsg_len) + addr_rta->rta_len;

	/* send request */
	struct iovec iov = { &req, req.hdr.nlmsg_len };
	struct msghdr msg = { &sa, sizeof(sa), &iov, 1, NULL, 0, 0 };
	sendmsg(fd, &msg, 0);

	return 0;
}

int main(int argc, char **argv) {
	int fd = create_socket();
	send_request(fd);
}
