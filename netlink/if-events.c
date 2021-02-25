/* based on example code from "man 7 netlink":
 * The following example creates a NETLINK_ROUTE netlink socket which will
 * listen to the RTMGRP_LINK (network interface create/delete/up/down events)
 * and RTMGRP_IPV4_IFADDR (IPv4 addresses add/delete events) and
 * RTMGRP_IPV6_IFADDR (IPv6 addresses add/delete events) multicast groups.
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


/* create netlink socket and return socket fd */
int create_socket() {
	struct sockaddr_nl sa;

	memset(&sa, 0, sizeof(sa));
	sa.nl_family = AF_NETLINK;
	sa.nl_groups = RTMGRP_LINK | RTMGRP_IPV4_IFADDR | RTMGRP_IPV6_IFADDR;

	int fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
	if (fd < 0) {
		return fd;
	}
	if (bind(fd, (struct sockaddr *) &sa, sizeof(sa))) {
		return -1;
	}
	return fd;
}

/* parse routing attribute */
int parse_rta(struct rtattr *rta) {
	struct rtnl_link_stats *stats;

	switch (rta->rta_type) {
	case IFLA_UNSPEC:
		printf("  unspec\n");
		break;
	case IFLA_ADDRESS:
		printf("  hw addr: ");
		for (int i = 0; i < RTA_PAYLOAD(rta); i++) {
			printf("%x", ((unsigned char *) RTA_DATA(rta))[i]);
		}
		printf("\n");
		break;
	case IFLA_BROADCAST:
		printf("  bc addr: ");
		for (int i = 0; i < RTA_PAYLOAD(rta); i++) {
			printf("%x", ((unsigned char *) RTA_DATA(rta))[i]);
		}
		printf("\n");
		break;
	case IFLA_IFNAME:
		printf("  ifname: %s\n", (char *) RTA_DATA(rta));
		break;
	case IFLA_MTU:
		printf("  mtu: %u\n", *((unsigned int *) RTA_DATA(rta)));
		break;
	case IFLA_LINK:
		printf("  link: %d\n", *((int *) RTA_DATA(rta)));
		break;
	case IFLA_QDISC:
		printf("  qdisc: %s\n", (char *) RTA_DATA(rta));
		break;
	case IFLA_STATS:
		stats = RTA_DATA(rta);
		printf("  stats: \n"
		       "    rx pkts: %d,\n"
		       "    tx pkts: %d,\n"
		       "    ...\n",
		       stats->rx_packets, stats->tx_packets);
		break;
	default:
		printf("  other\n");
	}

	return 0;
}

/* parse link netlink message */
int parse_link_message(struct nlmsghdr *nh) {
	struct ifinfomsg *ifi = (struct ifinfomsg *)(nh + 1);

	printf("interface info msg:\n"
	       "  family: %d,\n"
	       "  type: %d,\n"
	       "  index: %d,\n"
	       "  flags: %d,\n"
	       "  change: %d,\n",
	       ifi->ifi_family, ifi->ifi_type, ifi->ifi_index, ifi->ifi_flags,
	       ifi->ifi_change);

	/* parse attributes */
	int len = nh->nlmsg_len - sizeof(*ifi);
	struct rtattr *rta;
	for (rta = (struct rtattr *) (ifi + 1); RTA_OK (rta, len);
	     rta = RTA_NEXT (rta, len)) {
		printf("routing attribute:"
		       "  len: %d,\n"
		       "  type: %d,\n",
		       rta->rta_len, rta->rta_type);
		parse_rta(rta);
	}

	return 0;
}

/* parse netlink message */
int parse_message(struct nlmsghdr *nh) {
	switch (nh->nlmsg_type) {
	case RTM_NEWLINK:
		printf("new link ");
		parse_link_message(nh);
		break;
	case RTM_DELLINK:
		printf("del link ");
		parse_link_message(nh);
		break;
	case RTM_NEWADDR:
		printf("new addr ");
		break;
	case RTM_DELADDR:
		printf("del addr ");
		break;
	default:
		printf("other ");
	}

	printf("netlink msg:\n"
	       "  len: %d,\n"
	       "  type: %d,\n"
	       "  flags: %d,\n"
	       "  seq: %d,\n"
	       "  pid: %d\n",
	       nh->nlmsg_len, nh->nlmsg_type, nh->nlmsg_flags,
	       nh->nlmsg_seq, nh->nlmsg_pid);

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
			return 0;
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

int main(int arc, char **argv) {
	int fd = create_socket();
	read_messages(fd);
}
