//
//  sockets.c
//  Packages
//
//  Created by Theo Weidmann on 06.07.2016.
//  Copyright (c) 2016 Theo Weidmann. All rights reserved.
//

#define _XOPEN_SOURCE 500
#include "EmojicodeAPI.h"
#include "EmojicodeString.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>

static Class *CL_SOCKET;

PackageVersion getVersion() {
    return (PackageVersion){0, 1};
}

void serverInitWithPort(Thread *thread, Something *destination) {
    int listenerDescriptor = socket(PF_INET, SOCK_STREAM, 0);
    if (listenerDescriptor == -1) {
        stackGetThisObject(thread)->value = NULL;
        return;
    }
    
    struct sockaddr_in name;
    name.sin_family = PF_INET;
    name.sin_port = htons(stackGetVariable(0, thread).raw);
    name.sin_addr.s_addr = htonl(INADDR_ANY);
    
    int reuse = 1;
    if (setsockopt(listenerDescriptor, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(int)) == -1 ||
        bind(listenerDescriptor, (struct sockaddr *)&name, sizeof(name)) == -1 ||
        listen(listenerDescriptor, 10) == -1) {
        stackGetThisObject(thread)->value = NULL;
        return;
    }
    
    *(int *)stackGetThisObject(thread)->value = listenerDescriptor;
}

void serverAccept(Thread *thread, Something *destination) {
    int listenerDescriptor = *(int *)stackGetThisObject(thread)->value;
    struct sockaddr_storage clientAddress;
    unsigned int addressSize = sizeof(clientAddress);
    int connectionAddress = accept(listenerDescriptor, (struct sockaddr *)&clientAddress, &addressSize);
    
    if (connectionAddress == -1) {
        *destination = NOTHINGNESS;
        return;
    }
    
    Object *socket = newObject(CL_SOCKET);
    *(int *)socket->value = connectionAddress;
    *destination = somethingObject(socket);
}

void socketSendData(Thread *thread, Something *destination) {
    int connectionAddress = *(int *)stackGetThisObject(thread)->value;
    Data *data = stackGetVariable(0, thread).object->value;
    if (send(connectionAddress, data->bytes, data->length, 0) == -1) {
        *destination = EMOJICODE_FALSE;
    }
    else {
        *destination = EMOJICODE_TRUE;
    }
}

void socketClose(Thread *thread, Something *destination) {
    int connectionAddress = *(int *)stackGetThisObject(thread)->value;
    close(connectionAddress);
    *destination = NOTHINGNESS;
}

void socketReadBytes(Thread *thread, Something *destination) {
    int connectionAddress = *(int *)stackGetThisObject(thread)->value;
    EmojicodeInteger n = unwrapInteger(stackGetVariable(0, thread));
    
    Object *bytesObject = newArray(n);
    
    size_t read = recv(connectionAddress, bytesObject->value, n, 0);
    
    if (read < 1) {
        *destination = NOTHINGNESS;
        return;
    }
    
    stackPush(somethingObject(bytesObject), 0, 0, thread);
    
    Object *obj = newObject(CL_DATA);
    Data *data = obj->value;
    data->length = read;
    data->bytesObject = stackGetThisObject(thread);
    data->bytes = data->bytesObject->value;
    
    stackPop(thread);
    *destination = somethingObject(obj);
}

void socketInitWithHost(Thread *thread, Something *destination) {
    char *string = stringToChar(stackGetVariable(0, thread).object->value);

    struct hostent *server = gethostbyname(string);
    if (!server) {
        *destination = NOTHINGNESS;
        return;
    }

    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    memcpy(&address.sin_addr.s_addr, server->h_addr_list[0], server->h_length);
    address.sin_family = PF_INET;
    address.sin_port = htons(stackGetVariable(1, thread).raw);

    free(string);

    int socketDescriptor = socket(AF_INET, SOCK_STREAM, 0);
    if (socketDescriptor == -1 || connect(socketDescriptor, (struct sockaddr *) &address, sizeof(address)) == -1) {
        *destination = NOTHINGNESS;
        return;
    }
    *(int *)stackGetThisObject(thread)->value = socketDescriptor;
}

void socketDestruct(void *d) {
    close(*(int *)d);
}

LinkingTable {
    [1] = serverAccept,
    [2] = socketSendData,
    [3] = socketClose,
    [4] = socketReadBytes,
    [5] = socketInitWithHost,
    [6] = serverInitWithPort,
};

Marker markerPointerForClass(EmojicodeChar cl) {
    return NULL;
}

uint_fast32_t sizeForClass(Class *cl, EmojicodeChar name) {
    switch (name) {
        case 0x1f3c4: //🏄
            return sizeof(int);
        case 0x1f4de: //📞
            CL_SOCKET = cl;
            return sizeof(int);
    }
    return 0;
}
