/* start xdp socket on device specified in first command line argument and queue
 * id specified in the second command line argument and send a batch of
 * BATCH_SIZE dummy ethernet packets
 */

/* bpf */
#include <bpf/xsk.h>

/* mmap */
#include <sys/mman.h>

/* poll */
#include <poll.h>

/* sendto */
#include <sys/socket.h>

/* atoi */
#include <stdlib.h>

/* ethernet */
#include <net/ethernet.h>

/* number of frames in umem */
#define NUM_FRAMES 4096

/* send batch size */
#define BATCH_SIZE 1

/* packet size (min size of 64 bytes minus 4 bytes of FCS) */
#define PACKET_SIZE 60

/* complete sending packets */
int complete_send(struct xsk_socket *xsk, struct xsk_ring_prod *tx,
		  struct xsk_ring_cons *comp) {
	/* notify kernel if needs wakeup is set */
	if (xsk_ring_prod__needs_wakeup(tx)) {
		sendto(xsk_socket__fd(xsk), NULL, 0, MSG_DONTWAIT, NULL, 0);
	}

	/* release packets on completion ring */
	__u32 comp_index;
	__u32 num_comp;
	num_comp = xsk_ring_cons__peek(comp, BATCH_SIZE, &comp_index);
	if (num_comp > 0) {
		xsk_ring_cons__release(comp, num_comp);
	}

	return 0;
}

/* send packets */
int send_packets(struct xsk_socket *xsk, struct xsk_ring_prod *tx,
	 struct xsk_ring_cons *comp)
{
	/* reserve number of packets on tx ring */
	__u32 tx_index;
	__u32 num_tx;
	num_tx = xsk_ring_prod__reserve(tx, BATCH_SIZE, &tx_index);
	while (num_tx != BATCH_SIZE) {
		complete_send(xsk, tx, comp);
		num_tx = xsk_ring_prod__reserve(tx, BATCH_SIZE, &tx_index);
	}

	/* put packets on tx ring */
	for (unsigned int i = 0; i < num_tx; i++) {
		struct xdp_desc *tx_desc;
		unsigned char *pkt;

		/* get next packet on tx ring */
		tx_desc = xsk_ring_prod__tx_desc(tx, tx_index);
		tx_index++;

		/* fill packet */
		tx_desc->addr = i << XSK_UMEM__DEFAULT_FRAME_SHIFT;
		tx_desc->len = PACKET_SIZE;
	}

	/* submit packets to tx ring and complete sending */
	xsk_ring_prod__submit(tx, BATCH_SIZE);
	complete_send(xsk, tx, comp);

	return 0;
}

int main(int argc, char **argv) {
	/* check command line arguments */
	if (argc < 3) {
		printf("Usage: %s <device> <queue_id>\n", argv[0]);
		return -1;
	}
	const char *ifname = argv[1];
	__u32 queue_id = atoi(argv[2]);

	/* create buffers for umem */
	int bufs_size = NUM_FRAMES * XSK_UMEM__DEFAULT_FRAME_SIZE;
	void *bufs = mmap(NULL, bufs_size, PROT_READ | PROT_WRITE,
			  MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (bufs == MAP_FAILED) {
		printf("error mmapping memory\n");
		return -1;
	}

	/* create umem */
	struct xsk_umem *umem;
	void *umem_area = bufs;
	__u64 size = bufs_size;
	struct xsk_ring_prod fill;
	struct xsk_ring_cons comp;
	const struct xsk_umem_config *umem_config = NULL; /* default config */
	int rc = xsk_umem__create(&umem, umem_area, size, &fill, &comp,
				  umem_config);
	if (rc) {
		printf("error creating umem\n");
		return -rc;
	}

	/* create dummy ethernet frames to be sent in umem */
	unsigned char pkt_data[PACKET_SIZE] = {};
	struct ethhdr *eth = (struct ethhdr *) pkt_data;
	memcpy(eth->h_dest, "\xab\xcd\xef\xab\xcd\xef", ETH_ALEN);
	memcpy(eth->h_source, "\x01\x23\x45\x67\x89\x01", ETH_ALEN);
	for (int i = 0; i < BATCH_SIZE; i++) {
		memcpy(xsk_umem__get_data(umem_area,
					  i * XSK_UMEM__DEFAULT_FRAME_SIZE),
		       pkt_data, PACKET_SIZE);
	}

	/* create socket */
	struct xsk_socket *xsk;
	struct xsk_ring_cons *rx = NULL;
	struct xsk_ring_prod tx;
	const struct xsk_socket_config *xsk_config = NULL; /* default config */

	rc = xsk_socket__create(&xsk, ifname, queue_id, umem, rx, &tx,
				xsk_config);
	if (rc) {
		printf("error creating socket\n");
		return -rc;
	}

	/* send batch of packets */
	printf("sending packets\n");
	struct pollfd fds[] = {{.fd = xsk_socket__fd(xsk), .events = POLLOUT}};
	nfds_t nfds = 1;
	int timeout = -1;
	if (poll(fds, nfds, timeout) <= 0) {
		printf("error sending packets\n");
		return -1;
	}
	send_packets(xsk, &tx, &comp);

	return 0;
}
