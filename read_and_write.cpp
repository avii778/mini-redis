#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>

int32_t read_full(int connfd, char *rbuf, int length) {

    while (length > 0) {
        ssize_t rv = recv(connfd, rbuf, length , 0);

        if (rv <= 0) {
            return -1; // error or EOF
        }

        assert((size_t) rv <= length);

        length -= (size_t) rv;
        rbuf += (size_t) rv;
    }

    return 0;
}

int32_t write_all(int connfd, char *rbuf, int length) {

    // write everything from buff into the connfd byte stream

    while (length > 0 ) {

        ssize_t rv = send(connfd, rbuf, length, 0);

        if (rv <= 0) {
            return -1; // error or EOF
        }

        assert((size_t) rv <= length);

        length -= (size_t) rv;
        rbuf += rv;
    }

    return 0;
}

void msg(const char* message) {
    perror(message);
}

void die(const char* message) { 

    perror(message);
    exit(1);

}