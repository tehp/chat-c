/*------------------------------------------------------------------------------------------------------------------
   -- SOURCE FILE: server.c
   --
   -- PROGRAM: chat
   --
   -- FUNCTIONS:
   -- int main (int argc, char **argv)
   -- void handle(int signal_number)
   --
   -- DATE: April 3, 2018
   --
   -- REVISIONS: (Date and Description)
   -- N/A
   --
   -- DESIGNER: Mackenzie Craig, Haley Booker
   --
   -- PROGRAMMER: Haley Booker
   --
   -- NOTES:
   -- This portion of the program is the server. The server listens for connections and allows chat messages to be sent
   -- from one client to all of the others via select.
   --
   -- RUNNING:
   -- ./server [OPTIONAL: port]
   --
   -- EXAMPLE:
   -- ./server (runs a server on your ip:7000)
   -- ./server 9000 (runs a server on your ip:9000)
   ----------------------------------------------------------------------------------------------------------------------*/

#include <stdio.h>
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>
#include <arpa/inet.h>

#define SERVER_TCP_PORT 7000  // Default port
#define BUFLEN  512   //Buffer length
#define TRUE  1
#define LISTENQ 5
#define MAXLINE 4096

void handle(int signal_number);
void listening();
char *ip_list[FD_SETSIZE];

int i, maxi, nready, bytes_to_read, arg;
int listen_sd, new_sd, sockfd, port, maxfd, client[FD_SETSIZE];
struct sockaddr_in server, client_addr;
char *bp, buf[BUFLEN], msg_buffer[BUFLEN], client_ip[BUFLEN];
unsigned int client_len;
ssize_t n;
fd_set rset, allset;

/*------------------------------------------------------------------------------------------------------------------
   -- FUNCTION: main
   --
   -- DATE: April 3, 2018
   --
   -- REVISIONS: (Date and Description)
   -- N/A
   --
   -- DESIGNER: Mackenzie Craig, Haley Booker
   --
   -- PROGRAMMER: Haley Booker
   --
   -- INTERFACE: int main (int argc, char **argv)
   --
   -- RETURNS: 0 (int)
   --
   -- NOTES:
   -- The entry point for the server portion of the program. The function parses command line arguments, then binds
   -- itself to the socket. The function then listens for its maximum number of connections.
   ----------------------------------------------------------------------------------------------------------------------*/
