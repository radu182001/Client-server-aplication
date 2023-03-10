#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "helpers.h"

#include <stdbool.h>

void usage(char *file)
{
	fprintf(stderr, "Usage: %s server_address server_port\n", file);
	exit(0);
}

int main(int argc, char *argv[])
{
	setvbuf(stdout, NULL, _IONBF, BUFSIZ);

	int sockfd, n, ret, found = 0;
	struct sockaddr_in serv_addr;
	char buffer[BUFLEN];
	char buffer2[BUFLEN*2];
	//char buffer3[BUFLEN];
	char *p;
	fd_set read_fds, tmp_fds;

	if (argc < 3) {
		usage(argv[0]);
	}

	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "socket");

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[3]));
	ret = inet_aton(argv[2], &serv_addr.sin_addr);
	DIE(ret == 0, "inet_aton");

	ret = connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
	DIE(ret < 0, "connect");
	
	strcpy(buffer, argv[1]);
	n = send(sockfd, buffer, strlen(buffer), 0);
	DIE(n < 0, "recv");

	FD_SET(STDIN_FILENO, &read_fds);
	FD_SET(sockfd, &read_fds);

	p = buffer2;

	while (1) {
		tmp_fds = read_fds; 
		
		ret = select(sockfd + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "select");

		if (FD_ISSET(STDIN_FILENO, &tmp_fds)) {

			/*if (!IDsent){
				memset(buffer, 0, BUFLEN);
				strcpy(buffer, argv[1]);
				IDsent = true;
			}else{*/
				// se citeste de la tastatura
				memset(buffer, 0, BUFLEN);
				fgets(buffer, BUFLEN - 1, stdin);
			//}

			if (strncmp(buffer, "exit", 4) == 0) {
				break;
			}

			// se trimite mesaj la server
			n = send(sockfd, buffer, strlen(buffer), 0);
			DIE(n < 0, "send");

			if (strncmp(buffer, "subscribe", 9) == 0)
				printf("Subscribed to topic.\n");
			else if (strncmp(buffer, "unsubscribe", 11) == 0)
				printf("Unsubscribed from topic.\n");
		}

		if (FD_ISSET(sockfd, &tmp_fds)) {
			memset(buffer, 0, BUFLEN);
			while (1){
			
				n = recv(sockfd, p, BUFLEN, 0);
				DIE(n < 0, "recv");
				//printf("%s\n", p);
				p = p + n;
				found = 0;
				for(int i = 0; i < BUFLEN; i++)
					if (p[i] == '@'){
						p -= BUFLEN;
						memcpy(buffer, buffer2, BUFLEN);
						memcpy(buffer2, buffer2+BUFLEN, BUFLEN);
						found = 1;
						break;
					}
				if (found){
					break;
				}

			}
			

			if (strncmp(buffer, "exit", 4) == 0)
					break;

			/*for (int i = 0; i < BUFLEN; i++)
				if (buffer[i] == '$'){
					buffer[i] = '\0';
					break;
				}*/

			buffer[strlen(buffer)-1] = 0;

			//strncpy(buffer3, buffer, strlen(buffer));

			printf("%s\n", buffer);	
			
		}
	}

	close(sockfd);

	return 0;
}
