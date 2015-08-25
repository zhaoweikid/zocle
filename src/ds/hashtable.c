#include <zocle/ds/hashtable.h>
#include <zocle/ds/hashset.h>
#include <zocle/log/logfile.h>
#include <zocle/mem/alloc.h>
#include <zocle/utils/funcs.h>
#include <zocle/base/defines.h>
#include <zocle/str/string.h>
#include <string.h>

static zcHashTableNode*
zc_hashtable_node_new(char *key, int klen, void *value)
{
    zcHashTableNode   *node;
    node = (zcHashTableNode*)zc_malloc(sizeof(zcHashTableNode));  
    memset(node, 0, sizeof(zcHashTableNode));
    node->key   = zc_strdup(key, klen);
    node->value = value;
    return node;
}

static void
zc_hashtable_node_delete(void *h, void *n)
{
    if (NULL == h) {
        ZCWARN("zc_hashtable_node_delete NULL pointer\n");
    }
    zcHashTable     *ht   = (zcHashTable*)h;
    zcHashTableNode *node = (zcHashTableNode*)n;
    
    if (ht->keydel) {
        ht->keydel(node->key);
    }
    if (ht->valdel) {
        ht->valdel(node->value);
    }
    zc_free(node);
    
    /*
    zcHashTableNode *node = (zcHashTableNode*)h;
    if (node->key) {
        zc_free(node->key);
    }
    // FIXME: value 为什么不释放？
    zc_free(node);
    */
}

zcHashTable*  
zc_hashtable_new(int size)
{
    zcHashTable   *ht;
    if (zc_hashtable_new2(&ht, size) != ZC_OK)
        return NULL;
    return ht;
}

zcHashTable*
zc_hashtable_new_full(int size, zcFuncHash hash, zcFuncCmp cmp, 
                      zcFuncDel kdel, zcFuncDel vdel, zcFuncDel2 ndel)
{
    zcHashTable   *ht;
    if (zc_hashtable_new2_full(&ht, size, hash, cmp, kdel, vdel, ndel) != ZC_OK)
        return NULL;
    return ht;
}

int 
zc_hashtable_new2(zcHashTable **ht, int size)
{
    zcHashTable   *h;

    h = (zcHashTable*)zc_malloc(sizeof(zcHashTable));
    memset(h, 0, sizeof(zcHashTable));
    h->size   = size;
    h->cmp    = zc_cmp_str;
    h->hash   = zc_hash_bkdr;
    h->keydel = zc_free_func;
    h->valdel = NULL;
    h->nodedel = zc_hashtable_node_delete;
 
    h->bunks = (void**)zc_malloc(sizeof(void*) * size);
    memset(h->bunks, 0, sizeof(void*) * size);

    *ht = h;
    return ZC_OK;
}

int
zc_hashtable_new2_full(zcHashTable **ht, int size, zcFuncHash hash, zcFuncCmp cmp, 
                       zcFuncDel kdel, zcFuncDel vdel, zcFuncDel2 ndel)
{
    int ret = zc_hashtable_new2(ht, size);
    if (ret != ZC_OK)
        return ret;

    (*ht)->hash = hash;
    (*ht)->cmp  = cmp;
    (*ht)->keydel = kdel;
    (*ht)->valdel = vdel;
    (*ht)->nodedel = ndel;
    
    return ZC_OK;
}

void
zc_hashtable_delete(void *h)
{
    zcHashTable  *ht = (zcHashTable*)h;
    if (NULL == h) {
        ZCERROR("zc_hashtable_destroy NULL pointer:%p\n", ht);
        return;
    }
    zc_hashtable_clear(ht); 
    zc_free(ht->bunks);
    zc_free(ht);
}

void
zc_hashtable_clear(zcHashTable *ht)
{
    //Fixme: value not free ?
    return zc_hashset_clear((zcHashSet*)ht);
}

