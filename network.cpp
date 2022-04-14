/* Standard libraries */
#include <iostream>
#include <cstring>
#include <ctime>
#include <cstdlib>


/* Custom headers */
#include <blockchain.hpp>
#include <network.hpp>
#include <socket.hpp>
#include <vector>

using namespace std;

extern BlockChain *bc;

/* Define Peer a bit more */
Peer::Peer(Socket *s) {
	sock = s;
	last_touch = std::time(nullptr);
	status = 1;
}

Peer::~Peer() {
	delete sock;
}


static Socket *listen_sock;

static vector<Peer*> peer_list;

/* Here are functions that are called upon recieving a certain command */

static int cmd_disconnect(Peer *p, string s) {
	if (s != "DISCONNECT") return -1;
	/* Remove the peer from peer_list */
	unsigned int i = 0;
	for (; i < peer_list.size(); i++) {
		if (peer_list[i] == p) {
			break;
		}
	}
	peer_list.erase(peer_list.begin() + i);
	
	/* Now we can safely delete the peer (which will disconnect) */
	delete p;
	return 0;
}

/* respond to a ping with a pong */
static int cmd_ping(Peer *p, string s) {
	if (s.compare(0, 4, "PING") != 0) return -1;
	p->status = 1;
	p->last_touch = time(nullptr);

	if (p->sock->SendStr("PONG") <= 0) {
		return -1;
	}
	return 0;
}

static int cmd_pong(Peer *p, string s) {
	if (s.compare(0, 4, "PONG") != 0) return -1;
	p->status = 1;
	p->last_touch = time(nullptr);
	return 0;
}

static int cmd_getlen(Peer *p, string s) {
	string ret = "RETLEN ";
	ret += to_string(bc->len);
	p->sock->SendStr(ret);
	if (s.length() >= 8) {
		/* The peer has specified their own length, check if it's higher
		 * than ours. if so, request the peer's chain.
		 */
		string data = ret.substr(7);
	}
	return 0;
}

static int cmd_retlen(Peer *p, string s) {
	if (s.length() < 8) return -1;
	string i = s.substr(7);
	cout << i; 
	if (stoi(i) > bc->len) {
		/* request their chain */ 
		p->sock->SendStr("GETCHAIN");
	}
	return 0;
}

static int send_block(Peer *p, Block *b) {
	p->sock->Send(b->data, 256);
	p->sock->Send(b->nonce, 32);
	p->sock->Send(&b->time, 8);
	return 0;
}

static int recv_block(Peer *p, BlockChain *nbc) {
	if (p == nullptr) return 1;
	Block *b = new Block();
	if (p->sock->Recv(b->data, 256) != 256) {
		delete b;
		return 1;
	}
	if (p->sock->Recv(b->nonce, 32) != 32) {
		delete b;
		return 1;
	}
	if (p->sock->Recv(&b->time, 8) != 8) {
		delete b;
		return 1;
	}
	if (nbc == nullptr) {
		delete b;
		return 0;
	}
	if (nbc->AddBlock(b) != 0) {
		/* Block is invalid, disconnect from peer. */
		delete b;
		return 1;
	}
	return 0;
}	

static int cmd_getchain(Peer *p, string s) {
	if (s != "GETCHAIN") return 1;
	
	string ret = "RETCHAIN";
	ret += to_string(bc->len);
	p->sock->SendStr("RETCHAIN");
	
	Block *i = bc->first_block;
	
	while (i != nullptr) {
		send_block(p, i);
		i = i->next;
	}
	
	return 0;
}

static int cmd_retchain(Peer *p, string s) {
	if (s.length() < 10) return -1;
	
	BlockChain *nbc = new BlockChain();
	nbc->data_list.clear();
		
	int block_count = stoi(s.substr(9));
	if (block_count < bc->len) return 1;
	
	for (int j = 0; j < block_count; j++) {
		if (recv_block(p, nbc)) {
			delete nbc;
			return 1;
		}
	}
	/* If we reach here, the new chain must be completely valid *and*
	 * longer than our current one.
	 */
	delete bc;
	bc = nbc;
	return 0;
}

static int cmd_newblock(Peer *p, string s) {
	/* Error if there's no argument */
	if (s.length() < 10) return -1;
	
	int index = stoi(s.substr(9));
	
	/* If our chain is already longer than theirs, ignore it. */
	if (index <= (bc->len - 1)) {
		recv_block(p, nullptr);
		return 0;
	}
	
	/* If the new block is exactly at the end of our chain, recieve it. */
	if (index == (bc->len)) {
		cout << "GOT NEWBLOCK" << endl;
		return recv_block(p, bc);
	}
	
	/* If the new block is way outside of our chain, request the entire
	 * chain of that */
	if (index > (bc->len)) {
		recv_block(p, nullptr);
		p->sock->SendStr("GETCHAIN");
	}
	return 0;
}

