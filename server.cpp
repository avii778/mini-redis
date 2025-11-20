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
#include "conn.h"

const size_t K_MAX_MSG = 4096;

Conn* handle_accept(int fd) {

    int connfd = accept(fd, nullptr, nullptr);

    if (connfd < 0) {
        return NULL;
    }

    // set it to be non blocking
    fd_set_nb(connfd);
    Conn* conn = new Conn();
    conn->fd = connfd;
    conn->want_read = true;
    return conn;
}

void handle_read(Conn* conn) {
    // do a non-blocking read
    uint8_t buf[64 * 1024];
    ssize_t rv = read(conn->fd, buf, sizeof(buf));

    if (rv <= 0) {  // handle IO error (rv < 0) or EOF (rv == 0)
        conn->want_close = true;
        return;
    }
    
    //append this onto the incoming buff
    buf_append(conn->incoming, buf, (size_t) rv);

    // 3. Try to parse the accumulated buffer.
    // 4. Process the parsed message.
    // 5. Remove the message from `Conn::incoming`
    while(try_one_request(conn)){
        
    }

    if (conn->outgoing.size() > 0) {
        conn->want_read = false;
        conn->want_write = true;
    }

    return;
}

void handle_write(Conn* conn) {

    assert(conn->outgoing.size() > 0);
    ssize_t rv = send(conn->fd, conn->outgoing.data(), conn->outgoing.size(), 0);

    if (rv <= 0) {
        conn->want_close = true;
        return;
    }

    buf_consume(conn->outgoing, (size_t) rv);

    if (conn->outgoing.size() == 0) {
        conn->want_write = false;
        conn->want_read = true;
    }

    return;
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
        std::vector<struct pollfd> poll_args; // should eventually transition to epoll

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

                    int32_t ready = poll_args[i].revents; //did something happen?
                    Conn* conn = fd2conn[poll_args[i].fd];

                    //bitwise masks
                    if (POLLIN & ready) {
                        handle_read(conn);
                    }
                    
                    if (POLLOUT & ready) {
                        handle_write(conn);
                    }

                    if ((ready & POLLERR) || conn->want_close) {
                        (void)close(conn->fd);
                        fd2conn[conn->fd] = NULL;
                        delete conn;
                    }
                } 
            }
        }
    }

    return 0;

}
