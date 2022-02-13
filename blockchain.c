#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>

#include <blockchain.h>
#include <hash.h>

static char *genesis_data = "THIS IS THE GENESIS BLOCK.";

void calculate_block_hash(Block *b);

/* Generates a random 32-byte string */
void generate_nonce(Block *b) {
	if (b == NULL) return;
	
	uint32_t buf[8];
	/* Since this is an example application, the nonce is simply generated
	 * randomly with a very light restriction: the last 2 bytes (16 bits) of the resulting
	 * hash *must* be zeroes. This is basically the proof-of-work used in bitcoin,
	 * it's just a bit simpler.
	 */
	while (1) {
		/* Randomly generate a 32-byte buffer */
		for (int i = 0; i < 8; i++) {	
			buf[i] = rand();
		}
		memcpy(b->block_data.nonce, buf, 32);
		calculate_block_hash(b);
		
		int valid = 1;
		for (int i = 31; i > (31 - 2); i--) {
			if (b->hash[i] != '\0') {
				valid = 0;
				break;
			}
		}
		if (valid) break;
	}
	return;
}

void calculate_block_hash(Block *b) {
	char *h = sha256(&b->block_data, sizeof(BlockData));
	memcpy(b->hash, h, 32);
	free(h);
}

int create_block(BlockChain *bc, char *data, int data_len) {
	if (data_len > 256) return -1;

	Block *nb = malloc(sizeof(*nb));
	if (nb == NULL) return -1;

	memset(nb, 0, sizeof(*nb));
	
	/* link the block */
	nb->next = NULL;
	nb->prev = bc->last_block;
	if (bc->last_block == NULL) {
		bc->genesis_block = nb;
	} else {
		bc->last_block->next = nb;
	}
	bc->last_block = nb;
	
	/* Copy data, hash and time */
	memcpy(nb->block_data.data, data, data_len);
	if (nb->prev != NULL) {
		memcpy(nb->block_data.prev_hash, nb->prev->hash, 32);
	}
	nb->block_data.time = time(NULL);
	generate_nonce(nb);

	calculate_block_hash(nb);
	bc->current_length++;
	return 0;
}

int genesis(BlockChain *bc) {
	if (bc == NULL) return -1;

	return create_block(bc, genesis_data, strlen(genesis_data));
}

void print_blockchain(BlockChain *bc) {
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
