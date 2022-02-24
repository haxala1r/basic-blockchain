/* Standard libs*/
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <poll.h>

/* Custom headers */
#include <network.h>
#include <blockchain.h>
#include <hash.h>

/* This is the port for the service in general. This is used for literally all
 * peers, no exceptions. I should probably make this configurable soon, but I
 * need to get verything else right first.
 */
#define PORT 51623

/* This is the descriptor of the socket this node will listen for requests from. */
static int serve_sd = -1;
static struct sockaddr_in serve_addr;

/* These are some command strings, they are used to display our intentions to
 * other peers i.e. whether we want to recieve the whole blockchain, or whether
 * we want to send transactions etc.
 */
const char dcon_cmd[] = "DCON"; // Disconnect

/* Add transaction. This command is used to announce transactions to peers,
 * who will then hopefully mine for it.
 */
const char addt_cmd[] = "ADD-TRANSACTION";

/* Used to discover new peers */
const char gpcmd[] = "GET-PEER";

const char ping_cmd[] = "PING"; // sort of a greeting. The expected response is PONG
/* if a peer doesn't say PONG in response to PING, it is assumed to be dead.
 * Processes regularly send PINGs to their peers, and send PONG in response
 * to PING when they get the chance.
 */
const char pong_cmd[] = "PONG";

/* Get Chain. This command is simply a request to recieve the *entire*
 * blockchain from a peer.
 * NOTE: transferring the ENTIRE CHAIN every time
 * we boot up is probably not the safest or the fastest choice. In fact it's really
 * unfeasible when the blockchain gets too long. That's probably fine for this
 * app though.
 *
 * A peer that recieves this command is expected to send the RET-CHAIN command,
 * then immediately send every block in its blockchain, in order, back to back.
 */
const char getc_cmd[] = "GET-CHAIN";
const char retc_cmd[] = "RET-CHAIN";

/* Gets/returns the length of the blockchain the peer is keeping. */
const char getl_cmd[] = "GET-LENGTH";
const char retl_cmd[] = "RET-LENGTH";

/* The argument is the index of the block. If the index isn't at the end,
 * a peer ignores the command. This command slightly helps us mitigate a
 * problem that can occur when two peers find a block before either of them
 * has a routine sync. This allows peers to announce that they have successfully
 * mined a new block.
 */
const char addb_cmd[] = "NEW-BLOCK";


Peer *peer_list;
int peer_count = 0;


/* Send a command to the peer. We can only send an integer argument in string
 * format. -1 in place of arg means there's no argument.
 *	commands cannot exceed 64 bytes. Command data (sent seperately) can.
 */
int send_peer(Peer *p, const char *cmd, int arg) {
	int len = strlen(cmd) + 1 + 10;
	char *buf = malloc(len);
	if (buf == NULL) return -1;
	sprintf(buf, "%s %d", cmd, arg);

	if (strlen(buf) > 63) {
		free(buf);
		return 1;
	}

	if (send(p->sock_fd, buf, strlen(buf) + 1, MSG_NOSIGNAL) < 0)  {
		return -1;
	}
	return 0;
}

/* Recieve a single command from peer. We use a '\0' byte as a seperator.
 */
char *recv_peer(Peer *p) {
	char buf[64];
	memset(buf, 0, 64);

	char c;
	int len = 0;
	while (1) {
		fflush(stdout);
		if (recv(p->sock_fd, &c, 1, 0) < 0) {
			return NULL; /* TIMEOUT */
		}
		buf[len++] = c;
		if (c == '\0') break;
		if (len >= 64) {
			/* If a message is too long, refuse it.
			 * TODO: investigate. Peers (for whatever reason) send infinite
			 * bytes through recv(), which overflows the stack if this isn't here.
			 * investigate the cause of this infinite byte stream.
			 */
			return NULL;
		}
	}
	if (!strcmp(buf, "")) {
		return NULL;
	}
	char *s = malloc(64);
	memcpy(s, buf, 64);
	return s;
}

