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

static string commands[] = {
	"DISCONNECT",
};

/* Actions to be called in response to the commands above */
static int (*command_actions[])(Peer *p, string s) = {
	cmd_disconnect,
};

/* Handle incoming command s from peer p */
static int handle_cmd(Peer *p, string s) {
	/* Find the command, and call the corresponding action */
	for (int i = 0; i < (int)(sizeof(commands)/sizeof(string)); i++) {
		if (s.compare(0, commands[i].length(), commands[i]) == 0) {
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
		
		/* Keep accepting until there's no more incoming connections */
		ns = listen_sock->Accept();
	}
	
	/* Check if any sockets can be read from. To do this, we just 
	 * set a really small timeout, and attempt to recieve from every
	 * available peer.
	 * TODO: this is inefficient, use a better algo here.
	 */
	for (unsigned int i = 0; i < peer_list.size(); i++) {
		peer_list[i]->sock->timeout = 10;
		/* TODO: RecvStr is kinda inefficient, might wanna use a custom
		 * function based on Recv instead.
		 */
		string s = peer_list[i]->sock->RecvStr();
		if (s.size() == 0) {
			continue;
		}
		/* Got a command. */
		if (handle_cmd(peer_list[i], s)) {
			cmd_disconnect(peer_list[i], s);
		}
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
