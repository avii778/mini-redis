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
#include "buff.h"
#include <vector>
#include <string>

const size_t K_MAX_MSG = 4096;

bool read_str(std::string& str, const uint32_t* src, uint32_t *curr, const uint32_t length, const uint32_t* src_end) {

    if (src + *curr + length > src_end) {
        return false;
    }
    
    str.assign(src + *curr, src + *curr + length);
    *curr += 4;
    return true;
}

bool read_u32(uint32_t* dest, const uint32_t* src, const uint32_t length, const uint32_t ptr) {
    // returns true on success, false otherwise

    if (length - ptr < 4) {
        return false; 
    }

    memcpy(dest, src + ptr, 4);
    return true;
}

int32_t parse_req(const uint32_t* b, uint32_t length, std::vector<std::string> &cmd){

    const uint32_t *end = b + length;
    uint32_t nstr = 0;
    uint32_t curr_ptr = 0;


    if (!read_u32(&nstr, b, length, curr_ptr)) {
        return -1;
    }

    curr_ptr += 4;
    
    while (cmd.size() < nstr) {
        
        uint32_t len;

        if (!read_u32(&len, b, length, curr_ptr)) {
            return -1;
        }

        curr_ptr += 4;
        std::string str;

        if (!read_str(str, b, &curr_ptr, len, end)) {
            return -1;
        }

        cmd.push_back(str);
    }

    return 0;

}

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

void fd_set_nb(int fd) {
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
}

int buf_append(struct Buffer *buf, const uint32_t *data, size_t len) {
    /**
     * Tries to append to the buffer, returns -1 if the buffer is full
     * @returns true if sucessful, false if non-successful
     */

    if (buf->data_end + len > buf->buffer_end) {return -1;}
    memcpy(buf->data_end, data, len);
    buf->data_end += len;
    return 0;
}

void buf_consume(struct Buffer *buf, size_t len) {
    buf->data_begin += len;
}

bool try_one_request(Conn *conn) {
    /**
     * Tries to parse the one request within conn incoming, and will parse the command as well as trying to create the response data.
     * @returns true if sucessful, false if non-successful
     */

    if (conn->incoming.size() < 4){
        return false;
    }
    
    uint32_t be_len = 0;
    memcpy(&be_len, conn->incoming.data(), 4);
    uint32_t length = ntohl(be_len);

    if (length > K_MAX_MSG) {
        conn->want_close = true; // fairly idiomatic what i'm doing here
        return false;
    }

    if (conn->incoming.size() < 4 + length) return false;

    std::vector<std::string> cmd;

    if (parse_req(conn->incoming.data(), length, cmd) < 0) {
        conn->want_close = true;
        return false;
    }

    // basically just responding with what they sent
    const uint32_t *request = conn->incoming.data() + 4;
    // otherwise handle the message
    int r = buf_append(&conn->outgoing, (const uint32_t *)&length, 4);
    buf_append(&conn->outgoing, request, length);

    // 5. Remove the message from `Conn::incoming`.
    buf_consume(&conn->incoming, 4 + length);

    return true;
} 

