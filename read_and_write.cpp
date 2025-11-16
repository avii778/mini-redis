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
#include <fcntl.h>      // for fcntl(), F_SETFL, F_GETFL, O_NONBLOCK
#include "conn.h"
#include <cstdint>     // for uint8_t

const size_t K_MAX_MSG = 4096;

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

static void fd_set_nb(int fd) {
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
}

void buf_append(std::vector<uint8_t> &buf, const uint8_t *data, size_t len) {
    buf.insert(buf.end(), data, data + len);
}

void buf_consume(std::vector<uint8_t> &buf, size_t len) {
    buf.erase(buf.begin(), buf.begin() + len);
}

static bool try_one_request(Conn *conn) {
    /**
     * Tries to parse the one request within conn incoming.
     * @returns true if sucessful, false if non-successful
     */

    if (conn->incoming.size() < 4){
        return false;
    }
    
    uint32_t length = 0;
    memcpy(&length, conn->incoming.data(), 4);

    if (length > K_MAX_MSG) {
        conn->want_close = true; // fairly idiomatic what i'm doing here
        return false;
    }

    const uint8_t *request = &conn->incoming[4];
    // otherwise handle the message
    buf_append(conn->outgoing, (const uint8_t *)&length, 4);
    buf_append(conn->outgoing, request, length);

    // 5. Remove the message from `Conn::incoming`.
    buf_consume(conn->incoming, 4 + length);

    return true;
} 

