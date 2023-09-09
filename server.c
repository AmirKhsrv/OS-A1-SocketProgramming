#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <time.h>

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

#define ROOM_CAPACITY 3

struct Room
{
    bool active;
    char roomType;
    int peopleCount;
    int peopleFd[ROOM_CAPACITY];
    int questionCount;
    int answerCount;
    int toAnswer;
    int currentAsk;
};

int toOrder[255];
int toOrderCount = 0;
struct Room rooms[1024];
int roomCounts = 0; 

int setupServer(int port) 
{
    struct sockaddr_in address;
    int server_fd;
    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    
    listen(server_fd, 4);

    return server_fd;
}


int acceptClient(int server_socket) 
{
   struct sockaddr_in cli_addr;
    socklen_t clilen;

    clilen = sizeof(cli_addr);
    int newsockfd = accept(server_socket, (struct sockaddr *) &cli_addr, &clilen);
    return newsockfd;
}


int client_fd[50]; 
int client_count = 0; 
char buffer[1024];
int buf_idx; 

void showRoomsForClient(int clientFd)
{
    buf_idx = 19;
    bzero(buffer, 1024);
    strcat(buffer, "Choose the room you want to go:\n 1: computer\n 2: electricity\n 3: civil\n 4: mechanic\n");
    int n = send(clientFd, buffer, strlen(buffer), 0);
}

void handle_group(int roomNumber)
{
	for (int i = 0 ; i < ROOM_CAPACITY ; i++)
	{
		int cur_fd = rooms[roomNumber].peopleFd[i];
		
		bzero(buffer, 1024);
		strcat(buffer, "Room created. You can ask and answer now");
		send(cur_fd, buffer, strlen(buffer), 0);
	}
    char q[2] = {'Q', '\n'};
    send(rooms[roomNumber].peopleFd[0], q, strlen(q), 0);
    rooms[roomNumber].currentAsk = 0;
}

int addClientToRoom(int clientFd, char roomType)
{
    int i = 0;
    for (i = 0; i < roomCounts; i++)
    {
        if (!rooms[i].active || rooms[i].roomType != roomType || rooms[i].peopleCount >= ROOM_CAPACITY)
            continue;
        break;
    }
    rooms[i].active = true;
    rooms[i].peopleFd[rooms[i].peopleCount] = clientFd;
    rooms[i].peopleCount++;
    rooms[i].roomType = roomType;
    if (i == roomCounts)
        roomCounts++;

    char buff[1024];
    strcat(buff, "You are going to connect to port ");
    char portNum[5] = {'8', '0', '0', roomCounts + '0', '\0'};
    strcat(buff, portNum);
    strcat(buff, ". wait for your room to be created.\n\0");
    
    write(clientFd, buff, strlen(buff));

	if (rooms[i].peopleCount == ROOM_CAPACITY)
		handle_group(i);
}

int getClientMessage(int clientFd)
{
    char clientMessage[1024];
    int n = recv(clientFd, clientMessage, 1024, 0);
    if (n == 0)
        return 0;

    printf("CLIENT: %d: %s\n\n", clientFd, clientMessage);
    if (clientMessage[0] == 'E')
    {
        if (clientMessage[2] != '1' && clientMessage[2] != '2' && clientMessage[2] != '3' && clientMessage[2] != '4')
        {
            bzero(clientMessage, 1024);
            strcat(clientMessage, "invalid room type\n");
            send(clientFd, clientMessage, strlen(clientMessage), 0);
        }
        else
            addClientToRoom(clientFd, clientMessage[2]);
    }
    if (clientMessage[0] == 'Q')
    {
        int i = 0;
        for (i = 0; i < roomCounts; i++)
        {   
            int j = 0;
            for (j = 0; j < rooms[i].peopleCount; j++)
            {
                if (rooms[i].peopleFd[j] == clientFd)
                {
                    rooms[i].currentAsk = j;
                    if (j = 0)
                    {
                        char q[2] = {'A', '\n'};
                        send(rooms[i].peopleFd[1], q, strlen(q), 0);
                        rooms[i].toAnswer = rooms[i].peopleFd[2];
                    }
                    if (j = 1)
                    {
                        char q[2] = {'A', '\n'};
                        send(rooms[i].peopleFd[0], q, strlen(q), 0);
                        rooms[i].toAnswer = rooms[i].peopleFd[2];
                    }
                    if (j = 2)
                    {
                        char q[2] = {'A', '\n'};
                        send(rooms[i].peopleFd[0], q, strlen(q), 0);
                        rooms[i].toAnswer = rooms[i].peopleFd[1];
                    }
                }
            }
        }
    }
    if (clientMessage[0] == 'A')
    {
        int i = 0;
        for (i = 0; i < roomCounts; i++)
        {   
            int j = 0;
            for (j = 0; j < rooms[i].peopleCount; j++)
            {
                if (rooms[i].peopleFd[j] == clientFd)
                {
                    if (clientFd == rooms[i].toAnswer)
                    {
                        if (rooms[i].currentAsk < 2)
                        {
                            char q[2] = {'Q', '\n'};
                            send(rooms[i].peopleFd[rooms[i].currentAsk], q, strlen(q), 0);
                            rooms[i].currentAsk++;
                        }
                        else
                        {
                            char q[2] = {'S', '\n'};
                            send(rooms[i].currentAsk, q, strlen(q), 0);
                        }
                    }
                    else
                    {
                        char q[2] = {'A', '\n'};
                        send(rooms[i].toAnswer, q, strlen(q), 0);
                    }
                }
            }
        }
    }
    return 1;
}

int main(int argc, char const *argv[])
{
    int k = 0;
    for (k = 0; k < 1024; k++)
    {
        rooms[k].active = false;
        rooms[k].peopleCount = 0;
        rooms[k].answerCount = 0;
        rooms[k].questionCount = 0;
    }
    int server_socket_fd, newsockfd, port;
    socklen_t clilen;

    port = atoi(argv[1]);
    
    server_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    
    struct sockaddr_in serv_addr; 
    bzero((char *) &serv_addr, sizeof(serv_addr));    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);


    if (bind(server_socket_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
        error("Binding error");
    listen(server_socket_fd,30);



    fd_set current_sockets, ready_sockets;
    FD_ZERO(&current_sockets);
    FD_SET(server_socket_fd, &current_sockets);

    while (1)
    {
        ready_sockets = current_sockets;

    	select(FD_SETSIZE, &ready_sockets, NULL, NULL, NULL);

        for (int i = 0 ; i < FD_SETSIZE ; i++)
        {
            if (FD_ISSET(i, &ready_sockets))
            {
                if (i == server_socket_fd)
                {
                    printf("Connection request recieved.\n");
                    int client_socket = acceptClient(server_socket_fd);
                    client_fd[client_count++] = client_socket;
                    FD_SET(client_socket, &current_sockets);
                    printf("Connection stablished. fd : %d\n", client_socket);
                    printf("Number of clients : %d\n\n", client_count); 
                    showRoomsForClient(client_socket);
                }
                else
                {
                    getClientMessage(i);
                }
            }
        }
    }
    return 0;
}