/*------------------------------------------------------------------------------------------------------------------
   -- SOURCE FILE: client.c
   --
   -- PROGRAM: chat
   --
   -- FUNCTIONS:
   -- int main (int argc, char **argv)
   -- void* readThreadFunc()
   -- void* sendThreadFunc()
   -- void handle(int signo)
   --
   -- DATE: April 3, 2018
   --
   -- REVISIONS: (Date and Description)
   -- N/A
   --
   -- DESIGNER: Mackenzie Craig, Haley Booker
   --
   -- PROGRAMMER: Mackenzie Craig
   --
   -- NOTES:
   -- This portion of the program is the client. The client connects to an online server, and sends and receives messages.
   --
   -- RUNNING:
   -- ./client [ip] [port] [OPTIONAL: bit for enabling log]
   --
   -- EXAMPLE:
   -- ./client 127.0.0.1 7000 1 (logging enabled)
   -- ./client 192.168.1.13 7000 0 (logging disabled)
   ----------------------------------------------------------------------------------------------------------------------*/

#include <stdio.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdlib.h>
#include <strings.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>

#define SERVER_TCP_PORT 7000  // Default port
#define BUFLEN 512    // Buffer length
#define TRUE 1


void* readThreadFunc();
void* sendThreadFunc();
void handle(int signo);
int n, bytes_to_read, sd;
char sbuf[BUFLEN], rbuf[BUFLEN], *bp;
char* log_buffer[8192];
FILE* fp;

int fileIndex = 0;
int log_bool;

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
   -- PROGRAMMER: Mackenzie Craig
   --
   -- INTERFACE: int main (int argc, char **argv)
   --
   -- RETURNS: 0 (int)
   --
   -- NOTES:
   -- The entry point for the client program. This function parses command line arguments, and starts the send
   -- and the receive threads.
   ----------------------------------------------------------------------------------------------------------------------*/
int main (int argc, char **argv) {
        signal(SIGINT, handle);

        int port;
        struct hostent  *hp;
        struct sockaddr_in server;
        char  *host,   **pptr;
        char str[16];
        pthread_t readThread, sendThread;

        switch(argc) {
        case 2:
                host =  argv[1];
                port =  SERVER_TCP_PORT;
                break;
        case 3:
                host =  argv[1];
                port =  atoi(argv[2]);
                break;
        case 4:
                host = argv[1];
                port = atoi(argv[2]);
                log_bool = atoi(argv[3]);
                if (log_bool == 1) {
                        printf("Logging enabled!\n");
                } else {
                        printf("Logging disabled!\n");
                }
                break;
        default:
                fprintf(stderr, "Usage: %s host [port]\n", argv[0]);
                exit(1);
        }

        if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
                perror("Cannot create socket");
                exit(1);
        }

        bzero((char *)&server, sizeof(struct sockaddr_in));
        server.sin_family = AF_INET;
        server.sin_port = htons(port);

        if ((hp = gethostbyname(host)) == NULL) {
                fprintf(stderr, "Unknown server address\n");
                exit(1);
        }

        bcopy(hp->h_addr, (char *)&server.sin_addr, hp->h_length);

        if (connect (sd, (struct sockaddr *)&server, sizeof(server)) == -1) {
                fprintf(stderr, "Can't connect to server\n");
                perror("connect");
                exit(1);
        }

        printf("Made connection to ip: %s\n", hp->h_name);
        pptr = hp->h_addr_list;
        printf("Your IP: %s\n", inet_ntop(hp->h_addrtype, *pptr, str, sizeof(str)));

        pthread_create (&readThread, NULL, readThreadFunc, NULL);
        pthread_create (&sendThread, NULL, sendThreadFunc, NULL);

        pthread_join(readThread, NULL);
        pthread_join(sendThread, NULL);

        close (sd);
        return (0);
}

/*------------------------------------------------------------------------------------------------------------------
   -- FUNCTION: readThreadFunc
   --
   -- DATE: April 3, 2018
   --
   -- REVISIONS: (Date and Description)
   -- N/A
   --
   -- DESIGNER: Mackenzie Craig, Haley Booker
   --
   -- PROGRAMMER: Mackenzie Craig
   --
   -- INTERFACE: void* readThreadFunc()
   --
   -- RETURNS: void
   --
   -- NOTES:
   -- This function loops calling recv (blocking). Once data has been received it is printed to the screen.
   ----------------------------------------------------------------------------------------------------------------------*/
void* readThreadFunc() {

        int k = 0;
        char* temp[1024];
        bp = rbuf;
        bytes_to_read = BUFLEN;

        while (TRUE) {
                n = 0;

                while ((n = recv (sd, bp, bytes_to_read, 0)) < BUFLEN)
                {
                        bp += n;
                        bytes_to_read -= n;
                }

                temp[k]= (char *) malloc(strlen(rbuf) + 1);
                memcpy(temp[k], rbuf, strlen(rbuf));

                log_buffer[fileIndex++] = temp[k++];
                printf ("%s", rbuf);
                fflush(stdout);
        }

}

/*------------------------------------------------------------------------------------------------------------------
   -- FUNCTION: sendThreadFunc
   --
   -- DATE: April 3, 2018
   --
   -- REVISIONS: (Date and Description)
   -- N/A
   --
   -- DESIGNER: Mackenzie Craig, Haley Booker
   --
   -- PROGRAMMER: Mackenzie Craig
   --
   -- INTERFACE: void* sendThreadFunc()
   --
   -- RETURNS: void
   --
   -- NOTES:
   -- This function loops calling send after data is entered in the shell. fgets() is the blocking call here.
   ----------------------------------------------------------------------------------------------------------------------*/
void* sendThreadFunc() {
        int k = 0;
        char* temp[1024];

        while (TRUE) {
                fgets (sbuf, BUFLEN, stdin);

                temp[k]= (char *) malloc(strlen(sbuf) + 1);
                memcpy(temp[k], sbuf, strlen(sbuf));
                log_buffer[fileIndex++] = temp[k++];

                send (sd, sbuf, BUFLEN, 0);
        }

        return NULL;
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
   -- PROGRAMMER: Mackenzie Craig
   --
   -- INTERFACE: void handle(int signo)
   --
   -- RETURNS: void
   --
   -- NOTES:
   -- This function logs the chat to the file on close if the log flag was used.
   ----------------------------------------------------------------------------------------------------------------------*/
void handle(int signo) {
        if(signo == SIGINT) {
                if (log_bool == 1) {
                        int i = 0;
                        fp = fopen("log.txt", "a");

                        while (log_buffer[i] != NULL) {
                                fputs(log_buffer[i++], fp);
                        }
                        fclose(fp);
                }
                char eof[2];
                eof[0] = EOF;
                send(sd, eof, BUFLEN, 0);
                kill(getpid(), SIGTERM);
        }
}
