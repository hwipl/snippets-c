/* bpf */
#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>

/* set license to gpl */
char _license[] SEC("license") = "GPL";

/* accept all packets */
SEC("accept_all")
int _accept_all(struct xdp_md *ctx)
{
	return XDP_PASS;
}
