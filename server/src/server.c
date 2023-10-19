#include <errno.h>
#include <unistd.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <netdb.h>
#include <stdbool.h>
#include <pthread.h>
#include <arpa/inet.h>

#include "utils.h"
#include "logger.h"
#include "common.h"

#define PORT "3693"
#define BACKLOG 10
#define THREADS 4

#define LEN(x) (sizeof(x)/sizeof(x[0]))

// static int ptr = 0;
// static int connections[THREADS];

typedef struct {
	int				value;
	int				connections[THREADS];
	pthread_mutex_t lock;
} connections_t;

void connections_t_init(connections_t* conn) {
	conn->value = 0;
}

void connection_t_decrement(connections_t* conn) {
	pthread_mutex_lock(&conn->lock);
	conn->value -= 1;
	pthread_mutex_unlock(&conn->lock);
}

void connections_t_add_client(connections_t* conn, int client) {
	pthread_mutex_lock(&conn->lock);
	conn->connections[conn->value] = client;
	conn->value += 1;
	pthread_mutex_unlock(&conn->lock);
}

int connections_t_get(connections_t* conn) {
	pthread_mutex_lock(&conn->lock);
	int rc = conn->value; 
	pthread_mutex_unlock(&conn->lock);
	return rc;
}

int connections_t_get_client(connections_t* conn) {
	pthread_mutex_lock(&conn->lock);
	int client = conn->connections[conn->value];
	pthread_mutex_unlock(&conn->lock);
	return client;
}

typedef struct {
	bool					is_running;
	int						sock;
	connections_t			connections;
	struct sockaddr_storage client_addr;
	struct addrinfo			hints;
	struct addrinfo*		info;
} server;

server server_init();
void server_close(server* s);
void* server_handle_connection(void* args);

void broadcast_to_receivers(server* s, int sender, int receivers[], int total_clients, client_message cm);
void broadcast_server_updates(server* s, int sender, int receivers[], int total_clients);

int handle_error(int status, char* message);

int main(void) {
	server server = server_init();

	char ip[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &(server.hints.ai_addr), ip, INET_ADDRSTRLEN);

	log_info("starting server at: %s:%s\n", ip, PORT);

	pthread_t ptids[THREADS];
	
	log_info("waiting for connections\n");

	for (int i = 0; i < THREADS; i++) {
		pthread_create(&ptids[i], NULL, &server_handle_connection, &server);
	}
	
	for (int i = 0; i < THREADS; i++) {
		pthread_join(ptids[i], NULL);
	}

	server_close(&server);

	return EXIT_SUCCESS;
}

void broadcast_to_receivers(server* s, int sender, int receivers[], int total_clients, client_message cm) {
	for (int i = 0; i < total_clients; i++) {
		if (receivers[i] != sender) {
			log_info("server send to sockfd: %i\n", receivers[i]);
			log_info("server send: %s to sockfd\n", cm.message);
			send(s->connections.connections[i], &cm, sizeof(cm), 0);
		}
	}
}

void broadcast_server_updates(server* s, int sender, int receivers[], int total_clients) {
	client_message client_msg;
	recv(sender, &client_msg, sizeof(client_msg), 0);

	char* buffer;

	client_message server_to_client;
	strncpy(server_to_client.username, client_msg.username, strlen(client_msg.username));

	buffer = "joined the chatroom";
	server_to_client.username[strcspn(server_to_client.username, "\n")] = '\0';
	log_info("@%s joined the chatroom\n", server_to_client.username);

	strncpy(server_to_client.message, buffer, strlen(buffer));
	
	for (int i = 0; i < total_clients; i++) {
		if (receivers[i] != sender) {
			send(s->connections.connections[i], &server_to_client, sizeof(server_to_client), 0);
		}
	}
}

void* server_handle_connection(void* args) {
	log_debug("server worker thread initialized\n");
	server* s = (server*)args;

	while (true) {

		socklen_t addr_size = sizeof(s->client_addr);
		log_info("server accepting incoming connections\n");
		int client_connection = handle_error(
				accept(s->sock, (struct sockaddr*)&s->client_addr, &addr_size),
				"Error: could not accept incoming connection");

		struct sockaddr_storage addr;

		socklen_t len = sizeof(addr);
		getpeername(client_connection, (struct sockaddr*)&addr, &len);

		struct sockaddr_in *ss = (struct sockaddr_in*)&addr;
		int port = ntohs(ss->sin_port);
		char ip[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &ss->sin_addr, ip, INET_ADDRSTRLEN);
		log_info("server accepted client from %s:%d\n", ip, port);
		
		// connections[connections_t_get(&s->connections)] = client_connection;
		
		connections_t_add_client(&s->connections, client_connection);

		log_info("server added client to connections array: %i\n", connections_t_get(&s->connections));

		broadcast_server_updates(s, client_connection, s->connections.connections, LEN(s->connections.connections));
		
		while (true) {
			client_message client_msg;
			recv(client_connection, &client_msg, sizeof(client_msg), 0);
			client_msg.message[strcspn(client_msg.message, "\n")] = '\0';

			if (strcmp(client_msg.message, "exit") == 0) {
				client_message server_to_client;
				strncpy(server_to_client.username, client_msg.username, strlen(client_msg.username));

				char* buffer = "left the chatroom";
				server_to_client.username[strcspn(server_to_client.username, "\n")] = '\0';
				log_info("@%s left the chatroom\n", server_to_client.username);

				strncpy(server_to_client.message, buffer, strlen(buffer));
				
				for (int i = 0; i < LEN(s->connections.connections); i++) {
					if (s->connections.connections[i] != client_connection) {
						send(s->connections.connections[i], &server_to_client, sizeof(server_to_client), 0);
					}
				}

				close(client_connection);
				connection_t_decrement(&s->connections);
				break;
			}

			if (connections_t_get(&s->connections) > 1) {
				broadcast_to_receivers(s, client_connection, s->connections.connections, LEN(s->connections.connections), client_msg);
			}
		}

	}

	return NULL;
}

int handle_error(int status, char* message) {
	if (status == -1) {
		log_error("s\n", message);
		log_error("s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}
	return status;
}

void server_close(server* s) {
	// TODO
}

server server_init() {
	server s;

	s.is_running = true;
	memset(&s.hints, 0, sizeof(s.hints));
	s.hints.ai_flags    = AI_PASSIVE;
	s.hints.ai_family   = AF_UNSPEC;
	s.hints.ai_socktype = SOCK_STREAM;
	connections_t_init(&s.connections);

	int status;
	if ((status = getaddrinfo(NULL, PORT, &s.hints, &s.info)) != 0) {
		fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
		exit(EXIT_FAILURE);
	}

	for (struct addrinfo* node = s.info; node != NULL; node = s.hints.ai_next) {
		int listener = handle_error(
				socket(node->ai_family, node->ai_socktype, node->ai_protocol), 
				"Error: cannot create socket");
		s.sock = listener;

		const int opt = 1;
		handle_error(setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)), 
				"Error: unable to setsockopt" );
		handle_error(bind(listener, node->ai_addr, node->ai_addrlen), 
				"Error: unable to bind to socket");
		handle_error(listen(listener, BACKLOG), 
				"Error: could not listen to incoming connections from client");
	}

	freeaddrinfo(s.info);

	return s;
}

