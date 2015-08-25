#include <zocle/ds/hashset.h>
#include <zocle/log/logfile.h>
#include <zocle/mem/alloc.h>
#include <zocle/utils/funcs.h>
#include <zocle/base/defines.h>
#include <zocle/str/string.h>
#include <string.h>

zcHashSetNode*
zc_hashset_node_new(char *key, int keylen)
{
    zcHashSetNode *node = (zcHashSetNode*)zc_malloc(sizeof(zcHashSetNode));  
    memset(node, 0, sizeof(zcHashSetNode));
    node->key = zc_strdup(key, 0);
    return node;
}

void
zc_hashset_node_delete(void *n)
{
    if (NULL == n) {
        ZCWARN("zc_hashset_node_destroy NULL pointer\n");
    }
    //zcHashSet *h = (zcHashSet*)x;
    zcHashSetNode *node = (zcHashSetNode*)n;

    /*if (h->keydel) {
        h->keydel(n->key);
    }*/
    zc_free(node->key);
    zc_free(node);
}

zcHashSet*  
zc_hashset_new(int size)
{
    zcHashSet   *h;

    if (zc_hashset_new2(&h, size) != ZC_OK)
        return NULL;
    return h;
}

zcHashSet*
zc_hashset_new_full(int size, zcFuncHash hash, zcFuncCmp cmp, 
                     zcFuncDel keydel, zcFuncDel2 nodedel)
{
    zcHashSet   *h;

    if (zc_hashset_new2_full(&h, size, hash, cmp, keydel, nodedel) != ZC_OK)
        return NULL;

    return h;
}

int 
zc_hashset_new2(zcHashSet **hash, int size)
{
    zcHashSet   *h;

    h = (zcHashSet*)zc_malloc(sizeof(zcHashSet));
    memset(h, 0, sizeof(zcHashSet));
    h->size  = size;
    h->cmp   = zc_cmp_str;
    h->hash  = zc_hash_bkdr;
    h->keydel  = zc_free_func;
    h->nodedel = zc_hashset_node_delete;
 
    h->bunks = (void**)zc_malloc(sizeof(void*) * size);
    memset(h->bunks, 0, sizeof(zcHashSetNode*) * size);

    *hash = h;
    return ZC_OK;
}

int
zc_hashset_new2_full(zcHashSet **h, int size, zcFuncHash hash, zcFuncCmp cmp, 
                      zcFuncDel keydel, zcFuncDel2 nodedel)
{
    int ret = zc_hashset_new2(h, size);
    if (ret != ZC_OK)
        return ret;

    (*h)->hash = hash;
    (*h)->cmp  = cmp;
    (*h)->keydel  = keydel;
    (*h)->nodedel = nodedel;

    return ZC_OK;
}

void
zc_hashset_delete(void *h)
{
    if (NULL == h) {
        ZCERROR("zc_hashset_destroy NULL pointer:%p\n", h);
        return;
    }
    zcHashSet   *ha = (zcHashSet*)h;
    zc_hashset_clear(ha); 
    zc_free(ha->bunks);
    zc_free(ha);
}

void
zc_hashset_clear(zcHashSet *h)
{
    zcHashSetNode *node;
    zcHashSetNode *tmp;
    int i;
    for (i = 0; i < h->size; i++) {
        node = (zcHashSetNode*)h->bunks[i];
        while (node) {
            tmp  = (zcHashSetNode*)node;
            node = node->next;
            if (h->nodedel) {
                h->nodedel(h, tmp);
            }
            /*
            if (h->keydel) {
                h->keydel(tmp->key);
            }
            // FIXME: value 没释放
            zc_free(tmp);
            */
        }
    }
    memset(h->bunks, 0, sizeof(void*) * h->size);
}

void
zc_hashset_print(zcHashSet *h)
{
    zcHashSetNode   *node;
    int i = 0;
    ZCINFO("====== hashset size:%d, len:%d, bunks:%p ======\n", h->size, h->len, h->bunks);
    zc_hashset_foreach(h, node)
        ZCINFO("%p | %s\n", node, node->key);
        i++;
    zc_hashset_foreach_end
    ZCINFO("====== end %d ======\n", i);
}

