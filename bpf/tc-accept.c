/* bpf */
#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>

/* tc */
#include <linux/pkt_cls.h>

/* set license to gpl */
char _license[] SEC("license") = "GPL";

/* accept all packets */
SEC("accept_all")
int _accept_all(struct __sk_buff *skb)
{
	return TC_ACT_OK;
}
