#ifndef _WIN32
#include <zocle/net/sockets.h>
#include <zocle/log/logfile.h>
#include <zocle/mem/alloc.h>
#include <zocle/ds/list.h>
#include <zocle/str/string.h>

#if defined(__linux) || defined(__APPLE__) || defined(__FreeBSD__)
//#include <ifaddrs.h>
    #include <pthread.h>
    #include <errno.h>
    #include <sys/ioctl.h>
#else
    #include <sys/ioctl.h>
    #include <net/if.h>
    #ifdef __sun
        #include <sys/sockio.h>
    #endif
#endif

#ifdef __linux
    #include <linux/if.h>
    #include <sys/un.h>
#endif

#ifdef __sun
#define INADDR_NONE (unsigned long)-1
#endif

static int
_socket_setblock(SOCKET fd, int block)
{
    int delay_flag;

    delay_flag = fcntl(fd, F_GETFL, 0);
    if (block) {
        delay_flag &= (~O_NONBLOCK);
    }else{
        delay_flag |= O_NONBLOCK;
    }
    return fcntl(fd, F_SETFL, delay_flag);
    //return ZC_OK;
}

zcSockAddr*
zc_sockaddr_new(int family)
{
    zcSockAddr  *sa;
    sa = (zcSockAddr*)zc_calloct(zcSockAddr);
    if (NULL == sa) {
        return NULL;
    }
    zc_sockaddr_init(sa, family);
    return sa;
}

int
zc_sockaddr_init(zcSockAddr *sockaddr, int family)
{
    if (family == AF_INET) {
        if (sockaddr->addr == NULL) {
            sockaddr->addr = (struct sockaddr*)zc_malloct(struct sockaddr_in);
        }
        memset(sockaddr->addr, 0, sizeof(struct sockaddr_in));
        sockaddr->sa_len = sizeof(struct sockaddr_in);
        sockaddr->addr->sa_family = family;
    }else if (family == AF_INET6) {
        if (sockaddr->addr == NULL) {
            sockaddr->addr = (struct sockaddr*)zc_malloct(struct sockaddr_in6);
        }
        memset(sockaddr->addr, 0, sizeof(struct sockaddr_in6));
        sockaddr->sa_len = sizeof(struct sockaddr_in6);
        sockaddr->addr->sa_family = family;
    }else if (family == AF_UNIX) {
        if (sockaddr->addr == NULL) {
            sockaddr->addr = (struct sockaddr*)zc_malloct(struct sockaddr_un);
        }
        memset(sockaddr->addr, 0, sizeof(struct sockaddr_un));
        sockaddr->sa_len = sizeof(struct sockaddr_un);
        sockaddr->addr->sa_family = family;
    }else{
        return ZC_ERR;
    }
    return ZC_OK;
}

int
zc_sockaddr_init_unix(zcSockAddr *sockaddr, const char *path)
{
    zc_sockaddr_init(sockaddr, AF_UNIX);
    strcpy(((struct sockaddr_un*)sockaddr->addr)->sun_path, path);
    return ZC_OK;
}

int
zc_sockaddr_init_net(zcSockAddr *sockaddr, const char *ip, uint16_t port)
{
    strncpy(sockaddr->ip, ip, 15);
    sockaddr->port = port;
        
    zc_sockaddr_init(sockaddr, AF_INET);
    return ZC_OK;
}

int
zc_sockaddr_unpack(zcSockAddr *sockaddr)
{
    if (sockaddr->addr == NULL)
        return ZC_ERR;
    if (sockaddr->addr->sa_family != AF_INET) {
        sockaddr->ip[0] = 0;
        sockaddr->port  = 0;
        return ZC_OK;
    }

    struct sockaddr_in *la = (struct sockaddr_in*)sockaddr->addr;
    inet_ntop(AF_INET, &(la->sin_addr), sockaddr->ip, sockaddr->sa_len);
    sockaddr->port = ntohs((short)la->sin_port);
    return ZC_OK;
}

void
zc_sockaddr_delete(void* sa)
{
    zcSockAddr *sockaddr = (zcSockAddr*)sa;
    if (sockaddr->addr) {
        zc_free(sockaddr->addr);
    }
    zc_free(sa);
}


void
zc_sockaddr_delete4_(void* sa, void *obj, const char *filename, int line)
{
    //zc_free(sa);
    zc_sockaddr_delete(sa);
}


int
zc_socket_setblock(zcSocket *s, int block)
{
    if (block)
        s->blocked = ZC_TRUE;
    else
        s->blocked = ZC_FALSE; 
    return _socket_setblock(s->fd, block);
}

int
zc_socket_reuse(zcSocket *s)
{
    int n = 1;
    if (setsockopt(s->fd, SOL_SOCKET, SO_REUSEADDR, (const void*)&n, sizeof(int)) == -1) {
        ZCWARN("set socket addr %d reuse error! %s\n", s->fd, strerror(errno));
        return ZC_FAILED;
    }
    return ZC_OK;
}


int
zc_socket_select(zcSocket *s, int writing)
{
    int n;
    if (s->timeout <= 0)
        return ZC_OK;

    if (s->fd < 0)
        return ZC_ERR_SOCKET;

#ifdef HAVE_POLL
    {
        struct pollfd pollfd;
        //int timeout;
        
        while (1) {
            pollfd.fd = s->fd;
            pollfd.events = writing ? POLLOUT : POLLIN;

            n = poll(&pollfd, 1, s->timeout);
            if (n < 0) {
                if (errno == EINTR)
                    continue;
                return ZC_ERRNO;
            }else if (n == 0) {
                return ZC_ERR_TIMEOUT;            
            }
            if ((pollfd.revents & POLLOUT) && writing) {
                return ZC_OK;
            }
            if ((pollfd.revents & POLLIN) && !writing) {
                return ZC_OK;
            }
            return ZC_ERR;
        }
    }
#else

    if (s->fd >= FD_SETSIZE) {
        ZCERROR("socket fd is big than %d", FD_SETSIZE);
        return ZC_ERR;
    }

    while (1) {
        fd_set fds;
        struct timeval tv;
        
        tv.tv_sec  = (unsigned int)s->timeout/1000;
        tv.tv_usec = (unsigned int)((s->timeout%1000) * 1000);
        FD_ZERO(&fds);
        FD_SET(s->fd, &fds);

        if (writing)
            n = select(s->fd+1, NULL, &fds, NULL, &tv);
        else
            n = select(s->fd+1, &fds, NULL, NULL, &tv);

        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            //return  ZC_ERR_SOCKET;
            return ZC_ERRNO;
        }
        break;
    }

    if (n == 0)
        return ZC_ERR_TIMEOUT;
    return ZC_OK;
