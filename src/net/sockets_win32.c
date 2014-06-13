#ifdef _WIN32
#include <zocle/log/logfile.h>
#include <zocle/ds/list.h>
#include <zocle/mem/alloc.h>
#include <zocle/net/sockets.h>
#include <winsock2.h>
#include <winsock.h>
#include <windows.h>
#include <sys/types.h>


#define WIN32_LEAN_AND_MEAN




zcSockAddr*
zc_sockaddr_new()
{
    zcSockAddr  *sa;
    sa = (zcSockAddr*)zc_malloc(sizeof(zcSockAddr));
    if (NULL == sa) {
        return NULL;
    }
    memset(sa, 0, sizeof(zcSockAddr));
    return sa;
}

void
zc_sockaddr_delete(void *x)
{
    zc_free(x);
}
void
zc_sockaddr_delete4_(void* sa, void *obj, const char *filename, int line)
{
    zc_sockaddr_delete(sa);
}


int
zc_socket_setblock (zcSocket *s, int block)
{
    if(block) {
        unsigned long arg = 0;
        if(ioctlsocket(s->fd, FIONBIO, &arg) == SOCKET_ERROR) {
            return ERRORNUM;
        }
    }else{
        unsigned long arg = 1;
        if(ioctlsocket(s->fd, FIONBIO, &arg) == SOCKET_ERROR) {
            //close_socket(s->fd);
            return ERRORNUM;
        }
    }
    return 0;
}

int
zc_socket_select (zcSocket *s, int writing)
{
    int n;
    fd_set fds;
    struct timeval tv;
 
    if (s->fd < 0)
        return ZC_ERR;

    tv.tv_sec  = s->timeout/1000;
    tv.tv_usec = s->timeout%1000 * 1000;
    
    FD_ZERO(&fds);
    FD_SET(s->fd, &fds);

    if (writing)
        n = select(s->fd+1, NULL, &fds, NULL, &tv);
    else
        n = select(s->fd+1, &fds, NULL, NULL, &tv);

    if (n < 0)
        return ZC_ERR;
    if (n == 0)
        return ZC_ERR_TIMEOUT;
    return ZC_OK;
}

int
zc_socket_startup()
{
    WORD    version = MAKEWORD(2, 2);
    WSADATA data;
    
    if (WSAStartup(version, &data) != 0) {
        char    buf[1024] = {0};
        zc_socket_last_error_string(buf, 1023);
        ZCERROR("load winsock error: %s\n", buf);
        return -1;
    }

    return 0;
}

void 
zc_socket_cleanup()
{
    WSACleanup();
}

zcSocket*
zc_socket_new (int family, int type, int proto, int timeout)
{
    zcSocket *s;
    s = (zcSocket *) zc_malloc (sizeof(zcSocket));
    if (s != NULL) {
        memset(s, 0, sizeof(zcSocket));
        s->fd = socket(family, type, proto);
        //ZCINFO("fd:%d", s->fd);
        if (s->fd == INVALID_SOCKET) {
            zc_free(s);
            //WSACleanup();
            ZCINFO("socket create error! %s\n", strerror(errno));
            return  NULL;
        }
        
        s->family = family;
        s->type = type;
        s->proto = proto;
        s->timeout = timeout; 

        /*if (timeout >= 0) {
            ZCINFO("socket noblock!\n");
            zc_socket_setblock(s, 0);
        }*/

    }
    return s;
}

zcSocket*
zc_socket_new_tcp(int timeout)
{
    return zc_socket_new(AF_INET, SOCK_STREAM, IPPROTO_TCP, timeout);
}

zcSocket*
zc_socket_new_udp(int timeout)
{
    return zc_socket_new(AF_INET, SOCK_DGRAM, IPPROTO_UDP, timeout);
}

zcSocket*
zc_socket_create ()
{
    zcSocket   *s;
    s = (zcSocket *) zc_malloc (sizeof(zcSocket));
    if (NULL == s) 
        return NULL;
    memset(s, 0,  sizeof(zcSocket));
    return s;
}

zcSocket*
zc_socket_init (SOCKET fd, int family, int type, int timeout)
{
    zcSocket   *s;
    s = zc_socket_create();
    s->fd = fd;
    
    s->family = family;
    s->type = type;
    s->proto = 0;
    s->timeout = timeout; 

    return s;
}
void
zc_socket_close (zcSocket *s)
{
    int error = WSAGetLastError();
    closesocket(s->fd);
    WSASetLastError(error);
}

void
zc_socket_delete (void *x)
{
    zcSocket *s = (zcSocket*)x;
    // 这里是否要检测socket已经关闭?
    zc_socket_close(s);
    zc_free(s);
    //WSACleanup();
}

int
zc_socket_shutdown (zcSocket *s, int how)
{
    if (shutdown(s->fd, how) == SOCKET_ERROR) {
        int error = WSAGetLastError();
        if (error == WSAENOTCONN) {
            return 0;
        }
    }
    return 0;
}

