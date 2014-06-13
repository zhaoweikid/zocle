#ifndef ZOCLE_STR_CONFPARSE_H
#define ZOCLE_STR_CONFPARSE_H

#include <stdio.h>
#include <stdint.h>
#include <limits.h>

#define ZC_CONF_INT			1
#define ZC_CONF_FLOAT		2
#define ZC_CONF_STRING		3	
#define ZC_CONF_BOOL		4	
#define ZC_CONF_ENUM		5	
#define ZC_CONF_USER		6


typedef struct zc_confpair_t
{
	char	key[128];
	int		value;
}zcConfPair;

typedef struct zc_confpairs_t
{
	int count;
	int len; // used 
	zcConfPair	pair[0];
}zcConfPairs;

zcConfPairs* zc_confpairs_new(int count);
void		 zc_confpairs_delete(void *);
int			 zc_confpairs_add(zcConfPairs *, char *key, int val);
int			 zc_confpairs_find(zcConfPairs *, char *key, int *val);

typedef int (*zcConfFunc)(void *dst, char *key, char *value, int i);

typedef struct zc_confparam_t
{
	void	 *dst;
	int		 dsti; // index for dst
	char	 name[128];
	char	 type;
	uint8_t	 arraysize;
	union{
		zcConfPairs	*pairs; // enum have pair
		zcConfFunc	userfunc;
	}param;
	struct zc_confparam_t *next;
}zcConfParam;

typedef struct zc_confparser_t
{
	zcConfParam	*params;	
	char		filename[PATH_MAX];
}zcConfParser;

zcConfParser* zc_confparser_new(char *filename);
void		  zc_confparser_delete(void*);
int			  zc_confparser_add_param(zcConfParser*, 
                    void *addr, char *name, 
					char type, char arraysize, void *arg);
int			  zc_confparser_add(zcConfParser*, 
                    void *addr, char *name, char type);
int			  zc_confparser_add_array(zcConfParser*, 
                    void *addr, char *name, char type,
					char arraysize);
int			  zc_confparser_parse(zcConfParser*);

#endif