#endif
}

int 
zc_socket_startup()
{
    return 0;
}

void
zc_socket_cleanup()
{
}

zcSocket*
zc_socket_new(int family, int type, int proto, int timeout)
{
    zcSocket *s;
    s = (zcSocket *) zc_calloct(zcSocket);

    s->fd = socket(family, type, proto);
    if (s->fd < 0) {
        zc_free(s);
        ZCERROR("socket create error! %s\n", strerror(errno));
        return  NULL;
    }

    s->family  = family;
    s->type    = type;
    s->timeout = timeout; 
    s->blocked = zc_socket_isblock(s);

    zc_sockaddr_init(&s->local, family); 
    zc_sockaddr_init(&s->remote, family); 

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

#ifdef AF_UNIX
zcSocket*
zc_socket_new_unix(char *path, int type, int timeout)
{
    // unix domain socket
    zcSocket *s = (zcSocket *) zc_calloct(zcSocket);
    s->fd = socket(AF_UNIX, type, 0);
    if (s->fd < 0) {
        zc_free(s);
        ZCERROR("socket create error! %s\n", strerror(errno));
        return NULL;
    }

    s->family  = AF_UNIX;
    s->type    = type;
    s->timeout = timeout;
    s->blocked = zc_socket_isblock(s);

    zc_sockaddr_init_unix(&s->local, path);

    return s;
}

zcSocket*
zc_socket_new_unix_tcp(char *path, int timeout)
{
    return zc_socket_new_unix(path, SOCK_STREAM, timeout);
}

zcSocket*
zc_socket_new_unix_udp(char *path, int timeout)
{
    return zc_socket_new_unix(path, SOCK_DGRAM, timeout);
}

#endif

zcSocket*
zc_socket_create()
{
    zcSocket *s = (zcSocket *)zc_calloct(zcSocket);
    return s;
}

/*!
 \brief 从一个现有的套接字创建出zcSocket对象
 */
zcSocket*
zc_socket_init(SOCKET fd, int family, int type, int timeout)
{
    zcSocket *s = zc_socket_create();
    s->fd       = fd;
    s->family   = family;
    s->type     = type;
    s->timeout  = timeout; 
    s->blocked  = zc_socket_isblock(s);

    zc_sockaddr_init(&s->local, family); 
    zc_sockaddr_init(&s->remote, family); 

    if (getsockname(fd, (struct sockaddr*)(s->local.addr), &s->local.sa_len) < 0) {
        ZCWARN("getsockname error! %s\n", strerror(errno));
    }
    if (getpeername(fd, (struct sockaddr*)(s->remote.addr), &s->remote.sa_len) < 0) {
        ZCWARN("getpeername error!\n");
    }
    //ZCINFO("the remote addr is %u\n", s->remote_addr.sa.in.sin_addr);

    return s;
}

void
zc_socket_close(zcSocket  *s)
{
#ifdef ZOCLE_WITH_SSL
    if (s->sslobj && s->sslobj->peer_cert) {
        X509_free(s->sslobj->peer_cert);
        SSL_shutdown(s->sslobj->ssl);
    }
#endif
    if (s->local.addr) {
        zc_free(s->local.addr);
    }
    if (s->remote.addr) {
        zc_free(s->remote.addr);
    }

    if (s->fd > 0) {
        close(s->fd);
        s->fd = -1;
    }   
#ifdef ZOCLE_WITH_SSL
    if (s->sslobj) {
        if (s->sslobj->ssl)
            SSL_free(s->sslobj->ssl);
        if (s->sslobj->ctx)
            SSL_CTX_free(s->sslobj->ctx);
    }
#endif
    zc_free(s);
}
void
zc_socket_delete(void *s)
{
    zcSocket *sock = (zcSocket*)s;
    if (sock) {
        zc_socket_close(sock);
    }
#ifdef ZOCLE_WITH_SSL
    if (sock->sslobj) {
        zc_free(sock->sslobj);
    }
#endif
    zc_free(sock);
}

void
zc_socket_delete4_(void *s, void *obj, const char *file, int line)
{
    zc_socket_delete(s);
}


int
zc_socket_shutdown(zcSocket *s, int how)
{
    return (shutdown(s->fd, how) == -1) ? ERRORNUM : 0;
}

int 
zc_socket_shutdown_read(zcSocket *s)
{
    return (shutdown(s->fd, SHUT_RD) == -1) ? ERRORNUM : 0;
}

int 
zc_socket_shutdown_write(zcSocket *s)
{
    return (shutdown(s->fd, SHUT_WR) == -1) ? ERRORNUM : 0;
}

int
zc_socket_bind(zcSocket *s, char *host, int port)
{
#ifdef AF_UNIX
    if (s->family == AF_UNIX) {
        struct sockaddr_un  *sockaddr = (struct sockaddr_un*)s->local.addr;
        size_t slen = SUN_LEN(sockaddr);
        //ZCINFO("unix domain path:%s, len:%u\n", sockaddr->sun_path, (unsigned int)slen);
        if (bind(s->fd, (struct sockaddr*)sockaddr, slen) == -1) {
            ZCERROR("bind on %s error: %s\n", sockaddr->sun_path, strerror(errno));
            return ZC_ERRNO;
        }
        s->local.port = 0;
        return ZC_OK;
    }
#endif
    struct sockaddr_in  *sin = (struct sockaddr_in*)s->local.addr;
    sin->sin_family = AF_INET;
    sin->sin_port = htons((short)port);
    
    //ZCINFO("fd %d bind to: %s %d\n", s->fd, host, port);
    if (host == NULL) {
        strcpy(s->local.ip, "0.0.0.0");
        sin->sin_addr.s_addr = htonl(INADDR_ANY);
    }else{
        strncpy(s->local.ip, host, 15);
        sin->sin_addr.s_addr = inet_addr(host);
#if defined(__FreeBSD__) || defined(__APPLE__)
        memset(&(sin->sin_zero), 0, sizeof(sin->sin_zero));
#endif
    }
    s->local.port = port;

    if (bind(s->fd, (struct sockaddr*)sin, sizeof(struct sockaddr_in)) == -1) {
        ZCERROR("bind on %s:%d error: %s\n", host, port, strerror(errno));
        return ZC_ERRNO; 
    }

    return ZC_OK;
}

int
zc_socket_listen(zcSocket *s, int backlog)
{
    if (listen(s->fd, backlog) == -1) {
        return ZC_ERRNO;
    }else{
        return ZC_OK;
    }
}

zcSocket*
zc_socket_server(char *host, int port, int type, int backlog)
{
    zcSocket   *s;
    int         ret;
    
    s = zc_socket_new(AF_INET, type, 0, -1);
    if (NULL == s) {
        ZCERROR("socket create error!\n");
        return s;
    }
    zc_socket_reuse(s); 
    
    ret = zc_socket_bind(s, host, port);
    if (ret < 0) {
        //ZCERROR("socket bind error! %s\n", strerror(-1*ret));
        goto zc_socket_server_error_finally;
    }
    //ZCINFO("bind at %s:%d", s->local.ip, s->local.port);    
    if (type == SOCK_STREAM) { 
        ret = zc_socket_listen(s, backlog);
        if (ret < 0) {
            ZCERROR("socket listen error! %s\n", strerror(-1*ret));
            goto zc_socket_server_error_finally;
        }
    }
    return s;

zc_socket_server_error_finally:
    zc_socket_delete(s);
    
    return NULL;
}

zcSocket*  
zc_socket_server_tcp (char *host, int port, int backlog)
{
    return zc_socket_server(host, port, SOCK_STREAM, backlog);
}

zcSocket*  
zc_socket_server_udp (char *host, int port, int backlog)
{
    return zc_socket_server(host, port, SOCK_DGRAM, backlog);
}

zcSocket*   
zc_socket_client_tcp (char *host, int port, int timeout)
{
    zcSocket *s;
    int       ret;
    
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

#ifdef AF_UNIX
zcSocket*
zc_socket_server_unix(char *path, int type, int timeout)
{
    zcSocket *s;
    s = zc_socket_new_unix(path, type, timeout);
    if (NULL == s) {
        ZCERROR("socket create error!\n");
        return NULL;
    }
    
    int ret = zc_socket_connect(s, NULL, 0);
    if (ret < 0) {
        int err = ret * -1;
        switch(err) {
            case ECONNREFUSED:
                unlink(path);
                break;
            case ENOENT:
                break;
            default:
                zc_socket_delete(s);
                return NULL;
        }
    }else{
        ZCERROR("unix domain socket connect error: %s\n", strerror(-1*ret));
        goto zc_socket_server_unix_error_finally;
    }
        
    ret = zc_socket_bind(s, NULL, 0);
    if (ret < 0) {
        ZCERROR("socket bind error! %s\n", strerror(-1*ret));
        goto zc_socket_server_unix_error_finally;
    }
    
    if (type == SOCK_STREAM) {
        ret = zc_socket_listen(s, 64);
        if (ret < 0) {
            ZCERROR("socket listen error! %s\n", strerror(-1*ret));
            goto zc_socket_server_unix_error_finally;
        }
    }
    return s;

zc_socket_server_unix_error_finally:
    zc_socket_delete(s);

    return NULL;
}

zcSocket*
zc_socket_server_unix_tcp(char *path, int timeout)
{
    return zc_socket_server_unix(path, SOCK_STREAM, timeout);
}

zcSocket*
zc_socket_server_unix_udp(char *path, int timeout)
{
    return zc_socket_server_unix(path, SOCK_DGRAM, timeout);
}

int
zc_socket_pair(int type, zcSocket **sock1, zcSocket **sock2)
{
    int fds[2] = {0};
    int ret;

    ret = socketpair(AF_UNIX, type, 0, fds);
    if (ret < 0) {
        ZCERROR("socketpair error:%d %s", ret, strerror(errno));
        return ret;
    }

    *sock1 = zc_socket_init(fds[0], AF_UNIX, type, 0);
    *sock2 = zc_socket_init(fds[1], AF_UNIX, type, 0);
    return 0;
}

int
zc_socket_pair_tcp(zcSocket **sock1, zcSocket **sock2)
{
    return zc_socket_pair(SOCK_STREAM, sock1, sock2);
}

int
zc_socket_pair_udp(zcSocket **sock1, zcSocket **sock2)
{
    return zc_socket_pair(SOCK_DGRAM, sock1, sock2);
}

#endif

zcSocket*
zc_socket_accept(zcSocket *s)
{
    zcSocket      *newsock;
    zcSockAddr    *sockaddr;
   
    newsock = zc_socket_create(); 
    zc_sockaddr_init(&newsock->local, s->family);
    zc_sockaddr_init(&newsock->remote, s->family);
    newsock->type = s->type;

    sockaddr = &newsock->remote;
    //struct sockaddr_in *sa = (struct sockaddr_in*)sockaddr->addr;
    while (1) {
        newsock->fd = accept(s->fd, sockaddr->addr, &(sockaddr->sa_len));
        if (newsock->fd == -1) {
            if (zc_socket_accept_interrupt(errno)) {
                continue;
            } else {
                char buf[128] = {0};
                zc_socket_last_error_string(buf, 127);
                ZCERROR("accept error: %d %s\n", errno, buf);
                zc_socket_delete(newsock);
                return NULL;
            }
        }
        zc_sockaddr_unpack(sockaddr);
        //inet_ntop(AF_INET, &(sa->sin_addr), sockaddr->ip, sockaddr->sa_len);
        //sockaddr->port = ntohs((short)sa->sin_port);
        //ZCINFO("accept %s:%d", sockaddr->ip, sockaddr->port);
    
        break;
    }
    
    if (getsockname(newsock->fd, newsock->local.addr, &(newsock->local.sa_len)) < 0) {
        ZCWARN("getsockname error! %s\n", strerror(errno));
    }else{
        zc_sockaddr_unpack(&newsock->local);

        //ZCINFO("family: %d, %d\n", newsock->family, AF_INET);
        //sockaddr = &newsock->local;
        //struct sockaddr_in *la = (struct sockaddr_in*)sockaddr->addr;
        //inet_ntop(newsock->family, &(la->sin_addr), sockaddr->ip, sockaddr->sa_len);
        //sockaddr->port = ntohs((short)la->sin_port);
    }
    
    return newsock;
}

int
zc_socket_connect(zcSocket *s, char *host, int port)
{
    int     ret;
    int     isblock = zc_socket_isblock(s);
#ifdef AF_UNIX 
    if (s->family == AF_UNIX || s->family == AF_LOCAL) {
        struct sockaddr_un  *sin;
        sin = (struct sockaddr_un*)(s->local.addr);
        do {
            ret = connect(s->fd, (struct sockaddr*)sin, sizeof(*sin));
        } while (ret == -1 && errno == EINTR);
    }else{
#endif
        strncpy(s->remote.ip, host, 15);
        s->remote.port = port;

        struct sockaddr_in  sin;
        sin.sin_family = AF_INET;
        sin.sin_port = htons((short)(port));

        if (host == NULL) {
            sin.sin_addr.s_addr = htonl(INADDR_ANY);
        }else{
            sin.sin_addr.s_addr = inet_addr(host);
        }

        if (s->timeout > 0 && isblock) {
            zc_socket_setblock(s, ZC_FALSE);
        }
        do {
            ret = connect(s->fd, (struct sockaddr*)&sin, sizeof(sin));
        } while (ret == -1 && errno == EINTR);
#ifdef AF_UNIX
    }
#endif
    int errnox = errno;
    if (ret == -1 && (errnox == EINPROGRESS ||errnox == EALREADY)) {
        if (isblock && s->timeout > 0) {
            // maybe: connect error lead to read ok and write ok
            ret = zc_socket_select(s, ZC_SOCKET_WRITE);
            if (ret != ZC_OK) {
                zc_socket_setblock(s, ZC_TRUE);
                return ZC_ERRNO;
            }
        }else{
            return ZC_ERRNO;
        }
    }
    if (isblock)
        zc_socket_setblock(s, isblock);

    if (-1 == ret && errnox != EISCONN) {
        return ZC_ERRNO;
    }
    
    if (getsockname(s->fd, s->local.addr, &(s->local.sa_len)) < 0) {
        ZCWARN("getsockname error! %s\n", strerror(errno));
        return ZC_ERRNO;
    }
    if (getpeername(s->fd, s->remote.addr, &(s->remote.sa_len)) < 0) {
        ZCWARN("getpeername error! %s\n", strerror(errno));
        return ZC_ERRNO;
    }
    
    return ZC_OK;
}

int
zc_socket_reconnect(zcSocket *s)
{
    int fd = socket(s->family, s->type, 0);
    if (fd < 0) {
        ZCERROR("socket create error! %s\n", strerror(errno));
        return ZC_ERRNO;
    }
    close(s->fd);
    s->fd = fd;
    //ZCINFO("reconnect %s:%d\n", s->remote_addr.ip, s->remote_addr.port);
    return zc_socket_connect(s, s->remote.ip, s->remote.port);
} 


int	
zc_socket_peek(zcSocket *s)
{
    char buf[4];
#ifdef __linux
    int ret = recv(s->fd, buf, 1, MSG_PEEK|MSG_DONTWAIT);
#else
    int isblock = zc_socket_isblock(s);

    if (isblock)
        zc_socket_setblock(s, ZC_FALSE);
    int ret = recv(s->fd, buf, 1, MSG_PEEK);
    if (isblock)
        zc_socket_setblock(s, ZC_TRUE);
#endif
    if (ret == -1) {
        /*int err = errno;
        if (err == ECONNRESET || err == ENOTCONN) {
            return ZC_FALSE;
        }
        return ZC_TRUE;*/
        //return ZC_FALSE;
        return ZC_ERRNO;
    }
    // return ret>0;
    return ret;
}


#ifdef ZOCLE_WITH_SSL
int
zc_socket_ssl_send(zcSocket *s, char *buf, int len)
{
    int ret;

    if (s->timeout > 0) {
        ret = zc_socket_select(s, ZC_SOCKET_WRITE);
        if (ZC_OK != ret) {
            return ZC_ERRNO;        
        }
        int err, sockstate;
        do {
            err = 0;
            ZCINFO("ssl send: %s\n", buf);
            ret = SSL_write(s->sslobj->ssl, buf, len);
            err = SSL_get_error(s->sslobj->ssl, ret);
            // Fixme: sockstate check error 
            if (err == SSL_ERROR_WANT_READ) {
                sockstate = zc_socket_select(s, ZC_SOCKET_READ);
            } else if (err == SSL_ERROR_WANT_WRITE) {
                sockstate = zc_socket_select(s, ZC_SOCKET_WRITE);
            } else {
                sockstate = ZC_SSL_SOCKET_OPERATION_OK;
            }
            if (sockstate == ZC_SSL_SOCKET_HAS_TIMED_OUT) {
                ZCWARN("The write operation timed out");
                return -1;
            } else if (sockstate == ZC_SSL_SOCKET_HAS_BEEN_CLOSED) {
                ZCWARN("Underlying socket has been closed.");
                return -2;
            } else if (sockstate == ZC_SSL_SOCKET_IS_NONBLOCKING) {
                break;
            }
        } while (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE);
    }else{
        do {
            ret = SSL_write(s->sslobj->ssl, buf, len);
        } while (-1 == ret && errno == EINTR);

    }

    if (-1 == ret)
        return ZC_ERRNO;

    return ret;
}
#endif


int
zc_socket_send(zcSocket *s, char *buf, int len)
{
#ifdef ZOCLE_WITH_SSL
    if (s->sslobj) {
        return zc_socket_ssl_send(s, buf, len);
    }
#endif

    int ret;
    if (s->blocked && s->timeout > 0) {
        ret = zc_socket_select(s, ZC_SOCKET_WRITE);
        if (ZC_OK != ret) {
            return ZC_ERRNO;        
        }
        do {
            ret = write(s->fd, buf, len);

            if (-1 == ret) {
                if (errno == EINTR) {
                    continue;
                }else if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    ret = zc_socket_select(s, ZC_SOCKET_WRITE);
                    if (ret != ZC_OK)
                        return ZC_ERRNO;
                }else{
                    break;
                }
            }
        } while (-1 == ret);
    }else{
        do {
            ret = write(s->fd, buf, len);
        } while (-1 == ret && errno == EINTR);
    }

    if (-1 == ret)
        return ZC_ERRNO;

    return ret;
}

#ifdef ZOCLE_WITH_SSL
int 
zc_socket_ssl_recv(zcSocket *s, char *buf, int len)
{
    int ret;

    if (s->timeout > 0) {
        ret = zc_socket_select(s, ZC_SOCKET_READ);
        if (ZC_OK != ret) {
            return ZC_ERRNO;        
        }
        int err, sockstate;
        do{
            ret = SSL_read(s->sslobj->ssl, buf, len);
            err = SSL_get_error(s->sslobj->ssl, ret);
            // Fixme: sockstate check error
            ZCINFO("ssl read: %s\n", buf);
            if (err == SSL_ERROR_WANT_READ) {
                sockstate = zc_socket_select(s, ZC_SOCKET_READ);
            } else if (err == SSL_ERROR_WANT_WRITE) {
                sockstate = zc_socket_select(s, ZC_SOCKET_WRITE);
            } else {
                sockstate = ZC_SSL_SOCKET_OPERATION_OK;
            }
            if (sockstate == ZC_SSL_SOCKET_HAS_TIMED_OUT) {
                ZCWARN("The read operation timed out");
                return -1;
            } else if (sockstate == ZC_SSL_SOCKET_IS_NONBLOCKING) {
                break;
            }
        } while (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE);
    }else{
        do {
            ret = SSL_read(s->sslobj->ssl, buf, len);
        } while (-1 == ret && errno == EINTR);

    }

    if (-1 == ret)
        return ZC_ERRNO;

    return ret;
}
#endif

int 
zc_socket_recv(zcSocket *s, char *buf, int len)
{
#ifdef ZOCLE_WITH_SSL
    if (s->sslobj) {
        return zc_socket_ssl_recv(s, buf, len);
    }
#endif

    int ret;
    if (s->blocked && s->timeout > 0) {
        ret = zc_socket_select(s, ZC_SOCKET_READ);
        if (ZC_OK != ret) {
            return ret;
        }
        while (1){
            ret = read(s->fd, buf, len);
            if (-1 == ret) {
                if (errno == EINTR) {
                    continue;
                }else if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    ret = zc_socket_select(s, ZC_SOCKET_READ);
                    if (ret != ZC_OK) {
                        return ret;
                    }
                    continue;
                }
            }
            break;
        }
    }else{
        do {
            ret = read(s->fd, buf, len);
        } while (-1 == ret && errno == EINTR);
    }

    if (-1 == ret)
        return ZC_ERRNO;

    return ret;
}


