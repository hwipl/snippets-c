/* remove network namespace with name specified in first command line argument.
 * based on ipnetns.c in iproute2 source code:
 * https://github.com/shemminger/iproute2/blob/main/ip/ipnetns.c
 */

/* printf */
#include <stdio.h>

/* unlink */
#include <unistd.h>

/* umount2 */
#include <sys/mount.h>

int main(int argc, char **argv) {
	/* check command line arguments */
	if (argc < 2) {
		return -1;
	}

	/* netns path */
	char netns_path[100];
	snprintf(netns_path, sizeof(netns_path), "%s/%s", "/var/run/netns",
		 argv[1]);

	/* remove netns and netns file */
	umount2(netns_path, MNT_DETACH);
	if (unlink(netns_path) < 0) {
		printf("Error removing namespace file\n");
		return -1;
	}

	return 0;
}
