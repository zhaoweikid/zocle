#ifndef	ZOCLE_INTERNET_DNS_RESOLVER_H
#define	ZOCLE_INTERNET_DNS_RESOLVER_H

#include <stdio.h>
#include <stdint.h>
#include <zocle/ds/list.h>
#include <zocle/str/string.h>
#include <zocle/server/asynconn.h>

#define ZC_DNS_EBASE -(('d' << 24) | ('n' << 16) | ('s' << 8) | 64)

enum zcDNSError {
	ZC_DNS_ENOBUFS = ZC_DNS_EBASE,
    ZC_DNS_EILLEGAL,
    ZC_DNS_EORDER,
    ZC_DNS_ESECTION,
    ZC_DNS_EUNKNOWN,
    ZC_DNS_EADDRESS,
    ZC_DNS_ELAST,
};

enum zcDNSSection {
	ZC_DNS_S_QD = 0x01,
	ZC_DNS_S_AN	= 0x02,
	ZC_DNS_S_NS = 0x04,
	ZC_DNS_S_AR = 0x08,
	ZC_DNS_S_ALL = 0x0f
};

enum zcDNSClass {
	ZC_DNS_C_IN	= 1,
	ZC_DNS_C_ANY = 255
};

enum zcDNSType {
	ZC_DNS_T_A     = 1,
    ZC_DNS_T_NS    = 2,
    ZC_DNS_T_CNAME = 5,
    ZC_DNS_T_SOA   = 6,
    ZC_DNS_T_PTR   = 12,
    ZC_DNS_T_MX    = 15,
    ZC_DNS_T_TXT   = 16,
    ZC_DNS_T_AAAA  = 28,
    ZC_DNS_T_SRV   = 33,
    ZC_DNS_T_SSHFP = 44,
    ZC_DNS_T_SPF   = 99,
    ZC_DNS_T_ALL   = 255
};

enum zcDNSOpcode {
	ZC_DNS_OP_QUERY    = 0,
    ZC_DNS_OP_IQUERY   = 1,
    ZC_DNS_OP_STATUS   = 2,
    ZC_DNS_OP_NOTIFY   = 4,
    ZC_DNS_OP_UPDATE   = 5
};

enum zcDNSRcode {
	ZC_DNS_RC_NOERROR  = 0,
    ZC_DNS_RC_FORMERR  = 1,
    ZC_DNS_RC_SERVFAIL = 2,
    ZC_DNS_RC_NXDOMAIN = 3,
    ZC_DNS_RC_NOTIMP   = 4,
    ZC_DNS_RC_REFUSED  = 5,
    ZC_DNS_RC_YXDOMAIN = 6,
    ZC_DNS_RC_YXRRSET  = 7,
    ZC_DNS_RC_NXRRSET  = 8,
    ZC_DNS_RC_NOTAUTH  = 9,
    ZC_DNS_RC_NOTZONE  = 10,
};

typedef struct
{
	uint16_t id:16;
	uint16_t rd:1;
	uint16_t tc:1;
	uint16_t aa:1;
	uint16_t opcode:4;
	uint16_t qr:1;

	uint16_t rcode:4;
	uint16_t unused:3;
	uint16_t ra:1;

	uint16_t qdcount:16;
	uint16_t ancount:16;
	uint16_t nscount:16;
	uint16_t arcount:16;
}zcDNSHeader;

typedef struct 
{
	char ip[16];
}zcDNSA;

typedef struct
{
	uint16_t perf;
	char	 ip[16];
}zcDNSMX;

typedef struct
{
	zcString *ns;
	zcString *mailbox;
	uint32_t  serial;
	uint32_t  refresh;  // refresh interval
	uint32_t  retry;    // retry interval
	uint32_t  expire;
	uint32_t  minttl;
}zcDNSSOA;

typedef struct
{
	zcString *name;
}zcDNSNS;

typedef struct
{
	zcString *name;
}zcDNSCNAME;

typedef struct
{
	zcString *domain;
	uint16_t  type;
	uint16_t  cls;
	uint16_t  ttl;
	void     *data;
}zcDNSRR;

zcDNSRR* zc_dnsrr_new();
void	 zc_dnsrr_delete(void *);
void	 zc_dnsrr_print(zcDNSRR *);

void zc_dnsheader_print(zcDNSHeader *h);

int  zc_dns_pack_header(zcDNSHeader *h, char *store, int slen);
int  zc_dns_pack_query_body(const char *domain, uint16_t type, uint16_t cls, char *store, int slen);
int  zc_dns_pack_query(zcDNSHeader *h, const char *domain, uint16_t type, uint16_t cls, char *store, int slen);
int  zc_dns_pack_query_simple(const char *domain, uint16_t type, uint16_t cls, char *store, int slen);
int  zc_dns_pack_rr(const char *domain, uint16_t type, uint16_t cls, uint16_t ttl, const char *data);

int  zc_dns_unpack_header(zcDNSHeader *h, const char *data, int dlen);
int  zc_dns_unpack_resp_body(zcList *, int, const char *data, int used, int leftlen);
int	 zc_dns_unpack_resp(zcDNSHeader *h, zcList *, const char *data, int dlen);
int	 zc_dns_unpack_resp_simple(zcList *, const char *data, int dlen);

int  zc_dns_query(const char *dns, const char *domain, uint16_t type, uint16_t cls, zcList *result);
int  zc_dns_gethostbyname(const char *domain, zcList *result);

#ifdef ZOCLE_WITH_LIBEV
zcAsynConn* zc_asynconn_new_dns_client(const char *dns, int timeout, struct ev_loop *loop, const char *host);
#endif

#endif
