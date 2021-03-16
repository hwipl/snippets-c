/* add a bpf filter to an existing clsact qdisc (e.g., added by add-qdisc) on
 * the interface specified in first command line argument
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

/* bpf_insn, BPF_* */
#include <linux/bpf.h>

/* syscall */
#include <unistd.h>
#include <sys/syscall.h>

/* TCA_BPF_* */
#include <linux/pkt_cls.h>

/* ETH_P_ALL */
#include <linux/if_ether.h>

/* htons() */
#include <arpa/inet.h>

/* bpf program:
 * return TC_ACT_OK (r0 = 0) and exit
 */
struct bpf_insn prog[] = {
	{
		/* r0 = 0 */
		.code  = BPF_ALU64 | BPF_MOV | BPF_K,
		.dst_reg = BPF_REG_0,
		.src_reg = 0,
		.off   = 0,
		.imm   = 0,
	},
	{
		/* return r0 */
		.code  = BPF_JMP | BPF_EXIT,
		.dst_reg = 0,
		.src_reg = 0,
		.off   = 0,
		.imm   = 0,
	},
};


/* ptr to u64 helper from <kernel src>/tools/perf/util/bpf-event.c */
#define ptr_to_u64(ptr)    ((__u64)(unsigned long)(ptr))

/* bpf program loading helper based on example from bpf(2) */
int bpf_prog_load(enum bpf_prog_type type, const struct bpf_insn *insns, int
		  insn_cnt, const char *license) {
	union bpf_attr attr = {
		.prog_type = type,
		.insns     = ptr_to_u64(insns),
		.insn_cnt  = insn_cnt,
		.license   = ptr_to_u64(license),
		.log_buf   = 0,
		.log_size  = 0,
		.log_level = 0,
	};

	return syscall(__NR_bpf, BPF_PROG_LOAD, &attr, sizeof(attr));
}

/* load bpf program into the kernel and return fd */
int load_bpf() {
	return  bpf_prog_load(BPF_PROG_TYPE_SCHED_CLS, prog,
			      sizeof(prog) / sizeof(prog[0]), "GPL");
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

/* send netlink request */
int send_request(int fd, const char *if_name, int bpf_fd) {
	/* create socket address */
	struct sockaddr_nl sa;
	memset(&sa, 0, sizeof(sa));
	sa.nl_family = AF_NETLINK;

	/* create request message */
	struct {
		struct nlmsghdr hdr;
		struct tcmsg tcm;
		char attrbuf[512];
	} req;
	memset(&req, 0, sizeof(req));

	/* fill header */
	req.hdr.nlmsg_len = NLMSG_LENGTH(sizeof(req.tcm));
	req.hdr.nlmsg_pid = 0;
	req.hdr.nlmsg_seq = 1;
	req.hdr.nlmsg_type = RTM_NEWTFILTER;
	req.hdr.nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE;

	/* fill tc message */
	req.tcm.tcm_family = AF_UNSPEC;
	req.tcm.tcm_ifindex = if_nametoindex(if_name);
	req.tcm.tcm_handle = 0;
	req.tcm.tcm_parent = TC_H_MAKE(TC_H_CLSACT, TC_H_MIN_INGRESS);
	req.tcm.tcm_info = TC_H_MAKE(0, htons(ETH_P_ALL));

	/* add kind attribute */
	const char *kind = "bpf";
	struct rtattr *kind_rta;
	kind_rta = (struct rtattr *)(((char *) &req) +
				     NLMSG_ALIGN(req.hdr.nlmsg_len));
	kind_rta->rta_type = TCA_KIND;
	kind_rta->rta_len = RTA_LENGTH(strnlen(kind, 3) + 1);
	memcpy(RTA_DATA(kind_rta), kind, strnlen(kind, 3) + 1);

	/* update message length */
	req.hdr.nlmsg_len = NLMSG_ALIGN(req.hdr.nlmsg_len) + kind_rta->rta_len;

	/* add options attribute */
	struct rtattr *options_rta;
	options_rta = (struct rtattr *)(((char *) &req) +
					NLMSG_ALIGN(req.hdr.nlmsg_len));
	options_rta->rta_type = TCA_OPTIONS;
	options_rta->rta_len = RTA_LENGTH(0);

	/* add bpf fd attribute */
	struct rtattr *fd_rta = RTA_DATA(options_rta);
	fd_rta->rta_type = TCA_BPF_FD;
	fd_rta->rta_len = RTA_LENGTH(sizeof(int));
	memcpy(RTA_DATA(fd_rta), &bpf_fd, sizeof(int));

	/* update options length */
	options_rta->rta_len = RTA_ALIGN(options_rta->rta_len) +
		fd_rta->rta_len;

	/* add bpf name attribute */
	const char *name = "accept-all";
	struct rtattr *name_rta;
	name_rta = (struct rtattr *)(((char *) options_rta) +
				     RTA_ALIGN(options_rta->rta_len));
	name_rta->rta_type = TCA_BPF_NAME;
	name_rta->rta_len = RTA_LENGTH(strnlen(name, 10) + 1);
	memcpy(RTA_DATA(name_rta), name, strnlen(name, 10 + 1));

	/* update options length */
	options_rta->rta_len = RTA_ALIGN(options_rta->rta_len) +
		name_rta->rta_len;

	/* add bpf flags */
	__u32 flags = TCA_BPF_FLAG_ACT_DIRECT;
	struct rtattr *flags_rta;
	flags_rta = (struct rtattr *)(((char *) options_rta) +
				      RTA_ALIGN(options_rta->rta_len));
	flags_rta->rta_type = TCA_BPF_FLAGS;
	flags_rta->rta_len = RTA_LENGTH(sizeof(flags));
	memcpy(RTA_DATA(flags_rta), &flags, sizeof(flags));

	/* update options length */
	options_rta->rta_len = RTA_ALIGN(options_rta->rta_len) +
		flags_rta->rta_len;

	/* update message length */
	req.hdr.nlmsg_len = NLMSG_ALIGN(req.hdr.nlmsg_len) +
		options_rta->rta_len;

	/* send request */
	struct iovec iov = { &req, req.hdr.nlmsg_len };
	struct msghdr msg = { &sa, sizeof(sa), &iov, 1, NULL, 0, 0 };
	sendmsg(fd, &msg, 0);

	return 0;
}

int main(int argc, char **argv) {
	if (argc < 2) {
		return -1;
	}
	int sock_fd = create_socket();
	int bpf_fd = load_bpf();
	send_request(sock_fd, argv[1], bpf_fd);
}
