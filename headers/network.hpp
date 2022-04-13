#ifndef NETWORK_H
#define NETWORK_H 1

#include <blockchain.hpp>
#include <ctime>
#include <socket.hpp>

using namespace portsock;

class Peer {
public:
	Socket *sock;

	/* Timestamp of the last time we talked to this peer. */
	int last_touch;
	
	/* was the peer active last time we attempted to interact with them? */
	int active;
	
	Peer(Socket *s);
	~Peer();
};

int handle_network(void);
int add_peer(std::string IP, int port);
int init_network(std::string IP, int port);
int network_cleanup(void);

#endif
