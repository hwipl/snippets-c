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
	sa.nl_groups = RTMGRP_IPV4_IFADDR | RTMGRP_IPV6_IFADDR;

	int fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
	if (fd < 0) {
		return fd;
	}
	if (bind(fd, (struct sockaddr *) &sa, sizeof(sa))) {
		return -1;
	}
	return fd;
}

/* print address in routing attribute data */
void print_addr(struct rtattr *rta) {
	for (int i = 0; i < RTA_PAYLOAD(rta); i++) {
		printf("%x", ((unsigned char *) RTA_DATA(rta))[i]);
	}
	printf("\n");
}

void print_ifa_flags(__u32 *flags) {
	/* ifa_flags from linux/if_addr.h:
	 * #define IFA_F_SECONDARY		0x01
	 * #define IFA_F_TEMPORARY		IFA_F_SECONDARY
	 *
	 * #define IFA_F_NODAD			0x02
	 * #define IFA_F_OPTIMISTIC		0x04
	 * #define IFA_F_DADFAILED		0x08
	 * #define IFA_F_HOMEADDRESS		0x10
	 * #define IFA_F_DEPRECATED		0x20
	 * #define IFA_F_TENTATIVE		0x40
	 * #define IFA_F_PERMANENT		0x80
	 * #define IFA_F_MANAGETEMPADDR		0x100
	 * #define IFA_F_NOPREFIXROUTE		0x200
	 * #define IFA_F_MCAUTOJOIN		0x400
	 * #define IFA_F_STABLE_PRIVACY		0x800
	 */
	if (*flags & IFA_F_SECONDARY) {
		printf("    secondary/temporary\n");
	}
	if (*flags & IFA_F_NODAD) {
		printf("    no DAD\n");
	}
	if (*flags & IFA_F_OPTIMISTIC) {
		printf("    optimistic\n");
	}
	if (*flags & IFA_F_DADFAILED) {
		printf("    DAD failed\n");
	}
	if (*flags & IFA_F_HOMEADDRESS) {
		printf("    home address\n");
	}
	if (*flags & IFA_F_DEPRECATED) {
		printf("    deprecated\n");
	}
	if (*flags & IFA_F_TENTATIVE) {
		printf("    tentative\n");
	}
	if (*flags & IFA_F_PERMANENT) {
		printf("    permanent\n");
	}
	if (*flags & IFA_F_MANAGETEMPADDR) {
		printf("    manage temp address\n");
	}
	if (*flags & IFA_F_NOPREFIXROUTE) {
		printf("    no prefix route\n");
	}
	if (*flags & IFA_F_MCAUTOJOIN) {
		printf("    mc auto join\n");
	}
	if (*flags & IFA_F_STABLE_PRIVACY) {
		printf("    stable privacy\n");
	}
}

/* parse routing attribute */
int parse_rta(struct rtattr *rta) {
	struct ifa_cacheinfo *ci;

	switch (rta->rta_type) {
	/* routing attributes from rtnetlink(7):
	 * IFA_UNSPEC      -                      unspecified
	 * IFA_ADDRESS     raw protocol address   interface address
	 * IFA_LOCAL       raw protocol address   local address
	 * IFA_LABEL       asciiz string          name of the interface
	 * IFA_BROADCAST   raw protocol address   broadcast address
	 * IFA_ANYCAST     raw protocol address   anycast address
	 * IFA_CACHEINFO   struct ifa_cacheinfo   Address information
	 */
	case IFA_UNSPEC:
		printf("  unspec\n");
		break;
	case IFA_ADDRESS:
		printf("  address: ");
		print_addr(rta);
		break;
	case IFA_LOCAL:
		printf("  local: ");
		print_addr(rta);
		break;
	case IFA_LABEL:
		printf("  label: %s\n", (char *) RTA_DATA(rta));
		break;
	case IFA_BROADCAST:
		printf("  broadcast: ");
		print_addr(rta);
		break;
	case IFA_ANYCAST:
		printf("  anycast: ");
		print_addr(rta);
		break;
	case IFA_CACHEINFO:
		ci = (struct ifa_cacheinfo *) RTA_DATA(rta);
		printf("  cacheinfo: "
		       "prefered: %d, "
		       "valid: %d, "
		       "created timestamp: %d, "
		       "updated timestamp: %d\n",
		       ci->ifa_prefered, ci->ifa_valid, ci->cstamp, ci->tstamp);
		break;

	/* additional routing attributes from linux/if_addr.h: */
	case IFA_MULTICAST:
		printf("  multicast: ");
		print_addr(rta);
		break;
	case IFA_FLAGS:
		printf("  flags:\n");
		print_ifa_flags((__u32 *) RTA_DATA(rta));
		break;
	case IFA_RT_PRIORITY:  /* u32, priority/metric for prefix route */
		printf("  rt priority: ...\n");
		break;
	case IFA_TARGET_NETNSID:
		printf("  target netnsid: ...\n");
		break;
	default:
		printf("  other (%d)\n", rta->rta_type);
		break;
	}

	return 0;
}

/* parse address netlink message */
int parse_addr_message(struct nlmsghdr *nh) {
	struct ifaddrmsg *ifa = (struct ifaddrmsg *)(nh + 1);

	printf("family: %d, prefixlen: %d, flags: %d, scope: %d, index: %d\n",
	       ifa->ifa_family, ifa->ifa_prefixlen, ifa->ifa_flags,
	       ifa->ifa_scope, ifa->ifa_index);

	/* parse attributes */
	int len = nh->nlmsg_len - sizeof(*ifa);
	struct rtattr *rta;
	for (rta = (struct rtattr *) (ifa + 1); RTA_OK (rta, len);
	     rta = RTA_NEXT (rta, len)) {
		parse_rta(rta);
	}

	return 0;
}

/* parse netlink message */
int parse_message(struct nlmsghdr *nh) {
	switch (nh->nlmsg_type) {
	case RTM_NEWADDR:
		printf("NEW: ");
		parse_addr_message(nh);
		break;
	case RTM_DELADDR:
		printf("DEL: ");
		parse_addr_message(nh);
		break;
	default:
		printf("other\n");
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
