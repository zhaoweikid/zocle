#include <zocle/protocol/http/httpconn.h>
#include <zocle/protocol/http/httpheader.h>
#ifndef _WIN32
#include <netdb.h>
#endif
#include <zocle/net/socketio.h>
#include <zocle/str/string.h>
#include <zocle/enc/compress.h>
#include <zocle/utils/times.h>

void
zc_httpconnstat_print(zcHttpConnStat *stat)
{
    ZCINFO("dns:%lld, conn:%lld, ssl:%lld, send:%lld, recv_first:%lld, recv:%lld, all:%lld", 
        (long long)stat->dns_time, (long long)stat->conn_time, (long long)stat->ssl_time, 
        (long long)stat->send_time, (long long)stat->recv_first_time, (long long)stat->recv_time, 
        (long long)stat->all_time);
}

zcHttpConn* 
zc_httpconn_new()
{
    zcHttpConn *c = (zcHttpConn*)zc_malloc(sizeof(zcHttpConn));
    memset(c, 0, sizeof(zcHttpConn));
    //strncpy(c->host, host, 15);
    c->port = 80;
    return c;
}

void
zc_httpconn_delete(void *x)
{
    zcHttpConn *c = (zcHttpConn*)x;
    if (c->sock) {
        zc_socket_delete(c->sock);
    }
    zc_free(c);
}

int	
zc_httpconn_open(zcHttpConn *c, const char *host, int port, bool ssl)
{
    if (c->sock) {
        zc_socket_delete(c->sock);
    }
    
    strncpy(c->host, host, 15);
    c->port = port;
    ZCINFO("open %s:%d ssl:%d", c->host, c->port, ssl);
    int64_t starttm = zc_timenow();
    c->sock = zc_socket_client_tcp(c->host, c->port, c->conn_timeout);
    if (c->stat) {
        c->stat->conn_time = zc_timenow() - starttm;
    }
    if (c->sock == NULL) {
        ZCWARN("connect error");
        return ZC_ERR;
    }
    if (ssl) {
#ifdef ZOCLE_WITH_SSL
        starttm = zc_timenow();
        int ret = zc_socket_client_ssl(c->sock, NULL, NULL, ZC_SSL_CERT_NONE, ZC_SSL_VER_TLS1, NULL);
        if (c->stat) {
            c->stat->ssl_time = zc_timenow() - starttm;
        }
        if (ret != ZC_OK) {
            zc_socket_delete(c->sock);
            c->sock = NULL;
            return ZC_ERR;
        }
#else
        ZCERROR("not support ssl");
        return ZC_ERR;
#endif
    }
    return ZC_OK;
}

int	
zc_httpconn_send(zcHttpConn *c, zcHttpReq *req)
{
    int ret;
    int64_t starttm;

    if (c->stat) {
        memset(c->stat, 0, sizeof(zcHttpConnStat));
        c->stat->start = zc_timenow();
    }

    if (c->sock == NULL) {
        bool isssl = false;
        int  port = 80;
        if (strcmp(req->url.protocol.data, "https") == 0 || strcmp(req->url.protocol.data, "wss") == 0) {
            isssl = true;
            port = 443;
        }
        /*zcCStrList ips;
        char buf[16*10+100];
        zc_cstrlist_init_stack(&ips, buf, sizeof(buf), 10);
        */
        zcList *ips = zc_list_new();
        starttm = zc_timenow();
        zc_socket_gethostbyname(req->url.domain.data, ips);
        if (c->stat) {
            c->stat->dns_time = zc_timenow() - starttm;
        }
        //if (ips.n == 0) {
        if (ips->size == 0) {
            ZCWARN("dns error: %s", req->url.domain.data);
            zc_list_delete(ips);
            return ZC_ERR;
        }
        char *ip = zc_list_at(ips, 0, NULL);
        ZCINFO("try open %s:%d", ip, port);
        //ZCINFO("try open %s:%d", zc_cstrlist_get(&ips, 0), port);
        ret = zc_httpconn_open(c, ip, port, isssl);
        if (ret < 0) {
            zc_list_delete(ips);
            return ret;
        }
        ZCINFO("connect ok!");
        zc_list_delete(ips);
    }

    zcString s;
    zc_str_init(&s, 1024);

    c->sock->timeout = c->write_timeout;
    zc_httpreq_header_tostr(req, &s);
    zc_str_append(&s, "\r\n");

    starttm = zc_timenow();
    if (c->writefunc) {
        while (1) {
            ret = zc_socket_sendn(c->sock, s.data, s.len);
            if (ret != s.len) {
                ZCWARN("request send error: %d", ret);
                zc_str_destroy(&s);
                return ZC_ERR;
            }
            zc_str_clear(&s);
            ret = c->writefunc(s.data, s.size);
            if (ret <= 0) {
                break;
            }
            s.len = ret;
        }
    }else{
        zc_str_append_len(&s, req->body.data, req->body.len);

        ZCINFO("request: %d, %s", s.len, s.data); 
        ret = zc_socket_sendn(c->sock, s.data, s.len);
        if (ret != s.len) {
            ZCWARN("request send error: %d", ret);
            zc_str_destroy(&s);
            return ZC_ERR;
        }
    }
    if (c->stat) {
        c->stat->send_time = zc_timenow() - starttm;
    }
    zc_str_destroy(&s);

    return ZC_OK;
}

