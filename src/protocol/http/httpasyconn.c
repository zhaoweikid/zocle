#ifdef ZOCLE_WITH_LIBEV
#include <zocle/protocol/http/httpasyconn.h>
#include <zocle/protocol/http/httpheader.h>
#include <zocle/protocol/dns/resolver.h>
#include <zocle/base/defines.h>
#include <zocle/mem/alloc.h>
#include <zocle/utils/times.h>
#include <zocle/str/strbuf.h>
#include <zocle/server/asynconn.h>
#include <zocle/ds/dict.h>

zcHttpInfo*	
zc_httpinfo_new()
{
    zcHttpInfo *info = zc_calloct(zcHttpInfo);
    /*strncpy(info->host, host, 15); 
    info->port  = port;
    info->ssl   = isssl;*/
    info->stat  = zc_calloct(zcHttpConnStat);
    //info->time  = zc_timenow();
    info->header    = true;
    info->longconn  = true;
    info->websocket = false;
    info->bodyfirst = true;
    info->resp      = zc_httpresp_new();
    info->conn_timeout  = 30000;
    info->read_timeout  = 30000;
    info->write_timeout = 30000;

    //zc_asynhttp_assign_conn(conn);
    return info;
}

void
zc_httpinfo_delete(void *x)
{
    zcHttpInfo *info = (zcHttpInfo*)x;
    if (info->stat) {
        zc_free(info->stat);
    } 
    if (info->data) {
        zc_strbuf_delete((zcStrBuf*)info->data);
    }
    zc_free(x);
}

zcAsynConn* 
zc_asynconn_new_http(zcHttpInfo *info, int timeout, struct ev_loop *loop, const char *dns, int bufsize)
{
    zcAsynConn *conn;
    zcHttpReq  *req = info->req;
    /*zcHttpInfo *info = zc_httpinfo_new();
    info->req = req;
    info->http_ready = ready;*/
 
    zcProtocol p;
    zc_protocol_init(&p);
    zc_asynhttp_assign_protocol(&p);
   
    bool isip = zc_url_domain_isip(&req->url);
    if (isip) {
        conn = zc_asynconn_new_tcp_client(req->url.domain.data, req->url.port, timeout, &p, loop, bufsize, bufsize);
        conn->data = info;
        zc_asynhttp_send(conn, req);
    }else{
        ZCINFO("query dns:%s, domain:%s", dns, req->url.domain.data);
        conn = zc_asynconn_new_dns_client(dns, timeout, loop, req->url.domain.data, zc_asynhttp_handle_read_dns);
        conn->data = info;
    }

    return conn;
}

zcAsynConn* 
zc_asynconn_new_http_url(const char *url, int timeout, struct ev_loop *loop, const char *dns, int bufsize, zcHttpInfo *info)
{
    zcHttpReq *req = zc_httpreq_new(url);
    if (info == NULL) {
        info = zc_httpinfo_new();
    }
    info->req = req;

    return zc_asynconn_new_http(info, timeout, loop, dns, bufsize);
}

void
zc_asynconn_delete_http(void *x)
{
    zcAsynConn *conn = (zcAsynConn*)x;
    ZCINFO("asynconn delete http"); 
    zc_httpinfo_delete(conn->data);
    zc_asynconn_delete(x);
}


int 
zc_asynhttp_assign_protocol(zcProtocol *p)
{
    p->handle_connected  = zc_asynhttp_handle_connected;
    p->handle_read       = zc_asynhttp_handle_read;
    p->handle_wrote      = zc_asynhttp_handle_wrote;

    return ZC_OK;
}

int 
zc_asynhttp_send(zcAsynConn *conn, zcHttpReq *req)
{
    zcHttpInfo *info = conn->data;

    zcString *s = zc_str_new(4096);
    zc_httpreq_header_tostr(req, s);
    zc_str_append(s, "\r\n");
    
    if (s->len > conn->wbuf->size) { // too long
        ZCINFO("header too long");
        // FIXME: buf not free
        zcStrBuf *buf = zc_strbuf_new(s);
        zc_buffer_append(conn->wbuf, s->data, conn->wbuf->size);
        buf->pos += conn->wbuf->size;
        info->data = buf;
    }else{
        zc_buffer_append(conn->wbuf, s->data, s->len);
        zc_str_delete(s);
    }
    ZCINFO("http send:%d %s", zc_buffer_used(conn->wbuf), zc_buffer_data(conn->wbuf));
    zc_asynconn_start_write(conn);
    if (info->stat) {
        info->stat->start = zc_timenow();
    }
    return ZC_OK;
}

