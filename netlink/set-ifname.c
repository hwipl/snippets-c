/* netlink imports */
#include <asm/types.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>

/* memset */
#include <string.h>

/* printf */
#include <stdio.h>

/* if_nametoindex */
#include <net/if.h>

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
int send_request(int fd, const char *old_name, const char *new_name) {
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
	req.ifi.ifi_index = if_nametoindex(old_name);
	req.ifi.ifi_change = 0xffffffff;

	/* add mtu attribute */
	struct rtattr *rta = (struct rtattr *)(((char *) &req) +
					       NLMSG_ALIGN(req.hdr.nlmsg_len));
	rta->rta_type = IFLA_IFNAME;
	rta->rta_len = RTA_LENGTH(strnlen(new_name, 15));
	memcpy(RTA_DATA(rta), new_name, strnlen(new_name, 15));

	/* update message length */
	req.hdr.nlmsg_len = NLMSG_ALIGN(req.hdr.nlmsg_len) + rta->rta_len;

	/* send request */
	struct iovec iov = { &req, req.hdr.nlmsg_len };
	struct msghdr msg = { &sa, sizeof(sa), &iov, 1, NULL, 0, 0 };
	sendmsg(fd, &msg, 0);

	return 0;
}

int main(int argc, char **argv) {
	if (argc < 3) {
		return -1;
	}
	int fd = create_socket();
	send_request(fd, argv[1], argv[2]);
}
