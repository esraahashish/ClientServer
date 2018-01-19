/*
** server.c -- a stream socket server demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <pthread.h>
#define PORT "3490"  // the port users will be connecting to
#define MAXDATASIZE 10 // max number of bytes we can get at once
#define BACKLOG 10     // how many pending connections queue will hold

using namespace std;


int send_image(int socket)
{

    FILE *picture;
    int size, read_size, stat, packet_index;
    char send_buffer[10240], read_buffer[256];
    packet_index = 1;

    picture = fopen("capture.png", "r");
    printf("Getting Picture Size\n");

    if(picture == NULL)
    {
        printf("Error Opening Image File");
    }

    fseek(picture, 0, SEEK_END);
    size = ftell(picture);
    fseek(picture, 0, SEEK_SET);
    printf("Total Picture size: %i\n",size);

    //Send Picture Size
    printf("Sending Picture Size\n");
    send(socket, (void *)&size, sizeof(int),0);

    //Send Picture as Byte Array
    printf("Sending Picture as Byte Array\n");

    do   //Read while we get errors that are due to signals.
    {
        stat=recv(socket, &read_buffer , 255 , 0);
        printf("Bytes read: %i\n",stat);
    }
    while (stat < 0);

    printf("Received data in socket\n");
    printf("Socket data: %c\n", read_buffer);

    while(!feof(picture))
    {
        //while(packet_index = 1){
        //Read from the file into our send buffer
        read_size = fread(send_buffer, 1, sizeof(send_buffer)-1, picture);

        //Send data through our socket
        do
        {
            stat = send(socket, send_buffer, read_size, 0);
        }
        while (stat < 0);

        printf("Packet Number: %i\n",packet_index);
        printf("Packet Size Sent: %i\n",read_size);
        printf(" \n");
        printf(" \n");
        packet_index++;

        //Zero out our send buffer
        bzero(send_buffer, sizeof(send_buffer));
    }
}


int receive_image(int socket)
{ // Start function 

int buffersize = 0, recv_size = 0,size = 0, read_size, write_size, packet_index =1,stat;

char imagearray[10241],verify = '1';
FILE *image;

//Find the size of the image
do{
stat = recv(socket, &size, sizeof(int), 0);
}while(stat<0);

printf("Packet received.\n");
printf("Packet size: %i\n",stat);
printf("Image size: %i\n",size);
printf(" \n");

char buffer[] = "Got it";

//Send our verification signal
do{
stat = send(socket, &buffer, sizeof(int), 0);
}while(stat<0);

printf("Reply sent\n");
printf(" \n");

image = fopen("capture2.png", "w");

if( image == NULL) {
printf("Error has occurred. Image file could not be opened\n");
return -1; }

//Loop while we have not received the entire file yet


int need_exit = 0;
struct timeval timeout = {10,0};

fd_set fds;
int buffer_fd, buffer_out;

while(recv_size < size) {
//while(packet_index < 2){

    FD_ZERO(&fds);
    FD_SET(socket,&fds);

    buffer_fd = select(FD_SETSIZE,&fds,NULL,NULL,&timeout);

    if (buffer_fd < 0)
       printf("error: bad file descriptor set.\n");

    if (buffer_fd == 0)
       printf("error: buffer read timeout expired.\n");

    if (buffer_fd > 0)
    {
            do{
               read_size = recv(socket,imagearray, 10241, 0);
            }while(read_size <0);

            printf("Packet number received: %i\n",packet_index);
            printf("Packet size: %i\n",read_size);


        //Write the currently read data into our image file
         write_size = fwrite(imagearray,1,read_size, image);
         printf("Written image size: %i\n",write_size); 

             if(read_size != write_size) {
                 printf("error in read write\n");   
             }


             //Increment the total number of bytes read
             recv_size += read_size;
             packet_index++;
             printf("Total received image size: %i\n",recv_size);
             printf(" \n");
             printf(" \n");
    }

}


  fclose(image);
  printf("Image successfully Received!\n");
  return 1;
  }





void * handleClientRequests(void * childFd)
{
            int *new_fd = (int *) childFd;
            while(1)
            {
                    printf("in child thread\n" );

                char buf[MAXDATASIZE];
                int numbytes;

                /*timout interval*/
                struct timeval tv;
                fd_set readfds;
                tv.tv_sec = 10;
                tv.tv_usec = 500000;
                FD_ZERO(&readfds);
                FD_SET(*new_fd, &readfds);
                // don't care about writefds and exceptfds:
                select(*new_fd+1, &readfds, NULL, NULL, &tv);
                if (FD_ISSET(*new_fd, &readfds))
                {
                    printf("A data  was sent from client!\n");
                    if ((numbytes = recv(*new_fd, buf, MAXDATASIZE-1, 0)) == -1)
                    {
                        perror("recv:::");
                        exit(1);
                    }
                    buf[numbytes] = '\0';
                    if(numbytes > 0)
                    {
                        printf("server received '%s'\n",buf);
                    }
                    else
                    {
                        printf("client closed the connection\n");
                        close(*new_fd);
                        pthread_exit(NULL);
                    }
                    
                    if (send(*new_fd, "ok 200!\n", 8, 0) == -1)
                    {
                        perror("send");
                    }

                }
                else
                {
                    printf("Timed out.\n");
                    close(*new_fd);
                    pthread_exit(NULL);
                }
            /*send_image(*new_fd);
            close(*new_fd);
            pthread_exit(NULL);*/
            }
}







void sigchld_handler(int s)
{
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;

    while(waitpid(-1, NULL, WNOHANG) > 0);

    errno = saved_errno;
}


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(void)
{
    int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    struct sigaction sa;
    int yes=1;
    char s[INET6_ADDRSTRLEN];
    int rv;
    int sockets[BACKLOG + 1];
    pthread_t threads[BACKLOG];
    int next_thread = 0;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next)
    {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                             p->ai_protocol)) == -1)
        {
            perror("server: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                       sizeof(int)) == -1)
        {
            perror("setsockopt");
            exit(1);
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        break;
    }

    freeaddrinfo(servinfo); // all done with this structure

    if (p == NULL)
    {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }

    if (listen(sockfd, BACKLOG) == -1)
    {
        perror("listen");
        exit(1);
    }

    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1)
    {
        perror("sigaction");
        exit(1);
    }

    printf("server: waiting for connections...\n");

    while(1)    // main accept() loop
    {
        printf("server is searching for more connections\n");
        sin_size = sizeof their_addr;
        sockets[next_thread] = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (sockets[next_thread] == -1)
        {
            perror("accept");
            continue;
        }

        inet_ntop(their_addr.ss_family,
                  get_in_addr((struct sockaddr *)&their_addr),
                  s, sizeof s);
        printf("server: got connection from %s\n", s);

        if (next_thread < BACKLOG) {
            int status = pthread_create(&threads[next_thread], NULL,
                    &handleClientRequests, &sockets[next_thread]);
            if (status != 0) {
                fprintf(stderr, "Error creating thread\n");
                return -1;
            }
            next_thread++;
            // close(new_fd);
        } else {

            next_thread = 0;
            if (pthread_join(threads[next_thread], NULL)) {
                fprintf(stderr, "Error joining thread\n");
                return -2;
            }
            sockets[next_thread] = sockets[BACKLOG];
            int status = pthread_create(&threads[next_thread], NULL,
                    &handleClientRequests, &sockets[next_thread]);
            if (status != 0) {
                fprintf(stderr, "Error creating thread\n");
                return -1;
            }
       }

    }
    return 0;
}
