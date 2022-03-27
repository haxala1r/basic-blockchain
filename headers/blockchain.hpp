#ifndef BLOCKCHAIN_H
#define BLOCKCHAIN_H 1

#include <stdint.h>
#include <vector>

class BlockData {
public:
	char *data;
	int len;
}

class Block {
private:
	/* hash is the result of all of these and the hash of the previous 
	 * block being hashed together.
	 */
	char data[256];
	int64_t time;

public:
	char hash[32];
	char nonce[32];
	
	/* if no data is supplied on initialisation, the block waits*/
	Block() {}
	
	Block(char *d, int len);
	
	/* Sets the time as well . */
	int SetData(char *d, int len);
	
	/* Copies the result into hash */
	int CalculateHash(void);
	
	/* 0 -> valid hash , 1 -> invalid hash*/
	int CheckHash(void);
	
	/* Doubly linked list */
	Block *next;
	Block *prev;
};

class BlockChain {
private:
	Block *first_block;
	Block *last_block;

public:
	vector<BlockData> data_list;
	
	BlockChain();
	BlockChain(char *genesis_data, int len);
	
	/* Drops every block in the chain, and creates a genesis block in the
	 * chain with the data given.
	 */
	int Genesis(char *data, int len);
	
	/* Mines for the next dat on data_list. */
	int Mine(void);
	
	/* Adds the block to the end of the chain after checking if it's 
	 * valid. NOTE: adding data that is identical to data that's already
	 * in data_list will remove that data from the list.
	 */
	int AddBlock(Block *b);
};



#endif
