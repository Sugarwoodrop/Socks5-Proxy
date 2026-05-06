#include "SocksSettings.h"

int StartSettings(ClientSocket* client){
    if(client->settingContext.stateClientRequest == R_READ_VER){
        int error = RecvOneByte(client, &client->settingContext.VER);
        if(error == 2){
            client->settingContext.stateClientRequest = R_READ_CMD;
            printf("Settings:\nVER %u\n", client->settingContext.VER);
            if(client->settingContext.VER != 0x05){
                return ERROR;
            }
        }
        else{
            return error;
        }
    }
    if(client->settingContext.stateClientRequest == R_READ_CMD){
        int error = RecvOneByte(client, &client->settingContext.CMD);
        if(error == 2){
            if(client->settingContext.CMD == 0x01){
                client->settingContext.stateClientRequest = R_READ_RSV;
                printf("Settings:\nCMD %u\n", client->settingContext.CMD);
            }
            else{
                client->settingContext.returnStatus = 0x07;
            }
        }
        else{
            return error;
        }
    }
    if(client->settingContext.stateClientRequest == R_READ_RSV){
        int error = RecvOneByte(client, &client->settingContext.RSV);
        if(error == 2){
            if(client->settingContext.RSV != 0x00){
                    client->settingContext.returnStatus = 0x07;
            }
            client->settingContext.stateClientRequest = R_READ_ADDR;
            printf("Settings:\nRSV %u\n", client->settingContext.RSV);

        }
        else{
            return error;
        }
    }
    if(client->settingContext.stateClientRequest == R_READ_ADDR){
        if(client->settingContext.addr.StateReadAddr == A_READ_TYPE){
            int error = RecvOneByte(client, &client->settingContext.addr.TYPE);
            if(error == 2){
                if(client->settingContext.addr.TYPE == 0x01){
                    client->settingContext.addr.StateReadAddr = A_READ_IPv4;
                    printf("Settings: Addr: \nTYPE %u\n", client->settingContext.addr.TYPE);
                }
                if(client->settingContext.addr.TYPE == 0x03){
                    client->settingContext.addr.StateReadAddr = A_READ_lengthDomainName;
                    printf("Settings: Addr: \nTYPE %u\n", client->settingContext.addr.TYPE);
                }
                if(client->settingContext.addr.TYPE == 0x04){
                    client->settingContext.returnStatus = 0x08;
                }
            }
            else{
                return error;
            }
            return SUCCESS;
        }
        if(client->settingContext.addr.StateReadAddr == A_READ_IPv4){
            int readBytee = recv(client->socket, client->settingContext.addr.IPv4 + client->settingContext.addr.lengthReadIP, 
                                            4 - client->settingContext.addr.lengthReadIP, 0);
            if (readBytee < 0 && errno == EAGAIN){
                return SUCCESS;
            }
            if (readBytee < 0){
                perror("recv");
                return ERROR;
            }
            if (readBytee == 0){
                perror("recv");
                return ERROR;
            }
            if(readBytee > 0){
                client->settingContext.addr.lengthReadIP += readBytee;
            }
            if(client->settingContext.addr.lengthReadIP != 4){
                return SUCCESS;
            }
            client->settingContext.addr.StateReadAddr = A_DONE;
            client->settingContext.addr.StateIPv4 = DONE;
            printf("Settings: Addr: \n IPv4 ");
            for(int i = 0; i < 4; i++){
                if(i == 3){
                    printf("%u\n", client->settingContext.addr.IPv4[i]);
                    continue;
                }
                printf("%u.", client->settingContext.addr.IPv4[i]);
            }
            return SUCCESS;
        }
        if(client->settingContext.addr.StateReadAddr == A_READ_lengthDomainName){
            int error = RecvOneByte(client, &client->settingContext.addr.lengthDomainName);
            if(error == 2){
                client->settingContext.addr.StateReadAddr = A_READ_domainName;
                printf("Settings: Addr:\n lengthDomainName %u\n", client->settingContext.addr.lengthDomainName);
            }
            else{
                return error;
            }   
            return SUCCESS;
        }
        if(client->settingContext.addr.StateReadAddr == A_READ_domainName){
            int readBytee = recv(client->socket, client->settingContext.addr.domainName + client->settingContext.addr.lengthReadDomainName, 
                                client->settingContext.addr.lengthDomainName - client->settingContext.addr.lengthReadDomainName, 0);
            if (readBytee < 0 && errno == EAGAIN){
                return SUCCESS;
            }
            if (readBytee < 0){
                perror("recv");
                return ERROR;
            }
            if (readBytee == 0){
                perror("recv");
                return ERROR;
            }
            if(readBytee > 0){
                client->settingContext.addr.lengthReadDomainName += readBytee;
            }
            if(client->settingContext.addr.lengthReadDomainName != client->settingContext.addr.lengthDomainName){
                return SUCCESS;
            }
            client->settingContext.addr.StateReadAddr = A_DONE;
            client->settingContext.addr.StateIPv4 = WAIT;
            printf("Settings: Addr:\n Domain ");
            for(int i = 0; i < client->settingContext.addr.lengthDomainName; i++){
                printf("%c", client->settingContext.addr.domainName[i]);
            }
            printf("\n");
            return SUCCESS;
        }
        if(client->settingContext.addr.StateReadAddr == A_DONE){
            client->settingContext.stateClientRequest = R_READ_DSTPORT;
            return SUCCESS;
        }
    }
    if(client->settingContext.stateClientRequest == R_READ_DSTPORT){
        int error = RecvOneByte(client, &client->settingContext.portByte[client->settingContext.sizeReadPortByte]);
        if(error == 2){
            client->settingContext.sizeReadPortByte++;
            if(client->settingContext.sizeReadPortByte == 2){
                client->settingContext.DSTPORT = (client->settingContext.portByte[0]<<8) | (client->settingContext.portByte[1]);
                client->settingContext.stateClientRequest = R_DONE;
                printf("Settings:\n PORT %u\n", client->settingContext.DSTPORT);
            }
        }
        else{
            return error;
        }
    }
    if(client->settingContext.stateClientRequest == R_DONE){
        printf("R_DONE\n\n\n");
        return 2;
    }
    return SUCCESS;
}

