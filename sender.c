/* sender.c */
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
#define MAX_WINDOW_SIZE 128 

typedef struct
{
    int dataID,dataLength;      // dataID for data Packet or ack Packet; dataLength = sizeof(data)
    int flag;                   // -1:ack; 0:not written; 1:written;
    char data[BUFFER_SIZE];
} Packet;

int server_socket;
struct sockaddr_in server_addr, client_addr;
socklen_t clen = sizeof(client_addr);
Packet sendWindow[MAX_WINDOW_SIZE];

int initConnection()
{
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htons(INADDR_ANY);
    server_addr.sin_port = htons(SERVER_PORT); // SERVER_PORT 5155

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

int WaitShackHands(char *file_name, FILE **fp)
{
    printf("Waiting request ...\n");
    Packet fnpack;
    Recvfrom((char *)&fnpack, sizeof(Packet));
    bzero(file_name, FILE_NAME_MAX_SIZE+1);
    if (fnpack.dataID == -1)
        strncpy(file_name, fnpack.data, fnpack.dataLength>FILE_NAME_MAX_SIZE?FILE_NAME_MAX_SIZE:fnpack.dataLength);
    printf("File name: %s\n",file_name);
    *fp = fopen(file_name, "rb");
    if (NULL == *fp)
    {
        printf("File:\t %s is not found.\n",file_name);
        fnpack.dataID = -2;
        Sendto((char *)&fnpack, sizeof(Packet));
        return -2;
    }
    printf("File:\t %s Open.\n",file_name);
    fnpack.dataID = -1;
    Sendto((char *)&fnpack, sizeof(Packet));
    return 1;
}

int readable_timeo(int fd, int sec, int usec)
{
    fd_set rset;
    struct timeval tv;

    FD_ZERO(&rset);
    FD_SET(fd, &rset);

    tv.tv_sec = sec;
    tv.tv_usec = usec;

    return(select(fd+1, &rset, NULL, NULL, &tv));
        /* >0 if descriptor is readable*/
}

int Transfer(FILE **fp)
{
    int cnwd = 1; // Initialize the sending window size
    int recvack = 0, FileNotEnd = 1, file_block_length = 0, Nid = 0;
    char buffer[BUFFER_SIZE];
    printf("Begin transfer.\n");
    while(FileNotEnd)
    {
        int i;
        int losepack = 0;
        // Step1: Fill in Packets
        for(i = 0;i < cnwd;i++)
        {
            bzero(buffer, BUFFER_SIZE);
            file_block_length = fread(buffer, sizeof(char), BUFFER_SIZE, *fp);
            if (file_block_length < 0){
                printf("Read file buffer error.\n");
                return -1;
            }
            else if (file_block_length == 0){
                FileNotEnd = 0;
                Packet EndSig;
                EndSig.dataID = Nid++;
                strncpy(EndSig.data, "*EOF*", 5);
                EndSig.dataLength = strlen(EndSig.data);
                EndSig.flag = 0;
                sendWindow[i] = EndSig;
                cnwd = i+1;
                break;
            }
            Packet Contentpack;
            Contentpack.dataID = Nid++;
            Contentpack.dataLength = file_block_length;
            Contentpack.flag = 0;
            memcpy(Contentpack.data, buffer, BUFFER_SIZE);
            sendWindow[i] = Contentpack;
        }
        // Step2: Sending & Recv ack
        while(recvack<cnwd)
        {
            for(i = 0;i < cnwd;i++)
            {
                if (sendWindow[i].flag == 0){
                    Sendto((char *)&sendWindow[i], sizeof(Packet));
                    //printf("Sending ID:%d\n",sendWindow[i].dataID);
                }
            }
            int timeo = -1;
            while ( (timeo = readable_timeo(server_socket, 0, 50000) > 0) && (recvack < cnwd))
            {
                Packet ack;
                Recvfrom((char *)&ack, sizeof(Packet));
                for(i = 0;i < cnwd;i++)
                    if ( (sendWindow[i].dataID == ack.dataID) && (sendWindow[i].flag==0) ){
                        sendWindow[i].flag = -1;
                        //printf("Recv ACK:%d\n",sendWindow[i].dataID);
                        recvack++;
                        break;
                        }
            }
            if ( (timeo <= 0) && (recvack<cnwd) )
            {
                printf("Losepack happend.\n");
                losepack = 1;
            }
        }
        if (losepack==0 && cnwd<MAX_WINDOW_SIZE)
            cnwd *= 2;
        else if (losepack==1 && cnwd>1)
            cnwd = 1;

        recvack = 0;
    }
    return 0;
}
int main(int argc, char *argv[])
{
    initConnection();
    for(;;)
    {
        char file_name[FILE_NAME_MAX_SIZE+1];
        FILE *fp = NULL;
        if ( WaitShackHands(file_name, &fp)>0 )
        {
            Packet req;
            Recvfrom((char *)&req, sizeof(Packet));
            if ( req.dataID == 0 && req.flag ==-1)
                Transfer(&fp);
            printf("Transfer finished.\n");
        }
    }
    return 0;
}
