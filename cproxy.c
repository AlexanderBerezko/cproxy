#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

/* Create server socket */
int create_server_socket(int protocol, char* hostname, int port) {

	// create socket
	int socket_fd;
	printf("Create server socket ... ");
	if (protocol == IPPROTO_UDP) {
		// UDP
		socket_fd = socket(AF_INET, SOCK_DGRAM, 0);

	} else {
		// TCP
		socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	}

	if (socket_fd == -1) {
		perror("Error");
		return -1;
	} else {
		printf("OK\n");
	}

	// find IP of host
	printf("Resolve %s ... ", hostname);
	struct hostent* host = gethostbyname(hostname);
	if (host == (struct hostent*) NULL) {
		herror("Error");
		return -2;
	} else {
		printf("OK\n");
	}

	// bind socket to IP address and port
	printf("Bind to %s:%d ... ", hostname, port);
	struct sockaddr_in addr;
	memset(&addr, 0x00, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	memcpy(&addr.sin_addr, host->h_addr, sizeof(addr.sin_addr));
	addr.sin_port = htons(port);

	if (bind(socket_fd, (struct sockaddr*) &addr, sizeof(addr)) == -1) {
		perror("Error");
		return -3;
	} else {
		printf("OK\n");
	}

	// if protocol is TCP
	if (protocol != IPPROTO_UDP) {
		// listen()
		printf("Listen ... ");
		int backlog = 0;
		if (listen(socket_fd, backlog) < 0) {
			perror("Error");
			return -4;
		} else {
			printf("OK\n");
		}

	}

	return socket_fd;
}


/* Create client socket */
int create_client_socket(int protocol, char* hostname, int port) {

	// create socket
	int socket_fd;
	printf("Create client socket ... ");
	if (protocol == IPPROTO_UDP) {
		// UDP
		socket_fd = socket(AF_INET, SOCK_DGRAM, 0);

	} else {
		// TCP
		socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	}

	if (socket_fd == -1) {
		perror("Error");
		return -1;
	} else {
		printf("OK\n");
	}

	// find IP of host
	printf("Resolve %s ... ", hostname);
	struct hostent* host = gethostbyname(hostname);
	if (host == (struct hostent*) NULL) {
		herror("Error");
		return -2;
	} else {
		printf("OK\n");
	}

	// connect socket with remote host
	printf("Connect to %s:%d ... ", hostname, port);
	struct sockaddr_in addr;
	memset(&addr, 0x00, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	memcpy(&addr.sin_addr, host->h_addr, sizeof(addr.sin_addr));
	addr.sin_port = htons(port);

	if (connect(socket_fd, (struct sockaddr*) &addr, sizeof(addr)) == -1) {
		perror("Error");
		return -3;
	} else {
		printf("OK\n");
	}

	return socket_fd;
}


int main(int argc, char* argv[]) {

	// check input data
	if (argc < 4) {
		printf("usage: cproxy [-u] <host> <port> <host> <port>\n");
		return -1;
	}

	int protocol;
	char* hostname_from;
	int port_from;
	char* hostname_to;
	int port_to;
	if (strcmp(argv[1], "-u") == 0) {
		protocol = IPPROTO_UDP;
		hostname_from = argv[2];
		port_from = atoi(argv[3]);
		hostname_to = argv[4];
		port_to = atoi(argv[5]);

	} else {
		protocol = IPPROTO_TCP;
		hostname_from = argv[1];
		port_from = atoi(argv[2]);
		hostname_to = argv[3];
		port_to = atoi(argv[4]);
	}

	// not buffering stdout
	setbuf(stdout, NULL);

	// create proxy
	int socket_fd_server = create_server_socket(protocol, hostname_from, port_from);
	if (socket_fd_server < 0) {
		return -1;
	}
	printf("\n");

	int socket_fd_client = create_client_socket(protocol, hostname_to, port_to);
	if (socket_fd_client < 0) {
		return -1;
	}

	// fetch remote socket
	int socket_fd_remote;
	if (protocol == IPPROTO_TCP) {
		// TCP - from accept()
		struct sockaddr addr_remote;
		memset(&addr_remote, 0x00, sizeof(struct sockaddr));
		socklen_t addr_remote_len;
		printf("Accept waiting ... ");
		socket_fd_remote = accept(socket_fd_server, (struct sockaddr*) &addr_remote, &addr_remote_len);
		if (socket_fd_remote < 0) {
			perror("Error");
			return -1;
		} else {
			printf("OK\n");
		}

	} else {
		// UDP
		socket_fd_remote = socket_fd_server;
	}

	// exchange data
	int select_res = 0;
	int nfds = socket_fd_remote + 1;

	fd_set readfds;
	FD_ZERO(&readfds);
	FD_SET(socket_fd_remote, &readfds);
	FD_SET(socket_fd_client, &readfds);

	char buf[BUFSIZ];
	int count;
	while ((select_res = select(nfds, &readfds, NULL, NULL, NULL)) != 0) {
		if (select_res < 0) {
			perror("Select error");
			break;
		}

		if (FD_ISSET(socket_fd_remote, &readfds)) {
			count = read(socket_fd_remote, buf, BUFSIZ);
			if (count <= 0) {
				break;
			}

			write(socket_fd_client, buf, count);
			printf(">> ");
			write(1, buf, count);

		} else {
			FD_SET(socket_fd_remote, &readfds);
		}
		
		if (FD_ISSET(socket_fd_client, &readfds)) {
			count = read(socket_fd_client, buf, BUFSIZ);
			if (count <= 0) {
				break;
			}

			write(socket_fd_remote, buf, count);
			printf("<< ");
			write(1, buf, count);

		} else {
			FD_SET(socket_fd_client, &readfds);
		}
	}

	return 0;
}
