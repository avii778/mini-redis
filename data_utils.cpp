#include "data.h"
#include <functional>
#include <cstdint>
#include <assert.h>
#include <cstddef>
#include <type_traits>
#include "buff.h"
#include "serialization.h"
#include "write_and_read.h"

static int k_max_msg = 4096;

#define container_of(ptr, type, member) \
    reinterpret_cast<type*>( reinterpret_cast<std::uintptr_t>(ptr) - offsetof(type, member) )
    
static bool entry_eq(HNode *lhs, HNode *rhs) {
    struct Entry *le = container_of(lhs, struct Entry, node);
    struct Entry *re = container_of(rhs, struct Entry, node);
    return le->key == re->key;
}

// will transition to murmur eventually, this is just a small thingy rn
static uint64_t str_hash(uint8_t *data, size_t size) {

    return (uint64_t) 1;
}

void do_get(std::vector<std::string> &cmd, Buffer &out) {

    // will fill out the response using the cmds using the g_data

    Entry key;
    key.key.swap(cmd[1]);
    key.node.hcode = str_hash((uint8_t *)key.key.data(), key.key.size());

    HNode *candidate = hm_lookup(&g_data.db, &key.node, &entry_eq); // placeholder for now
    
    if (!candidate) {
        return out_int(out, 1);
    }

    const std::string &val = container_of(candidate, Entry, node)->val;
    assert(val.size() <= k_max_msg);
    return out_str(out, val.data(), val.size());
}

void do_set(std::vector<std::string> &cmd, Buffer &out) {

    Entry *key = new Entry{}; //allocation into the heap so it doesn't dangle
    key->key = cmd[1];
    key->val = cmd[2];
    key->node.hcode = str_hash((uint8_t *)key->key.data(), key->key.size());

    if (HNode *n = hm_delete(&g_data.db, &key->node, &entry_eq)) {
        Entry *e = container_of(n, Entry, node);
        delete e;
    }

    hm_insert(&g_data.db, &key->node);
    return out_int(out, 1);
}

void do_delete(std::vector<std::string> &cmd, Buffer &out) {

    Entry key;
    key.key.swap(cmd[1]);
    key.node.hcode = str_hash((uint8_t *)key.key.data(), key.key.size());

    HNode *n = hm_delete(&g_data.db, &key.node, &entry_eq);
    if (n) {
        Entry *e = container_of(n, Entry, node);
        delete e;
    }

    return n ? out_int(out, 0) : out_int(out, 1);
}

static void out_nil(Buffer &out) {

    uint8_t tag = TAG_NIL;
    buf_append(&out, &tag, 1);
}

static void out_str(Buffer &out, const char *str, size_t size) {

    uint8_t tag = TAG_STR;
    uint32_t size_addr = size;
    buf_append(&out, &tag, 1);
    buf_append(&out, (uint8_t *) &size_addr , 4);
    buf_append(&out, (uint8_t *) str, size);
}

static void out_int(Buffer &out, int64_t lel) {
    uint8_t tag = TAG_INT;
    uint32_t size = 8;
    buf_append(&out, &tag, 1);
    buf_append(&out, (uint8_t *) &size, sizeof(size));
    buf_append(&out, (uint8_t *) &lel, 8);
}

static void out_arr(Buffer &out, uint32_t n) {
    // lol 
    uint8_t tag = TAG_ARR;
    buf_append(&out, &tag, 1);
    buf_append(&out, (uint8_t *) &n, 4);    
}