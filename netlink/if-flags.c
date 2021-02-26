/* netlink imports */
#include <asm/types.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>

/* memset */
#include <string.h>

/* printf */
#include <stdio.h>

/* interface flags IFF_* */
#include <net/if.h>

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

/* parse link netlink message */
int parse_link_message(struct nlmsghdr *nh) {
	struct ifinfomsg *ifi = (struct ifinfomsg *)(nh + 1);

	/* parse flags (from netdevice(7)):
	 * IFF_UP            Interface is running.
	 * IFF_BROADCAST     Valid broadcast address set.
	 * IFF_DEBUG         Internal debugging flag.
	 * IFF_LOOPBACK      Interface is a loopback interface.
	 * IFF_POINTOPOINT   Interface is a point-to-point link.
	 * IFF_RUNNING       Resources allocated.
	 * IFF_NOARP         No arp protocol, L2 destination address not
	 *                   set.
	 * IFF_PROMISC       Interface is in promiscuous mode.
	 * IFF_NOTRAILERS    Avoid use of trailers.
	 * IFF_ALLMULTI      Receive all multicast packets.
	 *
	 * IFF_MASTER        Master of a load balancing bundle.
	 * IFF_SLAVE         Slave of a load balancing bundle.
	 * IFF_MULTICAST     Supports multicast
	 * IFF_PORTSEL       Is able to select media type via ifmap.
	 * IFF_AUTOMEDIA     Auto media selection active.
	 * IFF_DYNAMIC       The addresses are lost when the interface
	 *                   goes down.
	 * IFF_LOWER_UP      Driver signals L1 up (since Linux 2.6.17)
	 * IFF_DORMANT       Driver signals dormant (since Linux 2.6.17)
	 * IFF_ECHO          Echo sent packets (since Linux 2.6.25)
	 */
	printf("interface %d flags:\n", ifi->ifi_index);
	if (ifi->ifi_flags & IFF_UP) {
		printf("  up\n");
	}
	if (ifi->ifi_flags & IFF_BROADCAST) {
		printf("  broadcast\n");
	}
	if (ifi->ifi_flags & IFF_DEBUG) {
		printf("  debug\n");
	}
	if (ifi->ifi_flags & IFF_LOOPBACK) {
		printf("  loopback\n");
	}
	if (ifi->ifi_flags & IFF_POINTOPOINT) {
		printf("  point-to-point\n");
	}
	if (ifi->ifi_flags & IFF_RUNNING) {
		printf("  running\n");
	}
	if (ifi->ifi_flags & IFF_NOARP) {
		printf("  no arp\n");
	}
	if (ifi->ifi_flags & IFF_PROMISC) {
		printf("  promiscuous\n");
	}
	if (ifi->ifi_flags & IFF_NOTRAILERS) {
		printf("  no trailers\n");
	}
	if (ifi->ifi_flags & IFF_ALLMULTI) {
		printf("  all multicast\n");
	}
	if (ifi->ifi_flags & IFF_MASTER) {
		printf("  master\n");
	}
	if (ifi->ifi_flags & IFF_SLAVE) {
		printf("  slave\n");
	}
	if (ifi->ifi_flags & IFF_MULTICAST) {
		printf("  multicast\n");
	}
	if (ifi->ifi_flags & IFF_PORTSEL) {
		printf("  media select\n");
	}
	if (ifi->ifi_flags & IFF_AUTOMEDIA) {
		printf("  auto media select\n");
	}
	if (ifi->ifi_flags & IFF_DYNAMIC) {
		printf("  dynamic\n");
	}

#if __UAPI_DEF_IF_NET_DEVICE_FLAGS_LOWER_UP_DORMANT_ECHO
	if (ifi->ifi_flags & IFF_LOWER_UP) {
		printf("  lower up\n");
	}
	if (ifi->ifi_flags & IFF_DORMANT) {
		printf("  dormant\n");
	}
	if (ifi->ifi_flags & IFF_ECHO) {
		printf("  up\n");
	}
#endif

	return 0;
}

/* parse netlink message */
int parse_message(struct nlmsghdr *nh) {
	switch (nh->nlmsg_type) {
	case RTM_NEWLINK:
	case RTM_DELLINK:
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