int
zc_socket_sendn(zcSocket *s, char *buf, int len)
{
    int     sendn,  n;
    char    *sendbuf = buf;
    
    sendn = len;
    
    while (sendn > 0) {
        n = zc_socket_send(s, sendbuf, sendn);
        if (n < 0) {
            return n;
        }
        sendn -= n; 
        sendbuf += n;
    }

    return len;
}

int 
zc_socket_recvn(zcSocket *s, char *buf, int len)
{
    int     recvn,  n;
    char    *recvbuf = buf;
    
    recvn = len;
    
    while (recvn > 0) {
        n = zc_socket_recv(s, recvbuf, recvn);
        if (n < 0) {
            return n;
        }else if (n == 0) {
            break;
        }
        recvn -= n; 
        recvbuf += n;
    }
    *recvbuf = 0;

    return len-recvn;
}

void
zc_socket_add_remote_addr(zcSocket *s, const char *host, int port)
{
    struct sockaddr_in  *sin = (struct sockaddr_in*)(s->remote.addr);

    sin->sin_family = AF_INET;
    sin->sin_port = htons((short)(port));
    s->remote.port = port;

    if (host == NULL) {
        sin->sin_addr.s_addr = htonl(INADDR_ANY);
    }else{
        strncpy(s->remote.ip, host, 15);
        sin->sin_addr.s_addr = inet_addr(host);
    }
    
    s->remote.sa_len = sizeof(struct sockaddr_in);
}


