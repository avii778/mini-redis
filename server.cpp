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

const size_t K_MAX_MSG = 4096;

void die(const char* message) { 

    perror(message);
    exit(1);

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

void do_something(const int connfd) {

    char rbuf[64] = {};
    ssize_t rv = recv(connfd, rbuf, sizeof(rbuf) - 1, 0);

    if (rv <= 0) {
        die("recv() error");
        return;
    }

    printf("Client says %s\n", rbuf);

    char wbuf[] = "world";

    rv = send(connfd, wbuf, strlen(wbuf), 0); 

    if (rv == -1){
        die("Send() error");
    } 

    close(connfd);

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

        struct sockaddr_in client_addr = {};
        socklen_t addr_len = sizeof(client_addr);

        int connfd = accept(fd, (struct sockaddr *)&client_addr, &addr_len);

        if (connfd < 0) {
            continue;   // error
        }

        while (true) {

            int32_t err  = one_request(connfd);

            if (err) {
                break; // if err != 0 then something happened 
            }

        }

        close(connfd);

    }


    return 0;

}
