/*
** server.c -- a stream socket server demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <bits/stdc++.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define PORT "3490"  // the port users will be connecting to
#define MAXDATASIZE 50 // max number of bytes we can get at once
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
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1)
        {
            perror("accept");
            continue;
        }

        inet_ntop(their_addr.ss_family,
                  get_in_addr((struct sockaddr *)&their_addr),
                  s, sizeof s);
        printf("server: got connection from %s\n", s);

        if (!fork())   // this is the child process
        {
            close(sockfd); // child doesn't need the listener


            while(1)
            {

                char buf[MAXDATASIZE];
                int numbytes;
                  string request_type = "";
                bool request = true;

                /*timout interval*/
                struct timeval tv;
                fd_set readfds;
                tv.tv_sec = 10;
                tv.tv_usec = 500000;
                FD_ZERO(&readfds);
                FD_SET(new_fd, &readfds);
                // don't care about writefds and exceptfds:
                select(new_fd+1, &readfds, NULL, NULL, &tv);
                if (FD_ISSET(new_fd, &readfds))
                {
                    printf("A data  was sent from client!\n");

                    while(true)
                    {
                        if ((numbytes = recv(new_fd, buf, MAXDATASIZE-1, 0)) == -1)
                        {
                            perror("recv:::");
                            exit(1);
                        }
                        cout<<"lenofdata"<<numbytes<<endl;
                        buf[numbytes] = '\0';

                        if(numbytes > 0)
                        {
                            printf("server received '%s'\n",buf);
                        }
                        else
                        {
                            printf("client closed the connection\n");
                            close(new_fd);
                            return 0;
                        }


                        // check if command is request or data
                        if(request)
                        {

                            if(buf[0] == 'P')
                            {
                                 cout << "request if post"<<endl;
                                request_type = "POST";
                                request = false;
                            }
                            else
                            {
                             cout << "request if get"<<endl;
                                request_type = "GET";
                            }
                            // printf("server received '%s'\n",buf);
                            break;
                        }

                        if(buf[numbytes-4] == '\\' && buf[numbytes-3] == 'r' && buf[numbytes-2] == '\\' &&
                                buf[numbytes-1] == 'n')
                        {
                            request = true;
                            break;
                        }

                    }
                    printf("after while loop\n");
                    printf(request_type.c_str());
                    if(strcmp(request_type.c_str(),"POST") == 0)
                    {
                         printf("int strcmp for post\n");
                        if (send(new_fd, "ok 200!\n", 8, 0) == -1)
                        {
                            perror("send");
                        }

                    }
                    else//get command
                    {

                    }



                }
                else
                {
                    printf("Timed out.\n");
                    close(new_fd);
                    return 0;
                }
            }
            /*send_image(new_fd);
            close(new_fd);
            return 0;*/
        }
        //close(new_fd);  // parent doesn't need this
    }

    return 0;
}