int unlink_peer(Peer *p) {
	if (p == NULL) return -1;
	if (p->prev == NULL) {
		peer_list = p->next;
	}

	if (p->prev != NULL) {
		p->prev->next = p->next;
	}
	if (p->next != NULL) {
		p->next->prev = p->prev;
	}

	free(p);
	peer_count--;
	return 0;
}

int link_peer(Peer *p) {
	p->prev = NULL;
	p->next = peer_list;
	if (peer_list != NULL) {
		peer_list->prev = p;
	}
	peer_list = p;
	peer_count++;
	return 0;
}

/* Returns -2 if the connection is dead. */
int handle_cmd(Peer *p, char *line) {
	char cmd[64];
	int arg;
	if (sscanf(line, "%s %d", cmd, &arg) < 0) {
		/* Cannot parse command? */
		return -1;
	}

	fflush(stdout);

	/* Determine command, and handle each of them independantly */
	if (!strcmp(cmd, ping_cmd)) {
		/* resPONG to the ping. No, I am not sorry. */
		send_peer(p, pong_cmd, -1);
	} else if (!strcmp(cmd, dcon_cmd)) {
		/* Peer wants to disconnect */
		printf("A peer disconnected.\n");
		return -1;
	} else if (!strcmp(cmd, getl_cmd)) {
		/* Peer tells us the length of their chain, and wants ours. */
		send_peer(p, retl_cmd, get_chain_len());
		int cur_len = get_chain_len();
		if (arg > cur_len) {
			/* Request the peers blockchain if theirs is longer than ours */
			send_peer(p, getc_cmd, -1);
		}
	} else if (!strcmp(cmd, retl_cmd)) {
		/* Peer has returned their length */
		if (arg > get_chain_len()) {
			/* Request the peers blockchain if theirs is longer than ours */
			send_peer(p, getc_cmd, -1);
		}
	} else if (!strcmp(cmd, getc_cmd)) {
		/* Peer wants us to give them the entire chain as we know it.  */
		send_peer(p, retc_cmd, get_chain_len());
		send_blockchain(p->sock_fd);
	} else if (!strcmp(cmd, retc_cmd)) {
		/* Peer has returned their entire chain as data after the command */
		read_blockchain(p->sock_fd, arg);
	} else if (!strcmp(cmd, addt_cmd)) {
		/* The peer wants to announce a new transaction */
		printf("Added new transaction to list.\n");
		char *data = malloc(256);
		memset(data, 0, 256);
		if (read(p->sock_fd, data, 256) < 0) {
			disconnect_peer(p);
			unlink_peer(p);
		} else {
			add_data(data, arg);
		}
		free(data);
	} else if (!strcmp(cmd, addb_cmd)) {
		/* The peer has successfully mined a block, and wants to announce it */
		Block *b = malloc(sizeof(*b));
		if (b == NULL) return -1;
		memset(b, 0, sizeof(*b));

		if (recv(p->sock_fd, &b->block_data, sizeof(BlockData), 0) <= 0) {
			return -1;
		}
		if (arg != (get_chain_len())) {
			free(b);
			return 0; /* Ignore the command.  TODO: this may not be desirable. */
		}
		if (push_block(b)) {
			free(b);
			return -1;
		}
		printf("Recieved a new block from a peer!\n");
	}
	return 0;
}

int add_transaction(char *data, int data_len) {
	/* Pad the data to 256 bytes */
	char *buf = malloc(256);
	memset(buf, 0, 256);
	memcpy(buf, data, data_len);

	Peer *p = peer_list;

	while (p != NULL) {
		send_peer(p, addt_cmd, 256);
		send(p->sock_fd, buf, 256, 0);

		p = p->next;
	}
	add_data(buf, 256);
	free(buf);
	return 0;
}

int announce_block(BlockChain *bc) {
	if (bc == NULL) return -1;
	Block *b = bc->last_block;
	if (b == NULL) return -1;

	int off = bc->current_length - 1;
	Peer *p = peer_list;
	while (p != NULL) {
		if (p->active == 0) {
			p = p->next;
			continue;
		}
		if (send_peer(p, addb_cmd, off)) {
			// handle this maybe?
		}

		if (send(p->sock_fd, &b->block_data, sizeof(BlockData), MSG_NOSIGNAL) <= 0) {
			// also.
		}

		p = p->next;
	}
	printf("Announced a new block to known peers\n");

	return 0;
}

