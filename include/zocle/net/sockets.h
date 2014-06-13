#ifndef ZOCLE_NET_SOCKETS_H
#define ZOCLE_NET_SOCKETS_H

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <zocle/ds/list.h>
#include <zocle/str/cstrlist.h>

#ifdef __hpux
    #define _XOPEN_SOURCE_EXTENDED
#endif

#ifdef _WIN32
    #include <winsock2.h>

    #define ERRORNUM  WSAGetLastError()*(-1)
    //typedef int ssize_t;
    typedef int socklen_t;
#else // no win32
    #include <unistd.h>
    #include <fcntl.h>
    #include <sys/socket.h>

    #if defined(__hpux)
        #include <sys/time.h>
    #else
        #include <sys/select.h>
    #endif
    #include <ifaddrs.h>
    #include <sys/types.h>
    #include <sys/stat.h>
    #include <arpa/inet.h>
    #include <errno.h>
#ifdef __linux
    #include <linux/if.h>
#endif
    #include <sys/un.h>
#if defined(__APPLE__) || defined(__FreeBSD__)
    #include <net/if.h>
#endif

    #include <netinet/in.h>
    #include <netinet/tcp.h>
    #include <netdb.h>

    #define ERRORNUM  (-1)*errno
    #define SOCKET int
    #define SOCKET_ERROR -1
    #define INVALID_SOCKET -1
#endif


#ifdef ZOCLE_WITH_SSL
#include <openssl/rsa.h>
#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/rand.h>

typedef enum  {
    ZC_SSL_SOCKET_IS_NONBLOCKING,
    ZC_SSL_SOCKET_IS_BLOCKING,
    ZC_SSL_SOCKET_HAS_TIMED_OUT,
    ZC_SSL_SOCKET_HAS_BEEN_CLOSED,
    ZC_SSL_SOCKET_TOO_LARGE_FOR_SELECT,
    ZC_SSL_SOCKET_OPERATION_OK
}zcSSLTimeoutState;

typedef enum  {
    ZC_SSL_VER_TLS1,
    ZC_SSL_VER_SSL3,
    ZC_SSL_VER_SSL2,
    ZC_SSL_VER_SSL23
}zcSSLVer;

typedef enum  {
    ZC_SSL_CERT_NONE,
    ZC_SSL_CERT_OPTIONAL,
    ZC_SSL_CERT_REQUIRED
}zcSSLCertRequire;

#endif

#define X509_NAME_MAXLEN 256

#define ZC_SOCKET_WRITE 1
#define ZC_SOCKET_READ  0


typedef struct zc_sockaddr_t
{
    char        ip[16];
    uint16_t    port;
    socklen_t   sa_len;    
	struct sockaddr *addr;
}zcSockAddr;

zcSockAddr*	zc_sockaddr_new(int);
int			zc_sockaddr_init(zcSockAddr*, int family);
int			zc_sockaddr_init_unix(zcSockAddr*, const char *);
int			zc_sockaddr_init_net(zcSockAddr*, const char *ip, uint16_t port);
int			zc_sockaddr_unpack(zcSockAddr*);
void        zc_sockaddr_delete(void* sa);
void        zc_sockaddr_delete4_(void* sa, void*, const char*, int);
#define     zc_sockaddr_delete4(x) do{zc_sockaddr_delete4_(x,NULL,__FILE__,__LINE__);x=NULL;}while(0)

/*typedef struct
{
	SOCKET	fd;
	int		timeout:30;
	int		blocked:2;
}zcRawSocket;*/

#ifdef ZOCLE_WITH_SSL
typedef struct zc_ssl_t
{
    SSL_CTX*    ctx;
    SSL*        ssl;
    X509*       peer_cert;
    char        server[X509_NAME_MAXLEN];
    char        issuer[X509_NAME_MAXLEN];
    char        sslver;
}zcSSL;
#endif

typedef struct zc_socket_t
{
    SOCKET      fd;
    int         timeout:22;  // ms
    char        blocked:2;
    uint8_t     family:8;
    uint8_t     type;
    zcSockAddr  local;
    zcSockAddr  remote;
#ifdef ZOCLE_WITH_SSL
	zcSSL		*sslobj;
#endif
}zcSocket;

