//
// Created by Theo Weidmann on 26.03.18.
//

#include "../runtime/Runtime.h"
#include "../s/Data.h"
#include "../s/String.h"
#include "../s/Error.h"
#include <arpa/inet.h>
#include <cerrno>
#include <csignal>
#include <cstring>
#include <netdb.h>
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

extern "C" Socket* socketsSocketNewHost(String *host, runtime::Integer port, runtime::Raiser *raiser) {
    struct hostent *server = gethostbyname(host->stdString().c_str());
    if (server == nullptr) {
        EJC_RAISE(raiser, s::IOError::init());
    }

    struct sockaddr_in address{};
    std::memset(&address, 0, sizeof(address));
    std::memcpy(&address.sin_addr.s_addr, server->h_addr_list[0], server->h_length);
    address.sin_family = PF_INET;
    address.sin_port = htons(port);

    int socketDescriptor = socket(AF_INET, SOCK_STREAM, 0);
    if (socketDescriptor == -1 || connect(socketDescriptor, reinterpret_cast<struct sockaddr *>(&address),
                                          sizeof(address)) == -1) {
        EJC_RAISE(raiser, s::IOError::init());
    }

    auto socket = Socket::init();
    socket->socket_ = socketDescriptor;
    return socket;
}

extern "C" void socketsSocketClose(Socket *socket) {
    close(socket->socket_);
}

extern "C" void socketsSocketSend(Socket *socket, Data *data, runtime::Raiser *raiser) {
    EJC_COND_RAISE_IO_VOID(send(socket->socket_, data->data.get(), data->count, 0) != -1, raiser);
}

extern "C" Data* socketsSocketRead(Socket *socket, runtime::Integer count, runtime::Raiser *raiser) {
    auto bytes = runtime::allocate<runtime::Byte>(count);

    auto read = recv(socket->socket_, bytes.get(), count, 0);
    if (read == -1) {
        EJC_RAISE(raiser, s::IOError::init());
    }

    auto data = Data::init();
    data->count = read;
    data->data = bytes;
    return data;
}

extern "C" void socketsServerClose(Server *server) {
    close(server->socket_);
}

extern "C" Server* socketsServerNewPort(runtime::Integer port, runtime::Raiser *raiser) {
    int listenerDescriptor = socket(PF_INET, SOCK_STREAM, 0);
    if (listenerDescriptor == -1) {
        EJC_RAISE(raiser, s::IOError::init());
    }

    struct sockaddr_in name{};
    name.sin_family = PF_INET;
    name.sin_port = htons(port);
    name.sin_addr.s_addr = htonl(INADDR_ANY);

    int reuse = 1;
    if (setsockopt(listenerDescriptor, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<char *>(&reuse), sizeof(int)) == -1 ||
        bind(listenerDescriptor, reinterpret_cast<struct sockaddr *>(&name), sizeof(name)) == -1 ||
        listen(listenerDescriptor, 10) == -1) {
        EJC_RAISE(raiser, s::IOError::init());
    }

    auto server = Server::init();
    server->socket_ = listenerDescriptor;
    return server;
}

extern "C" Socket* socketsServerAccept(Server *server, runtime::Raiser *raiser) {
    std::signal(SIGPIPE, SIG_IGN);

    struct sockaddr_storage clientAddress{};
    unsigned int addressSize = sizeof(clientAddress);
    int connectionAddress = accept(server->socket_, reinterpret_cast<struct sockaddr *>(&clientAddress), &addressSize);

    if (connectionAddress == -1) {
        EJC_RAISE(raiser, s::IOError::init());
    }

    auto socket = Socket::init();
    socket->socket_ = connectionAddress;
    return socket;
}

}  // namespace sockets

SET_INFO_FOR(sockets::Socket, sockets, 1f4de)
SET_INFO_FOR(sockets::Server, sockets, 1f3c4)
