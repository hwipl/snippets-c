/* unload current bpf program on interface specified in first command line
 * argument by removing the clsact qdisc
 */

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
	char msg_buf[512] = { 0 };
	struct nlmsghdr *hdr = (struct nlmsghdr *) msg_buf;
	struct tcmsg *tcm = NLMSG_DATA(hdr);
	char *attr_buf = msg_buf + NLMSG_SPACE(sizeof(struct tcmsg));
	struct rtattr *kind_rta = (struct rtattr *) attr_buf;
	const char *kind = "clsact";

	/* fill header */
	hdr->nlmsg_len = NLMSG_SPACE(sizeof(struct tcmsg)) +
		RTA_LENGTH(strlen(kind) + 1);
	hdr->nlmsg_pid = 0;
	hdr->nlmsg_seq = 1;
	hdr->nlmsg_type = RTM_DELQDISC;
	hdr->nlmsg_flags = NLM_F_REQUEST;

	/* fill tc message */
	tcm->tcm_family = AF_UNSPEC;
	tcm->tcm_ifindex = if_nametoindex(if_name);
	tcm->tcm_handle = TC_H_MAKE(TC_H_CLSACT, 0);
	tcm->tcm_parent = TC_H_CLSACT;

	/* fill kind attribute */
	kind_rta->rta_type = TCA_KIND;
	kind_rta->rta_len = RTA_LENGTH(strlen(kind) + 1);
	memcpy(RTA_DATA(kind_rta), kind, strlen(kind) + 1);

	/* send request */
	struct iovec iov = { msg_buf, hdr->nlmsg_len };
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
