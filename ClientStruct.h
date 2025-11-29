#ifndef CLIENT_STRUCT_H
#define CLIENT_STRUCT_H

#include <stdio.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <sys/types.h>  
#include <netinet/in.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#define ERROR -1
#define METHODS_UNSET 0xFF

#define ERROR -1
#define SUCCES 0

typedef enum {
    C_NOT,
    C_WAIT,
    C_DONE
}StatConnect;

typedef enum {
    A_READ_TYPE,
    A_READ_IPv4,
    A_READ_lengthDomainName,
    A_READ_domainName,
    A_DONE
}StateReadAddr;

typedef enum {
    A_REPLAY_TYPE,
    A_REPLAY_IPv4,
    A_RDONE
}StateReplyAddr;

typedef enum {
    WAIT,
    DNSWAIT,
    DONE
}StateIPv4;

typedef struct SocksSettings
{
    StateReadAddr StateReadAddr;
    StateReplyAddr StateReplyAddr;
    StateIPv4 StateIPv4;
    uint8_t TYPE;
    uint8_t IPv4[4];
    uint8_t lengthReadIP;
    uint8_t lengthDomainName;
    uint8_t domainName[255];
    uint8_t lengthReadDomainName;
    
    uint8_t ReplayIPv4[4];
    uint8_t lengthReplayIP;
    
}AddressSocks5;

typedef enum {
    H_READ_VER,
    H_READ_NM,
    H_READ_METHODS,
    H_DONE
} HelloState;

typedef enum {
    H_REPLY_VER,
    H_REPLY_METHODS,
    H_DONEREPLY
} HelloReplyState;

typedef struct HelloContext{
    HelloState state;
    HelloReplyState replyState;
    uint8_t VER;
    uint8_t NumberMethods;
    uint8_t Methods;
    int readSizeMerhods;
}HelloContext;

typedef enum {
    R_READ_VER,
    R_READ_CMD,
    R_READ_RSV,
    R_READ_ADDR,
    R_READ_DSTPORT,
    R_DONE
}StateClientRequest;

typedef enum {
    R_REPLY_VER,
    R_REPLY_STATE,
    R_REPLY_RSV,
    R_REPLY_ADDR,
    R_REPLY_PORT,
    R_RDONE
}StateResponse;

typedef struct SettingContext
{
    StateClientRequest stateClientRequest;
    StateResponse stateResponse;
    uint8_t VER;
    uint8_t CMD;
    uint8_t RSV;
    AddressSocks5 addr;

    uint16_t DSTPORT; 
    uint8_t portBait[2];
    uint8_t sizeReadPortBait; 
    
    uint8_t connectPort;
    uint8_t connectPortBait[2];
    uint8_t sizeReplyPortBait; 

    uint8_t returnStatus;

}SettingContext;

typedef struct ClientSocket
{
    int socket;
    int socketEndPoint;
    uint32_t eventsClient;
    uint32_t eventsEndPoint;
    bool hello;
    HelloContext helloContext;

    bool setting;
    SettingContext settingContext;
    uint8_t IdDNS[2];
    StatConnect statConnect;

    uint8_t bufForClietn[512];
    uint8_t bufForEndPoint[512];
    int sizeBufForClietn;
    int sizeBufForEndPoint;

    time_t last_activity;
}ClientSocket;

ClientSocket* InitClientSocket(int client, int epfd);

ClientSocket** addClient(ClientSocket** clients, int client, int epfd, int NewSizeClients);

ClientSocket** DeliteClient(ClientSocket** clients, int numberClient, int epfd, int* SizeClients);

int SearchClient(ClientSocket** clients, int client, int sizeClients);

#endif