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
#include <pthread.h>
#define PORT "3490"  // the port users will be connecting to
#define MAXDATASIZE 200 // max number of bytes we can get at once
#define BACKLOG 10    // how many pending connections queue will hold

using namespace std;
int timeOuts[BACKLOG + 1];
struct thread_param {
    int arg1;
    int arg2;
};

void sigchld_handler(int s)
{
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;

    while(waitpid(-1, NULL, WNOHANG) > 0);

    errno = saved_errno;
}

// split function
vector<string> split(char *str,string sep)
{
    char* cstr=const_cast<char*>(str);
    char* current;
    vector<string> arr;
    current=strtok(cstr,sep.c_str());
    while(current!=NULL)
    {
        arr.push_back(current);
        current=strtok(NULL,sep.c_str());
    }
    return arr;
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


void get_server(char* data, int sockfd , int imageLen)
{

    int data_len  = 0;
    cout<< "Server will send: " << data << endl ;
    if(imageLen == -1)
    {
        while(data[data_len] != '\0')
        {
            data_len++;
        }
    }
    else
    {
        data_len = imageLen;
    }

    while(true)
    {
        int recv_corr;
        recv_corr = send(sockfd, data ,data_len , 0);

        if(recv_corr == -1) perror("send");

        else if(recv_corr < data_len)
        {
            int j = 0;
            for( j = 0 ; recv_corr < data_len ; j++ , recv_corr++)
            {
                data[j] = data[recv_corr];
            }

            data[j] = '\0';
            data_len = j;
        }
        else break;
    }

    return;
}

// server is reading data byte by byte and sending it to the client
int send_image(int socket , string path)
{
    FILE *picture;
    int read_size;
    char send_buffer[MAXDATASIZE];
    picture = fopen(path.c_str(), "r");
    if(picture == NULL)
    {
        printf("Error Opening Image File");
        if (send(socket, "404 not found!\n", 15, 0) == -1)
        {
            perror("send");
        }
        return -1;
    }
    else
    {
        if (send(socket, "OK 200!\n", 8, 0) == -1)
        {
            perror("send");
        }
    }

    while(!feof(picture))
    {
        read_size = fread(send_buffer, 1, sizeof(send_buffer)-2, picture);
        send_buffer[read_size] = '\0';
        get_server(send_buffer , socket, read_size);
        bzero(send_buffer, sizeof(send_buffer));
    }
    fclose(picture);
    if (send(socket,"\\r\\n",4, 0) == -1)
        perror("send");

    return 1;
}

// server is parsing the file of the GET request
void read_file(string path, int sockfd)
{
    char* buf = (char*)malloc((MAXDATASIZE+10) * sizeof(char));
    int pathLen = path.size();
    int extension_index = path.find('.');
    string extension = path.substr(extension_index+1,pathLen);

    if(extension.compare("txt")== 0 || extension.compare("html")== 0)
    {
        ifstream myfile (path.c_str());
        char c;
        int i = 0;
        if(myfile.is_open())
        {
            if (send(sockfd, "OK 200!\n", 8, 0) == -1)
            {
                perror("send");
            }
            while (myfile.get(c))
            {
                if(i <= MAXDATASIZE-2)
                {
                    buf[i] = c;
                    i++;
                }
                else
                {
                    buf[i] = '\0';
                    get_server(buf,sockfd,-1);
                    i = 0;
                    buf[i] = c;
                    i++;

                }

            }
            if(i != 0)
            {
                buf[i] = '\0';
                get_server(buf,sockfd,-1);
                free(buf);
            }
            myfile.close();
            if (send(sockfd,"\\r\\n",4, 0) == -1)
                perror("send");
        }
        else
        {
            if (send(sockfd, "404 not found!\n", 15, 0) == -1)
            {
                perror("send");
            }
        }

    }
    else
    {
        if(send_image(sockfd , path) == 1)
        {
            if (send(sockfd,"\\r\\n",4, 0) == -1)
                perror("send");
        }
    }

}

void * handleClientRequests(void * arguments)
{
            struct thread_param *args = (struct thread_param*)arguments;
            int new_fd = args->arg1;
            int timout = args->arg2;
            bool request = true;
            string request_type = "";
            FILE* write_file ;
            string file_name;


            while(1)
            {

                char buf[MAXDATASIZE];
                int numbytes;

                /*timout interval*/
                struct timeval tv;
                fd_set readfds;
                tv.tv_sec = timeOuts[timout];
                tv.tv_usec = 500000;
                FD_ZERO(&readfds);
                FD_SET(new_fd, &readfds);
                // don't care about writefds and exceptfds:
                select(new_fd+1, &readfds, NULL, NULL, &tv);
                if (FD_ISSET(new_fd, &readfds))
                {
                    printf("******A data  was sent from client*******\n");

                    while(true)
                    {
                        if ((numbytes = recv(new_fd, buf, MAXDATASIZE-1, 0)) == -1)
                        {
                            perror("recv:::");
                            pthread_exit(NULL);
                        }
                        buf[numbytes] = '\0';

                        if(numbytes > 0)
                        {
                            printf("server received data: %s\n",buf);

                            if (!request)
                            {
                                if(buf[numbytes-4] == '\\' && buf[numbytes-3] == 'r' && buf[numbytes-2] == '\\' &&
                                        buf[numbytes-1] == 'n')
                                {
                                    request = true;
                                    fwrite(buf,sizeof(char),numbytes-4,write_file);
                                    fclose(write_file);
                                    break;
                                }
                                fwrite(buf,sizeof(char),numbytes,write_file);
                            }
                        }
                        else
                        {
                            printf("client closed the connection :'( \n");
                            close(new_fd);
                            pthread_exit(NULL);
                        }


                        // check if command is request or data
                        if(request)
                        {
                            vector<string> tokens = split(buf," ");
                            if(buf[0] == 'P')
                            {
                                cout << "**request received is POST**"<<endl;
                                request_type = "POST";
                                write_file = fopen(tokens[1].c_str(),"w");
                                request = false;
                            }
                            else
                            {
                                cout << "**request received is GET**"<<endl;
                                file_name = tokens[1];
                                request_type = "GET";
                            }
                            break;
                        }



                    }

                    if(strcmp(request_type.c_str(),"POST") == 0)
                    {
                        if(!request)
                        {
                            if (send(new_fd, "ok 200!\n", 8, 0) == -1)
                            {
                                perror("send");
                            }
                        }

                    }
                    else//get command
                    {

                        read_file(file_name,new_fd);
                        cout<<"DATA IS ON ITS WAY TO THE CLIENT :D"<<endl;

                    }



                }
                else
                {
                    printf("TIME OUT!!!\n");
                    close(new_fd);
                    pthread_exit(NULL);
                }
            }
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
    for(int i = 0 ; i < BACKLOG ; i++)
    {
            timeOuts[i] = BACKLOG*2; 
    }

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
        // update time out interval
        for(int i = 0 ; i <= next_thread ; i++)
        {
            timeOuts[i]--; 
        }
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
            struct thread_param args;
            args.arg1 = sockets[next_thread];
            args.arg2 = next_thread;
            int status = pthread_create(&threads[next_thread], NULL,
                    &handleClientRequests, (void *)&args);
            if (status != 0) {
                fprintf(stderr, "Error creating thread\n");
                return -1;
            }
            next_thread++;
        } else {

            next_thread = 0;
            for(int i = 0 ; i < BACKLOG ; i++){
                if (pthread_join(threads[i], NULL)) {
                    fprintf(stderr, "Error joining thread\n");
                    return -2;
                }
                cout<< " thread : " << i << "has terminated" <<endl;
                timeOuts[i] = BACKLOG*2; 
            }

            sockets[next_thread] = sockets[BACKLOG];
            struct thread_param args;
            args.arg1 = sockets[next_thread];
            args.arg2 = next_thread;
            int status = pthread_create(&threads[next_thread], NULL,
                    &handleClientRequests, (void *)&args);
            if (status != 0) {
                fprintf(stderr, "Error creating thread\n");
                return -1;
            }
            next_thread++;
       }

    }
    return 0;

}