int
zc_socket_bind (zcSocket *s, char *host, int port)
{
    int  ret;
    socklen_t len;
    struct sockaddr_in  *sin, addr;
    
    sin = &(s->local_addr.sa.in);
    sin->sin_family = s->family;
    sin->sin_port = htons((short)(port));
    s->local_addr.port = port;
    
    if (NULL == host) {
        sin->sin_addr.s_addr = htonl(INADDR_ANY);
        strcpy(s->local_addr.ip, "0.0.0.0");
    }else{
        sin->sin_addr.s_addr = inet_addr(host);
        strcpy(s->local_addr.ip, host);
    }
    ret = bind(s->fd, (struct sockaddr*)(sin), (int)(sizeof(struct sockaddr_in)));
    if (ret == SOCKET_ERROR)
    {
        char buf[128] = {0};
        zc_socket_last_error_string(buf, 127);
 
        ZCERROR("bind error: %s, host: %s, port: %d\n", buf, host, port);
        return ERRORNUM;
    }

    len = (socklen_t)(sizeof(struct sockaddr_in));
    s->local_addr.sa_len = len;
#ifdef NDEBUG
    getsockname(s->fd, (struct sockaddr*)(addr), &len);
#else
    ret = getsockname(s->fd, (struct sockaddr*)(&addr), &len);
    if (ret == SOCKET_ERROR)
    {
        char buf[128] = {0};
        zc_socket_last_error_string(buf, 127);
        ZCERROR("bind error: %s\n", buf);
        return ERRORNUM;
    }
#endif

    return 0;
}

int
zc_socket_listen (zcSocket *s, int backlog)
{
    if (listen(s->fd, backlog) == -1) {
        return ERRORNUM;
    }else{
        return 0;
    }
}

int
zc_socket_reuse(zcSocket *s)
{
    return 0;
}

zcSocket*
zc_socket_server (char *host, int port, int type, int backlog)
{
    zcSocket   *s;
    int         ret;
    
    if (type == SOCK_STREAM) {
        s = zc_socket_new_tcp(-1);
    }else{
        s = zc_socket_new_udp(-1);
    }
    if (NULL == s) {
        //ZCERROR("socket create error! %s:%d\n", host, port);
        return s;
    }
    
    //zc_socket_reuse(s); 
    if (host) {
        ret = zc_socket_bind(s, host, port);
        if (ret < 0) {
            goto zc_socket_server_error_finally;
        }
    }
    
    if (type == SOCK_STREAM) { 
        ret = zc_socket_listen(s, backlog);
        if (ret < 0) {
            goto zc_socket_server_error_finally;
        }
    }
    return s;

zc_socket_server_error_finally:
    ZCINFO("socket bind error!\n", ERRORNUM);
    zc_socket_delete(s);
    
    return NULL;
}

zcSocket*
zc_socket_server_tcp(char *host, int port, int backlog)
{
    return zc_socket_server(host, port, SOCK_STREAM, backlog);
}

zcSocket*
zc_socket_server_udp(char *host, int port, int backlog)
{
    return zc_socket_server(host, port, SOCK_DGRAM, backlog);
}

zcSocket*   
zc_socket_client_tcp (char *host, int port, int timeout)
{
    zcSocket   *s;
    int         ret;
    
    s = zc_socket_new(AF_INET, SOCK_STREAM, 0, timeout);
    if (NULL == s) {
        ZCERROR("socket create error!\n");
        return s;
    }
    
    ret = zc_socket_connect(s, host, port);
    if (ret < 0) {
        ZCERROR("socket connect error! %s %s:%d\n", strerror(-1*ret), host, port);
        goto zc_socket_client_error_finally;
    }
    return s;

zc_socket_client_error_finally:
    zc_socket_delete(s);
    
    return NULL;
}

zcSocket*
zc_socket_accept (zcSocket *s)
{
    zcSocket   *newsock;
    SOCKET    sock; 
    
    while (1) {
        //sock = accept(s->fd, (struct sockaddr*)&(newsock->remote_addr.sa), &(newsock->remote_addr.sa_len));
        sock = accept(s->fd, 0, 0);
        if (sock == INVALID_SOCKET) {
            char    buf[128] = {0};
            if (zc_socket_accept_interrupt(WSAGetLastError())) {
                continue;
            }
            zc_socket_last_error_string(buf, 127);
            ZCERROR("accept error: %s\n", buf);
            return NULL;
        }
        break;
    }

    newsock = zc_socket_create(); 
    if (NULL == newsock) {
        return NULL;
    }
    newsock->fd = sock; 
    newsock->family = s->family;
    newsock->proto = s->proto;
    newsock->type = s->type;
   
    newsock->local_addr.sa_len = sizeof(newsock->local_addr.sa.in);
    if (getsockname(newsock->fd, (struct sockaddr*)&(newsock->local_addr.sa.in), &(newsock->local_addr.sa_len)) < 0) {
        ZCINFO("getsockname error! %d\n", ERRORNUM);
        zc_socket_delete(newsock);
        return NULL;
    }
    
    return newsock;
}

