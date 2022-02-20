/* Standard libs*/
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/select.h>

/* Custom headers */
#include <blockchain.h>
#include <hash.h>
#include <network.h>


/* Reads a line of at most max characters */
char *read_line(void) {
	char *buf = NULL;
	int cur_len = 0;
	while (1) {
		buf = realloc(buf, cur_len + 1);
		if (fread(buf + cur_len, 1, 1, stdin) < 1) {
			/* Couldn't read for some reason */
			buf[cur_len] = '\0';
			break;
		}

		if (buf[cur_len] == '\n') {
			buf[cur_len] = '\0';
			break;
		}
		cur_len++;
	}
	return buf;
}

int main(int argc, char *argv[]) {
	if (argc < 2) {
		printf("Please specify a port to set up the service\n");
		return -1;
	}
	if (init_network(atoi(argv[1]))) {
		printf("ERROR: init_network() failed.\n");
		return -1;
	}

	srand(time(NULL));
	BlockChain *bc = malloc(sizeof(*bc));
	if (bc == NULL) return -1;

	memset(bc, 0, sizeof(*bc));
	genesis(bc);

	while (1) {
		check_network();

		/* Wait up to 25 milliseconds for the user to enter a command.
		 * If the user hasn't entered a command, keep looping.
		 */
		fd_set fds;
		FD_ZERO(&fds);
		FD_SET(0, &fds);

		struct timeval tv;
		tv.tv_sec = 0;
		tv.tv_usec = 25000;
		if (select(1, &fds, NULL, NULL, &tv) <= 0) {
			continue;
		}

		/* The user has entered a line. */
		char *line = read_line();
		if (!strncmp(line, "add-peer", 8)) {
			char IP[32];
			int port;
			if (sscanf(line, "add-peer %s %d", IP, &port) < 0) {
				printf("Invalid parameters.\n");
				free(line);
				continue;
			}
			if (add_peer(IP, port) == 0) {
				printf("Connected to peer successfully.\n");
			} else {
				printf("Host unavailable.\n");
			}
		} else if (!strncmp(line, "add", 3)) {
			if (create_block(bc, line + 4, strlen(line))) {
				printf("error occured? maybe?\n");
			}
		} else if (!strcmp(line, "exit")) {
			free(line);
			break;
		} else if (!strcmp(line, "print")) {
			print_blockchain(bc);
		}

		free(line);
	}

	return 0;
}
