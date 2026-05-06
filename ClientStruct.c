#include "ClientStruct.h"

ClientSocket* InitClientSocket(int client, int epfd){
    int error;
    error = fcntl(client, F_SETFL, O_NONBLOCK);
    if (error == ERROR) {
        perror("fcntl:InitClientSocket");
        close(client);
    }

    struct epoll_event epev;
    epev.events = EPOLLIN;
    epev.data.fd = client;
    error = epoll_ctl(epfd, EPOLL_CTL_ADD, client, (struct epoll_event*)&epev);
    
    ClientSocket* c = malloc(sizeof(ClientSocket));
    memset(c, 0, sizeof(ClientSocket));
    c->socket = client;
    c->socketEndPoint = socket(AF_INET, SOCK_STREAM, 0);
    if (c->socketEndPoint != ERROR) {
        error = fcntl(c->socketEndPoint, F_SETFL, O_NONBLOCK);
        if (error == ERROR) {
            perror("fcntl:InitClientSocket(endpoint)");
            close(c->socketEndPoint);
            c->socketEndPoint = ERROR;
        }
    }
    if(c->socketEndPoint != ERROR){
        struct epoll_event epevEndSocket;
        epevEndSocket.events = 0;
        epevEndSocket.data.fd = c->socketEndPoint;
        error = epoll_ctl(epfd, EPOLL_CTL_ADD, c->socketEndPoint, (struct epoll_event*)&epevEndSocket);
    }
    c->eventsClient = EPOLLIN;
    c->eventsEndPoint = EPOLLIN;
    c->hello = false;
    c->setting = false;
    c->statConnect = C_NOT;


    c->helloContext.state = H_READ_VER;
    c->helloContext.replyState = H_REPLY_VER;
    c->helloContext.VER = 0;
    c->helloContext.NumberMethods = 0;
    c->helloContext.Methods = 0xFF;    
    c->helloContext.readSizeMethods = 0;


    c->settingContext.stateClientRequest = R_READ_VER;
    c->settingContext.stateResponse = R_REPLY_VER;

    c->settingContext.VER = 0;
    c->settingContext.CMD = 0;
    c->settingContext.RSV = 0;

    c->settingContext.DSTPORT = 0;
    c->settingContext.portByte[0] = 0;
    c->settingContext.portByte[1] = 0;
    c->settingContext.sizeReadPortByte = 0;

    c->settingContext.connectPort = 0;
    c->settingContext.connectPortByte[0] = 0;
    c->settingContext.connectPortByte[1] = 0;
    c->settingContext.sizeReplyPortByte = 0;

    c->settingContext.returnStatus = 0x01; 

    AddressSocks5* a = &c->settingContext.addr;

    a->StateReadAddr  = A_READ_TYPE;
    a->StateReplyAddr = A_REPLY_TYPE;
    a->StateIPv4      = WAIT;

    a->TYPE = 0;

    memset(a->IPv4, 0, 4);
    a->lengthReadIP = 0;

    a->lengthDomainName = 0;
    a->lengthReadDomainName = 0;
    memset(a->domainName, 0, 255);

    memset(a->ReplyIPv4, 0, 4);
    a->lengthReplyIP = 0;


    c->IdDNS[0] = 0;
    c->IdDNS[1] = 0;

    memset(c->bufForClient, 0 , 512);
    memset(c->bufForEndPoint, 0, 512);
    c->sizeBufForClient = 0;
    c->sizeBufForEndPoint = 0;
    c->last_activity = time(NULL);
    return c;
}

ClientSocket** addClient(ClientSocket** clients, int client, int epfd, int NewSizeClients){
    /* Ёмкостью буфера управляет вызывающая сторона (main). */
    clients[NewSizeClients-1] = InitClientSocket(client, epfd);
    return clients;
}

ClientSocket** DeleteClient(ClientSocket** clients, int numberClient, int epfd, int* SizeClients)
{
    int lastIndex = *SizeClients - 1;

    ClientSocket* c = clients[numberClient];

    if (c->socket >= 0) {
        epoll_ctl(epfd, EPOLL_CTL_DEL, c->socket, NULL);
        close(c->socket);
    }

    if (c->socketEndPoint >= 0) {
        epoll_ctl(epfd, EPOLL_CTL_DEL, c->socketEndPoint, NULL);
        close(c->socketEndPoint);
    }

    free(c);

    if (numberClient != lastIndex) {
        clients[numberClient] = clients[lastIndex];
    }

    (*SizeClients)--;

    /* Ёмкость буфера не уменьшаем — это работа main (буфер всё равно
       освобождается при завершении программы). */
    return clients;
}


int SearchClient(ClientSocket** clients, int client, int sizeClients){
    for(int i = 0; i < sizeClients; i++){
        if(clients[i]->socket == client){
            return i;
        }
        if(clients[i]->socketEndPoint == client){
            return i;
        }
    }
    return ERROR;
}