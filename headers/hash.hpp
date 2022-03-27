#ifndef HASH_H
#define HASH_H 1

/* We're using sha256 as out hashing algorithm.
 * return value is a 32-byte (256-bit) large buffer that holds the hash.
 */
char *sha256(void *data, int len);
void print_hash(char *hash);

#endif
