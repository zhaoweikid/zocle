#include <zocle/protocol/dns/resolver.h>
#include <zocle/net/sockets.h>
#include <zocle/base/defines.h>
#include <zocle/mem/alloc.h>
#include <zocle/ds/list.h>
#include <zocle/log/logfile.h>
#include <string.h>
#include <time.h>
#include <zocle/server/asynconn.h>

#ifndef random
#define random rand
#endif

#ifndef srandom
#define srandom srand
#endif

void
zc_dnsheader_print(zcDNSHeader *h)
{
    ZCINFO("DNS Header id:%d, qr:%d, opcode:%d, aa:%d, tc:%d, rd:%d, ra:%d, rcode:%d, "
           "qdcount:%d, ancount:%d, nscount:%d, arcount:%d",
           h->id, h->qr, h->opcode, h->aa, h->tc, h->rd, h->ra, h->rcode, 
           h->qdcount, h->ancount, h->nscount, h->arcount);
}

void
zc_dnsrr_print(zcDNSRR *r)
{
    switch(r->type) {
    case ZC_DNS_T_A: {
        zcDNSA *x = (zcDNSA*)r->data;
        ZCINFO("DNS RR domain:%s, type:%d, class:%d, ttl:%d, ip:%s", 
            r->domain->data, r->type, r->cls, r->ttl, x->ip);
        break;
    }
    case ZC_DNS_T_NS: {
        zcDNSNS *x = (zcDNSNS*)r->data;
        ZCINFO("DNS RR domain:%s, type:%d, class:%d, ttl:%d, ns:%s", 
            r->domain->data, r->type, r->cls, r->ttl, x->name->data);
        break;
    }
    case ZC_DNS_T_CNAME: {
        zcDNSCNAME *x = (zcDNSCNAME*)r->data;
        ZCINFO("DNS RR domain:%s, type:%d, class:%d, ttl:%d, cname:%s", 
            r->domain->data, r->type, r->cls, r->ttl, x->name->data);
        break;
    }
    default:
        ZCINFO("DNS RR domain:%s, type:%d, class:%d, ttl:%d", 
            r->domain->data, r->type, r->cls, r->ttl);
    }
}

zcDNSRR* 
zc_dnsrr_new()
{
    zcDNSRR *r = zc_calloct(zcDNSRR);
    r->domain = zc_str_new(64);
    return r;
}

void
zc_dnsrr_delete(void *x)
{
    zcDNSRR *r = (zcDNSRR*)x;
    zc_str_delete(r->domain);
    
    if (r->data) {
        switch(r->type) {
        case ZC_DNS_T_A:
            break;
        case ZC_DNS_T_NS: {
            zcDNSNS *n = (zcDNSNS*)r->data;
            zc_str_delete(n->name);
            break;
        }
        case ZC_DNS_T_CNAME: {
            zcDNSCNAME *n = (zcDNSCNAME*)r->data;
            zc_str_delete(n->name);
            break;
        }
        case ZC_DNS_T_SOA:
            break;
        case ZC_DNS_T_PTR:
            break;
        case ZC_DNS_T_MX:
            break; 
        case ZC_DNS_T_TXT:
            break;
        case ZC_DNS_T_AAAA:
            break;
        case ZC_DNS_T_SRV:
            break;
        case ZC_DNS_T_SSHFP:
            break;
        case ZC_DNS_T_SPF:
            break;
        }
        zc_free(r->data);
    }
    zc_free(x);
}



int 
zc_dns_pack_header(zcDNSHeader *h, char *store, int slen)
{
    int i = 0;
    uint16_t v = htons(h->id);
    memcpy(store+i, &v, sizeof(short));
    i += sizeof(short);
    memcpy(store+i, (char*)h+sizeof(short), sizeof(short));
    i += sizeof(short);

    v = htons(h->qdcount);
    memcpy(store+i, &v, sizeof(uint16_t));
    i += sizeof(short);

    v = htons(h->ancount);
    memcpy(store+i, &v, sizeof(uint16_t));
    i += sizeof(short);

    v = htons(h->nscount);
    memcpy(store+i, &v, sizeof(uint16_t));
    i += sizeof(short);

    v = htons(h->arcount);
    memcpy(store+i, &v, sizeof(uint16_t));
    i += sizeof(short);
    return i;
}


