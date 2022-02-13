#ifndef BLOCKCHAIN_H
#define BLOCKCHAIN_H 1


/* This is the *raw* block data. This is the struct we pass 
 * *directly* into sha256()
 */
struct BlockData {
	char prev_hash[32];
	char nonce[32];  /* the stuff miners mine I guess? */
	int time; /* The time of creation */

	/* Here we store constant-size data. 
	 * Each block holds exactly one 256-byte string.
	 */
	char data[256];
};

typedef struct BlockData BlockData;

/* This is a wrapper struct around block_data.
 * This struct helps us keep track of our blocks using
 * a simple doubly-linked list
 */
struct Block {
	char hash[32];
	BlockData block_data;

	struct Block *next;
	struct Block *prev;
};

typedef struct Block Block;

/* This doesn't do much since we could have these variables be global,
 * but I thought it would still be better to encapsulate them in a struct.
 */
struct BlockChain {
	Block *genesis_block;
	Block *last_block;
	
	int current_length;
};

typedef struct BlockChain BlockChain;

int create_block(BlockChain *bc, char *data, int data_len);
int genesis(BlockChain *bc);
void print_blockchain(BlockChain *bc);

#endif
