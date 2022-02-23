#ifndef NETWORK_H
#define NETWORK_H 1

#include <netinet/in.h>
#include <blockchain.h>

struct Peer {
	/* is the connection active? 1 if yes */
	int active;

	/* holds the socket fd if the connection is active. */
	int sock_fd;

	/* Holds the address used to connect() to the peer. */
	struct sockaddr_in addr;

	/* we keep all peers in a linked list */
	struct Peer *next;
	struct Peer *prev;
};

typedef struct Peer Peer;

int announce_block(BlockChain *bc);
int add_transaction(char *data, int data_len);
int disconnect_peer(Peer *p);
int add_peer(char *IP, int port);
int check_network(void);
int init_network();


#endif
