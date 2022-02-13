/* Standard libs*/
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

/* Custom headers */
#include <blockchain.h>
#include <hash.h>

/* Reads a line of at most max characters */
char *read_line(int max) {
	if (max <= 0) return NULL;
	char *buf = malloc(max);
	int cur_len = 0;
	while (1) {
		if (fread(buf + cur_len, 1, 1, stdin) < 1) {
			/* Couldn't read for some reason */
			if (cur_len < max) {
				buf[cur_len] = '\0';
			}
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

int main(void) {
	srand(time(NULL));
	BlockChain *bc = malloc(sizeof(*bc));
	if (bc == NULL) return -1;

	memset(bc, 0, sizeof(*bc));
	genesis(bc);

	while (1) {
		char *line = read_line(256);
		if (!strcmp(line, "exit")) {
			free(line);
			break;
		}
		if (!strcmp(line, "print")) {
			free(line);
			print_blockchain(bc);
			continue;
		}
		
		if (create_block(bc, line, strlen(line))) {
			printf("err?\n");
		}
		free(line);
	}

	// TODO: print all blocks ? 
	
	return 0;
}
