#include <zocle/internet/http/httpclient.h>

zcHttpClient* 
zc_httpclient_new()
{
    zcHttpClient *c = zc_calloc(sizeof(zcHttpClient));
    return c;
}

void		  
zc_httpclient_delete(void *x)
{
    zcHttpClient *c = (zcHttpClient*)x;

    /*if (c->req) {
        zc_httpreq_delete(c->req);
    }*/
    if (c->resp) {
        zc_httpresp_delete(c->resp);
    }
    if (c->conn) {
        zc_httpresp_delete(c->conn);
    }
    zc_free(x);
}

int			  
zc_httpclient_open(zcHttpClient *c, zcHttpReq *req)
{
    if (c->conn == NULL) {
        c->conn = zc_httpconn_new();
    }
    int ret = zc_httpconn_send(c->conn, req);
    if (ret < 0) {
        ZCWARN("req send error: %d", ret);
        return ret;
    }
    
    int trycount = c->retry;
    while (trycount > 0) {
        c->resp = zc_httpconn_recv(c->conn);
        if (c->resp == NULL) {
            ZCWARN("recv error");
            //return ret;
        }else{
            if (c->resp->isfinish) {
                break;
            }
        }
        trycount--;
    }

    return ZC_OK;
}