int 
zc_asynhttp_websocket_send(zcAsynConn *conn, uint8_t fin, uint8_t opcode, const char *data, int datalen)
{
    int headerlen = 2 + 4; // header + mask
    if (datalen >= 65535) {
        headerlen += 8;
    }else if (datalen >= 126) {
        headerlen += 2;
    }

    int size = headerlen + datalen;
    if (size > conn->wbuf->size) {
        ZCWARN("websocket send too long, datalen:%d, wbuf->size:%d", datalen, conn->wbuf->size);
        return ZC_ERR;
    }

    zcBuffer *last = conn->wbuf;
    while (last && last->next) {
        last = last->next;
    }
  
    zcBuffer *b;
    uint8_t *save;
    if (size > zc_buffer_idle(last)) {
        b = last;
        save = (uint8_t*)&b->data[b->end];
    }else{
        b = zc_buffer_new(conn->wbuf->size);
        save = (uint8_t*)b->data;
    }
    
    uint32_t mask = random();
    int pos = 2; 
    save[0] = ((fin & 0x01) << 7) | opcode;
    if (datalen < 126) {
        save[1] = 0x80 | datalen;
    }else if (datalen < 65535) {
        save[1] = 0x80 | 126;
        uint16_t len16 = datalen;
        len16 = htons(len16);
        memcpy(&save[2], &len16, sizeof(int16_t));
        pos += 2;
    }else{
        save[1] = 0x80 | 127;
        uint64_t len64 = datalen;
        len64 = htonl(len64);
        memcpy(&save[2], &len64, sizeof(int64_t));
        pos += 8;
    }
    
    memcpy(&save[pos], &mask, sizeof(int32_t));
    pos += 4;

    uint8_t *maskarray = (uint8_t*)&mask;
    int i;
    for (i=0; i<datalen; i++) {
        save[pos] = data[i] ^ maskarray[i%4];
        pos++;
    }
    b->end += pos;

    if (last != b) {
        last->next = b;
    }
    zc_asynconn_start_write(conn);

    return ZC_OK;
}

int zc_asynhttp_switch_websocket(zcAsynConn *conn)
{
    zcHttpInfo *info   = conn->data;
    info->websocket    = 1;
    info->websocket_close = 0;
    conn->p.handle_read  = zc_asynhttp_handle_websocket_read;
    //conn->handle_ready = NULL;
    return ZC_OK;
}


int 
zc_asynhttp_handle_wrote(zcAsynConn *conn)
{
    zcHttpInfo *info = conn->data;
    if (info->writefunc) {
        int ret = info->writefunc(conn->wbuf->data, conn->wbuf->size, info);
        if (ret <= 0) {
            return ZC_OK;
        }
        conn->wbuf->end = ret;
        return ret;
    }else{
        if (info->data) {
            zcStrBuf *buf = (zcStrBuf*)info->data;
            if (zc_strbuf_len(buf) > 0) {
                zc_strbuf_get(buf, conn->wbuf->data, conn->wbuf->size);
                if (zc_strbuf_len(buf) == 0) {
                    zc_strbuf_delete(buf);
                    info->data = NULL;
                }
            }else{
                zc_strbuf_delete(buf);
                info->data = NULL;
            }
        }
    }
    return ZC_OK;
}

int 
zc_asynhttp_handle_connected(zcAsynConn *conn)
{
    zcHttpInfo *info = conn->data;
    uint64_t now = zc_timenow();
    info->stat->conn_time = now - info->stat->start;
    //info->time = now;
    return ZC_OK;
}

