#include "Tunel.h"

int TunelSend(ClientSocket* client, int fd, int epfd){
    uint8_t* buf;
    int* len = NULL;
    uint32_t* firstEv;
    uint32_t* secondEv;
    int secondFd;
    if(client->socket == fd){
        buf = client->bufForClietn;
        len = &client->sizeBufForClietn;
        secondEv = &client->eventsEndPoint;
        firstEv = &client->eventsClient;
        secondFd = client->socketEndPoint;
    }
    else{
        buf = client->bufForEndPoint;
        len = &client->sizeBufForEndPoint;
        firstEv = &client->eventsEndPoint;
        secondEv = &client->eventsClient;
        secondFd = client->socket;
    }

    int size  = send(fd, buf, *len, 0);
    if (size < 0 && errno == EAGAIN){
        return SUCCES;
    }
    if(size < 0 && errno == EINPROGRESS){
        return SUCCES;
    }
    if (size <= 0){
        perror("send TUNEL\n");
        return ERROR;
    }
    for(int i = 0; i < 512-size; i++){
        buf[i] = buf[i+size];
    }
    *len -= size;
    if(*len == 0){
        *firstEv &= ~EPOLLOUT; 
    }
    *secondEv |= EPOLLIN;
    modify_epoll_events(epfd, fd, *firstEv);
    modify_epoll_events(epfd, secondFd, *secondEv);
    return SUCCES;
}

int TunelRecv(ClientSocket* client, int fd, int epfd){
    uint8_t buf[512];
    memset(buf, 0, 512);
    uint8_t *secondBuf;
    int len = 512;
    int *secondLen = NULL;
    int secondFd;
    uint32_t* firstEv;
    uint32_t* secondEv;

    if(client->socketEndPoint == fd){
        secondBuf = client->bufForClietn;
        len -= client->sizeBufForClietn;
        secondLen = &client->sizeBufForClietn;
        secondFd = client->socket;
        firstEv = &client->eventsEndPoint;
        secondEv = &client->eventsClient;
    }
    else{
        len -= client->sizeBufForEndPoint;
        secondBuf = client->bufForEndPoint;
        secondLen = &client->sizeBufForEndPoint;
        secondFd = client->socketEndPoint;
        secondEv = &client->eventsEndPoint;
        firstEv = &client->eventsClient;
    }

    int size  = recv(fd, buf, len, 0);
    if (size < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)){
        return SUCCES;
    }
    if (size < 0){
        printf("TunelRecv(fd=%d) state=%d eventsClient=%X eventsEndPoint=%X errno_before=%d\n",
       fd, client->statConnect, client->eventsClient, client->eventsEndPoint, errno);
        return ERROR;
    }
    if (size == 0) {
        return ERROR;
    }
    for(int i = 0; i < size; i++){
        secondBuf[i+*secondLen] = buf[i];
    }
    *secondLen += size;

    *secondEv |= EPOLLOUT;
    modify_epoll_events(epfd, secondFd, *secondEv);
    if(*secondLen == 512){
        *firstEv &= ~EPOLLIN;
        modify_epoll_events(epfd, fd, *firstEv);
    }
    return SUCCES;
}