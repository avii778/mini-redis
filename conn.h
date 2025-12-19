#ifndef CONN_H
#define CONN_H

#include <vector>      // for std::vector
#include <cstdint>    
#include "buff.h"

// Represents a single client connection
struct Conn {
    int fd = -1;                      // file descriptor for the socket

    bool want_read = false;           // whether we want to read from this connection
    bool want_write = false;          // whether we want to write to this connection
    bool want_close = false;          // whether this connection should be closed

    Buffer incoming;    // data received from the client
    Buffer outgoing;    // data to send to the client
};

#endif // CONN_H
