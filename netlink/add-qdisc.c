/* create clsact qdisc on interface specified in first command line argument */

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

/* TC_H_* */
#include <linux/pkt_sched.h>

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

/* send netlink request */
int send_request(int fd, const char *if_name) {
	/* create socket address */
	struct sockaddr_nl sa;
	memset(&sa, 0, sizeof(sa));
	sa.nl_family = AF_NETLINK;

	/* create request message */
	struct {
		struct nlmsghdr hdr;
		struct tcmsg tcm;
		char attrbuf[512];
	} req;
	memset(&req, 0, sizeof(req));

	/* fill header */
	req.hdr.nlmsg_len = NLMSG_LENGTH(sizeof(req.tcm));
	req.hdr.nlmsg_pid = 0;
	req.hdr.nlmsg_seq = 1;
	req.hdr.nlmsg_type = RTM_NEWQDISC;
	req.hdr.nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE;

	/* fill tc message */
	req.tcm.tcm_family = AF_UNSPEC;
	req.tcm.tcm_ifindex = if_nametoindex(if_name);
	req.tcm.tcm_handle = TC_H_MAKE(TC_H_CLSACT, 0);
	req.tcm.tcm_parent = TC_H_CLSACT;

	/* add kind attribute */
	const char *kind = "clsact";
	struct rtattr *kind_rta;
	kind_rta = (struct rtattr *)(((char *) &req) +
				     NLMSG_ALIGN(req.hdr.nlmsg_len));
	kind_rta->rta_type = TCA_KIND;
	kind_rta->rta_len = RTA_LENGTH(strnlen(kind, 6));
	memcpy(RTA_DATA(kind_rta), kind, strnlen(kind, 6));

	/* update message length */
	req.hdr.nlmsg_len = NLMSG_ALIGN(req.hdr.nlmsg_len) + kind_rta->rta_len;

	/* send request */
	struct iovec iov = { &req, req.hdr.nlmsg_len };
	struct msghdr msg = { &sa, sizeof(sa), &iov, 1, NULL, 0, 0 };
	sendmsg(fd, &msg, 0);

	return 0;
}

int main(int argc, char **argv) {
	if (argc < 2) {
		return -1;
	}
	int fd = create_socket();
	send_request(fd, argv[1]);
}