int
zc_socket_connect (zcSocket *s, char *host, int port)
{
    struct sockaddr_in  *sin;
    int     ret;
    
    sin = &(s->remote_addr.sa.in);
    sin->sin_family = AF_INET;
    sin->sin_port = htons((short)(port));
    
    s->remote_addr.port = port;

    if (host == NULL) {
        sin->sin_addr.s_addr = htonl(INADDR_ANY);
        strcpy(s->remote_addr.ip, "0.0.0.0");
    }else{
        sin->sin_addr.s_addr = inet_addr(host);
        strcpy(s->remote_addr.ip, host);
    }
    while (1) {
        ret = connect(s->fd, (struct sockaddr*)sin, sizeof(struct sockaddr_in));
        if (ret == SOCKET_ERROR) {
            if (zc_socket_interrupt(WSAGetLastError())) {
                continue;
            }else{
                return ERRORNUM;
            }
        }
        break; 
    }
    s->local_addr.sa_len = sizeof(s->local_addr);
    //ZCINFO("local addr len:%d", s->local_addr.sa_len);
    ret = getsockname(s->fd, (struct sockaddr*)&(s->local_addr.sa), &(s->local_addr.sa_len));
    if (ret < 0) {
        ZCWARN("getsockname error! %d %s\n", WSAGetLastError(), strerror(errno));
        return ZC_ERRNO;
    }
    
    s->remote_addr.sa_len = sizeof(s->remote_addr);
    ret = getpeername(s->fd, (struct sockaddr*)&(s->remote_addr.sa), &(s->remote_addr.sa_len));
    if (ret < 0) {
        ZCWARN("getpeername error! %d %s\n", ret, strerror(errno));
        return ZC_ERRNO;
    }

    return 0;
}

#ifdef ZOCLE_WITH_SSL
int
zc_socket_ssl_send(zcSocket *s, char *buf, int blen)
{
    //char *data;
    int len;
    //int count;
    int sockstate;
    int err;
    int nonblocking;

    /* just in case the blocking state of the socket has been changed */
    nonblocking =  !s->blocked; //(self->Socket->sock_timeout >= 0.0);
    BIO_set_nbio(SSL_get_rbio(s->ssl), nonblocking);
    BIO_set_nbio(SSL_get_wbio(s->ssl), nonblocking);

    //sockstate = check_socket_and_wait_for_timeout(self->Socket, 1);
    sockstate = zc_socket_select(s, 1);
    if (sockstate == ZC_SSL_SOCKET_HAS_TIMED_OUT) {
        ZCWARN("The write operation timed out");
        return ZC_ERR;
    } else if (sockstate == ZC_SSL_SOCKET_HAS_BEEN_CLOSED) {
        ZCWARN("Underlying socket has been closed.");
        return ZC_ERR;
    } else if (sockstate == ZC_SSL_SOCKET_TOO_LARGE_FOR_SELECT) {
        ZCWARN("Underlying socket too large for select().");
        return ZC_ERR;
    }
    do {
        err = 0;
        len = SSL_write(s->ssl, buf, blen);
        err = SSL_get_error(s->ssl, len);
 
        if (err == SSL_ERROR_WANT_READ) {
            sockstate = zc_socket_select(s, 0);
        } else if (err == SSL_ERROR_WANT_WRITE) {
            sockstate = zc_socket_select(s, 1);
        } else {
            sockstate = ZC_SSL_SOCKET_OPERATION_OK;
        }
        if (sockstate == ZC_SSL_SOCKET_HAS_TIMED_OUT) {
            ZCWARN("The write operation timed out");
            return ZC_ERR;
        } else if (sockstate == ZC_SSL_SOCKET_HAS_BEEN_CLOSED) {
            ZCWARN("Underlying socket has been closed.");
            return ZC_ERR;
        } else if (sockstate == ZC_SSL_SOCKET_IS_NONBLOCKING) {
            break;
        }
    } while (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE);
    if (len > 0)
        //return PyInt_FromLong(len);
        return len;
    else
        //return PySSL_SetError(self, len, __FILE__, __LINE__);
        return len;
}
#endif

int
zc_socket_send (zcSocket *s, char *buf, int len)
{
#ifdef ZOCLE_WITH_SSL
    if (s->ssl) {
        return zc_socket_ssl_send(s, buf, len);
    }
#endif
    return send(s->fd, buf, len, 0);     
}