#define ASCII_FLAG   0x01 /* bit 0 set: file probably ascii text */
#define HEAD_CRC     0x02 /* bit 1 set: header CRC present */
#define EXTRA_FIELD  0x04 /* bit 2 set: extra field present */
#define ORIG_NAME    0x08 /* bit 3 set: original file name present */
#define COMMENT      0x10 /* bit 4 set: file comment present */
#define RESERVED     0xE0 /* bits 5..7: reserved */

/* RFC 1952 Section 2.3 defines the gzip header:
 
  +---+---+---+---+---+---+---+---+---+---+
  |ID1|ID2|CM |FLG|     MTIME     |XFL|OS |
  +---+---+---+---+---+---+---+---+---+---+

  FLG:
  bit 0   FTEXT
  bit 1   FHCRC
  bit 2   FEXTRA
  bit 3   FNAME
  bit 4   FCOMMENT
  bit 5   reserved
  bit 6   reserved
  bit 7   reserved

  OS:
  0 - FAT filesystem (MS-DOS, OS/2, NT/Win32)
  1 - Amiga
  2 - VMS (or OpenVMS)
  3 - Unix
  4 - VM/CMS
  5 - Atari TOS
  6 - HPFS filesystem (OS/2, NT)
  7 - Macintosh
  8 - Z-System
  9 - CP/M
 10 - TOPS-20
 11 - NTFS filesystem (NT)
 12 - QDOS
 13 - Acorn RISCOS
255 - unknown

(if FLG.FEXTRA set)

  +---+---+=================================+
  | XLEN  |...XLEN bytes of "extra field"...|
  +---+---+=================================+

(if FLG.FNAME set)

  +=========================================+
  |...original file name, zero-terminated...|
  +=========================================+

(if FLG.FCOMMENT set)

  +===================================+
  |...file comment, zero-terminated...|
  +===================================+

(if FLG.FHCRC set)

  +---+---+
  | CRC16 |
  +---+---+

  +=======================+
  |...compressed blocks...|
  +=======================+

    0   1   2   3   4   5   6   7
  +---+---+---+---+---+---+---+---+
  |     CRC32     |     ISIZE     |
  +---+---+---+---+---+---+---+---+
 */

int
zc_httpconn_gzip_check_header(const char *s, int slen)
{
    uint8_t *c = (uint8_t*)s; 
    static uint8_t gz_magic[2] = {0x1f, 0x8b};
    //ZCINFO("check: %x %x %x", c[0], c[1], c[2]);
    if (c[0]!=gz_magic[0] || c[1]!=gz_magic[1] || c[2]!=Z_DEFLATED) {
        return 0;
    }
    //uint8_t flag = c[4];
    //uint8_t os = c[9];
    //ZCINFO("flag: %d, os: %d", flag, os);
    return 10;
}

int 
zc_httpconn_gzip_uncompress(zcHttpResp *resp, zcBuffer *buf, zcCompress *cm, bool *first)
{
    //ZCINFO("in gzip first:%d, buflen:%d", *first, zc_buffer_used(buf));
    if (*first && zc_buffer_used(buf) > 10) {
        int gziphead = zc_httpconn_gzip_check_header(zc_buffer_data(buf), zc_buffer_used(buf));
        int zlen = zc_buffer_used(buf)-gziphead;
        int err = zc_compress_dec(cm, (uint8_t*)buf->data+buf->pos+gziphead, zlen, &resp->bodydata, false);
        if (err < 0) {
            return ZC_ERR;
        }
        *first = false;
        //buf->pos += gziphead + zlen;
        //zc_buffer_compact(buf);
        return gziphead+zlen;
    }else if (!*first && zc_buffer_used(buf) > 0) {
        int zlen = buf->end;
        int err = zc_compress_dec(cm, (uint8_t*)buf->data, zlen, &resp->bodydata, false);
        if (err < 0) {
            return ZC_ERR;
        }
        //buf->pos += zlen;
        //zc_buffer_compact(buf);
        return zlen;
    }
    return 0;
}