void
zc_hashtable_print(zcHashTable *ht)
{
    zcHashTableNode   *node;
    int i = 0;
    ZCINFO("====== hashtable size:%d, len:%d, bunks:%p ======\n", ht->size, ht->len, ht->bunks);
    zc_hashtable_node_foreach_start(ht, node)
        ZCINFO("%p | %s | %p\n", node, node->key, node->value);
        i++;
    zc_hashtable_node_foreach_end
    ZCINFO("====== end %d ======\n", i);
}

static zcHashTableNode*
zc_hashtable_lookup_key(zcHashTable *ht, char *key, int keylen)
{
    return (zcHashTableNode*)zc_hashset_lookup_key((zcHashSet*)ht, key, keylen);
}

int     
zc_hashtable_add_node(zcHashTable *ht, zcHashTableNode *node)
{
    return zc_hashset_add_node((zcHashSet*)ht, (zcHashSetNode*)node);
}

int     
zc_hashtable_add(zcHashTable *ht, char *key, int klen, void *value)
{
    zcHashTableNode *node = zc_hashtable_node_new(key, klen, value);
    int ret = zc_hashset_add_node((zcHashSet*)ht, (zcHashSetNode*)node);
    if (ret != ZC_OK) {
        zc_hashtable_node_delete(ht, node);
    }
    return ret;
}

int 
zc_hashtable_rm(zcHashTable *ht, char *key, int keylen)
{
    return zc_hashset_rm((zcHashSet*)ht, key, keylen);
}

int
zc_hashtable_set(zcHashTable *ht, char *key, int keylen, void *value)
{
    zcHashTableNode *node = zc_hashtable_lookup_key(ht, key, keylen);
    if (node == NULL) {
        return ZC_ERR_NOT_FOUND;
    }
    if (ht->valdel && node->value) {
        ht->valdel(node->value);
    }
    node->value = value;
    return ZC_OK;
}

void*
zc_hashtable_get(zcHashTable *ht, char *key, int keylen, void *defv)
{
    zcHashTableNode *v = zc_hashtable_lookup_key(ht, key, keylen);
    if (v == NULL)
        return defv;
    return v->value;
}

int 
zc_hashtable_haskey(zcHashTable *ht, char *key, int keylen)
{
    return zc_hashset_haskey((zcHashSet*)ht, key, keylen);
}

int 
zc_hashtable_resize(zcHashTable *ht, int newsize)
{
    return zc_hashset_resize((zcHashSet*)ht, newsize);
}

/*int 
zc_hashtable_set_hash(zcHashTable *ht, zcFuncHash hash)
{
    return zc_hashset_set_hash((zcHashSet*)ht, hash);
}

int 
zc_hashtable_set_cmp(zcHashTable *ht, zcFuncCmp cmp)
{
    return zc_hashset_set_cmp((zcHashSet*)ht, cmp);
}

int 
zc_hashtable_set_keydel(zcHashTable *ht, zcFuncDel del)
{
    return zc_hashset_set_keydel((zcHashSet*)ht, del);
}

int 
zc_hashtable_set_valdel(zcHashTable *ht, zcFuncDel del)
{
    if (NULL == del)
        return ZC_ERR_NULL;

    ht->valdel = del;
    return ZC_OK;
}

int 
zc_hashtable_set_nodedel(zcHashTable *ht, zcFuncDel2 del)
{
    return zc_hashset_set_nodedel((zcHashSet*)ht, del);
}*/

uint32_t 
zc_hashtable_size(zcHashTable *ht)
{
    return zc_hashset_size((zcHashSet*)ht);
}

uint32_t 
zc_hashtable_len(zcHashTable *ht)
{
    return zc_hashset_len((zcHashSet*)ht);
}

void
zc_hashtable_walk(zcHashTable *ht, zcHashTableWalkFunc walk, void *userdata)
{
    zcHashTableNode *node;
    int i;
    for (i = 0; i < (int)ht->size; i++) {
        node = (zcHashTableNode*)ht->bunks[i];
        while (node) {
            if (walk(node->key, node->value, userdata) == ZC_FALSE)
                break;
            node = (zcHashTableNode*)node->next;
        }
    }
}