int         zc_socket_select (zcSocket *s, int writing);
int         zc_socket_startup();
void        zc_socket_cleanup();
zcSocket*   zc_socket_new (int family, int type, int proto, int timeout);
zcSocket*   zc_socket_new_tcp (int timeout);
zcSocket*   zc_socket_new_udp (int timeout);
#ifndef _WIN32
zcSocket*   zc_socket_new_unix (char *path, int type, int timeout);
zcSocket*   zc_socket_new_unix_tcp (char *path, int timeout);
zcSocket*   zc_socket_new_unix_udp (char *path, int timeout);
#endif
zcSocket*   zc_socket_create ();
zcSocket*   zc_socket_init (SOCKET fd, int family, int type, int timeout);
void        zc_socket_close (zcSocket *);
void        zc_socket_delete (void *s);
void        zc_socket_delete4_ (void *s, void*, const char *, int);
int         zc_socket_shutdown (zcSocket *s, int how);
int         zc_socket_shutdown_read (zcSocket *s);
int         zc_socket_shutdown_write (zcSocket *s);
int         zc_socket_bind (zcSocket *s, char *host, int port);
int         zc_socket_listen (zcSocket *s, int backlog);
zcSocket*   zc_socket_server (char *host, int port, int type, int backlog);
zcSocket*   zc_socket_server_tcp (char *host, int port, int backlog);
zcSocket*   zc_socket_server_udp (char *host, int port, int backlog);
#ifndef _WIN32
zcSocket*   zc_socket_server_unix (char *path, int type, int timeout);
zcSocket*   zc_socket_server_unix_tcp (char *path, int timeout);
zcSocket*   zc_socket_server_unix_udp (char *path, int timeout);
#endif
zcSocket*   zc_socket_client_tcp (char *host, int port, int timeout);

int         zc_socket_pair(int type, zcSocket **sock1, zcSocket **sock2);
int         zc_socket_pair_tcp(zcSocket **sock1, zcSocket **sock2);
int         zc_socket_pair_udp(zcSocket **sock1, zcSocket **sock2);

zcSocket*   zc_socket_accept (zcSocket *s);
int         zc_socket_connect (zcSocket *s, char *host, int port);
int         zc_socket_reconnect (zcSocket *s);

int			zc_socket_peek(zcSocket *s);

int         zc_socket_send (zcSocket *s, char *buf, int len);
int         zc_socket_recv (zcSocket *s, char *buf, int len);
int         zc_socket_sendn (zcSocket *s, char *buf, int len);
int         zc_socket_recvn (zcSocket *s, char *buf, int len);

void        zc_socket_add_remote_addr(zcSocket *s, const char *host, int port);
int         zc_socket_sendto (zcSocket *s, zcSockAddr*, char *buf, int len, int flags);
int         zc_socket_recvfrom (zcSocket *s, zcSockAddr*, char *buf, int len, int flags);

int         zc_socket_sendto_self (zcSocket *s, char *buf, int len, int flags);
int         zc_socket_recvfrom_self (zcSocket *s, char *buf, int len, int flags);

#ifdef ZOCLE_WITH_SSL

int         zc_socket_client_ssl (zcSocket *s, char*, char*, zcSSLCertRequire certreq, 
                    zcSSLVer ver, char *cacerts_file);
int         zc_socket_server_ssl (zcSocket *s, char*, char*, zcSSLCertRequire certreq,
                    zcSSLVer ver, char *cacerts_file);

int         zc_socket_ssl_send(zcSocket *s, char *buf, int len);
int         zc_socket_ssl_recv(zcSocket *s, char *buf, int len);
#endif

int         zc_socket_interrupt(int err);
int         zc_socket_accept_interrupt(int err);
int         zc_socket_no_buffers(int err);
int         zc_socket_would_block(int err);
int         zc_socket_conn_failed(int err);
int         zc_socket_conn_refused(int err);
int         zc_socket_conn_progress(int err);
int         zc_socket_conn_lost(int err);
int         zc_socket_not_connected(int err);
int         zc_socket_isconnected(zcSocket*);
int         zc_socket_isblock(zcSocket*);

int         zc_socket_setblock (zcSocket *s, int block);
int         zc_socket_reuse(zcSocket *s);
int         zc_socket_linger(zcSocket *s, int onoff, int linger);
int         zc_socket_tcp_nodelay(zcSocket*);
int         zc_socket_keepalive(zcSocket*);

int         zc_socket_get_send_buffer_size(zcSocket*);
int         zc_socket_set_send_buffer_size(zcSocket*, int sz);
int         zc_socket_get_recv_buffer_size(zcSocket*);
int         zc_socket_set_recv_buffer_size(zcSocket*, int sz);
int         zc_socket_error_string(int err, char *buf, int buflen);
int         zc_socket_last_error_string(char *buf, int buflen);
zcList*     zc_socket_local_addr();
int			zc_socket_gethostbyname(const char *name, zcCStrList *);

#define     zc_socket_delete4(x) do{zc_socket_delete4_(x,NULL,__FILE__,__LINE__);x=NULL;}while(0)

#endif