int main (int argc, char **argv){
        signal(SIGINT, handle);

        switch(argc) {
        case 1:
                port = SERVER_TCP_PORT; // Use the default port
                break;
        case 2:
                port = atoi(argv[1]); // Get user specified port
                break;
        default:
                fprintf(stderr, "Usage: %s [port]\n", argv[0]);
                exit(1);
        }

        // Create a stream socket
        if ((listen_sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
                printf("socket create error\n");
        }

        // set SO_REUSEADDR so port can be resused imemediately after exit, i.e., after CTRL-c
        arg = 1;
        if (setsockopt (listen_sd, SOL_SOCKET, SO_REUSEADDR, &arg, sizeof(arg)) == -1) {
                printf("setsockopt error\n");
        }

        // Bind an address to the socket
        bzero((char *)&server, sizeof(struct sockaddr_in));
        server.sin_family = AF_INET;
        server.sin_port = htons(port);
        server.sin_addr.s_addr = htonl(INADDR_ANY); // Accept connections from any client

        if (bind(listen_sd, (struct sockaddr *)&server, sizeof(server)) == -1) {
                printf("bind() failed\n");
        }

        listen(listen_sd, LISTENQ);

        maxfd = listen_sd; // initialize
        maxi = -1; // index into client[] array

        for (i = 0; i < FD_SETSIZE; i++) {
                client[i] = -1; // -1 indicates available entry
        }

        FD_ZERO(&allset);
        FD_SET(listen_sd, &allset);

        listening();

        return(0);
}

/*------------------------------------------------------------------------------------------------------------------
   -- FUNCTION: listening
   --
   -- DATE: April 3, 2018
   --
   -- REVISIONS: (Date and Description)
   -- N/A
   --
   -- DESIGNER: Mackenzie Craig, Haley Booker
   --
   -- PROGRAMMER: Haley Booker
   --
   -- INTERFACE: void listening()
   --
   -- RETURNS: 0 (int)
   --
   -- NOTES:
   -- The functions main loop listens for data from its connections and if received, distributes it to the other connected clients.
   ----------------------------------------------------------------------------------------------------------------------*/
void listening() {
        while (TRUE) {
                rset = allset; // structure assignment
                nready = select(maxfd + 1, &rset, NULL, NULL, NULL);

                if (FD_ISSET(listen_sd, &rset)) {
                        client_len = sizeof(client_addr);
                        if ((new_sd = accept(listen_sd, (struct sockaddr *) &client_addr, &client_len)) == -1) {
                                printf("accept() error\n");
                        }

                        printf("Connected:  %s\n", inet_ntoa(client_addr.sin_addr));

                        for (i = 0; i < FD_SETSIZE; i++) {
                                if (client[i] < 0) {
                                        client[i] = new_sd; // save descriptor.
                                        ip_list[i] = inet_ntoa(client_addr.sin_addr); // save address
                                        break;
                                }
                        }

                        if (i == FD_SETSIZE) {
                                printf ("Too many clients\n");
                                exit(1);
                        }

                        FD_SET (new_sd, &allset); // add new descriptor to set

                        if (new_sd > maxfd) {
                                maxfd = new_sd; // for select
                        }

                        if (i > maxi) {
                                maxi = i; // new max index in client[] array
                        }

                        if (--nready <= 0) {
                                continue; // no more readable descriptors
                        }
                }

                for (i = 0; i <= maxi; i++) { // iter through clients for data

                        if ((sockfd = client[i]) < 0) {
                                continue;
                        }

                        if (FD_ISSET(sockfd, &rset)) {

                                bp = buf;
                                bytes_to_read = BUFLEN;

                                while ((n = read(sockfd, bp, bytes_to_read)) > 0) {
                                        bp += n;
                                        bytes_to_read -= n;
                                }

                                for (int client_num = 0; client_num <= maxi; client_num++) { // iterate through each client

                                        if (buf[0] == EOF) { // EOF received from quitting client
                                                printf("Disconnect: %s\n", inet_ntoa(client_addr.sin_addr));
                                                FD_CLR(sockfd, &allset);
                                                client[i] = -1;
                                                break;
                                        }

                                        if ((sockfd = client[client_num]) < 0 || client_num == i) {
                                                continue;
                                        }

                                        strcpy(client_ip, ip_list[i]);
                                        sprintf(msg_buffer, "[%s] %s", client_ip, buf);
                                        write(client[client_num], msg_buffer, BUFLEN); // echo to client
                                }

                                if (--nready <= 0) {
                                        break; // no more readable descriptors
                                }
                        }
                }
        }
}

/*------------------------------------------------------------------------------------------------------------------
   -- FUNCTION: handle
   --
   -- DATE: April 3, 2018
   --
   -- REVISIONS: (Date and Description)
   -- N/A
   --
   -- DESIGNER: Mackenzie Craig, Haley Booker
   --
   -- PROGRAMMER: Haley Booker
   --
   -- INTERFACE: void handle(int signal_number)
   --
   -- RETURNS: void
   --
   -- NOTES:
   -- This function logs the list of clients on exit.
   ----------------------------------------------------------------------------------------------------------------------*/
void handle(int signal_number) {
        if(signal_number == SIGINT) {
                printf("\nActive clients:\n");
                for (int i = 0; i < 128; i++) {
                        if (ip_list[i] != 0) {
                                printf("%s\n", ip_list[i]);
                        }
                }
                kill(getpid(), SIGTERM);
        }
}
