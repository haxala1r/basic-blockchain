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
	active = 1;
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
	p->active = 1;
	p->last_touch = time(nullptr);

	if (p->sock->SendStr("PONG") <= 0) {
		return -1;
	}
	return 0;
}

static int cmd_pong(Peer *p, string s) {
	if (s.compare(0, 4, "PONG") != 0) return -1;
	p->active = 1;
	p->last_touch = time(nullptr);
	cout << "GOT PONG" << endl;
	return 0;
}

static int cmd_getlen(Peer *p, string s) {
	string ret = "RETLEN ";
	ret += itos(bc->len);
	p->sock->SendStr(ret);
	if (s.length() >= 8) {
		/* The peer has specified their own length, check if it's higher
		 * than ours. if so, request the peer's chain.
		 */
	}
	return 0;
}

static int cmd_retlen(Peer *p, string s) {
	if (s.length() < 8) return -1;
	string i = s.substr(7);
	cout << i; 
	if (stoi(i) > bc->len) {
		/* request their chain */ 
	}
	return 0;
}


static string commands[] = {
	"DISCONNECT",
	/* We use these for routine checks to see if a peer is still alive */
	"PING", 
	"PONG",
	
	/* GETLEN requests the peer's chain length, and RETLEN returns the
	 * length of the chain.
	 */
	"GETLEN",
	"RETLEN"
};

/* Actions to be called in response to the commands above */
static int (*command_actions[])(Peer *p, string s) = {
	cmd_disconnect,
	cmd_ping,
	cmd_pong,
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

int handle_network(void) {
	/* Check if there's any incoming connections */
	Socket *ns = listen_sock->Accept();
	while (ns != nullptr) {
		/* Add new peer based on the incoming connection */
		Peer *np = new Peer(ns);
		peer_list.push_back(np);
		if (ns->SendStr("PING") <= 0) {
			cout << "thfuck2" << endl;
		}
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
		peer_list[i]->sock->timeout = 100;
		/* TODO: RecvStr is kinda inefficient, might wanna use a custom
		 * function based on Recv instead.
		 */
		string s = peer_list[i]->sock->RecvStr();
		if (s.length() == 0) {
			continue;
		}
		/* Got a command. */
		if (handle_cmd(peer_list[i], s)) {
			cmd_disconnect(peer_list[i], "DISCONNECT");
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