int 
zc_dns_pack_query_body(const char *domain, uint16_t type, uint16_t cls, char *store, int slen)
{
    if (strlen(domain)+1+sizeof(type)+sizeof(cls) >= slen) 
        return ZC_ERR;

    const char *start = domain;
    uint8_t len = 0;
    int i = 0;
    while (*start) {
        len = 0;
        int oldi = i;
        i++;
        while (*start && *start != '.') {
            store[i] = *start;
            i++;
            len++;
            start++;
        }
        store[oldi] = len;
        if (*start == '.') start++; 
    }
    store[i] = 0;
    i++;

    uint16_t v = htons(type);
    memcpy(&store[i], &v, sizeof(short));
    i += sizeof(type);
    v = htons(cls);
    memcpy(&store[i], &v, sizeof(short));
    i += sizeof(cls);

    return i;
}

int
zc_dns_pack_query(zcDNSHeader *h, const char *domain, uint16_t type, uint16_t cls, char *store, int slen)
{
    if (slen < sizeof(zcDNSHeader)+strlen(domain)+1+sizeof(short)*2)
        return ZC_ERR;

    zc_dns_pack_header(h, store, slen);
    int n = zc_dns_pack_query_body(domain, type, cls, store+sizeof(zcDNSHeader), slen-sizeof(zcDNSHeader));

    return n+sizeof(zcDNSHeader);
}

int  zc_dns_pack_query_simple(const char *domain, uint16_t type, uint16_t cls, char *store, int slen)
{
    zcDNSHeader req;
    memset(&req, 0, sizeof(zcDNSHeader));
    
    srandom(time(NULL));

    req.id = random()%65535;
    req.qr = 0;
    req.opcode = 0;
    req.aa = 0;
    req.tc = 0;
    req.rd = 1;
    req.ra = 0;
    req.rcode = 0;
    req.qdcount = 1;

    return zc_dns_pack_query(&req, domain, type, cls, store, slen);
}

int 
zc_dns_unpack_header(zcDNSHeader *h, const char *data, int dlen)
{
    if (dlen < sizeof(zcDNSHeader))
        return ZC_ERR;
    memset(h, 0, sizeof(zcDNSHeader));
    
    uint16_t v;
    int i = 0;
    memcpy(&v, data, sizeof(short));
    h->id = ntohs(v);
    i += sizeof(short);

    memcpy((char*)h+sizeof(short), data+i, sizeof(short));
    i += sizeof(short);

    memcpy(&v, data+i, sizeof(short));
    h->qdcount = ntohs(v);
    i += sizeof(short);

    memcpy(&v, data+i, sizeof(short));
    h->ancount = ntohs(v);
    i += sizeof(short);
    
    memcpy(&v, data+i, sizeof(short));
    h->nscount = ntohs(v);
    i += sizeof(short);

    memcpy(&v, data+i, sizeof(short));
    h->arcount = ntohs(v);
    i += sizeof(short);
    return i;
}

/*static int
_unpack_pointer(zcString *name, const char *data, int i)
{
    uint16_t v;
    memcpy(&v, data+i, sizeof(short));
    uint16_t len2 = ntohs(v) & 0x3f;
    ZCINFO("len2:%d", len2);
    const char *newdata = data + len2;
    while (1) {
        uint8_t len1 = *newdata;
        //ZCINFO("len1:%d", len1);
        if (len1 == 0) {
            break;
        }
        if (name->len > 0) {
            zc_str_append(name, ".");
        }
        zc_str_append_len(name, newdata+1, len1);
        newdata += len1 + 1;
    } 
    return ZC_OK;
}*/

static int
_unpack_string(zcString *name, const char *data, int i)
{
    uint8_t len1;
    while (1) {
        len1 = data[i];
        //ZCINFO("s len1:%d", len1);
        if (len1 == 0) {
            i++;
            break;
        }
        if (len1 & 0xc0) {
            uint16_t v;
            memcpy(&v, data+i, sizeof(short));
            uint16_t len2 = ntohs(v) & 0x3f;
            _unpack_string(name, data, len2);
            i += 2;
            //continue;
            break;
        }
        i++;
        if (name->len > 0) {
            zc_str_append(name, ".");
        }
        zc_str_append_len(name, data+i, len1);
        i += len1;
    }
    return i;
}

