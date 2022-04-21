/* Standard libraries */
#include <iostream>
#include <cstring>
#include <ctime>
#include <cstdlib>


/* Custom headers */
#include <blockchain.hpp>
#include <hash.hpp>

using namespace std;

/* Amount of zeroes reqired at the beginning of a hash before it's considered
 * valid.
 */
static int pow_zeroes = 2;


Block::Block() {}

Block::Block(char *d, int len)  {
	this->SetData(d, len);
}

int Block::SetData(char *d, int len) {
	if (len > 256) return -1;
	if (len <= 0) return 0;
	
	memset(data, 0, sizeof(data));
	memcpy(data, d, len);

	this->time = std::time(NULL);
	return 0;
}

int Block::CalculateHash(void) {
	/* Copy everything into a continuous patch of memory before passing
	 * it to sha256()
	 */
	int size = sizeof(hash) + sizeof(data) + sizeof(nonce) + sizeof(time);
	char *tmp = (char*)malloc(size);

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
	first_block = nullptr;
	last_block = nullptr;
	string genesis_data = "THIS IS THE GENESIS BLOCK.";
	
	this->AddData((char*)genesis_data.c_str(), genesis_data.length());
}

BlockChain::BlockChain(char *data, int len) {
	first_block = nullptr;
	last_block = nullptr;
	this->AddData(data, len);
}

BlockChain::~BlockChain() {
	Block *i = first_block;
	
	while (i != nullptr) {
		Block *n = i->next;
		delete i;
		i = n;
	}
	data_list.clear();
}

int BlockChain::AddData(char *data, int len) {
	BlockData bd;
	memset(bd.data, 0, 256);
	memcpy(bd.data, data, len);
	bd.len = len;
	data_list.push_back(bd);
	return 0;
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
	len++;
	return 0;
}

/* The Mine() function will be called by main() in a loop with everything
 * else. Every time Mine() is called, it makes at most this many attempts
 * before giving up.
 */
static int mine_attempts = 100000;

int BlockChain::Mine(void) {
	if (data_list.size() == 0)
		return 0;
	srand(time(NULL));
	Block *b = new Block(data_list[0].data, data_list[0].len);
	char n[32];
	for (int i = 0; i < mine_attempts; i++) {
		/* Generate a random nonce */
		for (int j = 0; j < 32; j++) {
			n[j] = rand() % 0x100;
		}
		memcpy(b->nonce, n, 32);
		
		/* Test the generated nonce */
		if (AddBlock(b) == 0) {
			/* valid */
			data_list.erase(data_list.begin());
			return 1;
		}
	}
	delete b;
	return 0;
}

