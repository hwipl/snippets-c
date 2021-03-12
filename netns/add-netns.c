/* create network namespace with name specified in first command line argument.
 * based on ipnetns.c in iproute2 source code:
 * https://github.com/shemminger/iproute2/blob/main/ip/ipnetns.c
 */

#define _GNU_SOURCE

/* printf */
#include <stdio.h>

/* open */
#include <fcntl.h>

/* close */
#include <unistd.h>

/* unshare */
#include <sched.h>

/* mount */
#include <sys/mount.h>

int main(int argc, char **argv) {
	/* check command line arguments */
	if (argc < 2) {
		return -1;
	}

	/* create netns file */
	char netns_path[100];
	snprintf(netns_path, sizeof(netns_path), "%s/%s", "/var/run/netns",
		 argv[1]);

	int fd = open(netns_path, O_RDONLY | O_CREAT | O_EXCL, 0);
	if (fd < 0) {
		printf("Error creating namespace file\n");
		return -1;
	}
	close(fd);

	/* create netns */
	if (unshare(CLONE_NEWNET) < 0) {
		printf("Error creating namespace\n");
		return -1;
	}

	/* bind netns */
	if (mount("/proc/self/ns/net", netns_path, "none", MS_BIND, NULL) < 0) {
		printf("Error binding /proc/self/ns/net to netns file\n");
		return -1;
	}

	return 0;
}
