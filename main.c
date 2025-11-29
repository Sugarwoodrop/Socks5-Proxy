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
#include <signal.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include "SocksSettings.h"
#include "ConnectServer.h"
#include "Tunel.h"

#define EVENTS_SIZE 128
#define BACKLOG 128
#define SOCKET_PORT 1080
#define ERROR -1
#define SUCCES 0
#define ERROR_EXIT 1
#define CLIENT_TIMEOUT 60

volatile sig_atomic_t stateWork = SUCCES;

int initSock(){
    int socket_stream;
    socket_stream = socket(AF_INET, SOCK_STREAM, 0);
    if(socket_stream == ERROR){
        perror("Ошибка при инициализации сокета");
        exit(1);
    }
    
    int reuse = 1;
    setsockopt(socket_stream, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    struct sockaddr_in server_address = {0};

    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(SOCKET_PORT);
    int error;

    error = bind(socket_stream, (struct sockaddr*) &server_address, sizeof(server_address));
    if (error == ERROR) {
        perror("bind");
        close(socket_stream);
        exit(ERROR_EXIT);
    }

    error = listen(socket_stream, BACKLOG);
    if (error == ERROR) {
        perror("listen");
        close(socket_stream);
        exit(ERROR_EXIT);
    }

    error = fcntl(socket_stream, F_SETFL, O_NONBLOCK);
    if (error == ERROR) {
        perror("fcntl");
        close(socket_stream);
        exit(ERROR_EXIT);
    }
    
    return socket_stream;
}

int UDPSock(){
    int socket_stream;
    socket_stream = socket(AF_INET, SOCK_DGRAM, 0);
    if(socket_stream == ERROR){
        perror("Ошибка при инициализации сокета");
        exit(1);
    }

    int error;
    error = fcntl(socket_stream, F_SETFL, O_NONBLOCK);
    if (error == ERROR) {
        perror("fcntl");
        close(socket_stream);
        exit(ERROR_EXIT);
    }
    
    return socket_stream;
}

int initEpoll(int socket_stream, int UDPsock){
  int epfd = epoll_create1(EPOLL_CLOEXEC);
    if (epfd == ERROR) {
        perror("epoll_create1");
        exit(ERROR_EXIT);
    }

    struct epoll_event epev;
    epev.events = EPOLLIN;
    epev.data.fd = socket_stream;

    int error;
    error = epoll_ctl(epfd, EPOLL_CTL_ADD, socket_stream, (struct epoll_event*)&epev);
    if (error == ERROR) {
        perror("epoll_ctl");
        close(epfd);
        exit(ERROR_EXIT);
    }

    struct epoll_event epev2;
    epev2.events = EPOLLIN;
    epev2.data.fd = UDPsock;

    error = epoll_ctl(epfd, EPOLL_CTL_ADD, UDPsock, (struct epoll_event*)&epev2);
    if (error == ERROR) {
        perror("epoll_ctl");
        close(epfd);
        exit(ERROR_EXIT);
    }
    return epfd;
}

void sig_handler(int signo) {
    if(signo == SIGHUP){
        stateWork = 1;
    }
    else{
        stateWork = 2;
    }
}

int main() {
    struct sigaction sa;
    sa.sa_handler = sig_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGHUP, &sa, NULL);

    int socket_stream = initSock();
    int udpSock = UDPSock();
    int epfd = initEpoll(socket_stream, udpSock);

    struct epoll_event events[EVENTS_SIZE];

    ClientSocket** clients = malloc(sizeof(ClientSocket*) * 10);
    int sizeBufClients = 10;
    int sizeClients = 0;
    while(true){
        if(stateWork == 1){
            for(int i = 0; i < sizeClients; i++){
                DeliteClient(clients, i, epfd, &sizeClients);
            }
            printf("Перезапуск ждите 3 минуту до запуска");
            free(clients);
            close(epfd);
            close(socket_stream);
            close(udpSock);
            char *args[] = {"./main", NULL};
            execvp(args[0], args);
            perror("execvp");       
            exit(1);
        }
        else if(stateWork == 2){
            for(int i = 0; i < sizeClients; i++){
                DeliteClient(clients, i, epfd, &sizeClients);
            }
            free(clients);
            close(epfd);
            close(socket_stream);
            close(udpSock);
            printf("Всё освободилось\n");
            exit(0);
        }
        int n_events;
        n_events = epoll_wait(epfd, events, EVENTS_SIZE, -1);
        time_t now = time(NULL);
        for (int i = 0; i < sizeClients; i++) {
            if (now - clients[i]->last_activity > CLIENT_TIMEOUT) {
                printf("Client fd=%d timeout\n", clients[i]->socket);
                DeliteClient(clients, i, epfd, &sizeClients);
                i--;
            }
        }
        int* numerDelite = malloc(n_events*sizeof(int));
        if (n_events == ERROR) {
            perror("epoll_wait");
            close(epfd);
            close(socket_stream);
            exit(ERROR_EXIT);
        }  
        for(int i = 0; i < n_events; i++){
            int fd = events[i].data.fd;
            uint32_t ev = events[i].events;
            int addNumberDelite = 0;
            if(fd == socket_stream){
                int client;
                client = accept(socket_stream, NULL, NULL);
                if (client == ERROR) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        break; 
                    }
                    perror("accept");
                    break;
                }
                sizeClients++;
                if(sizeClients <= sizeBufClients - 5){
                    sizeBufClients += 300;
                    clients = realloc(clients, sizeof(ClientSocket*)*sizeBufClients);
                }
                clients = addClient(clients, client, epfd, sizeClients);
                if(clients[sizeClients-1]->socketEndPoint == ERROR){
                    DeliteClient(clients, sizeClients-1, epfd, &sizeClients);
                    numerDelite[addNumberDelite] = sizeClients-1;
                    addNumberDelite++;
                }
            }
            else if(fd == udpSock){
                int numberClient = RecvDNS(udpSock, clients, sizeClients);
                if(numberClient >= 0){
                    Connect(clients[numberClient], epfd);
                    clients[numberClient]->last_activity = time(NULL);
                }
            }   
            else{
                int numberClient = SearchClient(clients, fd, sizeClients);

                if (numberClient < 0 || numberClient >= sizeClients || clients == NULL) {
                    continue;
                }
                bool flag = false;
                for(int i = 0; i < n_events; i++){
                    if(numerDelite[i] == numberClient){
                        flag = true;
                        break;
                    }
                }
                if(flag){
                    continue;
                }
                if(clients[numberClient]->hello == false && clients[numberClient]->helloContext.state != H_DONE){
                    int error = StartUserVerification(clients[numberClient]);
                    if(error == ERROR){
                        DeliteClient(clients, numberClient, epfd, &sizeClients);
                        numerDelite[addNumberDelite] = numberClient;
                        addNumberDelite++;
                        printf("ОШИБКА В ВЕРЕФЕКАЦИИ %d\n\n", numberClient);
                    }
                    else if (error == 2) {
                        printf("Ok SOCKS5 %d\n\n", numberClient);
                        clients[numberClient]->last_activity = time(NULL);
                        clients[numberClient]->eventsClient = EPOLLIN | EPOLLOUT;
                        modify_epoll_events(epfd, clients[numberClient]->socket, clients[numberClient]->eventsClient);
                    }
                    continue;
                }
                else if(clients[numberClient]->hello == false && clients[numberClient]->helloContext.state == H_DONE){
                    int error = HelloReply(clients[numberClient], epfd);
                    if(error == ERROR){
                        DeliteClient(clients, numberClient, epfd, &sizeClients);
                        numerDelite[addNumberDelite] = numberClient;
                        addNumberDelite++;
                    }
                    if(error == 2){
                        clients[numberClient]->last_activity = time(NULL);
                    }
                    continue;
                }
                if(clients[numberClient]->setting == false && clients[numberClient]->settingContext.stateClientRequest != R_DONE){
                   int error = StartSettings(clients[numberClient]);
                   if(error == ERROR){
                    DeliteClient(clients, numberClient, epfd, &sizeClients);
                        numerDelite[addNumberDelite] = numberClient;
                        addNumberDelite++;
                        printf("ОШИБКА ПОЛУЧЕНИЯ НАСТРОЕК %d\n\n", numberClient);
                    }
                    else if (error == 2) {
                        printf("SETTING END OK %d\n\n", numberClient);
                        clients[numberClient]->last_activity = time(NULL);

                        if(clients[numberClient]->settingContext.addr.StateIPv4 == WAIT){
                            printf("ИЩУ DNS\n\n");
                            SendDNS(clients[numberClient], udpSock);
                        }
                    }
                    continue;
                }
                else if(clients[numberClient]->setting == false && clients[numberClient]->settingContext.stateClientRequest == R_DONE){
                    if(clients[numberClient]->settingContext.addr.StateIPv4 == DONE && clients[numberClient]->statConnect == C_WAIT){
                        if (ev & (EPOLLERR|EPOLLHUP)) { 
                            DeliteClient(clients, numberClient, epfd, &sizeClients);
                            numerDelite[addNumberDelite] = numberClient;
                            addNumberDelite++;
                        }
                        if (ev & EPOLLOUT) {
                            FinishConnect(clients[numberClient]);
                            clients[numberClient]->last_activity = time(NULL);
                            continue;
                        }
                    }
                    else if(clients[numberClient]->statConnect == C_DONE){
                        int error = SettingsReply(clients[numberClient]);
                        if(error == 2){
                            clients[numberClient]->last_activity = time(NULL);
                            clients[numberClient]->eventsClient = EPOLLIN;
                            clients[numberClient]->eventsEndPoint = EPOLLIN;
                            modify_epoll_events(epfd, clients[numberClient]->socket, clients[numberClient]->eventsClient);
                            modify_epoll_events(epfd, clients[numberClient]->socketEndPoint, clients[numberClient]->eventsEndPoint);
                            continue;
                        }
                        else if(error == ERROR){
                            DeliteClient(clients, numberClient, epfd, &sizeClients);
                            numerDelite[addNumberDelite] = numberClient;
                            addNumberDelite++;
                            printf("ОШИБКА ОТПРАВЛЕНИЯ НАСТРОЕК %d\n\n", numberClient);
                            continue;
                        }
                    }
                }
                if(clients[numberClient]->setting == true){
                    if(ev & EPOLLIN){
                        int error = TunelRecv(clients[numberClient], fd, epfd);
                        if(error == SUCCES){
                            clients[numberClient]->last_activity = time(NULL);
                        }
                        else{
                            DeliteClient(clients, numberClient, epfd, &sizeClients);
                            numerDelite[addNumberDelite] = numberClient;
                            addNumberDelite++; 
                        }
                    }
                    else if(ev & EPOLLOUT){
                        int error = TunelSend(clients[numberClient], fd, epfd);
                        if(error == SUCCES){
                            clients[numberClient]->last_activity = time(NULL);
                        }
                        else{
                            DeliteClient(clients, numberClient, epfd, &sizeClients);
                        }
                    }
                    printf("КЛИЕНТОВ: %d\n", sizeClients);
                }
            }
        }
        free(numerDelite);
    }

    close(udpSock);
    close(epfd);
    close(socket_stream);
}