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

#define PEER_NAME              	"MAIN"
#define PEER_IP_ADDRESS        	"10.240.21.196"
#define PEER_PORT              	53255

#define KNOWN_MAX 				10

// note: every peer should have its own files directory
#define FILES_DIR				"./files/"
#define MAX_WORD_NUM			100
#define MAX_CHARACTER_NUM		16

typedef struct _parsed_file{

    char words[MAX_WORD_NUM][MAX_CHARACTER_NUM];
    int words_count;

} parsed_file;

char data_buffer[1024];

char ser_out_req[1024];
char ser_in_req[1024];

char cli_out_req[1024];
char cli_in_req[1024];

node_and_files known[KNOWN_MAX];
int known_number;
int known_next;

int filenumber;
char files[MAX_FILENUMBER][16];

node parse_node(char *node_string) {
	node parsed;
	int i = 0;
	int k = 0;
	while (node_string[i] != ':') {
		parsed.nodename[k++] = node_string[i];
		i++;
	}
	i++;
	k = 0;
	while (node_string[i] != ':') {
		parsed.ip[k++] = node_string[i];
		i++;
	}
	i++;
	k = 0;
	char port[5];
	strcpy(port, "");
	while (node_string[i] != '\0') {
		port[k++] = node_string[i];
		i++;
	}
	parsed.port = atoi(port);
	return parsed;
}

node_and_files parse_node_and_files(char *string) {
	node_and_files parsed;
	int i = 0;
	int k = 0;
	while (string[i] != ':') {
		parsed.nodename[k++] = string[i];
		i++;
	}
	i++;
	k = 0;
	while (string[i] != ':') {
		parsed.ip[k++] = string[i];
		i++;
	}
	i++;
	k = 0;
	char port[6];
	strcpy(port, "");
	while (string[i] != ':') {
		port[k++] = string[i];
		i++;
	}
	parsed.port = atoi(port);
	i++;
	k = 0;
	int n = 0;
	while (string[i] != '\0') {
		if (string[i] == ',') {
			i++;
			k = 0;
			n++;
		}
		parsed.files[n][k++] = string[i];
		i++;
	}
	n++;
	parsed.filenumber = n;
	return parsed;
}

parsed_file parse_file(char* filepath) {
	parsed_file result;

	FILE *fp;
	fp = fopen(filepath, "r"); // read mode
	if (fp == NULL) {
		perror("Error while opening the file.\n");
		exit(EXIT_FAILURE);
	}

	int i = 0;
	while(1) {
		if (!(i < MAX_WORD_NUM)) {
			break;
		}
		char r = (char)fgetc(fp);
		int k = 0;
		while (r != ' ' && !feof(fp)) {
			result.words[i][k++] = &r;
			r = (char)fgetc(fp);
		}
		result.words[i][k] = 0;
		if (feof(fp)) {
			break;
		}
		i++;
	}
	result.words_count = i - 1;
	fclose(fp);
	return result;
}

