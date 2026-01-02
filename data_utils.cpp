#include "data.h"
#include <functional>
#include <cstdint>
#include <assert.h>
#include <cstddef>
#include <type_traits>

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

static void do_get(std::vector<std::string> &cmd, Response &out) {

    // will fill out the response using the cmds using the g_data

    Entry key;
    key.key.swap(cmd[1]);
    key.node.hcode = str_hash((uint8_t *)key.key.data(), key.key.size());

    HNode *candidate = hm_lookup(&g_data.db, &key.node, &entry_eq); // placeholder for now
    
    if (!candidate) {
        out.status = 1;
        return;
    }

    const std::string &val = container_of(candidate, Entry, node)->val;
    assert(val.size() <= k_max_msg);
    out.status = 0;
    out.data.assign(val.begin(), val.end());
    return;
}

static void do_set(std::vector<std::string> &cmd, Response &out) {

    Entry *key = new Entry{}; //allocation into the heap so it doesn't dangle
    key->key = cmd[1];
    key->val = cmd[2];
    key->node.hcode = str_hash((uint8_t *)key->key.data(), key->key.size());

    if (HNode *n = hm_delete(&g_data.db, &key->node, &entry_eq)) {
        Entry *e = container_of(n, Entry, node);
        delete e;
    }

    hm_insert(&g_data.db, &key->node);
    out.status = 0;
}

static void do_delete(std::vector<std::string> &cmd, Response &out) {

    Entry key;
    key.key.swap(cmd[1]);
    key.node.hcode = str_hash((uint8_t *)key.key.data(), key.key.size());

    HNode *n = hm_delete(&g_data.db, &key.node, &entry_eq);
    if (n) {
        Entry *e = container_of(n, Entry, node);
        delete e;
    }

    out.status = n ? 0 : 1;
}