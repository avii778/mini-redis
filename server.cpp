#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

const size_t K_MAX_MSG = 4096;

static int32_t read_full(int connfd, char *rbuf, int length) {

    while (length > 0) {
        int32_t rv = recv(connfd, rbuf, length , 0);

        if (rv <= 0) {
            return -1; // error or EOF
        }

        assert((size_t) rv <= length);

        length -= (size_t) rv;
        rbuf += (size_t) rv;
    }

    return 0;
}

static int32_t write_all(int connfd, char *rbuf, int length) {
    return 0;
}
int32_t write_all(int connfd, char *rbuf, int length) {
    return 0;
}

void msg(const char* message) {

}

void die(const char* message) { 

    perror(message);
    exit(1);

}

int32_t one_request(int connfd) {
    
    char rbuf[4 + K_MAX_MSG]; // 4 bytes for length header

    int errno = 0;

    int32_t err = read_full(connfd, rbuf, 4);

    if (err) {
        msg(errno == 0 ? "EOF" : "read() error");
        return err;
    }

    uint32_t length = 0;
    memcpy(&length, rbuf, 4); 

    if (length > K_MAX_MSG) {
        msg("too long");
        return -1;
    }

    // actually read the request
    err = read_full(connfd, &rbuf[4], length);


    if (err) {
        msg("read() error");
        return err;
    }





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
