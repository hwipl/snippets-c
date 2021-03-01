/* netlink imports */
#include <asm/types.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>

/* memset */
#include <string.h>

/* printf */
#include <stdio.h>

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
		struct rtgenmsg gen;
	} req;
	memset(&req, 0, sizeof(req));

	/* fill request message */
	req.hdr.nlmsg_len = NLMSG_LENGTH(sizeof(struct rtgenmsg));
	req.hdr.nlmsg_pid = 0;
	req.hdr.nlmsg_seq = 1;
	req.hdr.nlmsg_type = RTM_GETLINK;
	req.hdr.nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;
	req.gen.rtgen_family = AF_PACKET;

	/* send request */
	struct iovec iov = { &req, req.hdr.nlmsg_len };
	struct msghdr msg = { &sa, sizeof(sa), &iov, 1, NULL, 0, 0 };
	sendmsg(fd, &msg, 0);

	return 0;
}

/* parse routing attribute */
int parse_rta(struct rtattr *rta) {
	struct rtnl_link_stats *stats;

	switch (rta->rta_type) {
	case IFLA_IFNAME:
		printf("Link: %s\n", (char *) RTA_DATA(rta));
		break;
	}

	return 0;
}

/* parse link netlink message */
int parse_link_message(struct nlmsghdr *nh) {
	struct ifinfomsg *ifi = (struct ifinfomsg *)(nh + 1);

	/* parse attributes */
	int len = nh->nlmsg_len - sizeof(*ifi);
	struct rtattr *rta;
	for (rta = (struct rtattr *) (ifi + 1); RTA_OK (rta, len);
	     rta = RTA_NEXT (rta, len)) {
		parse_rta(rta);
	}

	return 0;
}

/* parse netlink message */
int parse_message(struct nlmsghdr *nh) {
	switch (nh->nlmsg_type) {
	case RTM_NEWLINK:
		parse_link_message(nh);
		break;
	}

	return 0;
}

/* read a single netlink messages from socket fd */
int read_message(int fd) {
	/* 8192 to avoid msg truncation on platforms with page size > 4096 */
	struct nlmsghdr buf[8192/sizeof(struct nlmsghdr)];
	struct iovec iov = { buf, sizeof(buf) };
	struct sockaddr_nl sa;
	struct nlmsghdr *nh;
	int len;

	struct msghdr msg = { &sa, sizeof(sa), &iov, 1, NULL, 0, 0 };

	len = recvmsg(fd, &msg, 0);
	if (len <= 0) {
		return -1;
	}

	for (nh = (struct nlmsghdr *) buf; NLMSG_OK (nh, len);
	     nh = NLMSG_NEXT (nh, len)) {
		/* end of multipart message */
		if (nh->nlmsg_type == NLMSG_DONE) {
			return 1; /* stop here */
		}
		/* error handling */
		if (nh->nlmsg_type == NLMSG_ERROR) {
			return -1;
		}
		/* parse payload */
		parse_message(nh);
	}

	return 0;
}

/* read netlink messages from fd */
void read_messages(int fd) {
	while (!read_message(fd));
}

int main(int argc, char **argv) {
	int fd = create_socket();
	send_request(fd);
	read_messages(fd);
}
