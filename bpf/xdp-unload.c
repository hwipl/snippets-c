/* unload current xdp program on interface specified in first command line
 * argument
 */

/* bpf */
#include <bpf/libbpf.h>

/* XDP_FLAGS_* */
#include <linux/if_link.h>

/* if_nametoindex() */
#include <net/if.h>

int main(int argc, char **argv) {
	if (argc < 2) {
		return -1;
	}

	int ifindex = if_nametoindex(argv[1]);
	__u32 xdp_flags = XDP_FLAGS_DRV_MODE;

	/* detach bpf program from interface */
	if (bpf_set_link_xdp_fd(ifindex, -1, xdp_flags)) {
		printf("Error removing xdp program\n");
		return -1;
	}

	return 0;
}
