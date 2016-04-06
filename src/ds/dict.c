#include <zocle/ds/dict.h>
#include <zocle/base/defines.h>
#include <zocle/mem/alloc.h>
#include <string.h>
#include <zocle/str/string.h>
#include <zocle/utils/funcs.h>

const char *ZC_KEY_DUMMY = "<DUMMY>";

zcDictNode* 
zc_dict_node_init(zcDictNode *htn, char *key, int keylen, void *value)
{
    memset(htn, 0, sizeof(zcDictNode));
    htn->key   = zc_strdup(key, keylen);
    htn->value = value;
    htn->hash  = zc_hash_bkdr(key, keylen);
    return htn;
}

void
zc_dict_node_destroy(void *hn, zcFuncDel keydel, zcFuncDel valdel)
{
    zcDictNode *htn = (zcDictNode*)hn;
    if (htn->key && htn->key != ZC_KEY_DUMMY && keydel) {
        keydel(htn->key);
        if (htn->value) {
            valdel(htn->value);
        }
    }
}

//zc_dict_new(int minsize=8, char valdel=ZC_DEL_NO, bool reduce=false) {
/**
 * create dict
 * @param minsize  minimal size of hash bunk
 * @param valdel   destroy function for value
 * @param reduce   bunk maybe reduce?
 */
zcDict*
zc_dict_new(int size) 
{
    return zc_dict_new_full(size, 0, zc_free_func, zc_nofree_func);
}

zcDict*
zc_dict_new_full(int minsize, char reduce, zcFuncDel keydel, zcFuncDel valdel) 
{
    if (minsize <= 0)
        minsize = 8;

    zcDict *ht = (zcDict*)zc_malloc(sizeof(zcDict));
    memset(ht, 0, sizeof(zcDict));

    int newsize;
    for(newsize=8; newsize<= minsize && newsize>0; newsize<<=1);
    
    ht->__type = ZC_DICT;
    ht->size   = newsize;
    ht->reduce = reduce;
    ht->table  = (zcDictNode*)zc_malloc(sizeof(zcDictNode)*newsize);//new zcDictNode<T>[size];
    memset(ht->table, 0, sizeof(zcDictNode)*newsize);
    ht->filled = ht->len = 0;
    ht->hash   = zc_hash_bkdr;
    ht->keydel = zc_free_func;
    ht->valdel = valdel;

    return ht;
}

void 
zc_dict_delete(void *h) 
{
    zcDict *ht = (zcDict*)h;
    zc_dict_clear(ht);
    zc_free(ht->table);
    zc_free(h);
}

static zcDictNode* 
zc_dict_lookup_h(zcDict *ht, const char *key, uint32_t khash, int klen) 
{
    uint32_t i = khash & (ht->size-1);
    zcDictNode *node = &ht->table[i];
    if (node->key == NULL || node->key == key)
        return node;

    zcDictNode *freenode = NULL;
    if (node->key == ZC_KEY_DUMMY) {
        freenode = node;
    }else{
        if (node->hash == khash && strcmp(node->key,key) == 0) {
            return node;
        }
        freenode = NULL;
    }
    uint32_t p;
    for (p=khash; ; p>>=5) {
        i = (i << 2) + i + p + 1;
        node = &ht->table[i & (ht->size-1)];
        if (node->key == NULL) {
            return (freenode==NULL) ? node : freenode;
        }
        if (node->key == key) {
            return node;
        }
        if (node->hash == khash && strcmp(node->key,key) == 0) {
            return node;
        }
        if (node->key == ZC_KEY_DUMMY && freenode == NULL)
            freenode = node;
    }
    ZCWARN("not arrive here!\n");
    return NULL;
}

static zcDictNode* 
zc_dict_lookup(zcDict *ht, const char *key, int klen) 
{
    if (klen <= 0)
        klen = strlen(key);
    uint32_t khash = ht->hash((void*)key,klen);
    return zc_dict_lookup_h(ht, key, khash, klen);
}


void 
zc_dict_clear(zcDict *ht)
{
    zcDictNode *node;
    uint32_t i;
    for (i=0; i<ht->size; i++) {
        node = &ht->table[i];
        if (node->key != NULL && node->key != ZC_KEY_DUMMY) {
            ht->keydel(node->key);
            if (node->value) {
                ht->valdel(node->value);
            }
        }
    }
    memset(ht->table, 0, sizeof(zcDictNode)*ht->size);
    ht->len = ht->filled = 0;
}

int 
zc_dict_add(zcDict *ht, const char *key, int keylen, void *value) 
{
    zcDictNode *node = zc_dict_lookup(ht, key, keylen);
    if (NULL == node)
        return ZC_ERR;

    if (node->key == NULL || node->key == ZC_KEY_DUMMY) {
        uint32_t khash = ht->hash((void*)key,keylen);
        if (node->key == NULL) {
            ht->filled++;
        }
        node->key   = zc_strdup(key, keylen);
        node->value = value;
        node->hash  = khash;
        ht->len++;

        if (ht->filled*3 >= ht->size*2)
            zc_dict_resize(ht, 0);
    }
    return ZC_OK;
}


