#include <cstdlib>
#include <cstring>
#include <stdint.h>
#include <stddef.h>
#include <iostream>


/* This is a hard-coded table of "round constants"
 * If you don't know what that means, you're not alone. Anyway, the sha256
 * algorithm uses these numbers. There is a way to calculate these at run-time
 * instead of hard-coding these, but I really don't see any benefit to doing
 * so.
 */
static uint32_t k[64] = {
	0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
	0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
	0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
	0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
	0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
	0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
	0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
	0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

/* Rotate right (magic!!) */
static uint32_t ror(uint32_t n, int r) {
	return (n >> r) | (n << (32 - r));
}

/* We're using sha256, because it's secure enough (not that it needs to be for this test app, anyway)
 * and it's so popular it basically took 5 seconds to find a good, detailed description of the algorithm.
 * NOTE: this function probably isn't very memory-efficient, as it makes a basically 1-to-1 copy of the
 * "data". This isn't too much of an issue for this app, since the blocks we're using are pretty short
 * anyway.
 * returned buffer is always 256-bits long (32 bytes) and is the hash of the given data
 */
char *sha256(void *data, uint64_t len) {
	/* The initial "hashes" */
	uint32_t h0 = 0x6a09e667;
	uint32_t h1 = 0xbb67ae85;
	uint32_t h2 = 0x3c6ef372;
	uint32_t h3 = 0xa54ff53a;
	uint32_t h4 = 0x510e527f;
	uint32_t h5 = 0x9b05688c;
	uint32_t h6 = 0x1f83d9ab;
	uint32_t h7 = 0x5be0cd19;

	/* Copy the data into a new, bigger buffer */
	int pad_bytes = 64 - (len + 1 + 8) % 64;
	int new_len = len + 1 + 8 + pad_bytes;

	char *buf = (char*)malloc(new_len);
	memcpy(buf, data, len);

	/* Padding */
	buf[len] = (char)(1 << 7); // 0b10000000
	for (int i = 0; i < pad_bytes; i++) {
		buf[len + 1 +  i] = '\0';
	}

	/* Add the length (in bits) in big endian to the end*/
	int tmp_len = len * 8;
	for (int i = 0; i < 8; i++) {
		buf[new_len - 1 - i] = tmp_len & 0xFF;
		tmp_len >>= 8;
	}

	/* now we can start hashing */
	for (int i = 0; i < new_len; i += 64) {
		char *block = buf + i;

		/* Copy the block as 16 big endian integers to w */
		uint32_t w[64];
		for (int j = 0; j < 16; j++) {
			for (int k = 0; k < 4; k++) {
				w[j] <<= 8;
				w[j] |= (block[j * 4 + k] & 0xFF);
			}
		}

		/* Extend the first 16 ints to the rest of w */
		for (int j = 16; j < 64; j++) {
			uint32_t s0 = ror(w[j-15], 7) ^ ror(w[j-15], 18) ^ (w[j-15] >> 3);
			uint32_t s1 = ror(w[j-2], 17) ^ ror(w[j-2],  19) ^ (w[j-2] >> 10);
			w[j] = w[j-16] + s0 + w[j-7] + s1;
		}

		uint32_t a = h0;
		uint32_t b = h1;
		uint32_t c = h2;
		uint32_t d = h3;
		uint32_t e = h4;
		uint32_t f = h5;
		uint32_t g = h6;
		uint32_t h = h7;

		/* Compress */
		for (int j = 0; j < 64; j++) {
			uint32_t s1, ch, tmp1, s0, maj, tmp2;
			s1 = ror(e, 6) ^ ror(e, 11) ^ ror(e, 25);
			ch = (e & f) ^ ((~e) & g);
			tmp1 = h + s1 + ch + k[j] + w[j];

			s0 = ror(a, 2) ^ ror(a, 13) ^ ror(a, 22);
			maj = (a & b) ^ (a & c) ^ (b & c);
			tmp2 = s0 + maj;

			h = g;
			g = f;
			f = e;
			e = d + tmp1;
			d = c;
			c = b;
			b = a;
			a = tmp1 + tmp2;
		}
		/* Add the compressed chunk to the hash */
		h0 += a;
		h1 += b;
		h2 += c;
		h3 += d;
		h4 += e;
		h5 += f;
		h6 += g;
		h7 += h;
	}
	free(buf);

	uint32_t h[8] = {
		h0, h1, h2, h3, h4, h5, h6, h7
	};
	char *digest = (char*)malloc(32);

	for (int i = 0; i < 8; i++) {
		for (int j = 3; j >= 0; j--) {
			digest[i * 4 + j] = h[i] & 0xFF;
			h[i] >>= 8;
		}
	}
	return digest;
}


/* Some helpers */

/* Get a hexadecimal digit as a character */
static char get_hex_digit(uint8_t num) {
	if (num > 9) {
		return 'A' + num - 10;
	} else {
		return '0' + num;
	}
}

/* Prints a 32-byte hash (the output of sha256)
 * Kinda inefficient but I tried to make it easy to read if nothing else.
 * I mean... it literally processes 32 bytes, if you need this to be 
 * optimised you have FAR more serious problems.
 */
void print_hash(char *hash) {
	for (int i = 0; i < 32; i++) {
		uint32_t val = hash[i] & 0xFF;
		std::cout << get_hex_digit((val / 16) & 0x0F) << get_hex_digit(val & 0x0F);
	}
}
