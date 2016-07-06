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

static Class *CL_SOCKET;

PackageVersion getVersion() {
    return (PackageVersion){0, 1};
}

void serverInitWithPort(Thread *thread) {
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

Something serverAccept(Thread *thread) {
    int listenerDescriptor = *(int *)stackGetThisObject(thread)->value;
    struct sockaddr_storage clientAddress;
    unsigned int addressSize = sizeof(clientAddress);
    int connectionAddress = accept(listenerDescriptor, (struct sockaddr *)&clientAddress, &addressSize);
    
    if (connectionAddress == -1) {
        return NOTHINGNESS;
    }
    
    Object *socket = newObject(CL_SOCKET);
    *(int *)socket->value = connectionAddress;
    return somethingObject(socket);
}

Something socketSendData(Thread *thread) {
    int connectionAddress = *(int *)stackGetThisObject(thread)->value;
    Data *data = stackGetVariable(0, thread).object->value;
    if (send(connectionAddress, data->bytes, data->length, 0) == -1) {
        return EMOJICODE_FALSE;
    }
    return EMOJICODE_TRUE;
}

Something socketClose(Thread *thread) {
    int connectionAddress = *(int *)stackGetThisObject(thread)->value;
    close(connectionAddress);
    return NOTHINGNESS;
}

Something socketReadBytes(Thread *thread) {
    int connectionAddress = *(int *)stackGetThisObject(thread)->value;
    EmojicodeInteger n = unwrapInteger(stackGetVariable(0, thread));
    
    Object *bytesObject = newArray(n);
    
    size_t read = recv(connectionAddress, bytesObject->value, n, 0);
    
    if (read < 1) {
        return NOTHINGNESS;
    }
    
    stackPush(somethingObject(bytesObject), 0, 0, thread);
    
    Object *obj = newObject(CL_DATA);
    Data *data = obj->value;
    data->length = read;
    data->bytesObject = stackGetThisObject(thread);
    data->bytes = data->bytesObject->value;
    
    stackPop(thread);
    return somethingObject(obj);
}

void socketInitWithHost(Thread *thread) {
    char *string = stringToChar(stackGetVariable(0, thread).object->value);
    char *service = stringToChar(stackGetVariable(1, thread).object->value);
    
    struct addrinfo *res;
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(string, service, &hints, &res) == -1) {
        stackGetThisObject(thread)->value = NULL;
        return;
    }
    free(string);
    free(service);
    int socketDescriptor = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (socketDescriptor == -1 || connect(socketDescriptor, res->ai_addr, res->ai_addrlen) == -1) {
        freeaddrinfo(res);
        stackGetThisObject(thread)->value = NULL;
        return;
    }
    freeaddrinfo(res);
    *(int *)stackGetThisObject(thread)->value = socketDescriptor;
}

void socketDestruct(void *d) {
    close(*(int *)d);
}

FunctionFunctionPointer handlerPointerForMethod(EmojicodeChar cl, EmojicodeChar symbol, MethodType t) {
    switch (symbol) {
        case 0x1f64b: //🙋
            return serverAccept;
        case 0x1f4ac: //💬
            return socketSendData;
        case 0x1f645: //🙅
            return socketClose;
        case 0x1f442: //👂
            return socketReadBytes;
    }
    return NULL;
}

InitializerFunctionFunctionPointer handlerPointerForInitializer(EmojicodeChar cl, EmojicodeChar symbol) {
    switch (cl) {
        case 0x1f4de: //📞
            return socketInitWithHost;
        case 0x1f3c4: //🏄
            return serverInitWithPort;
    }
    return NULL;
}

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

Deinitializer deinitializerPointerForClass(EmojicodeChar cl) {
    switch (cl) {
        case 0x1f3c4: //🏄
        case 0x1f4de: //📞
            return socketDestruct;
    }
    return NULL;
}
