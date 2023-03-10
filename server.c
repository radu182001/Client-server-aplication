#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <limits.h>
#include <netinet/tcp.h>
#include "helpers.h"
#include "List.h"

#include <stdbool.h>

#define INVALID_MESSAGE ("Format invalid pentru mesaj!")
#define MAX_CLIENTS_NO 100
#define FD_START 4

void usage(char *file)
{
	fprintf(stderr, "Usage: %s server_port\n", file);
	exit(0);
}

void init_clients(int* clients) {
	int i;

	for (i = 0; i < MAX_CLIENTS_NO; i++) {
		clients[i] = 0;
	}
}

void print_clients(int* clients) {
	int i;

	printf("Clienti conectat: ");
	for (i = 0; i < MAX_CLIENTS_NO; i++) {
		if (clients[i]) {
			printf("%d ", i + FD_START);
		}
	}
	printf("\n");
}

void write_clients(char* buffer, int* clients) {
	int i;
	int len;

	snprintf(buffer, BUFLEN - 1, "Clienti conectati: ");
	for (i = 0; i < MAX_CLIENTS_NO; i++) {
		if (clients[i]) {
			len = strlen(buffer);
			snprintf(buffer + len, BUFLEN - len - 1, "%d ", i + FD_START);
		}
	}
	len = strlen(buffer);
	snprintf(buffer + len, BUFLEN - len - 1, "\n");
}

void add_client(int id, int* clients) {
	clients[id - FD_START] = 1;
	//printf("%d\n", id - FD_START);
}

void rm_client(int id, int* clients) {
	clients[id - FD_START] = 0;
}

int is_client_connected(int id, int* clients) {
	return id >= FD_START && clients[id - FD_START];
}

void send_info_to_clients(int* clients) {
	int i, n;
	char buffer[BUFLEN];

	write_clients(buffer, clients);

	for (i = 0; i < MAX_CLIENTS_NO; i++) {
		if (clients[i]) {
			n = send(i + FD_START, buffer, strlen(buffer) + 1, 0);
			DIE(n < 0, "send");
		}
	}
}