int 
zc_dns_unpack_resp_body(zcList *result, int n, const char *data, int used, int leftlen)
{
    //ZCINFO("used:%d, leftlen:%d, n:%d", used, leftlen, n);
    if (leftlen < 1+2+2+4+2)
        return ZC_ERR;

    uint8_t  len1;
    int i = used;
    
    while (1) {
        len1 = data[i];
        if (len1 == 0) {
            i++;
            break;
        }
        i++;
        i += len1;
    }
    i += sizeof(short)*2;

    //uint16_t len2 = 0; 
    uint16_t v;
    zcDNSRR *r = NULL;

    while (n > 0) {
        r = zc_dnsrr_new();
        //ZCINFO("======================");
        //ZCINFO("before i:%d", i);
        i = _unpack_string(r->domain, data, i);
        //ZCINFO("after i:%d", i);
        ZCINFO("domain:%d %s", r->domain->len, r->domain->data);

        memcpy(&v, data+i, sizeof(short));
        r->type = ntohs(v);
        i += sizeof(short);
        //ZCINFO("type: %d", r->type); 

        memcpy(&v, data+i, sizeof(short));
        r->cls = ntohs(v);
        i += sizeof(short);
        //ZCINFO("class: %d", r->cls); 

        uint32_t vint;
        memcpy(&vint, data+i, sizeof(int));
        r->ttl = ntohl(vint);
        i += sizeof(int);
        //ZCINFO("ttl: %d", r->ttl); 

        uint16_t size;
        memcpy(&v, data+i, sizeof(short));
        size = ntohs(v);
        i += sizeof(short);
        //ZCINFO("size: %d", size); 

        switch (r->type) {
            case ZC_DNS_T_A: {
                memcpy(&vint, data+i, sizeof(int));
                uint8_t *x = (uint8_t*)&vint;
                zcDNSA *da = zc_calloct(zcDNSA);
                snprintf(da->ip, 16, "%d.%d.%d.%d", x[0], x[1], x[2], x[3]);
                i += size;
                r->data = da;
                break;
            }
            case ZC_DNS_T_NS: {
                zcDNSNS *n = zc_calloct(zcDNSNS);
                n->name = zc_str_new(64);
                _unpack_string(n->name, data, i); 
                i += size;
                r->data = n;
                break;
            }
            case ZC_DNS_T_CNAME: {
                zcDNSCNAME *n = zc_calloct(zcDNSCNAME);
                n->name = zc_str_new(64);
                _unpack_string(n->name, data, i); 
                i += size;
                r->data = n;
                break;
            }
            case ZC_DNS_T_SOA:
                i += size;
                break;
            case ZC_DNS_T_PTR:
                i += size;
                break;
            case ZC_DNS_T_MX: {
                zcDNSMX *dmx = zc_calloct(zcDNSMX);
                memcpy(&v, data+i, sizeof(short));
                dmx->perf = ntohs(v);
                i += sizeof(short);
                //i = _unpack_string(r->data, data, i);
                memcpy(&vint, data+i, sizeof(int));
                uint32_t ip = ntohl(vint);
                uint8_t *x = (uint8_t*)&ip;
                snprintf(dmx->ip, 16, "%d.%d.%d.%d", x[0], x[1], x[2], x[3]);
                i += size-sizeof(short);
                r->data = dmx;
                break; 
            }
            case ZC_DNS_T_TXT:
                i += size;
                break;
            case ZC_DNS_T_AAAA:
                i += size;
                break;
            case ZC_DNS_T_SRV:
                i += size;
                break;
            case ZC_DNS_T_SSHFP:
                i += size;
                break;
            case ZC_DNS_T_SPF:
                i += size;
                break;
            default:
                ZCWARN("type error:%d", r->type);
                goto unpack_error;
        }
        //zc_dnsrr_print(r);
        zc_list_append(result, r); 
        n--;
        //ZCINFO("n:%d", n);
    }
    return ZC_OK;

unpack_error:
    if (r) {
        zc_dnsrr_delete(r);
    }
    zc_list_clear(result);
    return ZC_ERR;
}

int	 
zc_dns_unpack_resp(zcDNSHeader *h, zcList *result, const char *data, int dlen)
{
    int ret = zc_dns_unpack_header(h, data, dlen);
    ret = zc_dns_unpack_resp_body(result, h->ancount + h->nscount + h->arcount, 
            data, ret, dlen-ret);
    //ZCINFO("unpack body: %d, n:%d, len:%d", ret, h->ancount + h->nscount + h->arcount, ret);
    return ret; 
}

int	 
zc_dns_unpack_resp_simple(zcList *result, const char *data, int dlen)
{
    zcDNSHeader h;
    memset(&h, 0, sizeof(zcDNSHeader));
    return zc_dns_unpack_resp(&h, result, data, dlen);
}


