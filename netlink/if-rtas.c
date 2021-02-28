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

/* parse routing attribute */
int parse_rta(struct rtattr *rta) {
	struct rtnl_link_stats *stats;

	/* routing attributes from linux/if_link.h:
	 * IFLA_UNSPEC,
	 * IFLA_ADDRESS,
	 * IFLA_BROADCAST,
	 * IFLA_IFNAME,
	 * IFLA_MTU,
	 * IFLA_LINK,
	 * IFLA_QDISC,
	 * IFLA_STATS,
	 * IFLA_COST,
	 * IFLA_PRIORITY,
	 * IFLA_MASTER,
	 * IFLA_WIRELESS,	Wireless Extension event - see wireless.h
	 * IFLA_PROTINFO,	Protocol specific information for a link
	 * IFLA_TXQLEN,
	 * IFLA_MAP,
	 * IFLA_WEIGHT,
	 * IFLA_OPERSTATE,
	 * IFLA_LINKMODE,
	 * IFLA_LINKINFO,
	 * IFLA_NET_NS_PID,
	 * IFLA_IFALIAS,
	 * IFLA_NUM_VF,		Number of VFs if device is SR-IOV PF
	 * IFLA_VFINFO_LIST,
	 * IFLA_STATS64,
	 * IFLA_VF_PORTS,
	 * IFLA_PORT_SELF,
	 * IFLA_AF_SPEC,
	 * IFLA_GROUP,		Group the device belongs to
	 * IFLA_NET_NS_FD,
	 * IFLA_EXT_MASK,	Extended info mask, VFs, etc
	 * IFLA_PROMISCUITY,	Promiscuity count: > 0 means acts PROMISC
	 * IFLA_NUM_TX_QUEUES,
	 * IFLA_NUM_RX_QUEUES,
	 * IFLA_CARRIER,
	 * IFLA_PHYS_PORT_ID,
	 * IFLA_CARRIER_CHANGES,
	 * IFLA_PHYS_SWITCH_ID,
	 * IFLA_LINK_NETNSID,
	 * IFLA_PHYS_PORT_NAME,
	 * IFLA_PROTO_DOWN,
	 * IFLA_GSO_MAX_SEGS,
	 * IFLA_GSO_MAX_SIZE,
	 * IFLA_PAD,
	 * IFLA_XDP,
	 * IFLA_EVENT,
	 * IFLA_NEW_NETNSID,
	 * IFLA_IF_NETNSID,
	 * IFLA_TARGET_NETNSID = IFLA_IF_NETNSID,	new alias
	 * IFLA_CARRIER_UP_COUNT,
	 * IFLA_CARRIER_DOWN_COUNT,
	 * IFLA_NEW_IFINDEX,
	 * IFLA_MIN_MTU,
	 * IFLA_MAX_MTU,
	 * IFLA_PROP_LIST,
	 * IFLA_ALT_IFNAME,	Alternative ifname
	 * IFLA_PERM_ADDRESS,
	 * IFLA_PROTO_DOWN_REASON,
	 */
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
	case IFLA_COST:
		printf("  cost\n");
		break;
	case IFLA_PRIORITY:
		printf("  priority\n");
		break;
	case IFLA_MASTER:
		printf("  master\n");
		break;
	case IFLA_WIRELESS:
		printf("  wireless\n");
		break;
	case IFLA_PROTINFO:
		printf("  protinfo\n");
		break;
	case IFLA_TXQLEN:
		printf("  txqlen\n");
		break;
	case IFLA_MAP:
		printf("  map\n");
		break;
	case IFLA_WEIGHT:
		printf("  weight\n");
		break;
	case IFLA_OPERSTATE:
		printf("  operstate\n");
		break;
	case IFLA_LINKMODE:
		printf("  linkmode\n");
		break;
	case IFLA_LINKINFO:
		printf("  linkinfo\n");
		break;
	case IFLA_NET_NS_PID:
		printf("  net ns pid\n");
		break;
	case IFLA_IFALIAS:
		printf("  ifalias\n");
		break;
	case IFLA_NUM_VF:
		printf("  num_vf\n");
		break;
	case IFLA_VFINFO_LIST:
		printf("  vfinfo list\n");
		break;
	case IFLA_STATS64:
		printf("  stats64\n");
		break;
	case IFLA_VF_PORTS:
		printf("  vf ports\n");
		break;
	case IFLA_PORT_SELF:
		printf("  port self\n");
		break;
	case IFLA_AF_SPEC:
		printf("  af spec\n");
		break;
	case IFLA_GROUP:
		printf("  group\n");
		break;
	case IFLA_NET_NS_FD:
		printf("  net ns fd\n");
		break;
	case IFLA_EXT_MASK:
		printf("  ext mask\n");
		break;
	case IFLA_PROMISCUITY:
		printf("  promiscuity\n");
		break;
	case IFLA_NUM_TX_QUEUES:
		printf("  num rx queues\n");
		break;
	case IFLA_NUM_RX_QUEUES:
		printf("  num rx queues\n");
		break;
	case IFLA_CARRIER:
		printf("  carrier\n");
		break;
	case IFLA_PHYS_PORT_ID:
		printf("  phys port id\n");
		break;
	case IFLA_CARRIER_CHANGES:
		printf("  carrier changes\n");
		break;
	case IFLA_PHYS_SWITCH_ID:
		printf("  phys switch id\n");
		break;
	case IFLA_LINK_NETNSID:
		printf("  link netnsid\n");
		break;
	case IFLA_PHYS_PORT_NAME:
		printf("  port name\n");
		break;
	case IFLA_PROTO_DOWN:
		printf("  proto down\n");
		break;
	case IFLA_GSO_MAX_SEGS:
		printf("  gso max segs\n");
		break;
	case IFLA_GSO_MAX_SIZE:
		printf("  gso max size\n");
		break;
	case IFLA_PAD:
		printf("  pad\n");
		break;
	case IFLA_XDP:
		printf("  xdp\n");
		break;
	case IFLA_EVENT:
		printf("  event\n");
		break;
	case IFLA_NEW_NETNSID:
		printf("  new netnsid\n");
		break;
	/* case IFLA_IF_NETNSID: */
	case IFLA_TARGET_NETNSID:
		printf("  if netnsid/target netnsid\n");
		break;
	case IFLA_CARRIER_UP_COUNT:
		printf("  carrier up count\n");
		break;
	case IFLA_CARRIER_DOWN_COUNT:
		printf("  carrier down count\n");
		break;
	case IFLA_NEW_IFINDEX:
		printf("  new ifindex\n");
		break;
	case IFLA_MIN_MTU:
		printf("  min mtu\n");
		break;
	case IFLA_MAX_MTU:
		printf("  max mtu\n");
		break;
	case IFLA_PROP_LIST:
		printf("  prop list\n");
		break;
	case IFLA_ALT_IFNAME:
		printf("  alt ifname\n");
		break;
	case IFLA_PERM_ADDRESS:
		printf("  perm address\n");
		break;
	case IFLA_PROTO_DOWN_REASON:
		printf("  proto down reason\n");
		break;
	default:
		printf("  other\n");
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
	case RTM_DELLINK:
		parse_link_message(nh);
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
