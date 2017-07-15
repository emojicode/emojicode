//
//  sockets.c
//  Packages
//
//  Created by Theo Weidmann on 06.07.2016.
//  Copyright (c) 2016 Theo Weidmann. All rights reserved.
//

#include "../../EmojicodeReal-TimeEngine/EmojicodeAPI.hpp"
#include "../../EmojicodeReal-TimeEngine/Class.hpp"
#include "../../EmojicodeReal-TimeEngine/Data.hpp"
#include "../../EmojicodeReal-TimeEngine/String.hpp"
#include "../../EmojicodeReal-TimeEngine/Thread.hpp"
#include <arpa/inet.h>
#include <cerrno>
#include <netdb.h>
#include <cstring>
#include <sys/socket.h>
#include <unistd.h>

using Emojicode::Thread;
using Emojicode::Data;
using Emojicode::stringToCString;

static Emojicode::Class *CL_SOCKET;

Emojicode::EmojicodeInteger errnoToError() {
    switch (errno) {
        case EACCES:
            return 1;
        case EEXIST:
            return 2;
        case ENOMEM:
            return 3;
        case ENOSYS:
            return 4;
        case EDOM:
            return 5;
        case EINVAL:
            return 6;
        case EILSEQ:
            return 7;
        case ERANGE:
            return 8;
        case EPERM:
            return 9;
    }
    return 0;
}

void serverInitWithPort(Thread *thread) {
    int listenerDescriptor = socket(PF_INET, SOCK_STREAM, 0);
    if (listenerDescriptor == -1) {
        thread->returnErrorFromFunction(errnoToError());
        return;
    }

    struct sockaddr_in name{};
    name.sin_family = PF_INET;
    name.sin_port = htons(thread->variable(0).raw);
    name.sin_addr.s_addr = htonl(INADDR_ANY);

    int reuse = 1;
    if (setsockopt(listenerDescriptor, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<char *>(&reuse), sizeof(int)) == -1 ||
        bind(listenerDescriptor, reinterpret_cast<struct sockaddr *>(&name), sizeof(name)) == -1 ||
        listen(listenerDescriptor, 10) == -1) {
        thread->returnErrorFromFunction(errnoToError());
        return;
    }

    *thread->thisObject()->val<int>() = listenerDescriptor;
    thread->returnOEValueFromFunction(thread->thisObject());
}

void serverAccept(Thread *thread) {
    int listenerDescriptor = *thread->thisObject()->val<int>();
    struct sockaddr_storage clientAddress{};
    unsigned int addressSize = sizeof(clientAddress);
    int connectionAddress = accept(listenerDescriptor, reinterpret_cast<struct sockaddr *>(&clientAddress), &addressSize);

    if (connectionAddress == -1) {
        thread->returnNothingnessFromFunction();
        return;
    }

    Emojicode::Object *socket = newObject(CL_SOCKET);
    *socket->val<int>() = connectionAddress;
    thread->returnOEValueFromFunction(socket);
}

void socketSendData(Thread *thread) {
    int connectionAddress = *thread->thisObject()->val<int>();
    auto *data = thread->variable(0).object->val<Data>();
    thread->returnFromFunction(send(connectionAddress, data->bytes, data->length, 0) == -1);
}

void socketClose(Thread *thread) {
    int connectionAddress = *thread->thisObject()->val<int>();
    close(connectionAddress);
    thread->returnFromFunction();
}

void socketReadBytes(Thread *thread) {
    int connectionAddress = *thread->thisObject()->val<int>();
    Emojicode::EmojicodeInteger n = thread->variable(0).raw;

    auto bytesObject = thread->retain(Emojicode::newArray(n));
    size_t read = recv(connectionAddress, bytesObject->val<char>(), n, 0);

    if (read < 1) {
        thread->release(1);
        thread->returnNothingnessFromFunction();
        return;
    }

    Emojicode::Object *obj = newObject(Emojicode::CL_DATA);
    auto *data = obj->val<Data>();
    data->length = read;
    data->bytesObject = bytesObject.unretainedPointer();
    data->bytes = data->bytesObject->val<char>();

    thread->release(1);
    thread->returnOEValueFromFunction(obj);
}

void socketInitWithHost(Thread *thread) {
    struct hostent *server = gethostbyname(Emojicode::stringToCString(thread->variable(0).object));
    if (server == nullptr) {
        thread->returnErrorFromFunction(errnoToError());
        return;
    }

    struct sockaddr_in address{};
    memset(&address, 0, sizeof(address));
    memcpy(&address.sin_addr.s_addr, server->h_addr_list[0], server->h_length);
    address.sin_family = PF_INET;
    address.sin_port = htons(thread->variable(1).raw);

    int socketDescriptor = socket(AF_INET, SOCK_STREAM, 0);
    if (socketDescriptor == -1 || connect(socketDescriptor, reinterpret_cast<struct sockaddr *>( &address), sizeof(address)) == -1) {
        thread->returnErrorFromFunction(errnoToError());
        return;
    }
    *thread->thisObject()->val<int>() = socketDescriptor;
    thread->returnOEValueFromFunction(thread->thisObject());
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

extern "C" void prepareClass(Emojicode::Class *klass, EmojicodeChar name) {
    switch (name) {
        case 0x1f3c4: //🏄
            klass->valueSize = sizeof(int);
            break;
        case 0x1f4de: //📞
            CL_SOCKET = klass;
            klass->valueSize = sizeof(int);
            break;
    }
}