#ifdef ZOCLE_WITH_SSL
int 
zc_socket_ssl_recv(zcSocket *s, char *buf, int len)
{
    int count = 0;
    int sockstate;
    int err;
    int nonblocking;


    /* just in case the blocking state of the socket has been changed */
    nonblocking = !s->blocked; //(self->Socket->sock_timeout >= 0.0);
    BIO_set_nbio(SSL_get_rbio(s->ssl), nonblocking);
    BIO_set_nbio(SSL_get_wbio(s->ssl), nonblocking);

    /* first check if there are bytes ready to be read */
    count = SSL_pending(s->ssl);
    if (!count) {
        sockstate = zc_socket_select(s, 0);
        if (sockstate == ZC_SSL_SOCKET_HAS_TIMED_OUT) {
            ZCWARN("The read operation timed out");
            return ZC_ERR;
        } else if (sockstate == ZC_SSL_SOCKET_TOO_LARGE_FOR_SELECT) {
            ZCWARN("Underlying socket too large for select().");
            return ZC_ERR;
        } else if (sockstate == ZC_SSL_SOCKET_HAS_BEEN_CLOSED) {
            if (SSL_get_shutdown(s->ssl) != SSL_RECEIVED_SHUTDOWN) {
                ZCWARN("Socket closed without SSL shutdown handshake");
                return ZC_ERR;
            } else {
                /* should contain a zero-length string */
                //_PyString_Resize(&buf, 0);
                //return buf;
                return 0;
            }
        }
    }
    do {
        err = 0;
        count = SSL_read(s->ssl, buf, len);
        err = SSL_get_error(s->ssl, count);

        if (err == SSL_ERROR_WANT_READ) {
            sockstate = zc_socket_select(s, 0);
        } else if (err == SSL_ERROR_WANT_WRITE) {
            sockstate = zc_socket_select(s, 1);
        } else if ((err == SSL_ERROR_ZERO_RETURN) &&
            (SSL_get_shutdown(s->ssl) == SSL_RECEIVED_SHUTDOWN)) {
            //_PyString_Resize(&buf, 0);
            return 0;
        } else {
            sockstate = ZC_SSL_SOCKET_OPERATION_OK;
        }
        if (sockstate == ZC_SSL_SOCKET_HAS_TIMED_OUT) {
            ZCWARN("The read operation timed out");
            return ZC_ERR;
        } else if (sockstate == ZC_SSL_SOCKET_IS_NONBLOCKING) {
            break;
        }
    } while (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE);
    if (count <= 0) {
        ZCERROR("count error: %d", count);
        return ZC_ERR;
    }

    return count;
}
#endif


int
zc_socket_recv (zcSocket *s, char *buf, int len)
{
#ifdef ZOCLE_WITH_SSL
    if (s->ssl) {
        return zc_socket_ssl_recv(s, buf, len);
    }
#endif
    return recv(s->fd, buf, len, 0); 
}

int
zc_socket_sendn (zcSocket *s, char *buf, int len)
{
    int left, idx, ret;     

    left = len;
    idx = 0;

    while (left > 0) {
        ret = send(s->fd, &buf[idx], left, 0);
        if (ret == SOCKET_ERROR) {
            return idx;
        }
        left -= ret;
        idx += ret;
    }

    return idx;
}

int
zc_socket_recvn (zcSocket *s, char *buf, int len)
{
    int left, idx, ret;     
    left = len;
    idx = 0;

    while (left > 0) {
        ret = recv(s->fd, &buf[idx], left, 0);
        if (ret == SOCKET_ERROR) {
            return idx;
        }
        left -= ret;
        idx += ret;
    }

    return idx;
}

void
zc_socket_add_remote_addr(zcSocket *s, const char *host, int port)
{
    struct sockaddr_in  *sin;

    memset(&s->remote_addr, 0, sizeof(zcSockAddr));

    sin = (struct sockaddr_in*)&(s->remote_addr.sa);

    sin->sin_family = AF_INET;
    sin->sin_port = htons((short)(port));

    if (host == NULL) {
        sin->sin_addr.s_addr = htonl(INADDR_ANY);
    }else{
        sin->sin_addr.s_addr = inet_addr(host);
    }

    s->remote_addr.sa_len = sizeof(struct sockaddr_in);
}


int
zc_socket_sendto (zcSocket *s, zcSockAddr *addr, char *buf, int len, int flags)
{
    int ret;
    ret = sendto(s->fd, buf, len, flags, (struct sockaddr*)&(addr->sa.in), addr->sa_len);
    return ret;
}

int
zc_socket_recvfrom (zcSocket *s, zcSockAddr *addr, char *buf, int len, int flags)
{
    int ret;
    addr->sa_len = sizeof(addr->sa);
    ret = recvfrom(s->fd, buf, len, flags, (struct sockaddr*)&(addr->sa), &(addr->sa_len));

    return ret;
}

int
zc_socket_sendto_self (zcSocket *s, char *buf, int len, int flags)
{
    return zc_socket_sendto(s, &(s->remote_addr), buf, len, flags); 
}
int
zc_socket_recvfrom_self (zcSocket *s, char *buf, int len, int flags)
{
    return zc_socket_recvfrom(s, &(s->remote_addr), buf, len, flags); 
}

int
zc_socket_reconnect(zcSocket *s)
{
    int fd = socket(s->family, s->type, s->proto);
    if (fd < 0) {
        ZCERROR("socket create error! %s\n", strerror(errno));
        return ZC_ERRNO;
    }
    closesocket(s->fd);
    s->fd = fd;
    //ZCINFO("reconnect %s:%d\n", s->remote_addr.ip, s->remote_addr.port);
    return zc_socket_connect(s, s->remote_addr.ip, s->remote_addr.port);
} 

#ifdef ZOCLE_WITH_SSL