int 
zc_asynhttp_handle_websocket_read(zcAsynConn *conn)
{
    zcHttpInfo *info = conn->data;
    zcBuffer *buf = conn->rbuf;
    uint64_t datalen;
    uint32_t mask;
    uint8_t  opcode = 0;
    int      headerlen = 2;
    uint8_t  fin = 1;

    if (zc_buffer_used(buf) < 2) {
        ZCINFO("buffer len:%d, return", zc_buffer_used(buf));
        return 0;
    }

    char *bdata = &buf->data[buf->pos];
    //ZCINFO("header: %02x %02x", bdata[0], bdata[1]);
    fin = (bdata[0] & 0x80) >> 7;
    opcode = bdata[0] & 0x0f;
    bool havemask = bdata[1] & 0x80;
    datalen = bdata[1] & 0x7f; 
    if (datalen == 126) {
        headerlen += 2;
        if (buf->end < headerlen) {
            ZCINFO("header not complete, again");
            return 0;
        }
        uint16_t fl;
        memcpy(&fl, &bdata[2], 2);
        datalen = ntohs(fl);
    }else if (datalen == 127) {
        headerlen += 8;
        if (buf->end < headerlen) {
            ZCINFO("header not complete, again");
            return 0;
        }
        memcpy(&datalen, &bdata[2], 8);
        datalen = ntohl(datalen); // must in 64bit
    }
    if (havemask) {
        memcpy(&mask, &bdata[headerlen], 4);
        headerlen += 4;
    }
    ZCINFO("websocket fin:%d opcode:%d mask:%d headerlen:%d datalen:%llu", 
            fin, opcode, havemask, headerlen, (unsigned long long)datalen);
    
    if (zc_buffer_used(buf) < headerlen + datalen) {
        ZCINFO("buffer not have complete package, rbuf->len:%d package len:%llu", 
                zc_buffer_used(buf), (unsigned long long)(headerlen+datalen));
        return 0;
    }

    if (opcode == ZC_WS_CLOSE) { // close
        ZCNOTE("recv websocket close, len:%llu", (unsigned long long)datalen);
        int closeflag = info->websocket_close;
        if (closeflag == ZC_WS_CLOSE_NO) {
            info->websocket_close = 1;
            zc_asynhttp_websocket_send(conn, 1, 0x80, NULL, 0);
        }else if (closeflag == ZC_WS_CLOSE_SEND) { // ok, recv close response
            zc_asynconn_delete_http(conn); // close connection
        }
        return headerlen+datalen;
    }
    if (opcode == ZC_WS_PING) {
        ZCNOTE("recv websocket ping, len:%llu", (unsigned long long)datalen);
        if (info->websocket_close == 0) { // not close, must return pong
            ZCNOTE("recv websocket ping, response pong");
            zc_asynhttp_websocket_send(conn, 1, 0x0a, NULL, 0);
        }else{
            ZCNOTE("recv websocket ping, but close before, ignore");
        }
        return headerlen+datalen;
    }

    //int64_t pos = headerlen;
    if (havemask) {
        uint8_t *maskarray = (uint8_t*)&mask;
        char *finaldata = zc_malloc(datalen);
        int i;
        for (i=0; i<datalen; i++) {
            finaldata[i] = bdata[i] ^ maskarray[i%4];
        }
        int ret = info->readfunc(finaldata, datalen, fin, conn);
        if (ret < 0) {
            ZCWARN("readfunc return err:%d", ret);
            zc_socket_shutdown_read(conn->sock);
            zc_asynhttp_websocket_send(conn, 1, ZC_WS_CLOSE, NULL, 0);
            info->websocket_close = ZC_WS_CLOSE_SEND;
            zc_free(finaldata);
            return headerlen+datalen;
        }
        zc_free(finaldata);
    }else{
        int ret = info->readfunc(&bdata[headerlen], datalen, fin, conn);
        if (ret < 0) {
            ZCWARN("readfunc return err:%d", ret);
            zc_socket_shutdown_read(conn->sock);
            zc_asynhttp_websocket_send(conn, 1, ZC_WS_CLOSE, NULL, 0);
            info->websocket_close = ZC_WS_CLOSE_SEND;
            return headerlen+datalen;
        }
    }
    
    return headerlen+datalen;
}

