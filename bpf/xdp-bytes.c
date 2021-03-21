/* bpf */
#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>

/* set license to gpl */
char _license[] SEC("license") = "GPL";

/* map definitions */
struct bpf_elf_map {
	__u32 type;
	__u32 size_key;
	__u32 size_value;
	__u32 max_elem;
	__u32 flags;
	__u32 id;
	__u32 pinning;
};

/* map for byte count */
struct bpf_elf_map SEC("maps") rx_bytes = {
	.type = BPF_MAP_TYPE_ARRAY,
	.size_key = sizeof(__u32),
	.size_value = sizeof(long),
	.max_elem = 1,
};

/* count all bytes */
SEC("count_bytes")
int _accept_all(struct xdp_md *ctx)
{
	__u32 key = 0;
	long *value;

	value = bpf_map_lookup_elem(&rx_bytes, &key);
	if (value) {
		*value += ctx->data_end - ctx->data;
	}

	return XDP_PASS;
}
