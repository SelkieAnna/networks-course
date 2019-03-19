//Taken from Abhishek Sagar

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

#define PEER1_NAME              "PEER1"
#define PEER1_PORT              53255
#define PEER1_IP_ADDRESS        "127.0.0.1"

test_struct_t test_struct;
char data_buffer[1024];

#define PEER2_PORT              53266
#define PEER2_IP_ADDRESS        "127.0.0.1"

test_struct_t client_data;

void* setup_tcp_server_communication(void* args){

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
   server_addr.sin_port = PEER1_PORT;/*Server will process any data arriving on port no 2000*/
   
   /*Server's IP address*/
   server_addr.sin_addr.s_addr = INADDR_ANY; 

   addr_len = sizeof(struct sockaddr);

   /* Bind the server.*/

   if (bind(master_sock_tcp_fd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1)
   {
       printf("Server: socket bind failed\n");
       return 0;
   }

   /*Tell the Linux OS to maintain the queue of max length to Queue incoming
    * client connections.*/
   if (listen(master_sock_tcp_fd, 5)<0)  
   {
       printf("Server: listen failed\n");
       return 0;
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

            // 
        //    printf("Connection accepted from client : %s:%u\n", 
        //        inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

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

               test_struct_t *test_struct = (test_struct_t *)data_buffer;
               
               /* If the client sends a special msg to server, then server close the client connection forever*/

            //    
            //    if (client_data->nodename == 0 && client_data->ip == 0 && client_data->port == 0) {
            //        close(comm_socket_fd);
            //        printf("Server closes connection with client : %s:%u\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
            //        break;/*Get out of inner while loop, server is done with this client, time to check for new connection request by executing selct()*/
            //    }

                printf("\nServer: request from node @%s@:@%s@:@%d@\n", test_struct->nodename, test_struct->ip, test_struct->port);

           }
       }
   }/*wait for new client request again*/   
   pthread_exit(NULL); 
}

void setup_tcp_client_communication() {
    /*Initialization*/
    /*Socket handle*/
    int sockfd = 0, 
        sent_recv_bytes = 0;

    int addr_len = 0;

    addr_len = sizeof(struct sockaddr);

    /*to store socket addesses : ip address and port*/
    struct sockaddr_in dest;

    /*specify server information*/
    /*Ipv4 sockets, Other values are IPv6*/
    dest.sin_family = AF_INET;

    dest.sin_port = PEER2_PORT;
    struct hostent *host = (struct hostent *)gethostbyname(PEER2_IP_ADDRESS);
    dest.sin_addr = *((struct in_addr *)host->h_addr);

    /*Create a TCP socket*/
    /*Create a socket finally. socket() is a system call, which asks for three paramemeters*/
    sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    /*get the data to be sent to server*/
    /*Our client is now ready to send data to server. sendto() sends data to Server*/

    strcpy(client_data.nodename, PEER1_NAME);
    strcpy(client_data.ip, PEER1_IP_ADDRESS);
    client_data.port = PEER1_PORT;

    while(1) {

	    printf("CLient: Press ENTER to send request to the server\n");
	    getchar();

        connect(sockfd, (struct sockaddr *)&dest,sizeof(struct sockaddr));
	    sent_recv_bytes = sendto(sockfd, 
		   &client_data,
		   sizeof(test_struct_t), 
		   0, 
		   (struct sockaddr *)&dest, 
		   sizeof(struct sockaddr));
	    
	    printf("Client: No of bytes sent = %d\n", sent_recv_bytes);
    }
}

int main(int argc, char **argv){
    pthread_t* thread = malloc(sizeof(pthread_t));
	
    pthread_create(&thread, NULL, setup_tcp_server_communication, NULL);

    setup_tcp_client_communication();

    pthread_join(thread, NULL);
	free(thread);
    return 0;
}