//
//  sockets.c
//  Packages
//
//  Created by Theo Weidmann on 06.07.2016.
//  Copyright (c) 2016 Theo Weidmann. All rights reserved.
//

#include "../../EmojicodeReal-TimeEngine/EmojicodeAPI.hpp"
#include "../../EmojicodeReal-TimeEngine/Thread.hpp"
#include "../../EmojicodeReal-TimeEngine/String.h"
#include "../../EmojicodeReal-TimeEngine/standard.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>

using Emojicode::Thread;
using Emojicode::Value;
using Emojicode::String;
using Emojicode::Data;
using Emojicode::stringToCString;

static Emojicode::Class *CL_SOCKET;

void serverInitWithPort(Thread *thread, Value *destination) {
    int listenerDescriptor = socket(PF_INET, SOCK_STREAM, 0);
    if (listenerDescriptor == -1) {
        destination->makeNothingness();
        return;
    }
    
    struct sockaddr_in name;
    name.sin_family = PF_INET;
    name.sin_port = htons(thread->getVariable(0).raw);
    name.sin_addr.s_addr = htonl(INADDR_ANY);

    int reuse = 1;
    if (setsockopt(listenerDescriptor, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(int)) == -1 ||
        bind(listenerDescriptor, (struct sockaddr *)&name, sizeof(name)) == -1 ||
        listen(listenerDescriptor, 10) == -1) {
        destination->makeNothingness();
        return;
    }
    
    *(int *)thread->getThisObject()->value = listenerDescriptor;
    destination->optionalSet(thread->getThisObject());
}

void serverAccept(Thread *thread, Value *destination) {
    int listenerDescriptor = *(int *)thread->getThisObject()->value;
    struct sockaddr_storage clientAddress;
    unsigned int addressSize = sizeof(clientAddress);
    int connectionAddress = accept(listenerDescriptor, (struct sockaddr *)&clientAddress, &addressSize);
    
    if (connectionAddress == -1) {
        destination->makeNothingness();
        return;
    }
    
    Emojicode::Object *socket = newObject(CL_SOCKET);
    *(int *)socket->value = connectionAddress;
    destination->optionalSet(socket);
}

void socketSendData(Thread *thread, Value *destination) {
    int connectionAddress = *(int *)thread->getThisObject()->value;
    Data *data = static_cast<Data *>(thread->getVariable(0).object->value);
    destination->raw = send(connectionAddress, data->bytes, data->length, 0) == -1;
}

void socketClose(Thread *thread, Value *destination) {
    int connectionAddress = *(int *)thread->getThisObject()->value;
    close(connectionAddress);
}

void socketReadBytes(Thread *thread, Value *destination) {
    int connectionAddress = *(int *)thread->getThisObject()->value;
    Emojicode::EmojicodeInteger n = thread->getVariable(0).raw;
    
    Emojicode::Object *const &bytesObject = thread->retain(Emojicode::newArray(n));
    size_t read = recv(connectionAddress, bytesObject->value, n, 0);
    
    if (read < 1) {
        thread->release(1);
        destination->makeNothingness();
        return;
    }
    
    Emojicode::Object *obj = newObject(Emojicode::CL_DATA);
    Data *data = static_cast<Data *>(obj->value);
    data->length = read;
    data->bytesObject = bytesObject;
    data->bytes = static_cast<char *>(data->bytesObject->value);

    thread->release(1);
    destination->optionalSet(obj);
}

void socketInitWithHost(Thread *thread, Value *destination) {
    struct hostent *server = gethostbyname(Emojicode::stringToCString(thread->getVariable(0).object));
    if (!server) {
        destination->makeNothingness();
        return;
    }

    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    memcpy(&address.sin_addr.s_addr, server->h_addr_list[0], server->h_length);
    address.sin_family = PF_INET;
    address.sin_port = htons(thread->getVariable(1).raw);

    int socketDescriptor = socket(AF_INET, SOCK_STREAM, 0);
    if (socketDescriptor == -1 || connect(socketDescriptor, (struct sockaddr *) &address, sizeof(address)) == -1) {
        destination->makeNothingness();
        return;
    }
    *(int *)thread->getThisObject()->value = socketDescriptor;
    destination->optionalSet(thread->getThisObject());
}

void socketDestruct(void *d) {
    close(*(int *)d);
}

Emojicode::PackageVersion version(0, 1);

LinkingTable {
    nullptr,
    serverAccept,
    socketSendData,
    socketClose,
    socketReadBytes,
    socketInitWithHost,
    serverInitWithPort,
};

extern "C" Emojicode::Marker markerPointerForClass(EmojicodeChar cl) {
    return NULL;
}

extern "C" uint_fast32_t sizeForClass(Emojicode::Class *cl, EmojicodeChar name) {
    switch (name) {
        case 0x1f3c4: //🏄
            return sizeof(int);
        case 0x1f4de: //📞
            CL_SOCKET = cl;
            return sizeof(int);
    }
    return 0;
}
