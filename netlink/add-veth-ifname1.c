/* add veth interface pair and set name of one interface to IF_NAME1 */

/* netlink imports */
#include <asm/types.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>

/* veth */
#include <linux/veth.h>

/* ARPHRD_* */
#include <linux/if_arp.h>

/* memset */
#include <string.h>

/* printf */
#include <stdio.h>

/* ifnames */
#define IF_NAME1 "vethA"
#define IF_MAXLEN 15

/* link info kind attribute */
#define KIND_VETH "veth"
#define KIND_MAXLEN 5

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
		char attrbuf[1024];
	} req;
	memset(&req, 0, sizeof(req));

	/* fill request header */
	req.hdr.nlmsg_len = NLMSG_LENGTH(sizeof(req.ifi));
	req.hdr.nlmsg_pid = 0;
	req.hdr.nlmsg_seq = 1;
	req.hdr.nlmsg_type = RTM_NEWLINK;
	req.hdr.nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE;

	/* fill request interface info */
	req.ifi.ifi_type = ARPHRD_ETHER;
	req.ifi.ifi_change = 0xffffffff;

	/* add ifname attribute */
	struct rtattr *ifname1_rta;
	ifname1_rta = (struct rtattr *)(((char *) &req) +
					NLMSG_ALIGN(req.hdr.nlmsg_len));
	ifname1_rta->rta_type = IFLA_IFNAME;
	ifname1_rta->rta_len = RTA_LENGTH(strnlen(IF_NAME1, IF_MAXLEN));
	memcpy(RTA_DATA(ifname1_rta), IF_NAME1, strnlen(IF_NAME1, IF_MAXLEN));

	/* update message length */
	req.hdr.nlmsg_len = NLMSG_ALIGN(req.hdr.nlmsg_len) +
		ifname1_rta->rta_len;

	/* add link info attribute */
	struct rtattr *info_rta;
	info_rta = (struct rtattr *)(((char *) &req) +
				     NLMSG_ALIGN(req.hdr.nlmsg_len));
	info_rta->rta_type = IFLA_LINKINFO | NLA_F_NESTED;

	/* add nested kind attribute to info attribute */
	struct rtattr *kind_rta = RTA_DATA(info_rta);
	kind_rta->rta_type = IFLA_INFO_KIND;
	kind_rta->rta_len = RTA_LENGTH(strnlen(KIND_VETH, KIND_MAXLEN));
	memcpy(RTA_DATA(kind_rta), KIND_VETH, strnlen(KIND_VETH, KIND_MAXLEN));

	/* update info attribute length */
	info_rta->rta_len = RTA_LENGTH(kind_rta->rta_len);

	/* update message length */
	req.hdr.nlmsg_len = NLMSG_ALIGN(req.hdr.nlmsg_len) + info_rta->rta_len;

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