static int
zc_asynhttp_handle_read_header(zcAsynConn *conn)
{
    zcBuffer *rbuf = conn->rbuf;
    char *data  = zc_buffer_data(rbuf);
    int datalen = zc_buffer_used(rbuf);
    int used = 0;

    zcHttpInfo *info = conn->data; 

    ZCINFO("read header buffer:%d pos:%d %s", datalen, rbuf->pos, data);
   
    char *check_data = data;
    char *p; // = strstr(check_data, "\r\n");
    //if (p != NULL) { // have header line
    while ((p = strstr(check_data, "\r\n")) != NULL) {
        if (p == check_data) { // end of header
            used += 2;
            ZCINFO("parse header");
            info->header = false;
            zc_httpresp_parse_header(info->resp);

            // try switch to websocket
            if (strcasecmp(zc_dict_get_str(info->resp->header, "Upgrade",""), "websocket") == 0) {
                int ret = zc_httpresp_websocket(info->resp, info->req); 
                if (ret != ZC_OK) {
                    ZCNOTE("websocket response error!");
                    return ZC_ERR;
                }
                zc_asynhttp_switch_websocket(conn);
                ZCNOTE("websocket switch ok");
                if (info->finalfunc) 
                    info->finalfunc(conn);
                return used;
            }

            if (!info->resp->chunked && info->resp->bodylen == 0) {
                if (info->finalfunc) 
                    info->finalfunc(conn);
            }

            if (info->resp->compress) {
                if (info->compress)
                    zc_compress_delete(info->compress);
                switch (info->resp->compress) {
                    case ZC_HTTP_GZIP:
                        info->compress = zc_compress_new(ZC_GZIP_DEC, 0);
                        break;
                    default:
                        ZCERROR("not support compress:%d", info->resp->compress);
                }
            }
            break;
        }else{
            used += p - check_data + 2;
            zc_str_append_len(&info->resp->headdata, check_data, p-check_data+2);
            check_data = p + 2;
        }
    }
    return used;
}

static int
zc_asynhttp_handle_read_body(zcAsynConn *conn)
{
    zcBuffer *rbuf = conn->rbuf;
    const char *data = zc_buffer_data(rbuf);
    int datalen = zc_buffer_used(rbuf);
    zcHttpInfo *info = conn->data; 
    int used = 0;

    ZCINFO("read body buffer:%d, chunklen:%d", datalen, info->chunklen);
    if (info->resp->chunked) {
        if (info->chunklen == 0) {
            // find next chunk len
            const char *p = zc_strnstr(data, "\r\n", datalen-used);
            if (p == NULL) { // go ahead
                return used;
            } 
            if (p == data) { // end of response
                return 2;
            }
            char linebuf[32] = {0};
            strncpy(linebuf, data, p-data); 
            info->chunklen = strtol(linebuf, NULL, 16); 
            ZCINFO("chunk str:%ld %s(%d)", p-data, linebuf, info->chunklen);
            used += p-data+2;
            //info->readlen += p-data+2;
            data += p-data+2;
            if (info->chunklen == 0 && info->finalfunc) { // chunk end 
                info->finalfunc(conn);
            }
            info->readlen += used;
            return used;
        }else{
            if (info->chunklen+2 > datalen-used) {
                return ZC_AGAIN;
            }
            datalen = info->chunklen;
            info->chunklen = 0;
        }
    }

    ZCINFO("chunk datalen:%d", datalen);
    int ret;
    if (info->resp->compress) {
        //ZCINFO("umcompress len:%d data: %x %x %x %x", datalen, data[0], data[1], data[2], data[3]);
        ret = zc_httpconn_gzip_uncompress_str(info->resp, data, datalen, 
                    info->compress, &info->bodyfirst);
        if (ret < 0) {
            ZCWARN("umcompress error:%d", ret);
            goto read_body_error;
        }
        if (info->readfunc) { // data callback
            ret = info->readfunc(info->resp->bodydata.data, info->resp->bodydata.len, false, conn);
            if (ret < 0) {
                ZCWARN("readfunc error:%d", ret);
                goto read_body_error;
            }
            zc_str_clear(&info->resp->bodydata);
        }
    }else{
        if (info->readfunc) {
            ret = info->readfunc(data, datalen, false, conn);
            if (ret < 0) {
                ZCWARN("readfunc error:%d", ret);
                goto read_body_error;
            }
        }else{
            zc_str_append_len(&info->resp->bodydata, data, datalen);
        }
    }
    used += datalen;
    // chunk end have \r\n, so +2
    if (info->resp->chunked && strncmp(&data[used], "\r\n", 2) == 0) {
        used += 2;
    }
    info->readlen += used;

    if (!info->resp->chunked && info->readlen == info->resp->bodylen && info->finalfunc) {
        info->finalfunc(conn);
    }
    
    return used;

read_body_error:
    return ZC_ERR;
}

int 
zc_asynhttp_handle_read(zcAsynConn *conn)
{
    zcHttpInfo *info = conn->data; 
    if (info-> header) {
        return zc_asynhttp_handle_read_header(conn);
    }else{
        return zc_asynhttp_handle_read_body(conn);
    }
}

