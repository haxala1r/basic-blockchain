#ifndef HASH_H
#define HASH_H

/* We're using sha256 as out hashing algorithm. */
char *sha256(void *data, int len);

void print_hash(char *hash);

#endif