static string commands[] = {
	"DISCONNECT",
	/* We use these for routine checks to see if a peer is still alive */
	"PING", 
	"PONG",
	
	/* GETLEN requests the peer's chain length
	 * RETLEN returns the length of the chain to a peer who sent GETLEN
	 */
	"GETLEN",
	"RETLEN",
	
	/* GETCHAIN requests a peer's entire blockchain
	 * RETCHAIN returns the entire chain to a peer who sent GETCHAIN
	 */
	"GETCHAIN",
	"RETCHAIN",
	
	/* Announce a new block */
	"NEWBLOCK"
};

/* Actions to be called in response to the commands above */
static int (*command_actions[])(Peer *p, string s) = {
	cmd_disconnect,
	cmd_ping,
	cmd_pong,
	cmd_getlen,
	cmd_retlen,
	cmd_getchain,
	cmd_retchain,
	cmd_newblock
};

/* Handle incoming command s from peer p */
static int handle_cmd(Peer *p, string s) {
	/* Find the command, and call the corresponding action */
	for (int i = 0; i < (int)(sizeof(commands)/sizeof(string)); i++) {
		if (s.compare(0, commands[i].length(), commands[i]) == 0) {
			if (i == 0) return 1;
			return command_actions[i](p, s);
		}
	}
	return -1;
}

/* Check if this specific peer has sent any commands to us */
static int handle_peer(Peer *p) {
	p->sock->timeout = 100;
	if (p->sock->CheckRead() == false) return 0;
	
	string s;
	
	/* Recieve a string. (we can't really use RecvStr from portsock because 
	 * it doesn't terminate at a NUL-byte but when no data can be read)
	 */
	char c;
	while (1) {
		if (p->sock->Recv(&c, 1) <= 0) {
			/* Error reading, or no data. Either way, we gotta disconnect. */
			cmd_disconnect(p, "DISCONNECT");
			return 1;
		}
		if (c == '\0') break;
		
		s += c;
	}
	
	/* Got a command. */
	if (handle_cmd(p, s)) {
		cmd_disconnect(p, "DISCONNECT");
		return 1;
	}
	return 0;
}

int handle_network(void) {
	/* Check if there's any incoming connections */
	Socket *ns = listen_sock->Accept();
	while (ns != nullptr) {
		/* Add new peer based on the incoming connection */
		Peer *np = new Peer(ns);
		peer_list.push_back(np);
		ns->SendStr("PING");
		cout << "Got a connection? " << endl;
		/* Keep accepting until there's no more incoming connections */
		ns = listen_sock->Accept();
	}
	
	/* Check if any sockets can be read from. To do this, we just 
	 * set a really small timeout, and attempt to recieve from every
	 * available peer.
	 * TODO: this is inefficient, use a better algo here.
	 */
	for (unsigned int i = 0; i < peer_list.size(); i++) {
		if (handle_peer(peer_list[i])) {
			i -= 1;
		}
	}
	return 0;
} 

int add_peer(string IP, int port) {
	Socket *ns = new Socket();
	
	if (ns->Connect(IP, port)) {
		delete ns;
		return -1;
	}
	Peer *np = new Peer(ns);
	peer_list.push_back(np);
	/* The other peer should in theory send a PING to us first. */
	return 0;
}

int announce_last_block(void) {
	string cmd = "NEWBLOCK ";
	cmd += to_string(bc->len - 1);
	
	for (unsigned int i = 0; i < peer_list.size(); i++) {
		if (peer_list[i]->sock->SendStr(cmd) <= 0)
			return 1;
		if (send_block(peer_list[i], bc->last_block))
			return 1;
	}
	
	return 0;
}

int init_network(string IP, int port) {
	if (port <= 0) return -1;
	if (listen_sock != nullptr) 
		delete listen_sock;
	
	/* Create the listening socket and make it listen. */
	listen_sock = new Socket();
	if (listen_sock->Listen(IP, port)) {
		delete listen_sock;
		listen_sock = nullptr;
		return -1;
	}
	listen_sock->timeout = 100;
	
	return 0;
}

int network_cleanup(void) {
	delete listen_sock;
	listen_sock = nullptr;
	
	for (unsigned int i = 0; i < peer_list.size(); i++) {
		peer_list[i]->sock->SendStr(commands[0]);
		delete peer_list[i];
	}
	peer_list.clear();
	return 0;
}