int main(int argc, char *argv[])
{
	setvbuf(stdout, NULL, _IONBF, BUFSIZ);

	int sockfd, newsockfd, portno, sockUDP;
	char buffer[BUFLEN];
	struct sockaddr_in serv_addr, cli_addr, udp_addr, udp;
	int n, i, ret;
	char *command;
	socklen_t clilen;
	int clients[MAX_CLIENTS_NO];

	//List for clients
	list *clientsList = (list *)malloc(sizeof(list));
	initList(clientsList);

	fd_set read_fds;	// multimea de citire folosita in select()
	fd_set tmp_fds;		// multime folosita temporar
	int fdmax;			// valoare maxima fd din multimea read_fds

	if (argc < 2) {
		usage(argv[0]);
	}

	init_clients(clients);

	// se goleste multimea de descriptori de citire (read_fds) si multimea temporara (tmp_fds)
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "socket");

	//UDP socket
	sockUDP = socket(AF_INET, SOCK_DGRAM, 0);

	portno = atoi(argv[1]);
	DIE(portno == 0, "atoi");

	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portno);
	serv_addr.sin_addr.s_addr = INADDR_ANY;

	//UDP
	memset(&udp_addr, 0, sizeof(udp_addr));
	udp_addr.sin_family = AF_INET;
	udp_addr.sin_addr.s_addr = INADDR_ANY;
	udp_addr.sin_port = htons(portno);

	//UDP bind
	ret = bind(sockUDP, (struct sockaddr *) &udp_addr, sizeof(udp_addr));
	DIE(ret < 0, "bind");

	ret = bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr));
	DIE(ret < 0, "bind");

	int enable = 1;
	setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &enable, 4);

	ret = listen(sockfd, MAX_CLIENTS);
	DIE(ret < 0, "listen");

	// se adauga noul file descriptor (socketul pe care se asculta conexiuni) in multimea read_fds
	FD_SET(STDIN_FILENO, &read_fds);
	FD_SET(sockfd, &read_fds);
	FD_SET(sockUDP, &read_fds);
	fdmax = sockfd;

	while (1) {
		tmp_fds = read_fds; 
		
		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "select");

		if (FD_ISSET(STDIN_FILENO, &tmp_fds)) {
			// se citeste de la tastatura
			memset(buffer, 0, BUFLEN);
			fgets(buffer, BUFLEN - 1, stdin);

			if (strncmp(buffer, "exit", 4) == 0) {
				//printf("da\n");
				Client *x = clientsList->head;
				while(x != NULL && x->socket != -1){
					//printf("%d\n", x->socket);
					//rm_client(x->socket, clients);
					//close(x->socket);
					//findClient(clientsList, x->socket)->socket = -1;
					//FD_CLR(x->socket, &read_fds);
					//checkExit = 1;
					n = send(x->socket, "exit@", BUFLEN, 0);
					DIE(n < 0, "send");
					x = x->next;
				}
				break;
			}
		}else{

		for (i = 0; i <= fdmax; i++) {

			if (FD_ISSET(i, &tmp_fds)) {
				if (i == sockfd) {
					// a venit o cerere de conexiune pe socketul inactiv (cel cu listen),
					// pe care serverul o accepta
					clilen = sizeof(cli_addr);
					newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
					DIE(newsockfd < 0, "accept");

					// se adauga noul socket intors de accept() la multimea descriptorilor de citire
					FD_SET(newsockfd, &read_fds);
					if (newsockfd > fdmax) { 
						fdmax = newsockfd;
					}

					add_client(newsockfd, clients);
					Client *cl = (Client *)malloc(sizeof(Client)); 
					initClient(cl, newsockfd); 
					addClient(clientsList, cl);
					//printf("%d\n", newsockfd);
					//print_clients(clients);
					//send_info_to_clients(clients);
				}else if (i == sockUDP) {
					memset(buffer, 0, BUFLEN);
					socklen_t udplen = sizeof(udp);
					int rc = recvfrom(sockUDP, buffer, BUFLEN, 1, (struct sockaddr *)&udp, &udplen);
    				DIE(rc < 0, "recv");
					int tip_date = buffer[50];
					char *topic = (char *) malloc(sizeof(char)*50);
					strncpy(topic, buffer, 50);
					topic = strtok(topic, " \0");

					if (tip_date == 0){
						int semn = buffer[51];
						uint32_t res;
						memcpy(&res, buffer+52, 4);
						memset(buffer, 0, BUFLEN);
						if (semn == 0)
							sprintf(buffer,"%s:%d - %s - %s - %d@", inet_ntoa(udp.sin_addr), ntohs(udp.sin_port), topic, "INT", ntohl(res));
						else sprintf(buffer,"%s:%d - %s - %s - -%d@", inet_ntoa(udp.sin_addr), ntohs(udp.sin_port), topic, "INT", ntohl(res));
					}else if (tip_date == 1){
						uint16_t res;
						char *x = (char *) malloc(sizeof(char)*2);
						x[0] = buffer[52];
						x[1] = buffer[51];
						memcpy(&res, x, 2);
						memset(buffer, 0, BUFLEN);
						sprintf(buffer, "%s:%d - %s - %s - %.2f@", inet_ntoa(udp.sin_addr), ntohs(udp.sin_port), topic, "SHORT_REAL", (float) res / 100);
					}else if (tip_date == 2){
						int semn = buffer[51];
						uint32_t res;
						memcpy(&res, buffer+52, 4);
						uint8_t putere = buffer[56];
						memset(buffer, 0, BUFLEN);
						if (semn == 0)
							sprintf(buffer, "%s:%d - %s - %s - %.*f@", inet_ntoa(udp.sin_addr), ntohs(udp.sin_port), topic, "FLOAT", putere, (float) ntohl(res) / power(10, putere));
						else sprintf(buffer, "%s:%d - %s - %s - -%.*f@", inet_ntoa(udp.sin_addr), ntohs(udp.sin_port), topic, "FLOAT", putere, (float) ntohl(res) / power(10, putere));
					}else if (tip_date == 3){
						char *line = malloc(sizeof(char)*BUFLEN);
						line[BUFLEN-1] = '\0'; 
						strncpy(line, buffer+51, BUFLEN);
						memset(buffer, 0, BUFLEN);
						sprintf(buffer, "%s:%d - %s - %s - %s@", inet_ntoa(udp.sin_addr), ntohs(udp.sin_port), topic, "STRING", line);
						//memset(buffer, 0, BUFLEN);
					} 

					Client *c = clientsList->head;
					while(c != NULL){
						if (stringExists(c->topics, topic, strlen(topic)) || stringExists(c->topicsWithFlag, topic, strlen(topic))){
							if (c->socket != -1){
								n = send(c->socket, buffer, BUFLEN, 0);
								DIE(n < 0, "send");
							}else if (stringExists(c->topicsWithFlag, topic, strlen(topic))){
								char *a = (char *) malloc(sizeof(char) * strlen(buffer));
								strcpy(a, buffer);
								addString(c->coada, a);
							}
						}
						c = c->next;
					}
					//n = send(5, buffer, BUFLEN, 0);
					//DIE(n < 0, "send");
					//printf("msg: %s %d\n", topic, tip_date);
					DIE(n < 0, "send");
				}else {
					// s-au primit date pe unul din socketii de client,
					// asa ca serverul trebuie sa le receptioneze
					memset(buffer, 0, BUFLEN);
					n = recv(i, buffer, sizeof(buffer), 0);
					DIE(n < 0, "recv");

					//printList(clientsList);
					if (findClient(clientsList, i)->ID == NULL){
						//printList(clientsList);
						char *newID = (char *)malloc(sizeof(char)*11);
						strncpy(newID, buffer, 11);
						//printf("%ld\n", strlen(newID));
						Client *aux = findClientByID(clientsList, newID);

						if (aux != NULL){
							//printf("da\n");
							if (aux->socket != -1){
								printf("Client %s already connected.\n", aux->ID);
								n = send(i, "exit", 4, 0);
								DIE(n < 0, "send");
								rm_client(i, clients);
								close(i);
								FD_CLR(i, &read_fds);
							}
							else{
								aux->socket = i;
								printf("New client %s connected from %s:%d.\n",
									buffer, inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
								for(int j = 0; j < aux->coada->size; j++){
									n = send(aux->socket, aux->coada->strings[j], BUFLEN, 0);
									DIE(n < 0, "send");
								}
								for (int j = 0; j < aux->coada->size; j++){
									removeString(aux->coada, aux->coada->strings[j], strlen(aux->coada->strings[j]));
								}
							}
							removeLast(clientsList);
						}else{
							findClient(clientsList, i)->ID = newID;
							printf("New client %s connected from %s:%d.\n",
									buffer, inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
						}
						//printList(clientsList);
					}else {
						if (n == 0) {
							// conexiunea s-a inchis
							printf("Client %s disconnected.\n", findClient(clientsList, i)->ID);
							rm_client(i, clients);
							close(i);

							//Dezactivez clientul
							findClient(clientsList, i)->socket = -1;

							//print_clients(clients);
							//send_info_to_clients(clients);
							
							// se scoate din multimea de citire socketul inchis 
							FD_CLR(i, &read_fds);
						} else {

							/*if (checkExit == 1){
								n = send(i, "exit", 4, 0);
								DIE(n < 0, "send");
							}*/

							command = malloc(sizeof(char)*BUFLEN);
							strcpy(command, buffer);
	
							char *arg = strtok(command, " ");
							/*if (strcmp(command, "subscribe") != 0) {
								n = strlen(INVALID_MESSAGE) + 1;
								snprintf(buffer, n, INVALID_MESSAGE);
								//dest = i;
							}*/
							if (strcmp(arg, "subscribe") == 0){	

								DIE(n < 0, "send");
								arg = strtok(NULL, " ");
								char *t = malloc(sizeof(char)*strlen(arg));
								strncpy(t, arg, strlen(arg));
								arg = strtok(NULL, " ");
								int flag = atoi(arg);
								if (flag == 0)
									addString(findClient(clientsList, i)->topics, t);
								else if (flag == 1)
									addString(findClient(clientsList, i)->topicsWithFlag, t);
								//printf("%d\n", flag);
							}
							else if (strcmp(arg, "unsubscribe") == 0){
								DIE(n < 0, "send");
								arg = strtok(NULL, " ");
								if (stringExists(findClient(clientsList, i)->topics, arg, strlen(arg)-1))
									removeString(findClient(clientsList, i)->topics, arg, strlen(arg)-1);
								else if (stringExists(findClient(clientsList, i)->topicsWithFlag, arg, strlen(arg)-1))
									removeString(findClient(clientsList, i)->topicsWithFlag, arg, strlen(arg)-1);
							}
							//printStrings(findClient(clientsList, i)->topics);
							//printStrings(findClient(clientsList, i)->topicsWithFlag);
						}
					}
				}
			}
		}
		}
		
	}

	close(sockfd);

	return 0;
}