int SettingsReply(ClientSocket* client){
    if(client->settingContext.stateResponse == R_REPLY_VER){
        int error = SendOneByte(client, &client->settingContext.VER);
        if(error != 2){
            return error;
        }
        client->settingContext.stateResponse = R_REPLY_STATE;
        return SUCCESS;
    }
    if(client->settingContext.stateResponse == R_REPLY_STATE){
        int error = SendOneByte(client, &client->settingContext.returnStatus);
        if(error != 2){
            return error;
        }
        client->settingContext.stateResponse = R_REPLY_RSV;
        return SUCCESS;
    }
    if(client->settingContext.stateResponse == R_REPLY_RSV){
        int error = SendOneByte(client, &client->settingContext.RSV);
        if(error != 2){
            return error;
        }
        client->settingContext.stateResponse = R_REPLY_ADDR;
        return SUCCESS;
    }
    if(client->settingContext.stateResponse == R_REPLY_ADDR){
        if(client->settingContext.addr.StateReplyAddr == A_REPLY_TYPE){
            uint8_t type = 0x01;
            int error = SendOneByte(client, &type);
            if(error != 2){
                return error;
            }
            client->settingContext.addr.StateReplyAddr = A_REPLY_IPv4;
            return SUCCESS;
        }
        if(client->settingContext.addr.StateReplyAddr == A_REPLY_IPv4){
            int sendByte = send(client->socket, client->settingContext.addr.ReplyIPv4 + client->settingContext.addr.lengthReplyIP, 
                                    4-client->settingContext.addr.lengthReplyIP, 0);
            if(sendByte < 0 && errno == EAGAIN){
                return SUCCESS;
            }
            if(sendByte <= 0){
                perror("send");
                return ERROR;
            }
            client->settingContext.addr.lengthReplyIP += sendByte;
            if(client->settingContext.addr.lengthReplyIP == 4){
                client->settingContext.addr.StateReplyAddr = A_RDONE;
            }
            return SUCCESS;
        }
        if(client->settingContext.addr.StateReplyAddr == A_RDONE){
            client->settingContext.stateResponse = R_REPLY_PORT;
            return SUCCESS;
        }
    }
    if(client->settingContext.stateResponse == R_REPLY_PORT){
        int sendByte = send(client->socket, client->settingContext.connectPortByte + client->settingContext.sizeReplyPortByte, 
                                2-client->settingContext.sizeReplyPortByte, 0);
        if(sendByte < 0 && errno == EAGAIN){
            return SUCCESS;
        }
        if(sendByte <= 0){
            perror("send");
            return ERROR;
        }
        client->settingContext.sizeReplyPortByte += sendByte;
        if(client->settingContext.sizeReplyPortByte == 2){
            client->settingContext.stateResponse = R_RDONE;
        }
    }
    if(client->settingContext.stateResponse == R_RDONE){
        client->setting = true;
        printf("НАСТРОЙКИ ОТПРАВЛЕНЫ \n\n");
        return 2;
    }
    return SUCCESS;
}
