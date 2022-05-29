/* load bpf program in bpf elf file specified in first command line argument
 * and attach it to interface specified in second command line argument using
 * tc and libbpf
 */

/* bpf */
#include <bpf/libbpf.h>

/* if_nametoindex() */
#include <net/if.h>

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

/* attach bpf program in prog_fd to network interface identified by if_name */
int attach_bpf(const char *if_name, int prog_fd) {
	int rc;

	// create bpf hook
	struct bpf_tc_hook hook;
	memset(&hook, 0, sizeof(hook));
	hook.sz			= sizeof(struct bpf_tc_hook);
	hook.ifindex		= if_nametoindex(if_name);
	hook.attach_point	= BPF_TC_INGRESS; // BPF_TC_EGRESS, BPF_TC_CUSTOM
	rc = bpf_tc_hook_create(&hook);
	if (rc) {
		printf("Error creating tc hook\n");
		return rc;
	}

	// attach bpf program
	struct bpf_tc_opts opts;
	memset(&opts, 0, sizeof(opts));
	opts.sz		= sizeof(struct bpf_tc_opts);
	opts.prog_fd	= prog_fd;
	rc = bpf_tc_attach(&hook, &opts);
	if (rc) {
		printf("Error during tc attach\n");
		return rc;
	}

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

	/* attach bpf program */
	if (attach_bpf(if_name, prog_fd)) {
		printf("Error attaching bpf program\n");
		return -1;
	}

	return 0;
}
