//
// Created by Theo Weidmann on 26.03.18.
//

#include "../runtime/Runtime.h"
#include "../s/String.hpp"
#include "../s/Data.hpp"
#include <arpa/inet.h>
#include <cerrno>
#include <netdb.h>
#include <cstring>
#include <sys/socket.h>
#include <unistd.h>

using s::String;
using s::Data;

namespace sockets {

class Socket : public runtime::Object<Socket> {
public:
    int socket_;
};

class Server : public runtime::Object<Server> {
public:
    int socket_;
};

runtime::Enum errorEnumFromErrno() {
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
        default:
            return 0;
    }
}

runtime::SimpleOptional<runtime::Enum> returnOptional(bool success) {
    if (success) {
        return runtime::NoValue;
    }
    return errorEnumFromErrno();
}

extern "C" runtime::SimpleError<Socket*> socketsSocketNewHost(String *host, runtime::Integer port) {
    struct hostent *server = gethostbyname(host->cString());
    if (server == nullptr) {
        return runtime::SimpleError<Socket*>(runtime::MakeError, errorEnumFromErrno());
    }

    struct sockaddr_in address{};
    std::memset(&address, 0, sizeof(address));
    std::memcpy(&address.sin_addr.s_addr, server->h_addr_list[0], server->h_length);
    address.sin_family = PF_INET;
    address.sin_port = htons(port);

    int socketDescriptor = socket(AF_INET, SOCK_STREAM, 0);
    if (socketDescriptor == -1 || connect(socketDescriptor, reinterpret_cast<struct sockaddr *>(&address), sizeof(address)) == -1) {
        return runtime::SimpleError<Socket*>(runtime::MakeError, errorEnumFromErrno());
    }

    auto socket = Socket::allocateAndInitType();
    socket->socket_ = socketDescriptor;
    return socket;
}

extern "C" void socketsSocketClose(Socket *socket) {
    close(socket->socket_);
}

extern "C" runtime::SimpleOptional<runtime::Enum> socketsSocketSend(Socket *socket, Data *data) {
    return returnOptional(send(socket->socket_, data->data, data->count, 0) != -1);
}

extern "C" runtime::SimpleError<Data*> socketsSocketRead(Socket *socket, runtime::Integer count) {
    auto bytes = runtime::allocate<runtime::Byte>(count);

    auto read = recv(socket->socket_, bytes, count, 0);
    if (read == -1) {
        return runtime::SimpleError<Data *>(runtime::MakeError, errorEnumFromErrno());;
    }

    auto data = Data::allocateAndInitType();
    data->count = read;
    data->data = bytes;
    return data;
}

extern "C" void socketsServerClose(Server *server) {
    close(server->socket_);
}

extern "C" runtime::SimpleError<Server*> socketsServerNewPort(runtime::Integer port) {
    int listenerDescriptor = socket(PF_INET, SOCK_STREAM, 0);
    if (listenerDescriptor == -1) {
        return runtime::SimpleError<Server *>(runtime::MakeError, errorEnumFromErrno());
    }

    struct sockaddr_in name{};
    name.sin_family = PF_INET;
    name.sin_port = htons(port);
    name.sin_addr.s_addr = htonl(INADDR_ANY);

    int reuse = 1;
    if (setsockopt(listenerDescriptor, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<char *>(&reuse), sizeof(int)) == -1 ||
        bind(listenerDescriptor, reinterpret_cast<struct sockaddr *>(&name), sizeof(name)) == -1 ||
        listen(listenerDescriptor, 10) == -1) {
        return runtime::SimpleError<Server *>(runtime::MakeError, errorEnumFromErrno());
    }

    auto server = Server::allocateAndInitType();
    server->socket_ = listenerDescriptor;
    return server;
}

extern "C" runtime::SimpleError<Socket *> socketsServerAccept(Server *server) {
    struct sockaddr_storage clientAddress{};
    unsigned int addressSize = sizeof(clientAddress);
    int connectionAddress = accept(server->socket_, reinterpret_cast<struct sockaddr *>(&clientAddress), &addressSize);

    if (connectionAddress == -1) {
        return runtime::SimpleError<Socket *>(runtime::MakeError, errorEnumFromErrno());
    }

    auto socket = Socket::allocateAndInitType();
    socket->socket_ = connectionAddress;
    return socket;
}

}  // namespace sockets

SET_META_FOR(sockets::Socket, sockets, 1f4de)
SET_META_FOR(sockets::Server, sockets, 1f3c4)