#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <netdb.h>
#include <stdbool.h>
#include <poll.h>
#include <time.h>

#include "utils.h"
#include "common.h"

#define PORT "3693"

void get_current_time(char* dest) {
	time_t now = time(NULL);
	struct tm* current_time = localtime(&now);
	sprintf(dest, "%02i:%02i:%02i", current_time->tm_hour, current_time->tm_min, current_time->tm_sec);
}

int main(void) {
	struct sockaddr_storage their_addr;	
	struct addrinfo hints;
	struct addrinfo* servinfo;

	socklen_t addr_size;
	
	memset(&hints, 0, sizeof(hints));
	hints.ai_flags    = AI_PASSIVE;
	hints.ai_family   = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	int status;
	if ((status = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
	}
	
	int listener = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
	if (listener == -1) {
		printf("Error: unable to open socket\n");
		exit(EXIT_FAILURE);
	}
	
	status = connect(listener, servinfo->ai_addr, servinfo->ai_addrlen);
	if (status == -1) {
		printf("Error: cannot connect\n");
		printf("Error code: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	
	printf("Please enter your name: ");
	char username[255];
	if (fgets(username, sizeof(username), stdin) == NULL) {
		printf("Error: cannot get input\n");
		printf("Error code: %d\n", errno);
		exit(EXIT_FAILURE);
	}

	username[strcspn(username, "\n")] = '\0';

	client_message client_msg;
	strncpy(client_msg.username, username, sizeof(username));

	send(listener, &client_msg, sizeof(client_msg), 0);
	
    while (true) {
		 struct pollfd client_fd[2] = {
			{ .fd = 0, .events = POLLIN },
			{ .fd = listener, .events = POLLIN | POLLOUT } 
		};
        if (poll(client_fd, 2, -1) > 0) {
            if (client_fd[0].revents & POLLIN) {
				char msg[255];
				if (fgets(msg, sizeof(msg), stdin) == NULL) {
					printf("Error: cannot get input\n");
					printf("Error code: %d\n", errno);
					exit(EXIT_FAILURE);
				}
				strncpy(client_msg.message, msg, sizeof(msg));
				msg[strcspn(msg, "\n")] = '\0';
				int bytes_sent = send(listener, &client_msg, sizeof(client_msg), 0);
				
				if (strcmp(client_msg.message, "exit\n") == 0) {
					break;
				}

				if (bytes_sent == -1) {
					printf("Error: failed to accept\n");
					printf("Error code: %d\n", errno);
					exit(EXIT_FAILURE);
				}
            }

			if (client_fd[1].revents & POLLIN) {
				client_message client_msg_recv;
				recv(listener, &client_msg_recv, sizeof(client_msg_recv), 0);
				client_msg_recv.message[strcspn(client_msg_recv.message, "\n")] = '\0';
				char today[10];
				get_current_time(today);
				printf("%s [ @%s ] | %s\n", today, client_msg_recv.username, client_msg_recv.message);
			}
			

		}
    }

	close(listener);

	return EXIT_SUCCESS;
}