int 
zc_socket_ssl_handshake(zcSocket *s)
{
    int ret;
    int err;
    int sockstate, nonblocking;

    /* just in case the blocking state of the socket has been changed */
    nonblocking = !s->blocked; //(self->Socket->sock_timeout >= 0.0);
    BIO_set_nbio(SSL_get_rbio(s->ssl), nonblocking);
    BIO_set_nbio(SSL_get_wbio(s->ssl), nonblocking);

    /* Actually negotiate SSL connection */
    /* XXX If SSL_do_handshake() returns 0, it's also a failure. */
    sockstate = 0;

    do {
        ret = SSL_do_handshake(s->ssl);
        err = SSL_get_error(s->ssl, ret);

        if (err == SSL_ERROR_WANT_READ) {
            sockstate = zc_socket_select(s, 0); //check_socket_and_wait_for_timeout(s->fd, 0);
        } else if (err == SSL_ERROR_WANT_WRITE) {
            sockstate = zc_socket_select(s, 1); //check_socket_and_wait_for_timeout(s->fd, 1);
        } else {
            sockstate = ZC_SSL_SOCKET_OPERATION_OK;
        }
        if (sockstate == ZC_SSL_SOCKET_HAS_TIMED_OUT) {
            //PyErr_SetString(PySSLErrorObject, ERRSTR("The handshake operation timed out"));
            ZCWARN("The handshake operation timed out");
            return ZC_ERR;
        } else if (sockstate == ZC_SSL_SOCKET_HAS_BEEN_CLOSED) {
            //PyErr_SetString(PySSLErrorObject, ERRSTR("Underlying socket has been closed."));
            ZCWARN("Underlying socket has been closed.");
            return ZC_ERR;
        } else if (sockstate == ZC_SSL_SOCKET_TOO_LARGE_FOR_SELECT) {
            //PyErr_SetString(PySSLErrorObject, ERRSTR("Underlying socket too large for select()."));
            ZCWARN("Underlying socket too large for select().");
            return ZC_ERR;
        } else if (sockstate == ZC_SSL_SOCKET_IS_NONBLOCKING) {
            break;
        }
    } while (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE);
    if (ret < 1) {
        ZCWARN("handshak error:%d", ret);
        return ZC_ERR;
    }
        //return PySSL_SetError(self, ret, __FILE__, __LINE__);

    if (s->peer_cert)
        X509_free (s->peer_cert);

    if ((s->peer_cert = SSL_get_peer_certificate(s->ssl))) {
        X509_NAME_oneline(X509_get_subject_name(s->peer_cert), s->server, X509_NAME_MAXLEN);
        X509_NAME_oneline(X509_get_issuer_name(s->peer_cert), s->issuer, X509_NAME_MAXLEN);
    }

    return ZC_OK;
}

int 
zc_socket_ssl(zcSocket *s, char *key_file, char *cert_file, zcSSLCertRequire certreq,
        zcSSLVer ver, char *cacerts_file, bool isclient)
{
    char *errstr = NULL;
    int ret;
    //int err;
    //int sockstate;

    if (!isclient && (key_file == NULL || cert_file == NULL)) {
        ZCERROR("both key and cert files must be specified for server");
        goto zc_socket_ssl_fail;
    }

    memset(s->server, '\0', sizeof(char) * X509_NAME_MAXLEN);
    memset(s->issuer, '\0', sizeof(char) * X509_NAME_MAXLEN);

    s->peer_cert = NULL;
    s->ssl = NULL;
    s->ctx = NULL;
    //s->Socket = NULL;

    /* Init OpenSSL */
    SSL_load_error_strings();
    SSL_library_init();
    OpenSSL_add_all_algorithms();

    (void) ERR_get_state();
    ERR_clear_error();

    if ((key_file && !cert_file) || (!key_file && cert_file)) {
        errstr = "Both the key & certificate files must be specified";
        goto zc_socket_ssl_fail;
    }
    
    //SSL_load_error_strings();
    //SSLeay_add_ssl_algorithms();

    if (s->sslver == ZC_SSL_VER_TLS1)
        s->ctx = SSL_CTX_new(TLSv1_method()); /* Set up context */
    else if (ver == ZC_SSL_VER_SSL3)
        s->ctx = SSL_CTX_new(SSLv3_method()); /* Set up context */
    else if (ver == ZC_SSL_VER_SSL2)
        s->ctx = SSL_CTX_new(SSLv2_method()); /* Set up context */
    else if (ver == ZC_SSL_VER_SSL23)
        s->ctx = SSL_CTX_new(SSLv23_method()); /* Set up context */

    //s->ctx = SSL_CTX_new(SSLv23_method()); /* Set up context */
    if (s->ctx == NULL) {
        errstr = "SSL_CTX_new error";
        goto zc_socket_ssl_fail;
    }

    if (certreq != ZC_SSL_CERT_NONE) {
        if (cacerts_file == NULL) {
            errstr = "No root certificates specified for verification of other-side certificates.";
            goto zc_socket_ssl_fail;
        } else {
            ret = SSL_CTX_load_verify_locations(s->ctx, cacerts_file, NULL);
            if (ret != 1) {
                //_setSSLError(NULL, 0, __FILE__, __LINE__);
                ZCERROR("load verify locations error: %d", ret);
                goto zc_socket_ssl_fail;
            }
        }
    }


    if (key_file) {
        ret = SSL_CTX_use_PrivateKey_file(s->ctx, key_file,
                                          SSL_FILETYPE_PEM);
        if (ret != 1) {
            //_setSSLError(NULL, ret, __FILE__, __LINE__);
            ZCERROR("use privatekey file error:%d", ret);
            goto zc_socket_ssl_fail;
        }

        ret = SSL_CTX_use_certificate_chain_file(s->ctx, cert_file);
        if (ret != 1) {
            /*
            fprintf(stderr, "ret is %d, errcode is %lu, %lu, with file \"%s\"\n",
                ret, ERR_peek_error(), ERR_peek_last_error(), cert_file);
                */
            if (ERR_peek_last_error() != 0) {
                //_setSSLError(NULL, ret, __FILE__, __LINE__);
                ZCERROR("peek last error failed:%d", ret);
                goto zc_socket_ssl_fail;
            }
        }
    }


    /* ssl compatibility */
    SSL_CTX_set_options(s->ctx, SSL_OP_ALL);

    int verification_mode = SSL_VERIFY_NONE;
    if (certreq == ZC_SSL_CERT_OPTIONAL)
        verification_mode = SSL_VERIFY_PEER;
    else if (certreq == ZC_SSL_CERT_REQUIRED)
        verification_mode = (SSL_VERIFY_PEER |
                             SSL_VERIFY_FAIL_IF_NO_PEER_CERT);
    SSL_CTX_set_verify(s->ctx, verification_mode, NULL); /* set verify lvl */

    
    s->ssl = SSL_new(s->ctx); /* New ssl struct */
    SSL_set_fd(s->ssl, s->fd);       /* Set the socket for SSL */
#ifdef SSL_MODE_AUTO_RETRY
    SSL_set_mode(s->ssl, SSL_MODE_AUTO_RETRY);
#endif

    /* If the socket is in non-blocking mode or timeout mode, set the BIO
     * to non-blocking mode (blocking is the default)
     */
    if (!s->blocked) {
        /* Set both the read and write BIO's to non-blocking mode */
        BIO_set_nbio(SSL_get_rbio(s->ssl), 1);
        BIO_set_nbio(SSL_get_wbio(s->ssl), 1);
    }

    if (isclient) {
        SSL_set_connect_state(s->ssl);
    }else{
        SSL_set_accept_state(s->ssl);
    }
    
    if (isclient) {
        ret = zc_socket_ssl_handshake(s);
        if (ret != ZC_OK) {
            ZCERROR("ssl handshake error: %d", ret);
            goto zc_socket_ssl_fail;
        }
    }
    return ZC_OK;

zc_socket_ssl_fail:
    if (errstr) {
        ZCERROR("ssl error: %s\n", errstr);
    }

    return -1;
}