void server_function(int comm_socket_fd, struct sockaddr_in client_addr, int addr_len) {

	char *incoming_request = (char *)data_buffer;

	if (strcmp(incoming_request, "1") == 0) {
		
		recvfrom(comm_socket_fd, (char *)data_buffer, sizeof(data_buffer), 0,
			(struct sockaddr *)&client_addr, &addr_len);
		printf("%s\n", incoming_request);
		node_and_files incoming_node = parse_node_and_files(incoming_request);
		memset(data_buffer, 0, sizeof(data_buffer));
		// 
		int matching_index = -1;
		for (int i = 0; i < known_number; i++) {
			if (strcmp(incoming_node.ip, known[i].ip) == 0 && incoming_node.port == known[i].port) {
				matching_index = i;
			}
		}
		if (matching_index == -1) {
			known[known_next] = incoming_node;
			if (known_number != KNOWN_MAX) {
				known_number++;
			}
			known_next = (known_next % 9) + 1;
		} else {
			known[matching_index].filenumber = incoming_node.filenumber;
			int i;
			for (i = 0; i < incoming_node.filenumber; i++) {
				strcpy(known[matching_index].files[i], incoming_node.files[i]);
			}
		}

		recvfrom(comm_socket_fd, (char *)data_buffer, sizeof(data_buffer), 0,
			(struct sockaddr *)&client_addr, &addr_len);
		printf("%s\n", incoming_request);
		int n = atoi(incoming_request);
		memset(data_buffer, 0, sizeof(data_buffer));

		if (n > 0) {

			int i;
			for (i = 0; i < n; i++) {

				recvfrom(comm_socket_fd, (char *)data_buffer, sizeof(data_buffer), 0,
					(struct sockaddr *)&client_addr, &addr_len);
				printf("%s\n", incoming_request);
				node incoming_n = parse_node(incoming_request);
				memset(data_buffer, 0, sizeof(data_buffer));
				node_and_files incoming_node;
				strcpy(incoming_node.nodename, incoming_n.nodename);
				strcpy(incoming_node.ip, incoming_n.ip);
				incoming_node.port = incoming_n.port;
				incoming_node.filenumber = 0;
				// 
				int matching_number = 0;
				for (int i = 0; i < known_number; i++) {
					if (strcmp(incoming_node.ip, known[i].ip) == 0 && incoming_node.port == known[i].port) {
						matching_number++;
					}
				}
				if (matching_number == 0) {
					known[known_next] = incoming_node;
					if (known_number != KNOWN_MAX) {
						known_number++;
					}
					known_next = (known_next % 9) + 1;
				}
			}

		}

	} else if (strcmp(incoming_request, "0") == 0) {

		int i;
		for (i = 0; i < filenumber; i++) {
			strcpy(ser_out_req, files[i]);
			printf("%s\n", ser_out_req);
			usleep(1000);
			sendto(comm_socket_fd, (char *)&ser_out_req, strlen(ser_out_req) * sizeof(char), 0,
				(struct sockaddr *)&client_addr, sizeof(struct sockaddr));

			char filepath[64];
			strcpy(filepath, FILES_DIR);
			strcat(filepath, files[i]);
			parsed_file file = parse_file(filepath);

			sprintf(ser_out_req, "%d", file.words_count);
			printf("%s\n", ser_out_req);
			usleep(1000);
			sendto(comm_socket_fd, (char *)&ser_out_req, strlen(ser_out_req) * sizeof(char), 0,
				(struct sockaddr *)&client_addr, sizeof(struct sockaddr));

			int j;
			for (j = 0; j < file.words_count; j++) {
				strcpy(ser_out_req, file.words[j]);
				printf("%s\n", ser_out_req);
				usleep(1000);
				sendto(comm_socket_fd, (char *)&ser_out_req, strlen(ser_out_req) * sizeof(char), 0,
					(struct sockaddr *)&client_addr, sizeof(struct sockaddr));
			}
		}
	}
}

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

				server_function(comm_socket_fd, client_addr, addr_len);
           	}
       	}
   	}/*wait for new client request again*/
   	pthread_exit(NULL); 
}

