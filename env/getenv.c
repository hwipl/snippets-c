// Reads greeting from environment variable GREETING and prints it.
//
// Building:
// gcc getenv.c -o getenv
//
// Example:
// GREETING="Hi!" ./getenv
// Hi!

#include <stdlib.h>
#include <stdio.h>

int main() {
	char *g = getenv("GREETING");
	if (g != NULL) {
		printf("%s\n", g);
	}
}
