#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <memory.h>
#include <errno.h>
#include "struct.h"
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>

#define PEER_NAME              "HEHE2"
#define PEER_IP_ADDRESS        "127.0.0.1"
#define PEER_PORT              53277

#define MAIN_NAME              "MAIN"
#define MAIN_IP_ADDRESS        "127.0.0.1"
#define MAIN_PORT              53255

#define KNOWN_MAX 				10

// here the data about this peer will be stored
node client_data;

node incoming_node;
node known[KNOWN_MAX];
int known_number;
int known_next;
char data_buffer[1024];

void setup_tcp_server_communication(){

	/*Initialization*/
	/*Socket handle and other variables*/
	/*Master socket file descriptor, used to accept new client connection only, no data exchange*/
	int master_sock_tcp_fd = 0, 
		sent_recv_bytes = 0, 
		addr_len = 0, 
		opt = 1;

	/*client specific communication socket file descriptor, 
		* used for only data exchange/communication between client and server*/
	int comm_socket_fd = 0;     
	/* Set of file descriptor on which select() polls. Select() unblocks whever data arrives on 
		* any fd present in this set*/
	fd_set readfds;             
	/*variables to hold server information*/
	struct sockaddr_in server_addr, /*structure to store the server and client info*/
						client_addr;

	/*tcp master socket creation*/
	if ((master_sock_tcp_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP )) == -1)
	{
		printf("Server: socket creation failed\n");
		exit(1);
	}

	/*specify server Information*/
	server_addr.sin_family = AF_INET;/*This socket will process only ipv4 network packets*/
	server_addr.sin_port = PEER_PORT;/*Server will process any data arriving on port no 2000*/
	
	/*Server's IP address*/
	server_addr.sin_addr.s_addr = INADDR_ANY; 

	addr_len = sizeof(struct sockaddr);

	/* Bind the server.*/

	if (bind(master_sock_tcp_fd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1)
	{
		printf("Server: socket bind failed\n");
		return;
	}

	/*Tell the Linux OS to maintain the queue of max length to Queue incoming
		* client connections.*/
	if (listen(master_sock_tcp_fd, 5)<0)  
	{
		printf("Server: listen failed\n");
		return;
	}

   /* Server infinite loop for servicing the client*/

   	while(1){

		/*initialze and dill readfds*/
		FD_ZERO(&readfds);                     /* Initialize the file descriptor set*/
		FD_SET(master_sock_tcp_fd, &readfds);  /*Add the socket to this set on which our server is running*/

		/*Wait for client connection*/
		select(master_sock_tcp_fd + 1, &readfds, NULL, NULL, NULL); 

		/*Some data on some fd present in set has arrived, Now check on which File descriptor the data arrives, and process accordingly*/
		if (FD_ISSET(master_sock_tcp_fd, &readfds))
		{ 
			/*Data arrives on Master socket only when new client connects with the server (that is, 'connect' call is invoked on client side)*/
			comm_socket_fd = accept(master_sock_tcp_fd, (struct sockaddr *)&client_addr, &addr_len);
			if(comm_socket_fd < 0){

				/* if accept failed to return a socket descriptor, display error and exit */
				printf("Server: accept error : errno = %d\n", errno);
				exit(0);
           	}
            
			// printf("Connection accepted from client : %s:%u\n", 
			// inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

           	while(1){
				/*Drain to store client info (ip and port) when data arrives from client, sometimes, server would want to find the identity of the client sending msgs*/
				memset(data_buffer, 0, sizeof(data_buffer));

				sent_recv_bytes = recvfrom(comm_socket_fd, (char *)data_buffer, sizeof(data_buffer), 0,
						(struct sockaddr *)&client_addr, &addr_len);

				if(sent_recv_bytes == 0){
					/*If server recvs empty msg from client, server may close the connection and wait
						* for fresh new connection from client - same or different*/
					close(comm_socket_fd);
					break; 
				}

				node *incoming_node = (node *)data_buffer;

				int matching_number = 0;
				for (int i = 0; i < known_number; i++) {
					if (strcmp(incoming_node->ip, known[i].ip) == 0 && incoming_node->port == known[i].port) {
						matching_number++;
					}
				}
				if (matching_number == 0) {
					known[known_next] = *incoming_node;
					if (known_number != KNOWN_MAX) {
						known_number++;
					}
					known_next = (known_next % 9) + 1;
					printf("\n\nServer: new known node @%s@:@%s@:@%d@\n\n", incoming_node->nodename, incoming_node->ip, incoming_node->port);
				}

           	}
       	}
   	}/*wait for new client request again*/
   	pthread_exit(NULL); 
}

void* setup_tcp_client_communication() {

	/*get the data to be sent to server*/
	strcpy(client_data.nodename, PEER_NAME);
	strcpy(client_data.ip, PEER_IP_ADDRESS);
	client_data.port = PEER_PORT;

    /*Initialization*/
    /*Socket handle*/
    int sockfd = 0, 
        sent_recv_bytes = 0;

    int addr_len = 0;
	addr_len = sizeof(struct sockaddr);

	/*to store socket addesses : ip address and port*/
	struct sockaddr_in dest;

    while(1) {

	    printf("Client: Press ENTER to ping all known nodes");
	    getchar();

		for (int i = 0; i < known_number; i++) {

			/*specify server information*/
			/*Ipv4 sockets, Other values are IPv6*/
			dest.sin_family = AF_INET;
			dest.sin_port = known[i].port;
			struct hostent *host = (struct hostent *)gethostbyname(known[i].ip);
			dest.sin_addr = *((struct in_addr *)host->h_addr);

			/*Create a TCP socket*/
			/*Create a socket finally. socket() is a system call, which asks for three paramemeters*/
			sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

			connect(sockfd, (struct sockaddr *)&dest,sizeof(struct sockaddr));
			sent_recv_bytes = sendto(sockfd, 
			&client_data,
			sizeof(node), 
			0, 
			(struct sockaddr *)&dest, 
			sizeof(struct sockaddr));
			
			printf("Client: No of bytes sent = %d\n", sent_recv_bytes);
		}

    }
}

int main(int argc, char **argv){

	// adding main node as a first known
	node main;
	strcpy(main.nodename, MAIN_NAME);
	strcpy(main.ip, MAIN_IP_ADDRESS);
	main.port = MAIN_PORT;
	known[0] = main;
	known_number = 1;
	known_next = 1;

    pthread_t* thread = malloc(sizeof(pthread_t));
	
    pthread_create(&thread, NULL, setup_tcp_client_communication, NULL);

    setup_tcp_server_communication();

    pthread_join(thread, NULL);
	free(thread);
    return 0;
}