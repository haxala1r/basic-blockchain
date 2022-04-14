/* Standard libraries */
#include <iostream>
#include <string>

/* Custem headers. 
 * NOTE: socket.h is copied over from the portsock repo
 * during build, see makefile. It's not part of this project, but we're
 * depending on the portsock library to provide a cross-platform socket
 * interface.
 */
#include <socket.hpp>
#include <blockchain.hpp>
#include <hash.hpp>
#include <network.hpp>


using namespace std;

BlockChain *bc;

static int handle_cmd(string cmd) {
	if (!cmd.compare(0, 4, "exit")) {
		return 1;
	} else if (!cmd.compare(0, 4, "add ")) {
		if (cmd.length() < 5) return 0; 
		string data = cmd.substr(4);
		bc->AddData((char*)data.c_str(), data.length());
		cout << "Adding data '" << data << "' to the blockchain" << endl; 
	} else if (!cmd.compare(0, 9, "add-peer ")) {
		if (cmd.length() < 10) return 0;
		/* Divide the IP and port */
		string data = cmd.substr(9);
		string port = "";
		for (unsigned int i = 0; i < data.length(); i++) {
			if (data[i] == ' ') {
				/* Split here and hope the rest of the string's an integer */
				port = data.substr(i + 1);
				data = data.substr(0, i);
			}
		}
		
		if (add_peer(data, stoi(port))) {
			cout << "Failed to connect to the host" << endl;
		} else {
			cout << "Successfully added peer" << endl;
		}
	}
	return 0;
}

int main(void) {
	cout << "port to bind: ";
	int port;
	cin >> port;
	
	if (init_network("127.0.0.1", port)) {
		cout << "ERROR: Cannot listen on the given port" << endl;
		return -1;
	}
	
	bc = new BlockChain();
	cout << "Bind successful." << endl;
	/* main loop */
	while (1) {
		/* TODO: find a cross-platform way of doing this. Currently,
		 * this is the ONLY reason this program isn't available on windows
		 * without cygwin.
		 */
		fd_set fds;
		FD_ZERO(&fds);
		FD_SET(0, &fds);
		
		struct timeval tv;
		tv.tv_sec = 0;
		tv.tv_usec = 100;
		if (select(1, &fds, nullptr, nullptr, &tv) == 1) {
			string cmd;
			getline(cin, cmd);
			if (handle_cmd(cmd)) {
				break;
			}
		}
		
		/* Once we're sure there's no input from user, we can go
		 * check if any peers want stuff from us.
		 */
		 if (handle_network()) {
			 break;
		 }
		 
		 /* Spend some time mining. */
		 if (bc->Mine()) {
			 cout << "A new block was successfully mined." << endl;
			 /* Announce the new block to all peers */
			 if (announce_last_block()) {
				 cout << "An error occured while announcing the new block to peers" << endl;
			 }
		 }
	}
	
	/* TODO: maybe save it in a file? */
	delete bc;
	if (network_cleanup()) {
		cout << "ERROR: network_cleanup returned an error" << endl;
		return -1;
	}
	return 0;
}
