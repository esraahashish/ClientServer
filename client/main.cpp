/*
** client.c -- a stream socket client demo
*/
#include <stdio.h>
#include <stdlib.h>
#include <bits/stdc++.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <fstream>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <pthread.h>
#include <arpa/inet.h>
#define PORT "3490" // the port client will be connecting to
#define MAXDATASIZE 200 // max number of bytes we can get at once
using namespace std;
// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
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

void post_server(char* data, int sockfd, int imageLen)
{

    int data_len  = 0;
    cout<< "Client is sending: " << data << endl ;
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

// read image byte by byte and send it to the server
int send_image(int socket , string path)
{



    FILE *picture;
    int read_size;
    char send_buffer[MAXDATASIZE];

    picture = fopen(path.c_str(), "r");

    if(picture == NULL)
    {
        printf("Error Opening Image File");
    }

    while(!feof(picture))
    {
        //Read from the file into our send buffer
        read_size = fread(send_buffer, 1, sizeof(send_buffer)-2, picture);
        send_buffer[read_size] = '\0';
        post_server(send_buffer , socket, read_size);

        //Zero out our send buffer
        bzero(send_buffer, sizeof(send_buffer));
    }
    fclose(picture);
}





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
                    post_server(buf,sockfd,-1);
                    i = 0;
                    buf[i] = c;
                    i++;

                }

            }
            if(i != 0)
            {
                buf[i] = '\0';
                post_server(buf,sockfd,-1);
                free(buf);
            }
            myfile.close();
        }

    }
    else
    {
        send_image(sockfd , path);
    }

}

//read commands from file
void read_commands(int sockfd, char *buf)
{
    string line ;
    string request_type ="";
    string file_name = "";
    ifstream myfile ("example.txt");


    if (myfile.is_open())
    {
        while ( getline (myfile,line) )
        {
            cout << "Current Command: "<< line << endl;
            char str_copy[line.size()];
            strcpy(str_copy,line.c_str());
            vector<string> tokens = split(str_copy," ");
            request_type = tokens[0];
            file_name = tokens[1];

            if(request_type.compare("GET") == 0)
            {
                int recv_corr;
                if (send(sockfd, line.c_str() , line.size() , 0) == -1)
                    perror("send");

                FILE* write_file ;

                bool ack = true;
                while(true)
                {
                    if ((recv_corr = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1)
                    {
                        perror("recv");
                        exit(1);
                    }
                    buf[recv_corr] = '\0';

                    if(buf[recv_corr-4] == '\\' && buf[recv_corr-3] == 'r' && buf[recv_corr-2] == '\\' &&
                            buf[recv_corr-1] == 'n')
                    {
                        fwrite(buf,sizeof(char),recv_corr-4,write_file);
                        fclose(write_file);
                        break;
                    }

                    if(!ack)
                    {
                        fwrite(buf,sizeof(char),recv_corr,write_file);
                    }
                    else
                    {
                        if(buf[0] == '4')
                        {
                            printf("client: received data: %s\n",buf);
                            break;
                        }
                        else
                        {
                            write_file = fopen(file_name.c_str(),"w");
                        }
                        ack = false;
                    }
                    printf("client: received data: %s\n",buf);
                }
            }
            else
            {
                int recv_corr ;
                if ( recv_corr = send(sockfd, line.c_str() , line.size()+1, 0) == -1)
                    perror("send");

                if ((recv_corr = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1)
                {
                    perror("recv");
                    exit(1);
                }
                buf[recv_corr] = '\0';
                printf("client: received %s\n",buf);

                read_file(file_name, sockfd);

                if (send(sockfd,"\\r\\n",4, 0) == -1)
                    perror("send");

                sleep(1);

            }

        }

        myfile.close();

    }
}


int main(int argc, char *argv[])
{
    int sockfd;
    char buf[MAXDATASIZE];
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];
    if (argc != 2)
    {
        fprintf(stderr,"usage: client hostname\n");
        exit(1);
    }
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    if ((rv = getaddrinfo(argv[1], PORT, &hints, &servinfo)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }
// loop through all the results and connect to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next)
    {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                             p->ai_protocol)) == -1)
        {
            perror("client: socket");
            continue;
        }
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(sockfd);
            perror("client: connect");
            continue;
        }
        break;
    }
    if (p == NULL)
    {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }
    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
              s, sizeof s);
    printf("client: connecting to %s\n", s);
    freeaddrinfo(servinfo); // all done with this structure

    // read commands
    read_commands(sockfd,buf);
    sleep(7); // test time out
    cout << "end of connection\n";
    close(sockfd);
    return 0;
}
