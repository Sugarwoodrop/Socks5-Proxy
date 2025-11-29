#include "HelloCheak.h"

int StartUserVerification(ClientSocket* client){
        if(client->helloContext.state == H_READ_VER){
            int error = RecvOneBait(client, &client->helloContext.VER);
            if(error == 2){
                client->helloContext.state = H_READ_NM;
                if(client->helloContext.VER != 0x05){
                    return ERROR;
                }
            }
            else{
                return error;
            }
        }
        if(client->helloContext.state == H_READ_NM){
            int error = RecvOneBait(client, &client->helloContext.NumberMethods) ;
            if(error == 2){
                client->helloContext.state = H_READ_METHODS;
            }
            else{
                return error;
            }
        }
        if(client->helloContext.state == H_READ_METHODS){
            uint8_t* buf_options = malloc(sizeof(uint8_t)*client->helloContext.NumberMethods);
            
            for(int i = 0; i<client->helloContext.NumberMethods; i++){
                buf_options[i] = 35;
            }

            ssize_t data_obtained = recv(client->socket, buf_options, client->helloContext.NumberMethods, 0);
            if(data_obtained == -1){
                perror("recv2");
                free(buf_options);
                return ERROR;
            } 
            if(data_obtained == 0){
                free(buf_options);
                return ERROR;
            }

            for(int i = 0; i<client->helloContext.NumberMethods; i++){
                if(buf_options[i] == 0){
                    client->helloContext.Methods = 0;
                }
            }

            client->helloContext.readSizeMerhods += data_obtained; 
            if(client->helloContext.readSizeMerhods == client->helloContext.NumberMethods && client->helloContext.Methods != 0){
                printf("Не поддерживается по форматам");
                client->helloContext.Methods = 0xFF;
            }
            if(client->helloContext.readSizeMerhods == client->helloContext.NumberMethods){
                client->helloContext.state = H_DONE;
            }
            free(buf_options);
        }
        if(client->helloContext.state == H_DONE){
            printf("SOCKET %u\nNM %u\nM %u\n\n", client->helloContext.VER, client->helloContext.NumberMethods, client->helloContext.Methods);
            return 2;
        }
        return SUCCES;
    }

int HelloReply(ClientSocket* client, int epfd){
        if(client->helloContext.replyState == H_REPLY_VER){
            int error = SendOneBait(client, &client->helloContext.VER);
            if(error == 2){
                client->helloContext.replyState = H_REPLY_METHODS;
            }
            else{
                return error;
            }
        }
        if(client->helloContext.replyState == H_REPLY_METHODS){
            int error = SendOneBait(client, &client->helloContext.Methods);
            if(error == 2){
                client->helloContext.replyState = H_DONEREPLY;
            }
            else{
                return error;
            }
        }
        if(client->helloContext.replyState == H_DONEREPLY){
            client->hello = true;
            printf("Hello ответ отправлен\n\n");
            client->eventsClient = EPOLLIN;
            modify_epoll_events(epfd, client->socket, client->eventsClient);
        }
        return 2;
    }

void modify_epoll_events(int epfd, int fd, uint32_t events) {
    struct epoll_event epev = {0};
    epev.events = events;
    epev.data.fd = fd;
    int error = epoll_ctl(epfd, EPOLL_CTL_MOD, fd, (struct epoll_event*)&epev);
    if(error == -1){
        perror("epoll_ctl mod");
    }
}