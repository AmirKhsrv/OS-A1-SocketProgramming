#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <signal.h>
#include <time.h>
#include <errno.h>

void error(const char *msg)
{
    perror(msg);
    // exit(0);
}

void alarm_handler(int signum)
{ 
    printf("time is up. next client.");
}


int main(int argc, char *argv[])
{
    int sockfd, port;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    char buffer[256];

    port = atoi(argv[1]);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    
    server = gethostbyname("127.0.0.1");

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(port);
    
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
        error("Connection error");


    printf("Connected to server.\n");

    fd_set current_sockets;

    int udp_port, udp_sockfd;



    while (1)
    {
        FD_ZERO(&current_sockets);
        FD_SET(sockfd, &current_sockets); 
        FD_SET(0, &current_sockets);  

        if (select(sockfd + 1, &current_sockets, NULL, NULL, NULL) < 0)
            perror("Select error");
        
        if (FD_ISSET(sockfd, &current_sockets)) 
        {
            if (sockfd == sockfd) 
            {
                bzero(buffer, 255);
                int n = read(sockfd, buffer, 255); 
            
                printf("SERVER: %s\n\n", buffer);

                if (buffer[0] == 'P') 
                {
                    printf("Connecting to UDP Socket. \n");
                    char port_numm[4] = {'8', '0', '0', buffer[26]};
                    udp_port = atoi(port_numm);
                }

                if (buffer[0] =='T')
                {
                    FD_CLR(sockfd, &current_sockets);
                    FD_CLR(0, &current_sockets);
                    break;
                }
            }
        }

        if (FD_ISSET(0, &current_sockets))
        {
            bzero(buffer, 255);
            fgets(buffer, 255, stdin);
            int n = write(sockfd, buffer, strlen(buffer));
        }

    }

    udp_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    
    int broadcast_en = 1, opt_en = 1, reuse_adr_en = 1;
    setsockopt(udp_sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast_en, sizeof(broadcast_en)); 
    setsockopt(udp_sockfd, SOL_SOCKET, SO_REUSEPORT, &opt_en, sizeof(opt_en));
    setsockopt(udp_sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse_adr_en, sizeof(reuse_adr_en));
    
    struct sockaddr_in bc_adr_sendto, bc_adr_recvfrom;

    bc_adr_recvfrom.sin_family = AF_INET;
    bc_adr_recvfrom.sin_addr.s_addr = htonl(INADDR_ANY);
    bc_adr_recvfrom.sin_port = htons(udp_port);

    bc_adr_sendto.sin_family = AF_INET;
    bc_adr_sendto.sin_addr.s_addr = htonl(INADDR_BROADCAST); 
    bc_adr_sendto.sin_port = htons(udp_port);

    
    if (bind(udp_sockfd, (struct sockaddr *) &bc_adr_recvfrom, sizeof(bc_adr_recvfrom)) < 0) 
        error("Binding error");
    

    printf("Connected to the socket.\n\n");

    signal(SIGALRM, alarm_handler); 
    printf("Room created. You can ask and answer now.\n");
    alarm(60);
    while (1)
    {
        signal(SIGALRM, alarm_handler);
        FD_ZERO(&current_sockets);
        FD_SET(udp_sockfd, &current_sockets);
        FD_SET(0, &current_sockets);  

        select(udp_sockfd + 1, &current_sockets, NULL, NULL, NULL);

        if (FD_ISSET(sockfd, &current_sockets))
        {
            bzero(buffer, 255);
                    // fgets(buffer, 255, stdin);
            sendto(udp_sockfd, buffer, strlen(buffer), 0,(struct sockaddr *)&bc_adr_sendto, sizeof(struct sockaddr_in)); 
            if (sockfd == sockfd) 
            {
                bzero(buffer, 255);
                read(sockfd, buffer, 255); 

                if (buffer[0] == 'Q') 
                {
                    strcat(buffer, "I have this question : ");
                    // fgets(buffer, 255, stdin);
                    sendto(udp_sockfd, buffer, strlen(buffer), 0,(struct sockaddr *)&bc_adr_sendto, sizeof(struct sockaddr_in));   
                }
                if (buffer[0] == 'A') 
                {
                    strcat(buffer, "Here is your answer : ");
                    // fgets(buffer, 255, stdin);
                    sendto(udp_sockfd, buffer, strlen(buffer), 0,(struct sockaddr *)&bc_adr_sendto, sizeof(struct sockaddr_in));   
                }
                if (buffer[0] =='S')
                {
                    FD_CLR(sockfd, &current_sockets);
                    FD_CLR(0, &current_sockets);
                    break;
                }
            }
        }

        if (FD_ISSET(0, &current_sockets)) 
        {
            bzero(buffer, 255);
            read(0, buffer, 255);
            sendto(udp_sockfd, buffer, strlen(buffer), 0,(struct sockaddr *)&bc_adr_sendto, sizeof(struct sockaddr_in));   
        }
        
        if (FD_ISSET(udp_sockfd, &current_sockets)) 
        {
            bzero(buffer, 255);
            socklen_t bc_adr_len = sizeof(bc_adr_recvfrom);
            recvfrom(udp_sockfd, buffer, 255, 0, (struct sockaddr *)&bc_adr_recvfrom, &bc_adr_len);
            printf("BROADCASTED MESSAGE: %s\n", buffer);
        }
        alarm(0);
    }
    return 0;
}