void* setup_tcp_client_communication() {

    /*Initialization*/
    /*Socket handle*/
    int sockfd = 0, 
        sent_recv_bytes = 0;

    int addr_len = 0;
	addr_len = sizeof(struct sockaddr);

	/*to store socket addesses : ip address and port*/
	struct sockaddr_in dest;

    while(1) {

	    char target_node_string[64];
		char flag;

		printf("Client: Enter a node you want to send request to in the format \"name:ip:port\"\n");
		fgets(target_node_string, 64, stdin);
		printf("Client: Enter a flag (1 for synchronization, 0 for request)\n");
		flag = (char)getchar();
		getchar();	// for not leaving '\n' in stdin

		node target_node = parse_node(target_node_string);
	
		if (flag == '1') {

			dest.sin_family = AF_INET;
			dest.sin_port = target_node.port;
			struct hostent *host = (struct hostent *)gethostbyname(target_node.ip);
			dest.sin_addr = *((struct in_addr *)host->h_addr);

			sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
			if (connect(sockfd, (struct sockaddr *)&dest,sizeof(struct sockaddr)) != 0) {
				printf("Client: Eroor during connection to the node\n");
				continue;
			}

			strcpy(cli_out_req, &flag);
			usleep(1000);
			printf("%s\n", cli_out_req);
			sent_recv_bytes = sendto(sockfd, 
			&cli_out_req,
			sizeof(char), 
			0, 
			(struct sockaddr *)&dest, 
			sizeof(struct sockaddr));
			printf("Client: No of bytes sent = %d\n", sent_recv_bytes);

			strcpy(cli_out_req, PEER_NAME);
			strcat(cli_out_req, ":");
			strcat(cli_out_req, PEER_IP_ADDRESS);
			strcat(cli_out_req, ":");
			char peer_port[6];
			sprintf(peer_port, "%d", PEER_PORT);
			strcat(cli_out_req, peer_port);
			strcat(cli_out_req, ":");
			int i;
			strcat(cli_out_req, files[0]);
			for (i = 1; i < filenumber; i++) {
				strcat(cli_out_req, ",");
				strcat(cli_out_req, files[i]);
			}

			printf("%s\n", cli_out_req);
			usleep(1000);
			sent_recv_bytes = sendto(sockfd, 
			&cli_out_req,
			strlen(cli_out_req) * sizeof(char), 
			0, 
			(struct sockaddr *)&dest, 
			sizeof(struct sockaddr));
			printf("Client: No of bytes sent = %d\n", sent_recv_bytes);

			sprintf(cli_out_req, "%d", known_number);
			printf("%s\n", cli_out_req);
			usleep(1000);
			sent_recv_bytes = sendto(sockfd, 
			&cli_out_req,
			strlen(cli_out_req) * sizeof(char), 
			0, 
			(struct sockaddr *)&dest, 
			sizeof(struct sockaddr));
			printf("Client: No of bytes sent = %d\n", sent_recv_bytes);

			for (i = 0; i < known_number; i++) {
				strcpy(cli_out_req, known[i].nodename);
				strcat(cli_out_req, ":");
				strcat(cli_out_req, known[i].ip);
				strcat(cli_out_req, ":");
				sprintf(peer_port, "%d", known[i].port);
				strcat(cli_out_req, peer_port);
				printf("%s\n", cli_out_req);
				usleep(1000);
				sent_recv_bytes = sendto(sockfd, 
				&cli_out_req,
				strlen(cli_out_req) * sizeof(char), 
				0, 
				(struct sockaddr *)&dest, 
				sizeof(struct sockaddr));
				printf("Client: No of bytes sent = %d\n", sent_recv_bytes);
			}

		} else if (flag == '0') {

			int target_node_index = -1;
			int i;
			for (i = 0; i < known_number; i++) {
				if (strcmp(target_node.nodename, known[i].nodename) == 0 &&
					strcmp(target_node.ip, known[i].ip) == 0 &&
					target_node.port == known[i].port) {
						target_node_index = i;
				}
			}
		
			if (target_node_index != -1) {

				dest.sin_family = AF_INET;
				dest.sin_port = target_node.port;
				struct hostent *host = (struct hostent *)gethostbyname(target_node.ip);
				dest.sin_addr = *((struct in_addr *)host->h_addr);

				sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
				if (connect(sockfd, (struct sockaddr *)&dest,sizeof(struct sockaddr)) != 0) {
				printf("Client: Error during connection to the node\n");
				continue;
				}

				// send "0"
				strcpy(cli_out_req, &flag);
				printf("%s\n", cli_out_req);
				usleep(1000);
				sent_recv_bytes = sendto(sockfd, 
				&cli_out_req,
				sizeof(char), 
				0, 
				(struct sockaddr *)&dest, 
				sizeof(struct sockaddr));
				printf("Client: No of bytes sent = %d\n", sent_recv_bytes);

				int i;
				for (i = 0; i < known[target_node_index].filenumber; i++) {

					// receive "filename"
					recvfrom(sockfd, (char *)&cli_in_req, sizeof(char[1024]), 0,
						(struct sockaddr *)&dest, &addr_len);
					printf("%s\n", cli_in_req);
					char filename[16];
					strcpy(filename, cli_in_req);
					memset(data_buffer, 0, sizeof(data_buffer));

					// receive "n" (number of words in the file)
					recvfrom(sockfd, (char *)&cli_in_req, sizeof(char[1024]), 0,
						(struct sockaddr *)&dest, &addr_len);
					printf("%s\n", cli_in_req);
					int n = atoi(cli_in_req);
					memset(data_buffer, 0, sizeof(data_buffer));

					// create file and open it for edit
					char filepath[64];
					strcpy(filepath, FILES_DIR);
					strcat(filepath, filename);
					FILE *fp;
					fp = fopen(filepath, "w");
					if (fp == NULL) {
						perror("Error while opening the file.\n");
						exit(EXIT_FAILURE);
					}

					// receive the file word by word
					int i;
					for (i = 0; i < n; i++) {
						if (i > 0) {
							fprintf(fp, " ");
						}
						recvfrom(sockfd, (char *)&cli_in_req, sizeof(char[1024]), 0,
							(struct sockaddr *)&dest, &addr_len);
						printf("%s\n", cli_in_req);
						fprintf(fp, "%s", cli_in_req);
						memset(data_buffer, 0, sizeof(data_buffer));
					}

					fclose(fp);
				}
			}
		} else printf("Client: Wrong flag entered\n");
    }
}

int main(int argc, char **argv){

	known_number = 0;
	known_next = 0;

	filenumber = 1;
	strcpy(files[0], "main.txt");

	pthread_t* thread = malloc(sizeof(pthread_t));
		
	pthread_create(&thread, NULL, setup_tcp_client_communication, NULL);

	setup_tcp_server_communication();

	pthread_join(thread, NULL);
		free(thread);
	return 0;
}