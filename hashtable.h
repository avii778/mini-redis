#ifndef HASHTABLE_H
#define HASHTABLE_H

#include <cstdint>
#include <cstddef>
#include <string>

struct HNode {
    HNode *next = nullptr;
    uint64_t hcode = 0; 

};

struct HTab {
    
    HNode **tab = nullptr; // an array of arrays (buckets)
    size_t mask = 0; // % n mask
    size_t size = 0; // size of current array ( may resize if it gets too large)
};

struct HMap {
    HTab newer;
    HTab older; // resizing
    size_t migrate_pos;
};


HNode *hm_lookup(HMap *hmap, HNode *key, bool (*eq)(HNode *, HNode *));
void hm_insert(HMap *hmap, HNode *node);
HNode *hm_delete(HMap *hmap, HNode *key, bool (*eq)(HNode *, HNode *));
const size_t k_max_load_factor = 8;
const size_t k_rehashing_work = 128;

#endif