int 
zc_socket_client_ssl(zcSocket *s, char *key_file, char *cert_file, zcSSLCertRequire certreq,
        zcSSLVer ver, char *cacerts_file)
{
    return zc_socket_ssl(s, key_file, cert_file, certreq, ver, cacerts_file, true);
}

int 
zc_socket_server_ssl(zcSocket *s, char *key_file, char *cert_file, zcSSLCertRequire certreq,
        zcSSLVer ver, char *cacerts_file)
{
    return zc_socket_ssl(s, key_file, cert_file, certreq, ver, cacerts_file, false);
}

#endif

int 
zc_socket_peek(zcSocket *s)
{
    return 1;
}

int
zc_socket_interrupt(int err)
{
    return WSAGetLastError() == WSAEINTR;
}


int
zc_socket_accept_interrupt(int err)
{
    int error = WSAGetLastError();
    if(zc_socket_interrupt(error)) {
        return 1;
    }
    return error == WSAECONNABORTED ||
           error == WSAECONNRESET ||
           error == WSAETIMEDOUT;
}

int
zc_socket_no_buffers(int err)
{
    int error = WSAGetLastError();
    return error == WSAENOBUFS ||
           error == WSAEFAULT;
}

int
zc_socket_would_block(int err)
{
    return WSAGetLastError() == WSAEWOULDBLOCK;    
}

int
zc_socket_conn_failed(int err)
{
    int error = WSAGetLastError();
    return error == WSAECONNREFUSED ||
           error == WSAETIMEDOUT ||
           error == WSAENETUNREACH ||
           error == WSAEHOSTUNREACH ||
           error == WSAECONNRESET ||
           error == WSAESHUTDOWN ||
           error == WSAECONNABORTED;
}

int
zc_socket_conn_refused(int err)
{
    int error = WSAGetLastError();
    return error == WSAECONNREFUSED;
}

int
zc_socket_conn_progress(int err)
{
    return WSAGetLastError() == WSAEWOULDBLOCK;    
}

int
zc_socket_conn_lost(int err)
{
    int error = WSAGetLastError();
    return error == WSAECONNRESET ||
           error == WSAESHUTDOWN ||
           error == WSAENOTCONN ||
           error == WSAECONNABORTED;
}

int
zc_socket_not_connected(int err)
{
    return WSAGetLastError() == WSAENOTCONN;    
}

int
zc_socket_is_connected(zcSocket *s)
{
    return 1;    
}