int 
zc_socket_sendto(zcSocket *s, zcSockAddr *sockaddr, char *buf, int len, int flags)
{
    int ret;
    
    do{
        ret = sendto(s->fd, buf, len, flags, sockaddr->addr, sockaddr->sa_len);
        if (-1 == ret) {
            ZCINFO("sendto error: %s\n", strerror(errno));
            if (errno == EINTR) {
                continue;
            }else if (errno == EAGAIN || errno == EWOULDBLOCK) {
                if (s->timeout <= 0)                
                    break;
                ret = zc_socket_select(s, ZC_SOCKET_WRITE);
                if (ZC_OK != ret) {
                    return ZC_ERRNO;
                }
            }else {
                break;
            }
        }
    }while(ret == -1);

    if (-1 == ret) {
        return ZC_ERRNO;
    }

    return ret;
}

int 
zc_socket_recvfrom(zcSocket *s, zcSockAddr *sockaddr, char *buf, int len, int flags)
{
    int     ret;

    do{
        sockaddr->sa_len = sizeof(struct sockaddr_in);
        ret = recvfrom(s->fd, buf, len, flags, sockaddr->addr, &(sockaddr->sa_len));
        if (-1 == ret) {
            if (errno == EINTR) {
                continue;
            }else if (errno == EAGAIN || errno == EWOULDBLOCK) {
                if (s->timeout <= 0)                
                    break;
                ret = zc_socket_select(s, ZC_SOCKET_READ);
                if (0 != ret) {
                    return ZC_ERRNO;
                }
            }else {
                break;
            }
        }
    }while(ret == -1);
    if (-1 == ret) {
        return ZC_ERRNO;
    }
    return ret;
}

