/* load bpf program in bpf elf file specified in first command line argument
 * and attach it to interface specified in second command line argument by
 * creating a clsact qdisc and adding a tc bpf filter to it
 */

/* bpf */
#include <bpf/libbpf.h>

/* netlink imports */
#include <asm/types.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>

/* if_nametoindex() */
#include <net/if.h>

/* TC_H_* */
#include <linux/pkt_sched.h>

/* TCA_BPF_* */
#include <linux/pkt_cls.h>

/* ETH_P_ALL */
#include <linux/if_ether.h>

/* htons() */
#include <arpa/inet.h>

/* load bpf program from file and return program fd */
int load_bpf(const char* file) {
	struct bpf_prog_load_attr prog_load_attr = {
		.prog_type      = BPF_PROG_TYPE_SCHED_CLS,
		.file		= file,
	};
	struct bpf_object *obj;
	int prog_fd;

	if (bpf_prog_load_xattr(&prog_load_attr, &obj, &prog_fd)) {
		return -1;
	}

	return prog_fd;
}

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

/* add qdisc with netlink request */
int send_request_qdisc(int fd, const char *if_name) {
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
		RTA_LENGTH(strlen(kind));
	hdr->nlmsg_pid = 0;
	hdr->nlmsg_seq = 1;
	hdr->nlmsg_type = RTM_NEWQDISC;
	hdr->nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE;

	/* fill tc message */
	tcm->tcm_family = AF_UNSPEC;
	tcm->tcm_ifindex = if_nametoindex(if_name);
	tcm->tcm_handle = TC_H_MAKE(TC_H_CLSACT, 0);
	tcm->tcm_parent = TC_H_CLSACT;

	/* fill kind attribute */
	kind_rta->rta_type = TCA_KIND;
	kind_rta->rta_len = RTA_LENGTH(strlen(kind));
	memcpy(RTA_DATA(kind_rta), kind, strlen(kind));

	/* send request */
	struct iovec iov = { msg_buf, hdr->nlmsg_len };
	struct msghdr msg = { &sa, sizeof(sa), &iov, 1, NULL, 0, 0 };
	sendmsg(fd, &msg, 0);

	return 0;
}

/* add tc filter with netlink request */
int send_request_filter(int fd, const char *if_name, int bpf_fd) {
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
	const char *kind = "bpf";
	struct rtattr *options_rta = (struct rtattr *)(attr_buf +
						       RTA_SPACE(strlen(kind)));
	struct rtattr *fd_rta = RTA_DATA(options_rta);
	struct rtattr *name_rta = (struct rtattr *)(((char *) fd_rta) +
						    RTA_SPACE(sizeof(bpf_fd)));
	const char *name = "accept-all";
	struct rtattr *flags_rta = (struct rtattr *)(((char *) name_rta) +
						     RTA_SPACE(strlen(name)));
	__u32 flags = TCA_BPF_FLAG_ACT_DIRECT;

	/* fill header */
	hdr->nlmsg_len = NLMSG_SPACE(sizeof(struct tcmsg)) +
		RTA_SPACE(strlen(kind)) +
		RTA_SPACE(0) +
		RTA_SPACE(sizeof(bpf_fd)) +
		RTA_SPACE(strlen(name)) +
		RTA_LENGTH(sizeof(flags));
	hdr->nlmsg_pid = 0;
	hdr->nlmsg_seq = 1;
	hdr->nlmsg_type = RTM_NEWTFILTER;
	hdr->nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE;

	/* fill tc message */
	tcm->tcm_family = AF_UNSPEC;
	tcm->tcm_ifindex = if_nametoindex(if_name);
	tcm->tcm_handle = 0;
	tcm->tcm_parent = TC_H_MAKE(TC_H_CLSACT, TC_H_MIN_INGRESS);
	tcm->tcm_info = TC_H_MAKE(0, htons(ETH_P_ALL));

	/* fill kind attribute */
	kind_rta->rta_type = TCA_KIND;
	kind_rta->rta_len = RTA_LENGTH(strlen(kind));
	memcpy(RTA_DATA(kind_rta), kind, strlen(kind));

	/* fill options attribute */
	options_rta->rta_type = TCA_OPTIONS;
	options_rta->rta_len = RTA_SPACE(0) + RTA_SPACE(sizeof(bpf_fd)) +
		RTA_SPACE(strlen(name)) + RTA_LENGTH(sizeof(flags));

	/* fill bpf fd attribute */
	fd_rta->rta_type = TCA_BPF_FD;
	fd_rta->rta_len = RTA_LENGTH(sizeof(bpf_fd));
	memcpy(RTA_DATA(fd_rta), &bpf_fd, sizeof(bpf_fd));

	/* fill bpf name attribute */
	name_rta->rta_type = TCA_BPF_NAME;
	name_rta->rta_len = RTA_LENGTH(strlen(name));
	memcpy(RTA_DATA(name_rta), name, strlen(name));

	/* fill bpf flags */
	flags_rta->rta_type = TCA_BPF_FLAGS;
	flags_rta->rta_len = RTA_LENGTH(sizeof(flags));
	memcpy(RTA_DATA(flags_rta), &flags, sizeof(flags));

	/* send request */
	struct iovec iov = { msg_buf, hdr->nlmsg_len };
	struct msghdr msg = { &sa, sizeof(sa), &iov, 1, NULL, 0, 0 };
	sendmsg(fd, &msg, 0);

	return 0;
}

int main(int argc, char **argv) {
	/* handle command line arguments */
	if (argc < 3) {
		return -1;
	}
	const char *bpf_file = argv[1];
	const char *if_name = argv[2];

	/* load bpf program */
	int prog_fd = load_bpf(bpf_file);
	if (prog_fd < 0) {
		printf("Error loading bpf program\n");
		return -1;
	}

	/* create netlink socket */
	int nl_fd = create_socket();
	if (nl_fd < 0) {
		printf("Error creating netlink socket\n");
		return -1;
	}

	/* add qdisc */
	send_request_qdisc(nl_fd, if_name);

	/* add tc filter */
	send_request_filter(nl_fd, if_name, prog_fd);

	return 0;
}