int 
zc_httpconn_gzip_uncompress_str(zcHttpResp *resp, char *buf, int blen, zcCompress *cm, bool *first)
{
    if (*first && blen > 10) {
        int gziphead = zc_httpconn_gzip_check_header(buf, blen);
        int zlen = blen-gziphead;
        int err = zc_compress_dec(cm, (uint8_t*)buf+gziphead, zlen, &resp->bodydata, false);
        if (err < 0) {
            return ZC_ERR;
        }
        *first = false;
        return gziphead+zlen;
    }else if (!*first && blen > 0) {
        int zlen = blen;
        int err = zc_compress_dec(cm, (uint8_t*)buf, zlen, &resp->bodydata, false);
        if (err < 0) {
            return ZC_ERR;
        }
        return zlen;
    }
    return 0;
}


int 
zc_httpconn_recv_header(zcHttpConn *c, zcSocketIO *sockio, zcHttpResp *resp, int64_t starttm)
{
    int ret;
    char linebuf[2048];
    zcBuffer *buf = zc_buffer_alloc_stack(16384);
    zc_buffer_init(buf, 16384);

    int count = 0;
    while (1) {
        ret = zc_sockio_readline(sockio, linebuf, sizeof(linebuf));
        if (ret <= 0) {
            ZCWARN("readline error: %d", ret);
            //goto recv_error;
            return ZC_ERR;
        }
        linebuf[ret] = 0;
        if (count == 0) {
            if (c->stat)
                c->stat->recv_first_time = zc_timenow() - starttm;
        }
        if (ret == 2 && strcmp(linebuf, "\r\n") == 0) {
            // found seperator
            break;
        }
        //ZCINFO("recv line: %s", buf);
        zc_str_append_len(&resp->headdata, linebuf, ret);
        count++;
    }
    zc_httpresp_parse_header(resp);

    ZCINFO("=== header ok, go body:%lld, chunk:%d, compress:%d ===", 
            (long long)resp->bodylen, resp->chunked, resp->compress);
    return ZC_OK;
}

    
int 
zc_httpconn_recv_body_len_compress(zcHttpConn *c, zcSocketIO *sockio, zcHttpResp *resp, zcCompress *cm)
{
    zcBuffer *rbuf = sockio->rbuf;
    int ret;
    int64_t rsize = resp->bodylen;
    //int blocksize = (rbuf->size>16384)?16384:rbuf->size;
    int blocksize = rbuf->size;
    if (rsize) {
        rsize -= zc_buffer_used(rbuf);
    }

    bool first = true;
    while (rsize > 0) {
        int r = blocksize-zc_buffer_used(rbuf);
        //r = rsize>r? r:rsize;
        ret = zc_sockio_read_nocopy(sockio, r);
        if (ret < 0)
            return ZC_ERR;
        rsize -= ret;
        ret = zc_httpconn_gzip_uncompress_str(resp, zc_buffer_data(rbuf), blocksize, cm, &first);
        if (ret < 0) {
            ZCWARN("gzip umcompress error:%d", ret);
            return ZC_ERR;
        }
        rbuf->pos += ret;
        if (c->readfunc) {
            ret = c->readfunc(resp->bodydata.data, resp->bodydata.len);
            if (ret < 0) {
                ZCWARN("readfunc error:%d", ret);
                return ZC_ERR;
            }
            zc_str_clear(&resp->bodydata);
        }
    }
    return ZC_OK;
}

int 
zc_httpconn_recv_body_len(zcHttpConn *c, zcSocketIO *sockio, zcHttpResp *resp)
{
    zcBuffer *rbuf = sockio->rbuf;
    int rbuflen = zc_buffer_used(rbuf);
    int rsize = (int)resp->bodylen;
    zcString *bodydata = &resp->bodydata;

    if (rbuflen > 0) {
        zc_str_append_len(bodydata, zc_buffer_data(rbuf), rbuflen);
        if (rsize > 0)
            rsize -= rbuflen;
    }
    zc_sockio_delete(sockio);
    sockio = NULL;

    if (resp->bodylen > 0) {
        zc_str_ensure_idle_size(bodydata, resp->bodylen);
    }

    int  ret;
    int  start = 0;
    while (rsize > 0) {
        ret = zc_socket_recv(c->sock, bodydata->data + bodydata->len, rsize);
        if (ret < 0) {
            ZCWARN("socket recv error:%d", ret);
            return ZC_ERR;
        }else if (ret == 0) {
            if (resp->bodylen == 0) {
                ZCINFO("socket closed normal");
                break;
            }
            ZCINFO("socket closed");
            return ZC_ERR;
        }
        bodydata->len += ret;
        rsize -= ret;

        if (c->readfunc) {
            ret = c->readfunc(bodydata->data+start, bodydata->len-start);
            if (ret < 0) {
                ZCWARN("readfunc error:%d", ret);
                return ZC_ERR;
            }
            //start += bodydata->len-start;
            start = bodydata->len;
        }
    }
    return ZC_OK;
}

