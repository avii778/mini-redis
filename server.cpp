#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include "write_and_read.h"
#include <vector>
#include <poll.h>

const size_t K_MAX_MSG = 4096;

struct Conn {
    int fd = -1;

    bool want_read = false;
    bool want_write = false;
    bool want_close = false;

    std::vector<uint8_t> incoming;  // data to be parsed by the application
    std::vector<uint8_t> outgoing;
};

Conn* handle_accept(int fd) {

    int rv = accept(fd, nullptr, nullptr); // i don't care where this request comes from

    if (rv < 0) {
        die("Lol fuck you");
    }
    Conn* conn = new Conn();
    conn->fd = rv;
    conn->want_read = true;

    return conn;
}

void handle_read(Conn* conn) {
    return;
}

void handle_write(Conn* conn) {
    return;
}

int32_t one_request(int connfd) {
    
    char rbuf[4 + K_MAX_MSG]; // 4 bytes for length header

    errno = 0;

    int32_t err = read_full(connfd, rbuf, 4); // parse length header

    if (err) {
        msg(errno == 0 ? "EOF" : "read() error");
        return err;
    }

    uint32_t be = 0;
    memcpy(&be, rbuf, 4); 
    uint32_t length = ntohl(be);

    if (length > K_MAX_MSG) {
        msg("too long");
        return -1;
    }

    // actually read the request
    err = read_full(connfd, &rbuf[4], length);


    if (err) {
        msg("read() error");
        return -1;
    }

    // respond using the same length protocol

    printf("Client says %.*s\n: ", length, &rbuf[4]);

    char reply[] = "world";
    char wbuf[4 + sizeof(reply)];
    uint32_t len = (uint32_t)strlen(reply);
    uint32_t blen = htonl(len);
    memcpy(wbuf, &blen, 4);
    memcpy(&wbuf[4], reply, len);
    return write_all(connfd, wbuf, 4 + len);
}

int main() {

    int fd = socket(AF_INET, SOCK_STREAM, 0);

    int val = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
    struct sockaddr_in addr = {};

    addr.sin_family = AF_INET;
    addr.sin_port = htons(1234);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    int rv = bind(fd, (const struct sockaddr *)&addr, sizeof(addr));
    if (rv) { die("bind()"); }

    rv = listen(fd, SOMAXCONN);

    while (true) {

        // main event loop will go here

        std::vector<Conn *> fd2conn; // vector filled with connections + states
        std::vector<struct pollfd> poll_args; //should eventually transition to epoll

        while (true){ 

            poll_args.clear();

            struct pollfd pfd = {fd, POLLIN, 0}; 
            poll_args.push_back(pfd);
                
            // prepare tha polls
            for (Conn* conn : fd2conn) {

                if (!conn) { // checking nullness
                    continue;
                }

                struct pollfd pfd = {conn->fd, POLLERR, 0};

                if (conn->want_read) {
                    pfd.events |= POLLIN;
                }

                if (conn->want_write) {
                    pfd.events |= POLLOUT;                    
                }

                poll_args.push_back(pfd);

            }

            int rv = poll(poll_args.data(), (nfds_t)poll_args.size(), -1); // kinda not great - could in theory block forever

            if (rv < 0 && errno == EINTR) {
                continue;
            }
            
            if (rv < 0) {
                die("poll");
            }

            if (poll_args[0].revents) {

                if (Conn* connfd = handle_accept(fd)) {
                    
                    if (connfd->fd > fd2conn.size()){
                        fd2conn.resize(connfd->fd + 1);
                    }

                    fd2conn[connfd->fd] = connfd;
                }

                for (size_t i = 1; i < poll_args.size(); ++i) { // skip the first one

                    int32_t ready = poll_args[i].fd;
                    Conn* conn = fd2conn[ready];

                    if (conn->want_read) {
                        handle_read(conn);
                    }
                    
                    if (conn->want_write) {
                        handle_write(conn);
                    }
                } 
            }
        }
    }

    return 0;

}