/* Synchronize the chain with other peers
 * This function simply checks if any of the peers we're connected to have a
 * longer chain than ours. We always accept a longer chain as more "secure" and
 * "legitimate", thus we replace our entire current chain with that of the peer's.
 */
int sync_peers(void) {
	/* Loop over all peers, send them get_length cmds */
	Peer *p = peer_list;
	while (p != NULL) {
		/* Send the peer our own chain length, and ask for theirs. */
		if (send_peer(p, getl_cmd, get_chain_len())) {
			disconnect_peer(p);
			if (p->next == NULL) {
				unlink_peer(p);
				break;
			} else {
				p = p->next;
				unlink_peer(p->prev);
			}
		}
		/* We won't handle the peers' responses here, we'll instead do the actual
		 * chain-syncing in the handle_cmd() routine.
		 */

		p = p->next;
	}
	return 0;
}

static int cur_tick = 0;

/* Checks if any peers have any requests from us, and if they do, handle them.
 * Returns -1 on a FATAL error. 0 on success.
 */
int check_network(void) {
	/* We don't spend much time waiting for anything.
	   The majority of our time is expected to go towards mining after all.
	 */
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 1000;

	int max_fd = -1;
	fd_set rfds;
	FD_ZERO(&rfds);

	Peer *p = peer_list;
	for (int i = 0; i < peer_count; i++) {
		if (p->active) {
			FD_SET(p->sock_fd, &rfds);
			if (p->sock_fd > max_fd) max_fd = p->sock_fd;
		}

		p = p->next;
	}

	/* Check if any of our peers want to talk */
	int ret = 0;
	if ((ret = select(max_fd + 1, &rfds, NULL, NULL, &tv)) > 0) {
		/* There are peers that want to talk */
		fflush(stdout);
		Peer *p = peer_list;
		for (int i = 0; i < peer_count; i++) {
			if (FD_ISSET(p->sock_fd, &rfds)) {
				char *cmd = recv_peer(p);
				if (cmd == NULL) {
					printf("A peer closed connection with no explanation.\n");
					fflush(stdout);
					disconnect_peer(p);
					if (p->next == NULL) {
						unlink_peer(p);
						break;
					} else {
						p = p->next;
						unlink_peer(p->prev);
						continue;
					}
				}

				if (handle_cmd(p, cmd)) {
					free(cmd);
					fflush(stdout);
					if (p->next == NULL) {
						unlink_peer(p);
						break;
					} else {
						p = p->next;
						unlink_peer(p->prev);
						continue;
					}
				}
				free(cmd);
			}
			p = p->next;
		}
	}

	FD_ZERO(&rfds);
	FD_SET(serve_sd, &rfds);

	/* Check if there are any incoming connections to our listening socket.
	 * If there are, accept them, and add them to the list of peers.
	 */
	if (select(serve_sd + 1, &rfds, NULL, NULL, &tv) > 0) {
		int new_fd = 0;
		struct sockaddr_in naddr;
		int size = sizeof(naddr);
		if ((new_fd = accept(serve_sd, (struct sockaddr *)&naddr, (socklen_t *)&size)) > 0) {
			if (size != sizeof(naddr)) return -1;

			struct Peer *np = malloc(sizeof(*np));
			if (np == NULL) return -1;
			memset(np, 0, sizeof(*np));

			memcpy(&np->addr, &naddr, size);
			np->sock_fd = new_fd;
			np->active = 1;
			link_peer(np);
			printf("Incoming connection from a peer was accepted.\n");
		}

	}
	FD_ZERO(&rfds);

	if ((cur_tick++ % 2000) == 0) {
		printf("Performing routine sync.\n");
		sync_peers();
	}

	/* Done. */
	return 0;
}

