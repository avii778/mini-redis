#ifndef BUFF_H
#define BUFF_H

#include <cstdint>
#include <cstdlib>

struct Buffer {
    // non copyable

    uint32_t *buffer_begin;
    uint32_t *buffer_end;
    uint32_t *data_begin;
    uint32_t *data_end;

    int size() {
        return data_end - data_begin;
    }

    uint32_t* data() {
        return data_begin;
    }

    Buffer() {
        buffer_begin = (uint32_t *)malloc(4096);
        buffer_end = buffer_begin + 4096;
        data_begin = buffer_begin;
        data_end = buffer_begin;
    }

    ~Buffer() {
        free(buffer_begin);
    }

    Buffer(Buffer&& other) noexcept {
        buffer_begin = other.buffer_begin;
        buffer_end   = other.buffer_end;
        data_begin   = other.data_begin;
        data_end     = other.data_end;

        other.buffer_begin = nullptr;
        other.buffer_end   = nullptr;
        other.data_begin   = nullptr;
        other.data_end     = nullptr;
    }

    Buffer& operator=(Buffer&& other) noexcept {

        if (this != &other) {
            free(buffer_begin);

            buffer_begin = other.buffer_begin;
            buffer_end = other.buffer_end;
            data_begin = other.data_begin;
            data_end = other.data_end;

            other.buffer_begin = nullptr;
            other.buffer_end = nullptr;
            other.data_begin = nullptr;
            other.data_end = nullptr;
        }

        return *this;
}

};


#endif