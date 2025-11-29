#include "SendRecvOneBait.h"

int RecvOneBait(ClientSocket* client, uint8_t* writeIn){
        ssize_t data_obtained = recv(client->socket, writeIn, 1, 0);
        if (data_obtained < 0 && errno == EAGAIN){
            return SUCCES;
        }
        if (data_obtained < 0){
            perror("recv");
            return ERROR;
        }
        if (data_obtained == 0){
            perror("recv");
            return ERROR;
        }
        return 2;
}

int SendOneBait(ClientSocket* client, uint8_t* sendBait ){
        ssize_t data_obtained = send(client->socket, sendBait, 1, 0);
        if (data_obtained < 0 && errno == EAGAIN){
            return SUCCES;
        }
        if (data_obtained <= 0){
            perror("send");
            return ERROR;
        }
    return 2;
}