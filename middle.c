/* middle.c */

#include <netinet/in.h>  // for sockaddr_in
#include <sys/types.h>   // for socket
#include <sys/socket.h>  // for socket
#include <sys/select.h>  // for select
#include <sys/time.h>    // for setitimer
#include <unistd.h>      
#include <signal.h>
#include <stdlib.h>      // for exit,rand
#include <string.h>      // for bzero
#include <stdio.h>       // for printf
#include <time.h>        // for clock
#include <pthread.h>

#define SERVER_PORT 5155
#define CLIENT_PORT 6155
#define S_PORT 6000
#define C_PORT 6001
#define BUFFER_SIZE 512 
#define FILE_NAME_MAX_SIZE 512
#define IP "127.0.0.1"
#define BAND_WIDTH 2200000

typedef struct
{
    int dataID,dataLength;
    int flag;
    char data[BUFFER_SIZE];
} Packet;  

int s_socket, c_socket;
struct sockaddr_in s_addr,server_addr;
struct sockaddr_in c_addr,client_addr;
socklen_t cn = sizeof(client_addr), sn = sizeof(server_addr);
int toC_Buf, toS_Buf, BandtoC, BandtoS, Ci, Si, maxi;
int BC[255] ,BS[255];

int max(int a, int b)
{
    return a>b?a:b;
}

int initConnection()
{
    // Create to_server_socket
    bzero(&s_addr, sizeof(s_addr));
    s_addr.sin_family = AF_INET;
    s_addr.sin_addr.s_addr = htons(INADDR_ANY);
    s_addr.sin_port = htons(S_PORT);

    s_socket = socket(PF_INET, SOCK_DGRAM, 0);
    if (s_socket < 0)
    {
        printf("Create Socket Failed!");
        exit(1);
    }
    if (bind(s_socket,(struct sockaddr*)&s_addr,sizeof(s_addr))<0)
    {
        printf("Server Bind Port:%d Failed!",S_PORT);
        exit(1);
    }

    bzero(&server_addr,sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    if (inet_aton(IP, &server_addr.sin_addr) == 0)
    {
        printf("Server IP Address Error.\n");
        exit(1);
    }
    server_addr.sin_port = htons(SERVER_PORT);

    // Create to_client_socket
    bzero(&c_addr, sizeof(c_addr));
    c_addr.sin_family = AF_INET;
    c_addr.sin_addr.s_addr = htons(INADDR_ANY);
    c_addr.sin_port = htons(C_PORT);

    c_socket = socket(PF_INET, SOCK_DGRAM, 0);
    if (c_socket < 0)
    {
        printf("Create Socket Failed!");
        exit(1);
    }
    if (bind(c_socket,(struct sockaddr*)&c_addr,sizeof(c_addr))<0)
    {
        printf("Client Bind Port:%d Failed!",C_PORT);
        exit(1);
    }

    bzero(&client_addr, sizeof(client_addr));
    client_addr.sin_family = AF_INET;
    if (inet_aton(IP, &client_addr.sin_addr) == 0)
    {
        printf("Client IP Address Error.\n");
        exit(1);
    }
    client_addr.sin_port = htons(CLIENT_PORT);
    return 0;
}

int sendtoC(char *buffer, int length)
{
    int nSendBytes = sendto(c_socket, buffer, length, 0, (struct sockaddr*)&client_addr, cn);
    if (nSendBytes <= 0)
    {
        printf("send error with result %d\n", nSendBytes);
        return -1;
    }
    return 0;
}

int sendtoS(char *buffer, int length)
{
    int nSendBytes = sendto(s_socket, buffer, length, 0, (struct sockaddr*)&server_addr, sn);
    if (nSendBytes <= 0)
    {
        printf("send error with result %d\n", nSendBytes);
        return -1;
    }
    return 0;
}

void clean() {
    toC_Buf = 0;
    toS_Buf = 0;
}

int SetTimer(int sec)
{
    struct itimerval t;
    t.it_value.tv_sec = 1;
    t.it_value.tv_usec = 0;
    t.it_interval.tv_sec = sec;
    t.it_interval.tv_usec = 0;

    int ret = setitimer(ITIMER_REAL, &t, NULL);
    if (ret != 0)
    {
        printf("Set timer error.\n");
        return -1;
    }
}

int main(int argc, char *argv[])
{
    int i,maxfdp1;
    fd_set readfd;
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 50000;
    struct timeval Ntime,Ptime;

    srand((unsigned)time(NULL));
    initConnection();
    gettimeofday(&Ntime,NULL);
    Ptime = Ntime;
    toC_Buf = 0;
    toS_Buf = 0;
    Ci = 0;
    Si = 0;
    freopen("Bandwidth.txt","r",stdin);
    scanf("%d",&maxi);
    for(i = 0;i < maxi;i++)
    {
        scanf("%d",&BC[i]);
        BS[i] = BC[i];
    }
    freopen("CON","r",stdin);
    while(1)
    {
        int ret;
        // Get request from client - (1)filename
        Packet pack;
        FD_ZERO(&readfd);
        FD_SET(c_socket, &readfd);
        FD_SET(s_socket, &readfd);
        maxfdp1 = max(c_socket, s_socket) + 1;
        ret = select(maxfdp1, &readfd, NULL, NULL, &timeout);
        if(FD_ISSET(c_socket, &readfd))
        {
            ret = recvfrom(c_socket,&pack,sizeof(Packet),0,(struct sockaddr*)&client_addr,&cn);
            if (ret < 0)
            {
                printf("Receive Data from client failed.\n");
                break;
            }
            int newz = toS_Buf+pack.dataLength;
            if (newz<=BandtoS)
            {
                sendtoS((char *)&pack, sizeof(Packet));
                toS_Buf = newz;
            }
        }
        if(FD_ISSET(s_socket, &readfd))
        {
            ret = recvfrom(s_socket, &pack, sizeof(Packet), 0, (struct sockaddr*)&server_addr, &sn);
            //printf("DataID:%d\n",pack.dataID);
            if (ret < 0)
            {
                printf("Server Recieve Data Failed!\n");
                break;
            }
            int newz = toC_Buf+pack.dataLength;
            if (newz <= BandtoC)
            {
                sendtoC((char *)&pack, sizeof(Packet));
                toC_Buf = newz; 
            }
        }
        gettimeofday(&Ntime,NULL);
        if ((double)((Ntime.tv_sec-Ptime.tv_sec)*1000000.0+Ntime.tv_usec-Ptime.tv_usec)/1000000.0-1.0 > 0)
        {
            Ptime = Ntime;
            toC_Buf = 0;
            toS_Buf = 0;
            Ci = (Ci+1)%maxi;
            BandtoC = BC[Ci];
            Si = (Si+1)%maxi; 
            BandtoS = BS[Si];
            printf("%d\n",BandtoS);
        }
    }

   exit(0);
}