int
zc_socket_sendto_self(zcSocket *s, char *buf, int len, int flags)
{
    return zc_socket_sendto(s, &(s->remote), buf, len, flags);
}

int
zc_socket_recvfrom_self(zcSocket *s, char *buf, int len, int flags)
{
    return zc_socket_recvfrom(s, &(s->remote), buf, len, flags);
}
#ifdef ZOCLE_WITH_SSL

static int
zc_socket_do_handshake(zcSocket *s)
{
    int ret;
    int err;
    int sockstate;

    /* just in case the blocking state of the socket has been changed */
    //nonblocking = s->blocked * -1; //(self->Socket->sock_timeout >= 0.0);
    if (s->blocked) {
        BIO_set_nbio(SSL_get_rbio(s->sslobj->ssl), 0);
        BIO_set_nbio(SSL_get_wbio(s->sslobj->ssl), 0);
    }else{
        BIO_set_nbio(SSL_get_rbio(s->sslobj->ssl), 1);
        BIO_set_nbio(SSL_get_wbio(s->sslobj->ssl), 1);
    }

    char *errstr = "";
    /* Actually negotiate SSL connection */
    /* XXX If SSL_do_handshake() returns 0, it's also a failure. */
    do {
        ret = SSL_do_handshake(s->sslobj->ssl);
        err = SSL_get_error(s->sslobj->ssl, ret);
        
        /*if(PyErr_CheckSignals()) {
            return NULL;
        }*/
        if (err == SSL_ERROR_WANT_READ) {
            sockstate = zc_socket_select(s, ZC_SOCKET_READ);
        } else if (err == SSL_ERROR_WANT_WRITE) {
            sockstate = zc_socket_select(s, ZC_SOCKET_WRITE);
        } else {
            sockstate = ZC_SSL_SOCKET_OPERATION_OK;
        }
        if (sockstate == ZC_SSL_SOCKET_HAS_TIMED_OUT) {
            errstr = "The handshake operation timed out";
            goto handshake_error;
        } else if (sockstate == ZC_SSL_SOCKET_HAS_BEEN_CLOSED) {
            errstr = "Underlying socket has been closed.";
            goto handshake_error;
        } else if (sockstate == ZC_SSL_SOCKET_TOO_LARGE_FOR_SELECT) {
            errstr = "Underlying socket too large for select().";
            goto handshake_error;
        } else if (sockstate == ZC_SSL_SOCKET_IS_NONBLOCKING) {
            break;
        }
    } while (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE);
    if (ret < 1) {
        //return PySSL_SetError(self, ret, __FILE__, __LINE__);
        ZCWARN("ssl handshake error: %d\n", ret);
        goto handshake_error;
    }

    if (s->sslobj->peer_cert)
        X509_free (s->sslobj->peer_cert);
    if ((s->sslobj->peer_cert = SSL_get_peer_certificate(s->sslobj->ssl))) {
        X509_NAME_oneline(X509_get_subject_name(s->sslobj->peer_cert),
                          s->sslobj->server, X509_NAME_MAXLEN);
        X509_NAME_oneline(X509_get_issuer_name(s->sslobj->peer_cert),
                          s->sslobj->issuer, X509_NAME_MAXLEN);
    }
    return ZC_OK;
handshake_error:
    ZCWARN("ssl error: %s\n", errstr); 
    return ZC_ERR;
}