int 
zc_httpconn_recv_body_chunked(zcHttpConn *c, zcSocketIO *sockio, zcHttpResp *resp, zcCompress *cm)
{
    int  ret;
    char linebuf[1024];
    int  chunklen = 0;
    bool first = true;

    while (1) {
        ret = zc_sockio_readline(sockio, linebuf, sizeof(linebuf));
        if (ret <= 0) {
            ZCWARN("readline error: %d", ret);
            return ZC_ERR;
        }
        linebuf[ret] = 0;
        chunklen = strtol(linebuf, NULL, 16);
        if (chunklen == 0) {
            ZCINFO("last chunk");
            break; // read \r\n
        }

        ret = zc_sockio_read_nocopy(sockio, chunklen);
        if (ret < 0) {
            ZCINFO("read chunk error! %d", ret);
            return ZC_ERR;
        }

        if (resp->compress) {
            ret = zc_httpconn_gzip_uncompress_str(resp, zc_buffer_data(sockio->rbuf), chunklen, cm, &first);
            if (ret < 0) {
                ZCWARN("gzip umcompress error:%d", ret);
                return ZC_ERR;
            }
            if (c->readfunc) {
                ret = c->readfunc(resp->bodydata.data, resp->bodydata.len);
                if (ret < 0) {
                    ZCWARN("readfunc error:%d", ret);
                    return ZC_ERR;
                }
                zc_str_clear(&resp->bodydata);
            }
        }else{
            if (c->readfunc) {
                ret = c->readfunc(zc_buffer_data(sockio->rbuf), chunklen);
                if (ret < 0) {
                    ZCWARN("readfunc error:%d", ret);
                    return ZC_ERR;
                }
            }else{
                zc_str_append_len(&resp->bodydata, zc_buffer_data(sockio->rbuf), chunklen);
            }
        }
         
        sockio->rbuf->pos += chunklen;
        zc_buffer_compact(sockio->rbuf);

        // read \r\n after trunk data
        ret = zc_sockio_readline(sockio, linebuf, sizeof(linebuf));
        if (ret <= 0) {
            ZCWARN("readline after trunk error: %d", ret);
            return ZC_ERR;
        }
    }

    // read \r\n after trunk data
    ret = zc_sockio_readline(sockio, linebuf, sizeof(linebuf));
    if (ret <= 0) {
        ZCWARN("readline after trunk error: %d", ret);
        return ZC_ERR;
    }
    return ZC_OK;
}

zcHttpResp*	
zc_httpconn_recv(zcHttpConn *c)
{
    int ret;
    zcHttpResp *resp = zc_httpresp_new(); 
    zcSocketIO *sockio = zc_sockio_new(c->sock, 8192);
    zcCompress cm;
    memset(&cm, 0, sizeof(zcCompress));
    c->sock->timeout = c->read_timeout;

    int64_t starttm = zc_timenow();
    ret = zc_httpconn_recv_header(c, sockio, resp, starttm);
    if (ret < 0) {
        goto recv_error;
    }

    int err;
    if (resp->compress != 0) {
        switch (resp->compress) {
        case ZC_HTTP_GZIP:
            err = zc_compress_init(&cm, ZC_GZIP_DEC, 0);
            if (err < 0) {
                goto recv_error;
            }
            break;
        default:
            ZCERROR("server encoding not supported: %d", resp->compress);
            goto recv_error;
        }
    }

    if (resp->chunked) {
        ret = zc_httpconn_recv_body_chunked(c, sockio, resp, &cm);
    }else{
        if (resp->compress == 0) {
            ret = zc_httpconn_recv_body_len(c, sockio, resp);
        }else{
            ret = zc_httpconn_recv_body_len_compress(c, sockio, resp, &cm);
        }
    }
    if (ret < 0)
        goto recv_error;

    resp->isfinish = 1;
    if (c->stat) {
        int64_t now = zc_timenow();
        c->stat->recv_time = now - starttm;
        c->stat->all_time = now - c->stat->start;
    }
    return resp;

recv_error:
    if (cm.algri > 0) {
        zc_compress_destroy(&cm);
    }
    if (sockio) {
        zc_sockio_delete(sockio);
    }
    zc_socket_delete(c->sock);
    c->sock = NULL;
    if (resp->code == 0) {
        zc_httpresp_delete(resp);
        resp = NULL;
    }
    if (c->stat) {
        int64_t now = zc_timenow();
        c->stat->recv_time = now - starttm;
        c->stat->all_time = now - c->stat->start;
    }

    return resp;
}