zcHashSetNode*
zc_hashset_lookup_key(zcHashSet *h, char *key, int keylen)
{
    uint32_t    hv = h->hash(key, keylen) % h->size;
    zcHashSetNode  *root = h->bunks[hv];    
    
    while (root) {
        if (h->cmp(root->key, key, 0) == 0) {
            return root;
        }
        root = (zcHashSetNode*)root->next;
    }
    return NULL;
}

int     
zc_hashset_add_node(zcHashSet *h, zcHashSetNode *node)
{
    if (NULL == node)
        return ZC_ERR_NULL;
    
    uint32_t hv = h->hash(node->key, strlen(node->key)) % h->size;
    zcHashSetNode *root = h->bunks[hv];    

    node->next = root;
    while (root) {
        if (h->cmp(root->key, node->key, 0) == 0) {
            return ZC_ERR_EXIST;
        }
        root = (zcHashSetNode*)root->next;
    }
    h->bunks[hv] = node; 
    h->len++;

    return ZC_OK;
}
// must override
int     
zc_hashset_add(zcHashSet *h, char *key, int keylen)
{
    if (NULL == key)
        return ZC_ERR_NULL;
    if (keylen <= 0)
        keylen = strlen(key);
    
    uint32_t hv = h->hash(key, keylen) % h->size;
    zcHashSetNode *root = h->bunks[hv];    
    
    while (root) {
        if (h->cmp(root->key, key, 0) == 0) {
            return ZC_ERR_EXIST;
        }
        root = (zcHashSetNode*)root->next;
    }
    
    zcHashSetNode *node = zc_hashset_node_new(key, keylen);
    node->next   = (zcHashSetNode*)h->bunks[hv];
    h->bunks[hv] = node; 
    h->len++;

    return ZC_OK;
}

int 
zc_hashset_rm(zcHashSet *h, char *key, int keylen)
{
    if (NULL == key)
        return ZC_ERR_NULL;
    if (keylen <= 0)
        keylen = strlen(key);
    
    uint32_t hv = h->hash(key, keylen) % h->size;
    zcHashSetNode *root = h->bunks[hv];    
    zcHashSetNode *prev = NULL;

    while (root) {
        if (h->cmp(root->key, key, 0) == 0) {
            if (prev) {
                prev->next = root->next;
            }else{
                h->bunks[hv] = (zcHashSetNode*)root->next;
            }
            //zc_hashset_delete_node(h, root);
            h->nodedel(h, root);
            /*
            if (h->keydel) {
                h->keydel(root->key);
            }
            // FIXME: value为什么不删除呢？
            zc_free(root);
            h->len--;
            */
            return ZC_OK;
        }
        prev = root;
        root = (zcHashSetNode*)root->next;
    }
    return ZC_ERR_NOT_FOUND;
}

int 
zc_hashset_haskey(zcHashSet *h, char *key, int keylen)
{
    if (keylen < 0)
        keylen = strlen(key);
    
    if (NULL == zc_hashset_lookup_key(h, key, keylen)) {
        return ZC_FALSE;
    }
    return ZC_TRUE;
}

int 
zc_hashset_resize(zcHashSet *h, int newsize)
{
    void **newbunks = (void**)zc_malloc(sizeof(void*) * newsize);
    zcHashSetNode  *node, *nextnode;
    int i; 
    for (i = 0; i < h->size; i++) {
        node = (zcHashSetNode*)h->bunks[i]; 
        while (node) {
            nextnode = (zcHashSetNode*)node->next;
            uint32_t hv = h->hash(node->key, strlen(node->key)) % newsize;
            zcHashSetNode  *root = (zcHashSetNode*)newbunks[hv];    
            node->next = root; 
            newbunks[hv] = node;
            node = nextnode;
        }
    }
    zc_free(h->bunks);
    h->bunks = newbunks;
    h->size  = newsize;

    return ZC_OK;
}

uint32_t 
zc_hashset_size(zcHashSet *h)
{
    return h->size;
}

uint32_t 
zc_hashset_len(zcHashSet *h)
{
    return h->len;
}