static int 
zc_socket_ssl(zcSocket *s, char *key_file, char *cert_file, 
        zcSSLCertRequire certreq, zcSSLVer ver, char *cacerts_file, int type)
{
    char *errstr = NULL;
    const SSL_METHOD *meth;
    int ret;
    
    if (s->sslobj) {
        memset(s->sslobj, 0, sizeof(zcSSL));
    }else{
        s->sslobj = zc_calloct(zcSSL);
    }

    if ((key_file && !cert_file) || (!key_file && cert_file)) {
        errstr = "Both the key & certificate files must be specified";
        goto zc_socket_ssl_fail;
    }
   
    (void) ERR_get_state();
    ERR_clear_error();
    
    SSL_load_error_strings();
    SSLeay_add_ssl_algorithms();

    if (type == ZC_SSL_CLIENT) {
        if (ver == ZC_SSL_VER_TLS1) {
            meth = TLSv1_client_method();
        }else if (ver == ZC_SSL_VER_SSL3) {
            meth = SSLv3_client_method();
        }else if (ver == ZC_SSL_VER_SSL23) {
            meth = SSLv23_client_method();
        }
    }else{
        if (ver == ZC_SSL_VER_TLS1) {
            meth = TLSv1_server_method();
        }else if (ver == ZC_SSL_VER_SSL3) {
            meth = SSLv3_server_method();
        }else if (ver == ZC_SSL_VER_SSL23) {
            meth = SSLv23_server_method();
        }
    }

    s->sslobj->ctx = SSL_CTX_new(meth);
    if (s->sslobj->ctx == NULL) {
        errstr = "SSL_CTX_new error";
        goto zc_socket_ssl_fail;
    }

    if (key_file) {
        ret = SSL_CTX_use_PrivateKey_file(s->sslobj->ctx, key_file, SSL_FILETYPE_PEM);
        if (ret <= 0) {
            errstr = "SSL_CTX_use_PrivateKey_file error";
            goto zc_socket_ssl_fail;
        }

        /*ret = SSL_CTX_use_certificate_chain_file(s->ctx, cert_file);
        SSL_CTX_set_options(s->ctx, SSL_OP_ALL); // ssl compatibility
        if (ret <= 0) {
            errstr = "SSL_CTX_use_certificate_chain_file error";
            goto zc_socket_ssl_fail;
        }*/

        ret = SSL_CTX_use_certificate_file(s->sslobj->ctx, cert_file, SSL_FILETYPE_PEM);
        if (ret <= 0) {
            char errbuf[128];
            ERR_error_string_n(ERR_get_error(), errbuf, sizeof(errbuf));
            ZCWARN("cert error:%s", errbuf);  
            goto zc_socket_ssl_fail;
        }

        if (!SSL_CTX_check_private_key(s->sslobj->ctx)) {
            errstr = "Private key does not match the certificate public key\n";
            goto zc_socket_ssl_fail;
        }
    }
    /* ssl compatibility */
    SSL_CTX_set_options(s->sslobj->ctx, SSL_OP_ALL & ~SSL_OP_DONT_INSERT_EMPTY_FRAGMENTS);


    zcSSLCertRequire verification_mode = SSL_VERIFY_NONE;
    if (certreq == ZC_SSL_CERT_OPTIONAL)
        verification_mode = SSL_VERIFY_PEER;
    else if (certreq == ZC_SSL_CERT_REQUIRED)
        verification_mode = (SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT);
    SSL_CTX_set_verify(s->sslobj->ctx, verification_mode, NULL); /* set verify lvl */

    s->sslobj->ssl = SSL_new(s->sslobj->ctx); /* New ssl struct */
    SSL_set_fd(s->sslobj->ssl, s->fd);   /* Set the socket for SSL */

    /* If the socket is in non-blocking mode or timeout mode, set the BIO
     * to non-blocking mode (blocking is the default)
     */
    if (!s->blocked) {
        /* Set both the read and write BIO's to non-blocking mode */
        BIO_set_nbio(SSL_get_rbio(s->sslobj->ssl), 1);
        BIO_set_nbio(SSL_get_wbio(s->sslobj->ssl), 1);
    }

    if (type == ZC_SSL_CLIENT) {
        SSL_set_connect_state(s->sslobj->ssl);
    }else{
        SSL_set_accept_state(s->sslobj->ssl);
    }

    ret = zc_socket_do_handshake(s);
    if (ret != ZC_OK) {
        return ZC_ERR;
    }
    return ZC_OK;

zc_socket_ssl_fail:
    ZCWARN("ssl error:%s\n", errstr);    
    if (s->sslobj->ctx) {
        SSL_CTX_free(s->sslobj->ctx);
    }
    if (s->sslobj->ssl) {
        SSL_free(s->sslobj->ssl);
    }
    if (s->sslobj) {
        zc_free(s->sslobj);
    }
    s->sslobj = NULL;
    return ZC_ERR;
}

