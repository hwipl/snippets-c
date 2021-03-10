/* set network namespace of interface specified in the first command line
 * argument to the network namespace identified by the file in the second
 * command line argument, e.g., /var/run/netns/testns
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

/* if_nametoindex */
#include <net/if.h>

/* open(), O_RDONLY */
#include <fcntl.h>

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
int send_request(int fd, const char *if_name, int target_fd) {
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
	req.hdr.nlmsg_type = RTM_SETLINK;
	req.hdr.nlmsg_flags = NLM_F_REQUEST;

	/* fill request interface info */
	req.ifi.ifi_family = AF_UNSPEC;
	req.ifi.ifi_index = if_nametoindex(if_name);
	req.ifi.ifi_change = 0xffffffff;

	/* add network namespace attribute */
	struct rtattr *rta = (struct rtattr *)(((char *) &req) +
					       NLMSG_ALIGN(req.hdr.nlmsg_len));
	rta->rta_type = IFLA_NET_NS_FD;
	rta->rta_len = RTA_LENGTH(sizeof(target_fd));
	memcpy(RTA_DATA(rta), &target_fd, sizeof(target_fd));

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
	int netns_fd = open(argv[2], O_RDONLY);
	if (netns_fd == -1) {
		printf("error opening %s\n", argv[2]);
		return -1;
	}
	int sock_fd = create_socket();
	send_request(sock_fd, argv[1], netns_fd);
}
