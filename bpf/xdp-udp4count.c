/* bpf */
#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>

/* ethernet */
#include <linux/if_ether.h>

/* ipv4 */
#include <linux/ip.h>

/* htons */
#include <arpa/inet.h>

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

/* map for packet count */
struct bpf_elf_map SEC("maps") rx_count = {
	.type = BPF_MAP_TYPE_ARRAY,
	.size_key = sizeof(__u32),
	.size_value = sizeof(long),
	.max_elem = 1,
};

/* count all packets */
SEC("count_pkts")
int _accept_all(struct xdp_md *ctx)
{
	void *data_end = (void *)(long)ctx->data_end;
	void *data = (void *)(long)ctx->data;
	struct ethhdr *eth = data;
	struct iphdr *ipv4;
	__u32 key = 0;
	__u64 nh_off;
	long *value;

	/* check packet length for verifier */
	if (data + sizeof(struct ethhdr) > data_end) {
		return XDP_PASS;
	}

	/* check ipv4 */
	if (eth->h_proto != htons(ETH_P_IP)) {
		return XDP_PASS;
	}
	ipv4 = data + sizeof(struct ethhdr);

	/* check packet length again for verifier */
	if (data + sizeof(struct ethhdr) + sizeof(struct iphdr) > data_end) {
		return XDP_PASS;
	}

	/* check udp */
	if (ipv4->protocol != IPPROTO_UDP) {
		return XDP_PASS;
	}

	/* increase counter */
	value = bpf_map_lookup_elem(&rx_count, &key);
	if (value) {
		*value += 1;
	}

	return XDP_PASS;
}
