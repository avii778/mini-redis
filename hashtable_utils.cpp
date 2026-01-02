#include "hashtable.h"
#include <cstddef>
#include <assert.h>
#include <cstdlib>

static void h_init(HTab *h, size_t n) {

    // make this hashtable point to the array of size n
    assert(n > 0 && ((n - 1) & n) == 0);
    h->tab = (HNode **) calloc(n, sizeof(HNode*));
    h->size = 0;
    h-> mask = n - 1;
}

static void h_insert(HTab *htab, HNode *node) {

    // hash the node and add it to the end of it's hash
    size_t pos = node->hcode & htab->mask;  // node->hcode & (n - 1)
    HNode *next = htab->tab[pos];
    node->next = next;
    htab->tab[pos] = node; // insert in one
    htab->size++;
}

static HNode **h_lookup(HTab *htab, HNode *key, bool (*eq)(HNode *, HNode *)) {

    // the real magic of this function is the **. it always makes it so that it points to the prev nodes pointer to the part, makes deleting and inserting trivial
    if (!htab->tab) {
        return nullptr;
    }

    size_t pos = key->hcode & htab->mask;
    HNode** ll = &htab->tab[pos]; // this is the linked list we must iterate on

    while (*ll) {

        if ((*ll)->hcode == key->hcode && eq(*ll, key)) {
            return ll;
        }

        ll = &(*ll)->next;
    }

    return nullptr;
}

static HNode *h_delete(HTab *htab, HNode **from) {
    // little pointer indirection, since we provide this with lookup, this will point to the previous node's next field, which we then modify
    // by derefrencing and changing it to be the derefrenced next field next field, a bit of a mouthful
    HNode *node = *from;    
    *from = node->next;
    htab->size--;
    return node;
}

static void hm_migrate_keys(HMap *h) {

    size_t work_done = 0;

    // go through all the shit in the older hmap and migrate it

    while (work_done < k_rehashing_work && h->older.size > 0) {

        HNode **from = &h->older.tab[h->migrate_pos]; // migrate this hash
        
        if (!*from) {
            h->migrate_pos++;
            continue;   // empty slot
        }

        h_insert(&h->newer, h_delete(&h->older, from));
        work_done++;
    }


    if (h->older.size == 0 && h->older.tab) {
        free(h->older.tab);
        h->older = HTab{};
    }
}

// to be used when we must resize
static void hm_trigger_rehashing(HMap *map) {
    map->older = map->newer;
    h_init(&map->newer, (map->newer.mask + 1) * 2);
    map->migrate_pos = 0;
}

HNode *hm_lookup(HMap *hmap, HNode *key, bool (*eq)(HNode *, HNode *)) {

    // look within the newer hashmap and the older one
    hm_migrate_keys(hmap); // O(1) on the hotpath, fine

    HNode **from = h_lookup(&hmap->newer, key, eq);
    if (!from) {
        from = h_lookup(&hmap->older, key, eq);
    }
    return from ? *from : NULL;

}

HNode *hm_delete(HMap *hmap, HNode *key, bool (*eq)(HNode *, HNode *)) {

    if (HNode **from = h_lookup(&hmap->newer, key, eq)) {
        return h_delete(&hmap->newer, from);
    }

    if (HNode **from = h_lookup(&hmap->older, key, eq)) {
        return h_delete(&hmap->older, from);
    }

    return nullptr;
}

void hm_insert(HMap *hmap, HNode *node) {
    
    
    // we may need to trigger resize
    if (!hmap->newer.tab) {
        h_init(&hmap->newer, 4);
    }

    h_insert(&hmap->newer, node);
    if (!hmap->older.tab) {
        
        size_t bound = (hmap->newer.mask + 1) * k_max_load_factor; // if it basically reached its threshold
        if (hmap->newer.size >= bound) {
            hm_trigger_rehashing(hmap);
    }

    hm_migrate_keys(hmap);
}