int 
zc_socket_client_ssl(zcSocket *s, char *key_file, char *cert_file, 
        zcSSLCertRequire certreq, zcSSLVer ver, char *cacerts_file)
{
    return zc_socket_ssl(s, key_file, cert_file, certreq, ver, cacerts_file, ZC_SSL_CLIENT);
}

int 
zc_socket_server_ssl(zcSocket *s, char *key_file, char *cert_file, 
        zcSSLCertRequire certreq, zcSSLVer ver, char *cacerts_file)
{
    return zc_socket_ssl(s, key_file, cert_file, certreq, ver, cacerts_file, ZC_SSL_SERVER);

}

#endif


int
zc_socket_interrupt(int err)
{
#ifdef EPROTO
    return err == EINTR || err == EPROTO;
#else
    return err == EINTR;
#endif

}

int
zc_socket_accept_interrupt(int err)
{
    if(zc_socket_interrupt(err)) {
        return 1;
    }

    return err == ECONNABORTED ||
           err == ECONNRESET ||
           err == ETIMEDOUT;
}

inline int
zc_socket_no_buffers(int err)
{
    return err == ENOBUFS;
}

inline int
zc_socket_would_block(int err)
{
    return err == EAGAIN || err == EWOULDBLOCK;
}

inline int
zc_socket_conn_failed(int err)
{
    return err == ECONNREFUSED ||
           err == ETIMEDOUT ||
           err == ENETUNREACH ||
           err == EHOSTUNREACH ||
           err == ECONNRESET ||
           err == ESHUTDOWN ||
           err == ECONNABORTED;

}

inline int
zc_socket_conn_refused(int err)
{
    return err == ECONNREFUSED;
}

inline int
zc_socket_conn_progress(int err)
{
    return err == EINPROGRESS;
}

inline int
zc_socket_conn_lost(int err)
{
    return err == ECONNRESET ||
           err == ENOTCONN ||
           err == ESHUTDOWN ||
           err == ECONNABORTED ||
           err == ESPIPE ||
           err == EPIPE;
}

inline int
zc_socket_not_connected(int err)
{
    return err == ENOTCONN;
}

int
zc_socket_isconnected(zcSocket *s)
{
    int error, ret;
    socklen_t   len;

    if (s->fd <= 0)
        return ZC_FALSE;
    len = sizeof(error);

    ret = getsockopt(s->fd, SOL_SOCKET,SO_ERROR, (char*)&error, &len);
    if (0 != ret || 0 != error)
        return ZC_FALSE;
    return ZC_TRUE;
}

int
zc_socket_isblock(zcSocket *s)
{
    int flag;

    flag = fcntl(s->fd, F_GETFL, 0);
    if (flag & O_NONBLOCK)
        return ZC_FALSE;
    return ZC_TRUE; 
}

int 
zc_socket_linger(zcSocket *s, int onoff, int linger)
{
    struct linger ling = {onoff, linger}; 
    int ret = setsockopt(s->fd, SOL_SOCKET, SO_LINGER, (void *)&ling, sizeof(ling));
    if (ret != 0) {
        char errbuf[128];
        strerror_r(errno, errbuf, 128);
        ZCERROR("setsockopt LINGER error: %s\n",  errbuf);
        return ZC_ERR; 
    }
    return ZC_OK;
}
int
zc_socket_tcp_nodelay(zcSocket *s)
{
    int flag = 1;
    if(setsockopt(s->fd, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(int)) == SOCKET_ERROR) {
        ZCERROR("set_tcp_nodelay error! %s\n", strerror(errno));
        //close_socket(s->fd);
        return ZC_ERRNO;
    }
    return 0;

}

int
zc_socket_keepalive(zcSocket *s)
{
    int flag = 1;
    if(setsockopt(s->fd, SOL_SOCKET, SO_KEEPALIVE, (char*)&flag, sizeof(int)) == SOCKET_ERROR) {
        ZCERROR("set_keepalive error! %s\n", strerror(errno));
        //close_socket(s->fd);
        return ZC_ERRNO;
    }
    return 0;

}

int
zc_socket_get_send_buffer_size(zcSocket *s)
{
    int sz;
    socklen_t len = sizeof(sz);
    if(getsockopt(s->fd, SOL_SOCKET, SO_SNDBUF, (char*)&sz, &len) == SOCKET_ERROR || len != sizeof(sz)) {
        ZCERROR("get_send_buffer_size error! %s\n", strerror(errno));
        //close_socket(s->fd);
        return ZC_ERRNO;
    }
    return sz;

}

int
zc_socket_set_send_buffer_size(zcSocket *s, int sz)
{
    if(setsockopt(s->fd, SOL_SOCKET, SO_SNDBUF, (char*)&sz, sizeof(int)) == SOCKET_ERROR) {
        //close_socket(s->fd);
        return ZC_ERRNO;
    }
    return 0;

}

int
zc_socket_get_recv_buffer_size(zcSocket *s)
{
    int sz;
    socklen_t len = sizeof(sz);
    if(getsockopt(s->fd, SOL_SOCKET, SO_RCVBUF, (char*)&sz, &len) == SOCKET_ERROR || len != sizeof(sz)) {
        ZCERROR("get_recv_buffer_size error! %s\n", strerror(errno));
        //close_socket(s->fd);
        return ZC_ERRNO;
    }
    return sz;

}


int
zc_socket_set_recv_buffer_size(zcSocket *s, int sz)
{
    if(setsockopt(s->fd, SOL_SOCKET, SO_RCVBUF, (char*)&sz, (int)(sizeof(int))) == SOCKET_ERROR) {
        ZCERROR("set_recv_buffer_size error! %s\n", strerror(errno));
        //close_socket(s->fd);
        return ZC_ERRNO;
    }
    return 0;

}

int
zc_socket_error_string(int err, char *buf, int buflen)
{
    strncpy(buf, (char*)strerror(err), buflen);
    return 0;
}

int
zc_socket_last_error_string(char *buf, int buflen)
{
    strncpy(buf, (char*)strerror(errno), buflen);
    return 0;
}

