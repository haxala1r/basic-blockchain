#ifndef HASH_H
#define HASH_H 1

#include <stdint.h>
#include <stddef.h>

/* We're using sha256 as out hashing algorithm.
 * return value is a 32-byte (256-bit) large buffer that holds the hash.
 */
char *sha256(void *data, uint64_t len);
void print_hash(char *hash);

#endif