int
zc_socket_tcp_nodelay(zcSocket *s)
{
    int flag = 1;
    if (setsockopt(s->fd, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(int)) == SOCKET_ERROR) {
	   return ERRORNUM;
    }
    return 0;
}
int
zc_socket_keepalive(zcSocket *s)
{
    int flag = 1;
    if (setsockopt(s->fd, SOL_SOCKET, SO_KEEPALIVE, (char*)&flag, sizeof(int)) == SOCKET_ERROR) {
        return ERRORNUM;
    }
    return 0;
}
int
zc_socket_get_send_buffer_size(zcSocket *s)
{
    int sz;
    socklen_t len = sizeof(sz);
    if (getsockopt(s->fd, SOL_SOCKET, SO_SNDBUF, (char*)&sz, &len) == SOCKET_ERROR || len != sizeof(sz)) {
        return ERRORNUM;
    }
    return sz;
    
}
int
zc_socket_set_send_buffer_size(zcSocket *s, int sz)
{
    if (setsockopt(s->fd, SOL_SOCKET, SO_SNDBUF, (char*)&sz, sizeof(int)) == SOCKET_ERROR) {
        return ERRORNUM;
    }
    return 0;    
}
int
zc_socket_get_recv_buffer_size(zcSocket *s)
{
    int sz;
    socklen_t len = sizeof(sz);
    if (getsockopt(s->fd, SOL_SOCKET, SO_RCVBUF, (char*)&sz, &len) == SOCKET_ERROR || len != sizeof(sz)) {
        return ERRORNUM;
    }
    return sz;
}
int
zc_socket_set_recv_buffer_size(zcSocket *s, int sz)
{
    if (setsockopt(s->fd, SOL_SOCKET, SO_RCVBUF, (char*)&sz, (int)(sizeof(int))) == SOCKET_ERROR) {
        return ERRORNUM;
    }
    return 0;
}

int
zc_socket_error_string(int err, char *buf, int buflen)
{
    /*
    if(error < WSABASEERR)
    {
	LPVOID lpMsgBuf = 0;
	DWORD ok = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
				FORMAT_MESSAGE_FROM_SYSTEM |
				FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL,
				error,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
				(LPTSTR)&lpMsgBuf,
				0,
				NULL);
	if(ok)
	{
	    LPCTSTR msg = (LPCTSTR)lpMsgBuf;
	    string result = (const char*)msg;

	    assert(msg && strlen((const char*)msg) > 0);
	    if(result[result.length() - 1] == '\n')
	    {
		result = result.substr(0, result.length() - 2);
	    }
	    LocalFree(lpMsgBuf);
	    snprintf(buf, result, buflen);
	    return 0;
	}else{
	    snprintf(buf, buflen, "unknown error: %d\n", error);
	    return 0;
	}
    }*/

    switch(err) {
    case WSAEINTR:
	   strncpy(buf, "WSAEINTR", buflen);
	   break;
    case WSAEBADF:
	   strncpy(buf, "WSAEBADF", buflen);
	   break;
    case WSAEACCES:
	   strncpy(buf, "WSAEACCES", buflen);
	   break;
    case WSAEFAULT:
	   strncpy(buf, "WSAEFAULT", buflen);
	   break;
    case WSAEINVAL:
	   strncpy(buf, "WSAEINVAL", buflen);
	   break;
    case WSAEMFILE:
	   strncpy(buf, "WSAEMFILE", buflen);
	   break;
    case WSAEWOULDBLOCK:
	   strncpy(buf, "WSAEWOULDBLOCK", buflen);
	   break;
    case WSAEINPROGRESS:
	   strncpy(buf, "WSAEINPROGRESS", buflen);
	   break;
    case WSAEALREADY:
	   strncpy(buf, "WSAEALREADY", buflen);
	   break;
    case WSAENOTSOCK:
	   strncpy(buf, "WSAENOTSOCK", buflen);
	   break;
    case WSAEDESTADDRREQ:
	   strncpy(buf, "WSAEDESTADDRREQ", buflen);
	   break;
    case WSAEMSGSIZE:
	   strncpy(buf, "WSAEMSGSIZE", buflen);
	   break;
    case WSAEPROTOTYPE:
	   strncpy(buf, "WSAEPROTOTYPE", buflen);
	   break;
    case WSAENOPROTOOPT:
	   strncpy(buf, "WSAENOPROTOOPT", buflen);
	   break;
    case WSAEPROTONOSUPPORT:
	   strncpy(buf, "WSAEPROTONOSUPPORT", buflen);
	   break;
    case WSAESOCKTNOSUPPORT:
	   strncpy(buf, "WSAESOCKTNOSUPPORT", buflen);
	   break;
    case WSAEOPNOTSUPP:
	   strncpy(buf, "WSAEOPNOTSUPP", buflen);
	   break;
    case WSAEPFNOSUPPORT:
	   strncpy(buf, "WSAEPFNOSUPPORT", buflen);
	   break;
    case WSAEAFNOSUPPORT:
	   strncpy(buf, "WSAEAFNOSUPPORT", buflen);
	   break;
    case WSAEADDRINUSE:
	   strncpy(buf, "WSAEADDRINUSE", buflen);
	   break;
    case WSAEADDRNOTAVAIL:
	   strncpy(buf, "WSAEADDRNOTAVAIL", buflen);
	   break;
    case WSAENETDOWN:
	   strncpy(buf, "WSAENETDOWN", buflen);
	   break;
    case WSAENETUNREACH:
	   strncpy(buf, "WSAENETUNREACH", buflen);
	   break;
    case WSAENETRESET:
	   strncpy(buf, "WSAENETRESET", buflen);
	   break;
    case WSAECONNABORTED:
	   strncpy(buf, "WSAECONNABORTED", buflen);
	   break;
    case WSAECONNRESET:
	   strncpy(buf, "WSAECONNRESET", buflen);
	   break;
    case WSAENOBUFS:
	   strncpy(buf, "WSAENOBUFS", buflen);
	   break;
    case WSAEISCONN:
	   strncpy(buf, "WSAEISCONN", buflen);
	   break;
    case WSAENOTCONN:
	   strncpy(buf, "WSAENOTCONN", buflen);
	   break;
    case WSAESHUTDOWN:
	   strncpy(buf, "WSAESHUTDOWN", buflen);
	   break;
    case WSAETOOMANYREFS:
	   strncpy(buf, "WSAETOOMANYREFS", buflen);
	   break;
    case WSAETIMEDOUT:
	   strncpy(buf, "WSAETIMEDOUT", buflen);
	   break;
    case WSAECONNREFUSED:
	   strncpy(buf, "WSAECONNREFUSED", buflen);
	   break;
    case WSAELOOP:
	   strncpy(buf, "WSAELOOP", buflen);
	   break;
    case WSAENAMETOOLONG:
	   strncpy(buf, "WSAENAMETOOLONG", buflen);
	   break;
    case WSAEHOSTDOWN:
	   strncpy(buf, "WSAEHOSTDOWN", buflen);
	   break;
    case WSAEHOSTUNREACH:
	   strncpy(buf, "WSAEHOSTUNREACH", buflen);
	   break;
    case WSAENOTEMPTY:
	   strncpy(buf, "WSAENOTEMPTY", buflen);
	   break;
    case WSAEPROCLIM:
	   strncpy(buf, "WSAEPROCLIM", buflen);
	   break;
    case WSAEUSERS:
	   strncpy(buf, "WSAEUSERS", buflen);
	   break;
    case WSAEDQUOT:
	   strncpy(buf, "WSAEDQUOT", buflen);
	   break;
    case WSAESTALE:
	   strncpy(buf, "WSAESTALE", buflen);
	   break;
    case WSAEREMOTE:
	   strncpy(buf, "WSAEREMOTE", buflen);
	   break;
    case WSAEDISCON:
	   strncpy(buf, "WSAEDISCON", buflen);
	   break;
    case WSASYSNOTREADY:
	   strncpy(buf, "WSASYSNOTREADY", buflen);
	   break;
    case WSAVERNOTSUPPORTED:
	   strncpy(buf, "WSAVERNOTSUPPORTED", buflen);
	   break;
    case WSANOTINITIALISED:
	   strncpy(buf, "WSANOTINITIALISED", buflen);
	   break;
    case WSAHOST_NOT_FOUND:
	   strncpy(buf, "WSAHOST_NOT_FOUND", buflen);
	   break;
    case WSATRY_AGAIN:
	   strncpy(buf, "WSATRY_AGAIN", buflen);
	   break;
    case WSANO_RECOVERY:
	   strncpy(buf, "WSANO_RECOVERY", buflen);
	   break;
    case WSANO_DATA:
	   strncpy(buf, "WSANO_DATA", buflen);
        break;
    default:
	   snprintf(buf, buflen, "unknown socket error: %d", err);
    }
    return 0;
}

