/* start xdp socket on device specified in first command line argument and queue
 * id specified in the second command line argument and print packets received
 * to the console
 */

/* bpf */
#include <bpf/xsk.h>

/* mmap */
#include <sys/mman.h>

/* poll */
#include <poll.h>

/* recvfrom */
#include <sys/socket.h>

/* atoi */
#include <stdlib.h>

/* number of frames in umem */
#define NUM_FRAMES 4096

/* read batch size */
#define BATCH_SIZE 64

/* print packet with length as hex on the console */
void print_packet(unsigned char *packet, int length) {
	printf("packet: ");
	for (int i = 0; i < length; i++) {
		printf("%02X", packet[i]);
	}
	printf("\n");
}

/* receive packets */
int receive(struct xsk_socket *xsk, struct xsk_ring_cons *rx, struct
	    xsk_ring_prod *fill, void *buffer)
{
	/* get number of available packets on rx ring */
	__u32 rx_index;
	__u32 num_rx;
	num_rx = xsk_ring_cons__peek(rx, BATCH_SIZE, &rx_index);
	if (!num_rx) {
		/* notify kernel if needs wakeup is set */
		if (xsk_ring_prod__needs_wakeup(fill)) {
			recvfrom(xsk_socket__fd(xsk), NULL, 0, MSG_DONTWAIT,
				 NULL, NULL);
		}
		return 0;
	}

	/* reserve number of available packets on fill ring */
	__u32 fill_index;
	__u32 num_fill;
	num_fill = xsk_ring_prod__reserve(fill, num_rx, &fill_index);
	while (num_fill != num_rx) {
		/* notify kernel if needs wakeup is set */
		if (xsk_ring_prod__needs_wakeup(fill)) {
			recvfrom(xsk_socket__fd(xsk), NULL, 0, MSG_DONTWAIT,
				 NULL, NULL);
		}
		num_fill = xsk_ring_prod__reserve(fill, num_rx, &fill_index);
	}

	/* handle packets */
	for (int i = 0; i < num_rx; i++) {
		const struct xdp_desc *rx_desc;
		unsigned char *pkt;

		/* get next packet on rx ring */
		rx_desc = xsk_ring_cons__rx_desc(rx, rx_index);
		rx_index++;

		/* dump packet to console */
		print_packet(xsk_umem__get_data(buffer, rx_desc->addr),
			     rx_desc->len);

		/* put packet back onto fill ring */
		*xsk_ring_prod__fill_addr(fill, fill_index) =
			xsk_umem__extract_addr(rx_desc->addr);
		fill_index++;
	}

	/* submit packets to fill ring and release packets on rx ring */
	xsk_ring_prod__submit(fill, num_rx);
	xsk_ring_cons__release(rx, num_rx);

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

	/* populate fill ring */
	__u32 idx;
	rc = xsk_ring_prod__reserve(&fill, XSK_RING_PROD__DEFAULT_NUM_DESCS,
				     &idx);
	if (rc != XSK_RING_PROD__DEFAULT_NUM_DESCS) {
		printf("error populating fill ring: %d\n", rc);
		return -rc;
	}
	for (int i = 0; i < XSK_RING_PROD__DEFAULT_NUM_DESCS; i++) {
		*xsk_ring_prod__fill_addr(&fill, idx++) =
			i * XSK_UMEM__DEFAULT_FRAME_SIZE;
	}
	xsk_ring_prod__submit(&fill, XSK_RING_PROD__DEFAULT_NUM_DESCS);

	/* create socket */
	struct xsk_socket *xsk;
	struct xsk_ring_cons rx;
	struct xsk_ring_prod *tx = NULL;
	const struct xsk_socket_config *xsk_config = NULL; /* default config */

	rc = xsk_socket__create(&xsk, ifname, queue_id, umem, &rx, tx,
				xsk_config);
	if (rc) {
		printf("error creating socket\n");
		return -rc;
	}

	/* wait for packets */
	printf("waiting for packets\n");
	while (true) {
		struct pollfd fds[] = {{.fd = xsk_socket__fd(xsk),
			.events = POLLIN}};
		nfds_t nfds = 1;
		int timeout = -1;
		rc = poll(fds, nfds, timeout);
		if (rc < 0) {
			continue;
		}
		receive(xsk, &rx, &fill, bufs);
	}

	return 0;
}
