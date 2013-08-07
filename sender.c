#include <netinet/in.h>  // for sockaddr_in
#include <sys/types.h>   // for socket
#include <sys/socket.h>  // for socket
#include <sys/select.h>  // for select
#include <unistd.h>      // for alarm      
#include <stdlib.h>      // for exit,rand
#include <string.h>      // for bzero
#include <stdio.h>       // for printf
#include <time.h>        // for clock

#define SERVER_PORT 5155
#define BUFFER_SIZE 512 
#define FILE_NAME_MAX_SIZE 512
#define MAX_WINDOW_SIZE 256

typedef struct
{
    int dataID,dataLength;
    int flag;
    char data[BUFFER_SIZE];
} Packet;

int server_socket;
struct sockaddr_in server_addr, client_addr;
socklen_t clen = sizeof(client_addr);
Packet sendwindow[MAX_WINDOW_SIZE];

int initConnection()
{
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htons(INADDR_ANY);
    server_addr.sin_port = htons(SERVER_PORT);

    server_socket = socket(PF_INET, SOCK_DGRAM, 0);
    if (server_socket < 0)
    {
        printf("Create Server Socket Failed.\n");exit(1);
    }
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)))
    {
        printf("Server Bind Port:%d Failed!\n", SERVER_PORT);exit(1);
    }
    return 0;
}

int Sendto(char *buffer, int length)
{
    int nSendBytes = sendto(server_socket, buffer, length, 0, (struct sockaddr*)&client_addr, clen);
    if ( nSendBytes <= 0)
    {
        printf("Send error with result: %d\n", nSendBytes);
        return -1;
    }
    return 0;
}

int Recvfrom(char *buffer, int length)
{
    int ret = recvfrom(server_socket, buffer, length, 0, (struct sockaddr*)&client_addr, &clen);
    if (ret < 0)
    {
        printf("Recv from Server error\n");
        return -1;
    }
    return 0;
}

int readable_timeo(int fd, int sec)
{
    fd_set rset;
    struct timeval tv;

    FD_ZERO(&rset);
    FD_SET(fd, &rset);

    tv.tv_sec = sec;
    tv.tv_usec = 0;

    return(select(fd+1, &rset, NULL, NULL, &tv));
        /* >0 if descriptor is readable*/
}

int main(int argc, char *argv[])
{
    initConnection();
    for(;;)
    {
        char file_name[FILE_NAME_MAX_SIZE+1];
        if ( WaitShackHands(&file_name) )
        {
            // Begin to transfer file contents
        }
    }
    return 0;
}