/*int 
zc_asynhttp_handle_ready(zcAsynConn *conn, char *data, int datalen)
{
    zcHttpInfo *info = conn->data;

    if (info->header) { // read header
        if (datalen == 2 && strncmp(data, "\r\n", 2) == 0) { // header complete
            info->header = false;
            zc_httpresp_parse_header(info->resp);

            // try switch to websocket
            if (strcasecmp(zc_dict_get_str(info->resp->header, "Upgrade",""), "websocket") == 0) {
                int ret = zc_httpresp_check_websocket(info->resp, info->req); 
                if (ret != ZC_OK) {
                    ZCNOTE("websocket response error!");
                    return ZC_OK;
                }
                zc_asynhttp_switch_websocket(conn);
                ZCNOTE("websocket switch ok");
                if (info->readyfunc) 
                    info->readyfunc(conn);
                return ZC_OK;
            }

            if (info->resp->compress) {
                if (info->compress)
                    zc_compress_delete(info->compress);
                switch (info->resp->compress) {
                case ZC_HTTP_GZIP:
                    info->compress = zc_compress_new(ZC_GZIP_DEC, 0);
                    break;
                default:
                    ZCERROR("not support compress:%d", info->resp->compress);
                }
            }
            return ZC_OK;
        }
        zc_str_append_len(&info->resp->headdata, data, datalen);
        return ZC_OK;
    }
    
    // ready body
    ZCINFO("body datalen:%d", datalen);
    info->readlen += datalen;
    int ret;
    if (info->resp->compress) {
        if (datalen > 0) {
            //ZCINFO("umcompress len:%d data: %x %x %x %x", datalen, data[0], data[1], data[2], data[3]);
            ret = zc_httpconn_gzip_uncompress_str(info->resp, data, datalen, info->compress, &info->bodyfirst);
            if (ret < 0) {
                ZCWARN("umcompress error:%d", ret);
                // close
                goto ready_error;
            }
        }
        if (info->readfunc) { // data callback
            ret = info->readfunc(info->resp->bodydata.data, info->resp->bodydata.len, false, conn);
            if (ret < 0) {
                ZCWARN("readfunc error:%d", ret);
                // close
                goto ready_error;
            }
            zc_str_clear(&info->resp->bodydata);
        }
    }else{
        if (info->readfunc) {
            ret = info->readfunc(data, datalen, false, conn);
            if (ret < 0) {
                ZCWARN("readfunc error:%d", ret);
                // close
                goto ready_error;
            }
        }else{
            if (datalen > 0)
                zc_str_append_len(&info->resp->bodydata, data, datalen);
        }
    }
    
    //ZCINFO("info->readlen:%lld, bodylen:%lld", info->readlen, info->resp->bodylen);
    if (datalen == 0 || conn->close || info->readlen == info->resp->bodylen) {
        if (info->readyfunc) 
            info->readyfunc(conn);
    }
    //ZCINFO("header:%s", info->resp->headdata.data);
    //ZCINFO("body:%s", info->resp->bodydata.data);

    return ZC_OK; 
ready_error:
    conn->p.handle_close(conn);
    zc_httpinfo_delete(conn->data);
    zc_asynconn_delete(conn);
    return ZC_ERR;
}
*/

int 
zc_asynhttp_handle_read_dns(zcAsynConn *conn)
{
    zcBuffer *buffer = conn->rbuf;
    char *data = zc_buffer_data(buffer);
    int len = zc_buffer_used(buffer);
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
    zcHttpInfo *info = conn->data;
    ZCINFO("ip:%s %d timeout:%d", ip, info->req->url.port, info->conn_timeout);

    zcProtocol p;
    zc_protocol_init(&p);
    zc_asynhttp_assign_protocol(&p);
 
    zcAsynConn *newconn = zc_asynconn_new_tcp_client(ip, info->req->url.port, 
            info->conn_timeout, &p, conn->loop, 65535, 65535);
    //zc_asynhttp_assign_protocol(newconn);
    newconn->data = info;
    conn->data = NULL;
    zc_asynhttp_send(newconn, info->req);

    zc_list_delete(result);
    
    conn->p.handle_close(conn);
    zc_asynconn_delete_delay(conn);
        
    return ZC_OK;
}



#endif
