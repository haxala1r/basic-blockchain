#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <unistd.h>

#include <blockchain.h>
#include <hash.h>

static char *genesis_data = "THIS IS THE GENESIS BLOCK.";

/* This is the proof-of-work zero count. I.e. the amount of bytes at the end
 * of a block's hash that are *required* to be zero.
 */
static int req_zero_count = 4;

static BlockChain *bc = NULL;

void calculate_block_hash(Block *b) {
	char *h = sha256(&b->block_data, sizeof(BlockData));
	memcpy(b->hash, h, 32);
	free(h);
}

/* Checks if a block's hash conforms to pow_zero_count
 * The hash of the block must be calculated before this.
 * returns 1 if the block is valid, 0 otherwise, and -1 in case of errors
 */
int check_block_hash(Block *b) {
	if (b == NULL) return -1;

	if (b->prev != NULL) {
		if (memcmp(b->prev->hash, b->block_data.prev_hash, 32)) {
			return 0;
		}
	}

	int valid = 1;
	for (int i = 31; i > (31 - req_zero_count); i--) {
		if (b->hash[i] != '\0') {
			valid = 0;
			break;
		}
	}
	return valid;
}

/* Generates a nonce that, when put into block and hashed, will produce a valid
 * block.
 */
void generate_nonce(Block *b) {
	if (b == NULL) return;

	uint8_t buf[32];
	/* Since
	 */
	while (1) {
		/* Randomly generate a 32-byte buffer */
		for (int i = 0; i < 32; i++) {
			buf[i] = rand() & 0xFF;
		}
		memcpy(b->block_data.nonce, buf, 32);
		calculate_block_hash(b);

		if (check_block_hash(b)) break;
	}
	return;
}

int push_block(Block *b) {
	if ((bc == NULL) || (b == NULL)) return -1;

	/* Check for proof-of-work */
	calculate_block_hash(b);
	if (!check_block_hash(b)) {
		/* Return Error if the block isn't valid. */
		return -1;
	}

	/* The block seems valid, add the block to the end of the linked list  */
	b->next = NULL;
	b->prev = bc->last_block;
	if (bc->last_block == NULL) {
		bc->genesis_block = b;
	} else {
		bc->last_block->next = b;
	}
	bc->last_block = b;

	return 0;
}

/* Mines for a block that was created locally  */
int create_block(char *data, int data_len) {
	if (data_len > 256) return -1;
	if (bc == NULL) {
		genesis();
	}

	Block *nb = malloc(sizeof(*nb));
	if (nb == NULL) return -1;

	/* NOTE: It is kind of important that we zero out the ENTIRE structure.
	 * This ensures that when we copy the data over, there's no garbage left
	 * in the data field of BlockData.
	 * If we didn't do this, two different machines
	 * executing the same program could reach different hash, due to the memory
	 * not being consistent.
	 */
	memset(nb, 0, sizeof(*nb));

	/* Copy data, hash and time */
	memcpy(nb->block_data.data, data, data_len);
	if (bc->last_block == NULL) {
		memset(nb->block_data.prev_hash, 0, 32);
	} else {
		memcpy(nb->block_data.prev_hash, bc->last_block->hash, 32);
	}
	nb->block_data.time = time(NULL);

	/* Call generate_nonce, which "mines" for a good nonce */
	generate_nonce(nb);

	if (push_block(nb)) {
		return -1;
	}
	return 0;
}

/* Creates an entirely new blockchain with a new genesis block */
int genesis(void) {
	bc = malloc(sizeof(*bc));
	if (bc == NULL) return -1;

	memset(bc, 0, sizeof(*bc));
	return create_block(genesis_data, strlen(genesis_data));
}

int get_chain_len(void) {
	if (bc == NULL) return 0;
	return bc->current_length;
}

/* Sends the entire blockchain over a file descriptor. NOTE: probably needs to
 * be checked, since endianness can vary over machines.
 */
int send_blockchain(int fd) {
	Block *b = bc->genesis_block;

	while (b != NULL) {
		if (write(fd, &b->block_data, sizeof(BlockData)) < 0) {
			return -1;
		}

		b = b->next;
	}
	return 0;
}

/* Reads the entire blockchain over a file descriptor. NOTE length to read
 * must be supplied.
 */
int read_blockchain(int fd, int len) {
	if (bc != NULL) {
		Block *b = bc->genesis_block;
		while (b != NULL) {
			free(b);
			b = b->next;
		}
	} else {
		bc = malloc(sizeof(*bc));
	}
	memset(bc, 0, sizeof(*bc));
	for (int i = 0; i < len; i++) {
		Block *b = malloc(sizeof(*b));
		memset(b, 0, sizeof(*b));
		if (read(fd, &b->block_data, sizeof(b->block_data)) < 0) {
			free(b);
			return -1;
		}

		if (push_block(b)) {
			free(b);
			return -1;
		}
	}
	return 0;
}

void print_blockchain(void) {
	if (bc == NULL) return;

	Block *i = bc->genesis_block;
	printf("Printing the current blockchain\n");
	while (i != NULL) {
		printf("\t|\n\tv\n-------------\n| prev_hash: ");
		print_hash(i->block_data.prev_hash);
		printf("\n| hash: ");
		print_hash(i->hash);
		printf("\n| nonce: ");
		print_hash(i->block_data.nonce);
		printf("\n| time: %d\n| data: '%s'\n-------------\n", i->block_data.time, i->block_data.data);
		i = i->next;
	}
}
