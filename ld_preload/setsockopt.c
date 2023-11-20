#define _GNU_SOURCE

#include <stdio.h>
#include <dlfcn.h>
#include <sys/socket.h>

#define IP_UNICAST_IF           50
#define IPV6_UNICAST_IF         76

// // constructor
// __attribute__((constructor))
// static void custom_setsockopt_init() {
// 	// initialize things here
// }

// // destructor
// __attribute__((destructor))
// static void custom_setsockopt_deinit() {
// 	// deinitialize things here
// }

// setsockopt implementation that filters IP_UNICAST_IF and IPV6_UNICAST_IF
int setsockopt(int socket, int level, int option_name,
	       const void *option_value, socklen_t option_len) {
	// debugging output
	printf("calling setsockopt: socket %d, level %d, option_name %d, "
	       "option_len %d\n", socket, level, option_name, option_len);

	// do not forward IP_UNICAST_IF
	if (option_name == IP_UNICAST_IF) {
		printf("filtering sockopt IP_UNICAST_IF\n");
		return 0;
	}
	// do not forward IPV6_UNICAST_IF
	if (option_name == IPV6_UNICAST_IF) {
		printf("filtering sockopt IPV6_UNICAST_IF\n");
		return 0;
	}

	// forward call to original setsockopt
	int (*orig_setsockopt)(int socket, int level, int option_name,
		       const void *option_value, socklen_t option_len);
	orig_setsockopt = dlsym(RTLD_NEXT, "setsockopt");
	return (*orig_setsockopt)(socket, level, option_name, option_value,
				  option_len);
}