static int setup_serve_socket(int port) {
	if (serve_sd != -1) {
		close(serve_sd);
	}

	/* Create socket */
	serve_sd = socket(AF_INET, SOCK_STREAM, 0);
	if (serve_sd == 0) {
		printf("ERROR: socket() call failed.\n");
		return -1;
	}

	serve_addr.sin_family = AF_INET;
	serve_addr.sin_addr.s_addr = INADDR_ANY;
	serve_addr.sin_port = htons(port);

	/* Bind and set to listen */
	if (bind(serve_sd, (struct sockaddr *)&serve_addr, sizeof(serve_addr)) < 0) {
		printf("ERROR: bind() call failed.\n");
		close(serve_sd);
		return -1;
	}

	if (listen(serve_sd, 10) < 0) {
		printf("ERROR: listen() call failed\n");
		close(serve_sd);
		return -1;
	}

	/* The loop in main() will periodically call check_network(), which is where
	 * we'll actually accept any requests.
	 */
	return 0;
}


int disconnect_peer(Peer *p) {
	if (p == NULL) return -1;
	if (p->active == 0) return 0; /* Already not connected. */
	/* Inform the peer that we're done talking */
	send_peer(p, dcon_cmd, -1);
	/* We don't wait for a response when disconnecting
	 * This is done because we want to be able to disconnect even if the peer
	 * doesn't respond to our messages.
	 */

	/* Close the socket */
	close(p->sock_fd);
	p->active = 0;
	p->sock_fd = 0;
	return 0;
}

int ping_peer(Peer *p) {
	if (p == NULL) return -1;

	if (send_peer(p, ping_cmd, -1)) {
		return -1;
	}

	char *res;
	if ((res = recv_peer(p)) == NULL) {
		return -1;
	}

	/* Check if response is PONG */
	while (strncmp(res, pong_cmd, strlen(pong_cmd))) {
		handle_cmd(p, res);
		if ((res = recv_peer(p)) == NULL) {
			return -1;
		}
	}
	return 0;
}

/* Attempts to open a data connection to the peer.
 * Returns -1 if the connection fails. 0 on success
 */
int connect_peer(Peer *p) {
	if (p == NULL) return -1;

	if (p->active) {
		disconnect_peer(p);
	}

	p->sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (p->sock_fd == 0) return -1;

	/* Set the timeout values of the socket to 500 ms */
	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 500000;
	if (setsockopt(p->sock_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(struct timeval)) < 0) {
		close(p->sock_fd);
		return -1;
	}
	if (setsockopt(p->sock_fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(struct timeval)) < 0) {
		close(p->sock_fd);
		return -1;
	}

	/* Attempt to connect. This will timeout after 500 ms */
	if (connect(p->sock_fd, (struct sockaddr *)&p->addr, sizeof(struct sockaddr_in)) < 0) {
		/* This isn't fatal, peers can go offline pretty frequently.
		 * Just remove this one from the linked list and maybe attempt to discover more
		 * peers
		 */
		close(p->sock_fd);
		return -1;
	}

	/* connect() succeeded. Try to PING the peer */
	if (ping_peer(p)) {
		/* Peer isn't valid if they don't respond to ping */
		close(p->sock_fd);
		return -1;
	}
	/* We have successfully connected to the peer in question. */
	p->active = 1;
	return 0;
}

/* This tries to connect to the given IP address and port. If successful, it
 * will return the Socket descriptor for the connection. -1 on error.
 */
int add_peer(char *IP, int port) {
	/* Attempt to establish communications with a peer */
	struct Peer *np = malloc(sizeof(*np));
	if (np == NULL) return -1;

	memset(np, 0, sizeof(*np));
	np->addr.sin_family = AF_INET;
	np->addr.sin_port = htons(port);

	if (inet_pton(AF_INET, IP, &np->addr.sin_addr) <= 0) {
		free(np);
		return -1;
	}

	if (connect_peer(np)) {
		free(np);
		return -1;
	}
	link_peer(np);
	return 0;
}

/* Returns 1 if a peer was found. If not, this enters a loop to wait until a
 * peer connects. The port parameter is necessary, since multiple peers need to
 * be able to work in the same host.
 */
int init_network(int port) {
	/* Next, create a server socket so that we can accept connections from
	 * other peers and... y'know, become a part of a p2p network?
	 */
	if (setup_serve_socket(port)) {
		return -1;
	}

	return 0;
}
