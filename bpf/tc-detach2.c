/* unload current bpf program on interface specified in first command line
 * argument using tc and libbpf
 */

/* bpf */
#include <bpf/libbpf.h>

/* if_nametoindex() */
#include <net/if.h>

/* detach bpf program from network interface identified by if_name */
int detach_bpf(const char *if_name) {
	int rc;

	// destroy bpf hook
	struct bpf_tc_hook hook;
	memset(&hook, 0, sizeof(hook));
	hook.sz			= sizeof(struct bpf_tc_hook);
	hook.ifindex		= if_nametoindex(if_name);
	// specify BPF_TC_INGRESS | BPF_TC_EGRESS to delete the qdisc;
	// specifying only BPF_TC_INGRESS or only BPF_TC_EGRESS
	// deletes the respective filter only
	hook.attach_point	= BPF_TC_INGRESS | BPF_TC_EGRESS;
	rc = bpf_tc_hook_destroy(&hook);
	if (rc) {
		printf("Error destroying tc hook\n");
		return rc;
	}

	return 0;
}

int main(int argc, char **argv) {
	/* handle command line arguments */
	if (argc < 2) {
		return -1;
	}
	const char *if_name = argv[1];

	/* detach bpf program */
	if (detach_bpf(if_name)) {
		printf("Error detaching bpf program\n");
		return -1;
	}

	return 0;
}