int
zc_socket_last_error_string(char *buf, int buflen)
{
    return zc_socket_error_string(WSAGetLastError(), buf, buflen);
}


zcList*
zc_socket_local_addr()
{
    zcList       *addrs;
    zcSockAddr  *addr;
    int     i; 
    SOCKET fd = socket(AF_INET, SOCK_STREAM, 0);
    //struct sockaddr_in   *addrtemp;
    unsigned char buffer[10240];
    unsigned long len = 0;
    struct sockaddr_in sin;
    DWORD rs;

    rs = WSAIoctl(fd, SIO_ADDRESS_LIST_QUERY, 0, 0, &buffer[0], 10240, &len, 0, 0);
    if (rs == SOCKET_ERROR) {
        closesocket(fd);
        return NULL;
    }
    //
    // Add the local interface addresses.
    //
    addrs = zc_list_new();
    SOCKET_ADDRESS_LIST* addrls = (SOCKET_ADDRESS_LIST*)(&buffer[0]);
    for (i = 0; i < addrls->iAddressCount; ++i) {
        addr = zc_sockaddr_new();
        memcpy(&addr->sa.in, addrls->Address[i].lpSockaddr, sizeof(struct sockaddr_in)); 
        zc_list_push_back(addrs, zc_listnode_new_data(addr));
    }
    //
    // Add the loopback interface address.
    //
    memset(&addr, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(0);
    sin.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    
    addr = zc_sockaddr_new();
    memcpy(&addr->sa.in, &sin, sizeof(struct sockaddr_in));
    zc_list_push_back(addrs, zc_listnode_new_data(addr));
    closesocket(fd);

    return addrs;        
}

int 
zc_socket_gethostbyname(const char *name, zcCStrList *ip)
{
    struct hostent *h = gethostbyname(name);
    char buf[16];
    char **p; 
    for (p=h->h_addr_list; *p!=NULL; p++) {
        struct in_addr *a = (struct in_addr*)*p;
        strcpy(buf, inet_ntoa(*a));
        zc_cstrlist_append(ip, buf, 0);
    }
    return ip->len;
}

#endif
