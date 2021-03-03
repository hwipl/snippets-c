/* based on set mtu example from rtnetlink(3) */

/* netlink imports */
#include <asm/types.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>

/* memset */
#include <string.h>

/* printf */
#include <stdio.h>

/* interface index (of loopback device) and mtu */
#define IF_INDEX 1
#define IF_MTU 65534

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
		struct ifinfomsg ifi;
		char attrbuf[512];
	} req;
	memset(&req, 0, sizeof(req));

	/* fill request header */
	req.hdr.nlmsg_len = NLMSG_LENGTH(sizeof(req.ifi));
	req.hdr.nlmsg_pid = 0;
	req.hdr.nlmsg_seq = 1;
	req.hdr.nlmsg_type = RTM_NEWLINK;
	req.hdr.nlmsg_flags = NLM_F_REQUEST;

	/* fill request interface info */
	req.ifi.ifi_family = AF_UNSPEC;
	req.ifi.ifi_index = IF_INDEX;
	req.ifi.ifi_change = 0xffffffff;

	/* add mtu attribute */
	unsigned int mtu = IF_MTU;
	struct rtattr *rta = (struct rtattr *)(((char *) &req) +
					       NLMSG_ALIGN(req.hdr.nlmsg_len));
	rta->rta_type = IFLA_MTU;
	rta->rta_len = RTA_LENGTH(sizeof(mtu));
	memcpy(RTA_DATA(rta), &mtu, sizeof(mtu));

	/* update message length */
	req.hdr.nlmsg_len = NLMSG_ALIGN(req.hdr.nlmsg_len) +
		RTA_LENGTH(sizeof(mtu));

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