int 
zc_dns_pack_rr(const char *domain, uint16_t type, uint16_t cls, uint16_t ttl, const char *data)
{
    return ZC_OK;
}

int 
zc_dns_query(const char *dns, const char *domain, uint16_t type, uint16_t cls, zcList *result)
{
    zcSocket *sock = zc_socket_new_udp(5);
    if (NULL == sock)
        return ZC_ERR;

    zcDNSHeader req;
    memset(&req, 0, sizeof(zcDNSHeader));

    req.id = random()%65535;
    req.qr = 0;
    req.opcode = 0;
    req.aa = 0;
    req.tc = 0;
    req.rd = 1;
    req.ra = 0;
    req.rcode = 0;
    req.qdcount = 1;
    
    char pack[512] = {0};
    int n;
    n = zc_dns_pack_query(&req, domain, type, cls, pack, sizeof(pack));
    ZCINFO("id:%d, %x, send len:%d", req.id, req.id, n);
    /*int i = 0;
    for (i=0; i<n; i++) {
        printf("%02x ", (unsigned char)pack[i]);
    }
    printf("\n");*/

    zc_socket_add_remote_addr(sock, dns, 53);
    int ret = zc_socket_sendto_self(sock, pack, n, 0);
    if (ret != n) {
        ZCERROR("sendto ret: %d", n);
        ret = ZC_ERR;
        goto query_over;
    }
    
    char recvpack[512] = {0};
    ret = zc_socket_recvfrom_self(sock, recvpack, sizeof(recvpack), 0);
    ZCINFO("recvfrom ret: %d", ret);
    if (ret <= 0) {
        ZCERROR("recvfrom ret:%d", ret);
        ret = ZC_ERR;
        goto query_over;
    }

    /*for (i=0; i<ret; i++) {
        printf("%02x ", (unsigned char)recvpack[i]);
    }
    printf("\n");*/
    
    n = ret;
    zcDNSHeader recvh;
    ret = zc_dns_unpack_header(&recvh, recvpack, n);
    zc_dnsheader_print(&recvh);

    zcList *list = zc_list_new();
    ret = zc_dns_unpack_resp_body(list, recvh.ancount + recvh.nscount + recvh.arcount, 
            recvpack, ret, n-ret);
    ZCINFO("unpack body: %d, n:%d, len:%d", ret, recvh.ancount + recvh.nscount + recvh.arcount, n);
    
    zcDNSRR *rr;
    zcListNode *node;
    zc_list_foreach(list, node) {
        rr = (zcDNSRR*)node->data;
        zc_dnsrr_print(rr);
    }

query_over:
    if (sock) {
        zc_socket_delete(sock);
    }
    return ret;
}

#ifdef ZOCLE_WITH_LIBEV

static int 
zc_dns_read_callback(zcAsynConn *conn)
{
    zcBuffer *rbuf = conn->rbuf;
    char *data = zc_buffer_data(rbuf);
    int len = zc_buffer_used(rbuf);

    zcList *result = zc_list_new();
    result->del = zc_dnsrr_delete;
    int ret = zc_dns_unpack_resp_simple(result, data, len);

    ZCINFO("dns result size:%d, ret:%d", result->size, ret);
    zcDNSRR *r;
    const char *ip = NULL;
    zcListNode *node;
    zc_list_foreach(result, node) {
        r = (zcDNSRR*)node->data;
        if (r->type == ZC_DNS_T_A) {
            zc_dnsrr_print(r);
            ip = ((zcDNSA*)r->data)->ip;
            break;
        }
    }

    if (NULL == ip) {
        zc_list_delete(result);
        return ZC_ERR;
    }
    
    return len;
}

zcAsynConn* 
zc_asynconn_new_dns_client(const char *dns, int timeout, struct ev_loop *loop, const char *host, int (*callback)(zcAsynConn*))
{
    zcProtocol p;
    zc_protocol_init(&p);
    p.handle_read = callback;
    zcAsynConn *conn = zc_asynconn_new_udp_client(dns, 53, timeout, &p, loop, 1024, 1024);
    int n = zc_dns_pack_query_simple(host, ZC_DNS_T_A, ZC_DNS_C_IN, 
            conn->wbuf->data, zc_buffer_idle(conn->wbuf));
    //ZCINFO("wbuf len:%d", n); 
    //ZCINFO("wbuf end:%u", conn->wbuf->end);
    conn->wbuf->end = (uint32_t)n;
    zc_asynconn_start_write(conn);

    return conn;
}
#endif


