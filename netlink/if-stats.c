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
	sa.nl_groups = RTMGRP_LINK;

	int fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
	if (fd < 0) {
		return fd;
	}
	if (bind(fd, (struct sockaddr *) &sa, sizeof(sa))) {
		return -1;
	}
	return fd;
}

/* print stats */
void print_stats(struct rtnl_link_stats *stats) {
	printf("  stats: \n"
	       "    rx packets: %d\n"
	       "    tx packets: %d\n"
	       "    rx bytes: %d\n"
	       "    tx bytes: %d\n"
	       "    rx errors: %d\n"
	       "    tx errors: %d\n"
	       "    rx dropped: %d\n"
	       "    tx dropped: %d\n"
	       "    multicast: %d\n"
	       "    collisions: %d\n"
	       "    rx length errors: %d\n"
	       "    rx over errors: %d\n"
	       "    rx crc errors: %d\n"
	       "    rx frame errors: %d\n"
	       "    rx fifo errors: %d\n"
	       "    rx missed errors: %d\n"
	       "    tx aborted errors: %d\n"
	       "    tx carrier errors: %d\n"
	       "    tx fifo errors: %d\n"
	       "    tx heartbeat errors: %d\n"
	       "    tx window errors: %d\n"
	       "    rx compressed: %d\n"
	       "    tx compressed: %d\n"
	       "    rx no handler: %d\n",
	       stats->rx_packets, stats->tx_packets, stats->rx_bytes,
	       stats->tx_bytes, stats->rx_errors, stats->tx_errors,
	       stats->rx_dropped, stats->tx_dropped, stats->multicast,
	       stats->collisions, stats->rx_length_errors,
	       stats->rx_over_errors, stats->rx_crc_errors,
	       stats->rx_frame_errors, stats->rx_fifo_errors,
	       stats->rx_missed_errors, stats->tx_aborted_errors,
	       stats->tx_carrier_errors, stats->tx_fifo_errors,
	       stats->tx_heartbeat_errors, stats->tx_window_errors,
	       stats->rx_compressed, stats->tx_compressed,
	       stats->rx_nohandler);
}

/* print stats64 */
void print_stats64(struct rtnl_link_stats64 *stats) {
	printf("  stats64: \n"
	       "    rx packets: %llu\n"
	       "    tx packets: %llu\n"
	       "    rx bytes: %llu\n"
	       "    tx bytes: %llu\n"
	       "    rx errors: %llu\n"
	       "    tx errors: %llu\n"
	       "    rx dropped: %llu\n"
	       "    tx dropped: %llu\n"
	       "    multicast: %llu\n"
	       "    collisions: %llu\n"
	       "    rx length errors: %llu\n"
	       "    rx over errors: %llu\n"
	       "    rx crc errors: %llu\n"
	       "    rx frame errors: %llu\n"
	       "    rx fifo errors: %llu\n"
	       "    rx missed errors: %llu\n"
	       "    tx aborted errors: %llu\n"
	       "    tx carrier errors: %llu\n"
	       "    tx fifo errors: %llu\n"
	       "    tx heartbeat errors: %llu\n"
	       "    tx window errors: %llu\n"
	       "    rx compressed: %llu\n"
	       "    tx compressed: %llu\n"
	       "    rx no handler: %llu\n",
	       stats->rx_packets, stats->tx_packets, stats->rx_bytes,
	       stats->tx_bytes, stats->rx_errors, stats->tx_errors,
	       stats->rx_dropped, stats->tx_dropped, stats->multicast,
	       stats->collisions, stats->rx_length_errors,
	       stats->rx_over_errors, stats->rx_crc_errors,
	       stats->rx_frame_errors, stats->rx_fifo_errors,
	       stats->rx_missed_errors, stats->tx_aborted_errors,
	       stats->tx_carrier_errors, stats->tx_fifo_errors,
	       stats->tx_heartbeat_errors, stats->tx_window_errors,
	       stats->rx_compressed, stats->tx_compressed,
	       stats->rx_nohandler);
}

/* parse routing attribute */
int parse_rta(struct rtattr *rta) {
	struct rtnl_link_stats64 *stats64;
	struct rtnl_link_stats *stats;

	switch (rta->rta_type) {
	case IFLA_IFNAME:
		printf("  ifname: %s\n", (char *) RTA_DATA(rta));
		break;
	case IFLA_STATS64:
		stats64 = RTA_DATA(rta);
		print_stats64(stats64);
		break;
	case IFLA_STATS:
		stats = RTA_DATA(rta);
		print_stats(stats);
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