zcList*
zc_socket_local_addr(zcSocket *s)
{
    zcList   *addrs;
    struct ifconf ifc;
    int sockfd;
    char buf[20000];
    zcSockAddr  *sockaddr;
    char *ptr;
    
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    addrs = zc_list_new();
    //zc_list_set_del(addrs, zc_sockaddr_delete);
    addrs->del = zc_sockaddr_delete;

    ifc.ifc_buf = buf;
    ifc.ifc_len = sizeof(buf);
    if (ioctl(sockfd, SIOCGIFCONF, &ifc) == -1) {
        zc_list_delete(addrs);
        close(sockfd);
        return NULL;
    }

    ptr = buf;
    while (ptr < buf + ifc.ifc_len) {
        struct ifreq *ifr;
        struct sockaddr_in *sin;
        int len;

        ifr = (struct ifreq *)ptr;
#if defined(__FreeBSD__) || defined(__APPLE__)
        len = sizeof(ifr->ifr_name) + ifr->ifr_addr.sa_len;

        if (ifr->ifr_addr.sa_family == AF_INET) {
            //printf("%s", ifr->ifr_name);
            sin = (struct sockaddr_in *)&ifr->ifr_addr;
            sockaddr = zc_sockaddr_new(ifr->ifr_addr.sa_family);
            memcpy(sockaddr->addr, sin, sizeof(struct sockaddr_in));
            strncpy(sockaddr->ip, inet_ntoa(sin->sin_addr), 15);
            /*printf("\t%s", inet_ntoa(sin->sin_addr));
            if (ioctl(s->fd, SIOCGIFFLAGS, ifr) == 0) {
                if (ifr->ifr_flags & IFF_UP) {
                    //printf("\tUP\n");
                } else {
                    //printf("\tDOWN\n");
                }
            }*/
            zc_list_append(addrs, sockaddr);
        }

        // 这段代码来自 qmail 的 ipme.c, 在 FreeBSD 上实测中没有出现过 len 小的情况
        if (len < sizeof(struct ifreq))
#else
            if (ioctl(sockfd, SIOCGIFFLAGS, ifr) == 0) {
                if (ifr->ifr_flags & IFF_UP) {
                    if (ioctl(sockfd, SIOCGIFADDR, ifr) == 0) {
                        if (ifr->ifr_addr.sa_family == AF_INET) {
                            sin = (struct sockaddr_in *)&ifr->ifr_addr;
                            sockaddr = zc_sockaddr_new(ifr->ifr_addr.sa_family);

                            memcpy(sockaddr->addr, sin, sizeof(struct sockaddr_in));
                            strncpy(sockaddr->ip, inet_ntoa(sin->sin_addr), 15);
                            //printf("%s\t%s\n", ifr->ifr_name, inet_ntoa(sin->sin_addr));
                            zc_list_append(addrs, sockaddr);
                        }
                    }
                }
            }
#endif
        len = sizeof(struct ifreq);
        ptr += len;
    }
    if (addrs->size == 0) {
        zc_list_delete(addrs);
        addrs = NULL;
    }
    close(sockfd);
    return addrs;
}

int	
zc_socket_gethostbyname(const char *name, zcList *ip)
{
    struct hostent *h = gethostbyname(name);
    char buf[16];
    char **p; 
    for (p=h->h_addr_list; *p!=NULL; p++) {
        inet_ntop(h->h_addrtype, *p, buf, sizeof(buf));
        //ZCINFO("domain:%s ip:%s", name, buf);
        //zc_cstrlist_append(ip, buf, 0);
        zc_list_append(ip, zc_strdup(buf, 0));
    }
    return ip->size;
}




#ifdef TEST_MAIN

int docmd(zcSocket *s, char *buf, int len)
{
    int ret, rlen = 0;
    char    *save = buf;

    ret = zc_socket_send(s, buf, strlen(buf));
    if (ret < 0) {
        perror("send error!!!");
        return -1;
    }
    //printf(">> %s\n", buf);
    memset(buf, 0, 1024);
    rlen = len;
    //while (1) {
        ret = zc_socket_recv(s, buf, rlen);
     /*   if (ret <= 0) {

            break;
        }
        rlen -= ret;
        buf += ret; 
        if (strchr(save, "\n") != NULL) {
            break;
        }
    }*/
    //printf("<< %s\n", buf);
    if (ret < 0) {
        perror("recv error!!!");
        return -2;
    }
    return 0;
}


int main()
{
    zcSocket   *s;
    char        *host = "172.16.102.52";
    int         port = 995;
    char        buf[10240];
    zcSockAddr  addr;
    int         ret, slen;

#ifndef ZOCLE_WITH_SSL
    s = zc_socket_server(host, port, 5, 1);
    if (NULL == s) {
        ////printf("socket create error!\n");
        return -1;
    }

    //printf("server listen %s:%d\n", host, port);
    while (1) {
        memset(buf, 1024, 0);
        if (s->type == SOCK_STREAM) {
            zcSocket *s1 = zc_socket_accept(s);
            if (s1) {
                //printf("accept: %d\n", s1->fd);
            }else{
                //printf("accept error!\n");
                continue;
            }

            zc_socket_recvn(s1, buf, 50);
            //printf("recv: %s\n", buf);
            sprintf(buf, "ok, %d\r\n", getpid());
            zc_socket_sendn(s1, buf, strlen(buf));
            zc_socket_delete(s1);
        }else{

            zc_socket_recvfrom_self(s, buf, 1023, 0);
            //printf("recv: %s\n", buf);
            sprintf(buf, "ok, %d\r\n", getpid());
            //printf("try reply %s, %d\n", buf, strlen(buf));
            ret = zc_socket_sendto_self(s, buf, strlen(buf), 0);
            if (ret < 0) {
                //printf("send error!!!!\n");
            }else{
                ZCINFO("ok!!!!\n");
            }
        }
    }
#else
    s = zc_socket_new(AF_INET, SOCK_STREAM, 0, -1);
    if (NULL == s) {
        //printf("socket new error!\n");
        return -2;
    }
    //printf("try connect to %s:%d\n", host, port);
    ret = zc_socket_connect(s, host, port);
    if (ret < 0) {
        ZCINFO("connect error!\n");
    }

    //printf("connect ok!\n");
    zc_socket_ssl(s, NULL, NULL);
    
    //printf("ssl ok!\n");

    memset(buf, 0, 1024);
    strcpy(buf, "user zhaowei@zhaowei.com\r\n");
    docmd(s, buf, 1023);
    
    memset(buf, 0, 1024);
    strcpy(buf, "pass aaaaa\r\n");
    docmd(s, buf, 1023);

    memset(buf, 0, 1024);
    strcpy(buf, "list\r\n");
    docmd(s, buf, 1023);

    memset(buf, 0, 1024);
    strcpy(buf, "quit\r\n");
    docmd(s, buf, 1023);

    zc_socket_close(s);


#endif

    return 0;
}

#endif // TEST_MAIN
#endif // _WIN32