int 
zc_dict_set(zcDict *ht, const char *key, int keylen, void *value) 
{
    if (keylen <= 0)
        keylen = strlen(key);
    uint32_t khash = ht->hash((void*)key,keylen);
    zcDictNode *node = zc_dict_lookup_h(ht, key, khash, keylen);
    if (NULL == node)
        return ZC_ERR;

    if (node->key != NULL && node->key != ZC_KEY_DUMMY) {
        ht->valdel(node->value);
        node->value = value;
    }else{
        if (node->key == NULL) {
            ht->filled++;
        }
        node->key   = zc_strdup(key, keylen);
        node->value = value;
        node->hash  = khash;
        ht->len++;

        if (ht->filled*3 >= ht->size*2)
            zc_dict_resize(ht, 0);
    }

    return ZC_OK;

    /*zcDictNode *node = zc_dict_lookup(ht, key, keylen);
    if (node == NULL)
        return ZC_ERR;

    uint32_t khash = ht->hash((void*)key,keylen);
    if (node->key == NULL) {
        ht->filled++;
    }else if (node->key != ZC_KEY_DUMMY){
        ht->keydel(node->key);
    }
    node->key   = zc_strdup(key, keylen);
    node->value = value;
    node->hash  = khash;
    ht->len++;

    if (ht->filled*3 >= ht->size*2)
        zc_dict_resize(ht, 0);

    return ZC_OK;
    */
}

void* 
zc_dict_get(zcDict *ht, const char *key, int keylen, void *defv) 
{
    zcDictNode *node = zc_dict_lookup(ht, key, keylen);
    if (node == NULL || node->key == NULL || node->key == ZC_KEY_DUMMY) {
        return defv;
    }
    return node->value;
}


int 
zc_dict_rm(zcDict *ht, const char *key, int keylen) 
{
    zcDictNode *node = zc_dict_lookup(ht, key, keylen);
    if (node != NULL && node->key != NULL && node->key != ZC_KEY_DUMMY) {
        ht->keydel(node->key);
        if (node->value) {
            ht->valdel(node->value);
        }
        node->key = (char*)ZC_KEY_DUMMY;
        node->value = NULL;
        ht->len--;
    }else{
        return ZC_ERR_NOT_FOUND;
    }
    return ZC_OK;
}

int 
zc_dict_haskey(zcDict *ht, const char *key, int keylen) 
{
    zcDictNode *node = zc_dict_lookup(ht, key, keylen);
    if (node != NULL && node->key != NULL && node->key != ZC_KEY_DUMMY) {
        return ZC_TRUE;
    }
    return ZC_FALSE;
}


int 
zc_dict_resize(zcDict *ht, uint32_t minsize) 
{
    // minsize default = 0
    uint32_t newsize;
    if (minsize <= 0) {
        //minsize = (len > 50000 ? 2 : 4)*len;
        minsize = (ht->len > 50000 ? 2 : 4)*ht->size;
    }
    for(newsize=8; newsize<= minsize && newsize>0; newsize<<=1);
        
    //ZCINFO("newsize:%d\n", newsize);
    if (newsize == ht->size)
        return ZC_OK;
    if (!ht->reduce && newsize <= ht->size)
        return ZC_OK;

    //ZCINFO("newsize:%d", newsize);
    zcDictNode *newtable = (zcDictNode*)zc_malloc(sizeof(zcDictNode)*newsize);
    memset(newtable, 0, sizeof(zcDictNode)*newsize);

    zcDictNode *node;
    int n = ht->len;
    uint32_t newmask = newsize-1;
    uint32_t p;
    uint32_t newlen = 0;
    for (node=ht->table; n>0; node++) {
        if (node->key != NULL && node->key != ZC_KEY_DUMMY) {
            uint32_t i = node->hash & newmask;
            zcDictNode *n2 = &newtable[i];
            for (p = node->hash; n2->key != NULL; p >>= 5) {
                i = (i << 2) + i + p + 1;
                n2 = &newtable[i & newmask];
            }
            newlen++;
            memcpy(n2, node, sizeof(zcDictNode));
            n--;
            node->key = NULL;
        }
    }
    ht->filled = ht->len = newlen;
    ht->size = newsize;
    zcDictNode *oldtable = ht->table;
    ht->table = newtable;
    zc_free(oldtable);
    return ZC_OK;
}

int 
zc_dict_walk(zcDict *ht, zcDictWalkFunc walkfunc, void *userdata) 
{
    zcDictNode *node;
    int n = ht->len;
    for (node=ht->table; n>0; node++) {
        if (node->key != NULL && node->key != ZC_KEY_DUMMY) {
            if (walkfunc(node->key, node->value, userdata) == ZC_FALSE)
                break;
            n--;
        }
    }
    return ZC_OK;
}

void 
zc_dict_print(zcDict *ht)
{
    zcDictNode *node;
    int n = ht->len;
    ZCINFO("------ dict %p ------", ht);
    for (node=ht->table; n>0; node++) {
        if (node->key != NULL && node->key != ZC_KEY_DUMMY) {
            ZCINFO("i:%d node:%p, key:%s value:%p", ht->len-n, node, node->key, node->value);
            n--;
        }
    }
    ZCINFO("------ dict end %p ------", ht);
}


