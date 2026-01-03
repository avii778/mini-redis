#ifndef DATA_H
#define DATA_H


#include <string>
#include "hashtable.h"
#include "response.h"

static struct {
    HMap db;
} g_data;

struct Entry {
    struct HNode node;
    std::string key;
    std::string val;
};

void do_get(std::vector<std::string> &cmd, Buffer &out);
void do_set(std::vector<std::string> &cmd, Buffer &out);
void do_delete(std::vector<std::string> &cmd, Buffer &out);

#endif