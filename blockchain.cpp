/* Standard libraries */
#include <iostream>
#include <cstring>
#include <ctime>
#include <cstdlib>


/* Custom headers */
#include <blockchain.hpp>

using namespace std;

/* Amount of zeroes reqired at the beginning of a hash before it's considered
 * valid.
 */
static int pow_zeroes = 3;


Block::Block() {};

Block::Block(char *d, int len)  {
	this->SetData(d, len);
}

int Block::SetData(char *d, int len) {
	if (len > 256) return -1;
	if (len <= 0) return 0;
	
	memset(data, 0, sizeof(data));
	memcpy(data, d, len);
	this->time = time(nullptr);
	return 0;
}

int Block::CalculateHash(void) {
	/* Copy everything into a continuous patch of memory before passing
	 * it to sha256()
	 */
	int size = sizeof(hash) + sizeof(data) + sizeof(nonce) + sizeof(time);
	char *tmp = malloc(size);

	if (prev == nullptr) {
		memset(tmp, 0, sizeof(hash));
	} else {
		memcpy(tmp, prev->hash, sizeof(hash));
	}
	memcpy(tmp + sizeof(hash), data, sizeof(data));
	memcpy(tmp + sizeof(hash) + sizeof(data), nonce, sizeof(nonce));
	memcpy(tmp + sizeof(hash) + sizeof(data) + sizeof(nonce), &time, sizeof(time));
	
	char *res = sha256(tmp, size);
	memcpy(hash, res, 32);
	free(res);
	free(tmp);
	
	return 0;
}

/* Check if hash is valid. 0 if valid, 1 if invalid */
int Block::CheckHash(void) {
	this->CalculateHash();
	for (int i = 0; i < pow_zeroes; i++) {
		if (hash[i] != '\0') {
			return 1;
		}
	}
	return 0;
}


BlockChain::BlockChain() {
	string genesis_data = "THIS IS THE GENESIS BLOCK.";
	this->AddData(genesis_data.c_str(), genesis_data.length());
}

BlockChain::BlockChain(char *data, int len) {
	this->AddData(data, len);
}

int BlockChain::AddData(char *data, int len) {
	BlockData bd;
	bd.data = data;
	bd.len = len;
	data_list.push_back(bd);
}

int BlockChain::AddBlock(Block *b) {
	b->prev = last_block;
	if (b->CheckHash()) {
		b->prev = nullptr;
		return -1;
	}
	
	if (last_block == nullptr) {
		last_block = b;
		first_block = b;
	} else {
		last_block->next = b;
		last_block = b;
	}
	b->next = nullptr;
	return 0;
}

int mine_attempts = 10000;

int BlockChain::Mine(void) {
	srand(time(NULL));
	Block *b = new Block(data_list[0].data, data_list[0].len);
	char n[32];
	for (int i = 0; i < mine_attempts; i++) {
		/* Generate a random nonce*/
		for (int j = 0; j < 32; j++) {
			n[j] = rand() % 0x100;
		}
		memcpy(b->nonce, n, 32);
		
		if (AddBlock(b) == 0) {
			data_list.erase(data_list.begin());
			return 1;
		}
	}
	return 0;
}

