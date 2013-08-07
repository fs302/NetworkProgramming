/* receiver.c */
#include <netinet/in.h>  // for sockaddr_in
#include <sys/types.h>   // for socket
#include <sys/socket.h>  // for socket
#include <stdio.h>       // for printf
#include <stdlib.h>      // for exit
#include <string.h>      // for bzero
#include <time.h>

#define SERVER_PORT 6001 
#define CLIENT_PORT 6155
#define BUFFER_SIZE 512 
#define FILE_NAME_MAX_SIZE 512
#define MAX_PACKET_NUM 256
#define IP "127.0.0.1"

typedef struct
{
    int dataID,dataLength;      // dataID for data Packet or ack Packet; dataLength = sizeof(data)
    int flag;                   // -1:ack; 0:not written; 1:written;
    char data[BUFFER_SIZE];
} Packet;

int client_socket;
struct sockaddr_in server_addr, client_addr;
socklen_t slen = sizeof(server_addr);
Packet recvWindow[MAX_PACKET_NUM];

int initConnection()
{
    bzero(&client_addr, sizeof(client_addr));
    client_addr.sin_family = AF_INET;
    client_addr.sin_addr.s_addr = htons(INADDR_ANY);
    client_addr.sin_port = htons(CLIENT_PORT);
    
    client_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (client_socket < 0) {
        printf("Create Client Socket Failed.\n");
        exit(1);
    }
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    if ( inet_aton(IP, &server_addr.sin_addr) == 0){
        printf("Server IP Address Error.\n");
        exit(1);
    }
    server_addr.sin_port = htons(SERVER_PORT); 
    return 0;
}

int Sendto(char *buffer, int length)
{
    int nSendBytes = sendto(client_socket, buffer, length, 0, (struct sockaddr*)&server_addr, slen);
    if ( nSendBytes <= 0)
    {
        printf("Send error with result: %d\n", nSendBytes);
        return -1;
    }
    return 0;
}

int Recvfrom(char *buffer, int length)
{
    int ret = recvfrom(client_socket, buffer, length, 0, (struct sockaddr*)&server_addr, &slen);
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

int ShackHands(char *file_name, FILE *fp)
{
    Packet fnpack;
    fnpack.dataID = -1;
    strncpy(fnpack.data, file_name, strlen(file_name));
    fnpack.dataLength = strlen(file_name);
    fnpack.flag = 0;
    
    printf("Sending request ...\n");
    Sendto((char *)&fnpack, sizeof(Packet));
    
    Packet ack;
    Recvfrom((char *)&ack, sizeof(Packet));
    if (ack.dataID == -2){
        printf("File not found.\n");
        return -2;
    }
    else{
        fp = fopen(file_name, "wb");
        if (NULL == fp){
            printf("File:\t %s can not open to write.\n",file_name);exit(1);
        }
        ack.dataID = 0;
        Sendto((char *)&ack, sizeof(Packet));
    }
    return 1;
}

int FileReceive(FILE *fp) {
    int rvwd = 4; // Static
    int recvfile = 0, FileNotEnd = 1, Nid = 0;
    while(FileNotEnd)
    {
        int i;
        for(i=0;i<rvwd;i++)
            bzero(&recvWindow[i], sizeof(Packet));
        while(recvfile < rvwd && readable_timeo(client_socket, 5)>0)
        {
           Packet Contentpack; 
           Recvfrom((char *)&Contentpack, sizeof(Packet));
           Contentpack.flag = -1;
           Sendto((char *)&Contentpack, sizeof(Packet));
           int newflag = 1;
           for(i=0;i<recvfile;i++)
           {
                if (Contentpack.dataID==recvWindow[i].dataID)
                    newflag = 0;
           }
           if (newflag)
           {
               recvWindow[recvfile++] = Contentpack;
               if (strncmp(Contentpack.data, "*EOF*", 5) == 0){
                   FileNotEnd = 0;
                   rvwd = recvfile;
                   break;
               }
           }

        }
        int id = Nid;
        for(id = Nid;id < Nid+rvwd;id++)
        {
            for(i=0;i<rvwd;i++)
            {
                if (recvWindow[i].dataID = id){
                    int write_len = fwrite(recvWindow[i].data, sizeof(char), recvWindow[i].dataLength, fp);
                    if (write_len < recvWindow[i].dataLength){
                        printf("Write failed.\n");
                        exit(1);
                    }
                    break;
                }
            }
        }
        Nid += rvwd;
        recvfile = 0;
    }
}

int main(int argc, char *argv[])
{
    initConnection();
    
    char file_name[FILE_NAME_MAX_SIZE+1];
    bzero(file_name, FILE_NAME_MAX_SIZE+1);
    printf("Please input File Name on Server: ");
    scanf("%s",file_name);

    FILE *fp;
    if ( ShackHands(file_name, fp)){
        printf("Receving...\n"); 
        FileReceive(fp);
        fclose(fp);
        printf("Receive File:\t %s From Server [%s] Finished.\n", file_name, IP);
    }
    
    return 0;
}
