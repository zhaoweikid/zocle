#include <zocle/str/confparse.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <zocle/mem/alloc.h>
#include <zocle/base/defines.h>
#include <zocle/log/logfile.h>
#include <zocle/ds/list.h>
#include <zocle/str/string.h>

zcConfPairs*    
zc_confpairs_new(int count)
{
    int size = sizeof(zcConfPairs) + sizeof(zcConfPair)*count;
    zcConfPairs *pairs = zc_malloc(size);
    memset(pairs, 0, size);
    pairs->count = count;
    pairs->len = 0;

    return pairs;
}

int
zc_confpairs_add(zcConfPairs *pairs, char *key, int val)
{
    if (pairs->len >= pairs->count)
        return -1;
    zcConfPair *p = pairs->pair + pairs->len;
    strncpy(p->key, key, 127);
    p->value = val;
    pairs->len++;

    return 0;
}

int 
zc_confpairs_find(zcConfPairs *pairs, char *key, int *val)
{
    int i;
    zcConfPair *p = pairs->pair;
    for (i = 0; i < pairs->len; i++) {
        //ZCERROR("key:%s, pair->key:%s\n", key, p->key);
        if (strcmp(key, p->key) == 0) {
            *val = p->value;
            return ZC_TRUE;
        }
        p++;
    }
    return ZC_FALSE;
}

void
zc_confpairs_delete(void *pairs)
{
    zc_free(pairs); 
}

zcConfParser* 
zc_confparser_new(char *filename)
{
    zcConfParser *parser = zc_malloc(sizeof(zcConfParser));
    memset(parser, 0, sizeof(zcConfParser));
     
    strncpy(parser->filename, filename, sizeof(parser->filename)-1);

    return parser;
}

void        
zc_confparser_delete(void* p)
{
    zcConfParser *parser = (zcConfParser*)p;
    zcConfParam *param = parser->params;
    zcConfParam *tmp;
    while (param) {
        tmp = param->next;
        zc_free(param);
        param = tmp;
    }
    zc_free(parser);
}

int    
zc_confparser_add_param(zcConfParser* parser, void *addr, char *name,
                char type, char arraysize, void *arg)
{
    zcConfParam  *param = zc_malloc(sizeof(zcConfParam));
    memset(param, 0, sizeof(zcConfParam));

    param->dst = addr;
    strncpy(param->name, name, 127);
    param->type = type;
    param->arraysize = arraysize;
    //param->pairs = pair;
    if (type == ZC_CONF_USER) {
        param->param.userfunc = arg;
    }else{
        param->param.pairs = arg;
    }
    param->next = NULL;

    if (parser->params == NULL) {
        parser->params = param;
        return 0;
    }
    zcConfParam *prev = NULL;
    zcConfParam *last = parser->params;
    while (last) {
        prev = last;
        last = last->next;
    }
    prev->next = param;
    
    return 0;
}

int    
zc_confparser_add(zcConfParser* parser, void *addr, char *name, char type)
{
    return zc_confparser_add_param(parser, addr, name, type, 0, NULL);
}

int    
zc_confparser_add_array(zcConfParser* parser, void *addr, char *name, 
                char type, char arraysize)
{
    return zc_confparser_add_param(parser, addr, name, type, arraysize, NULL);
}


static void
clean_end(char *start)
{
    char *end = start;  
    /*while (*end != 0) {
        if (*end == '\r' || *end == '\n') {
            end -= 1;
            while (end > start && isblank(*end)) {
                end--;
            }   
            *(end + 1) = 0;
            break;
        }   
        end += 1;
    } */

    while (*end)end++;  
    end--; // skip \0
    while (end > start && (isblank(*end) || *end=='\r' || *end=='\n')) {
        end--;
    }   
    *(end + 1) = 0;
}

static char*
find_array_item(char *start, char **last) 
{
    char *ret, *p; 

    if (*last) {
        ret = *last;
    }else{
        ret = start;
    }

    p = ret;
    
    while (*p) {
        if (*p == ',' || *p == ';') {
            *p = 0;
            *last = p + 1;
            return ret;
        }
        p++;
    }
    if (ret != p) {
        *last = p;
        return ret;
    }
    return NULL;
}

static int
bool_check(char *vstart)
{
    if (strcmp(vstart, "yes") == 0 || strcmp(vstart, "true") == 0) {
        return 1;
    }else if (strcmp(vstart, "no") == 0 || strcmp(vstart, "false") == 0) {
        return 0;
    }else{
        return -1;
    }
}

static int
enum_check(zcConfParam *param, char *vstart, int *v)
{
    return zc_confpairs_find(param->param.pairs, vstart, v);
}


static int
zc_confparser_parse_file(zcConfParser *parser, const char *fname)
{
    FILE *f;

    f = fopen(fname, "r");
    if (NULL == f) {
        ZCWARN("open file %s error:%s", fname, strerror(errno));
        return -1;
    }
    
    char buf[40960];
    char *pstart;
    int  lineno = 0;
    char key[100] = {0};
    while (fgets(buf, sizeof(buf), f) != NULL) {
        pstart = buf;
        lineno++;

        while (1) {
            int ch = fgetc(f);
            //ZCINFO("next line first char:%d, %c\n", ch, ch);
            if (ch != ' ' && ch != '\t') {
                ungetc(ch, f); 
                break;
            }   
            while (1) {  // read all first blank
                ch = fgetc(f);
                if (!isblank(ch) && ch != '\n' && ch != '\r') {
                    ungetc(ch, f); 
                    break;
                }   
            }   
            int  buflen = strlen(buf);
            char *bufpos = buf + buflen;
            fgets(bufpos, sizeof(buf)-buflen, f); 
            lineno++;
        } 
        while (isblank(*pstart)) pstart++;
        if (pstart[0] == '#' || pstart[0] == '\r' || pstart[0] == '\n') {
            continue;
        }

        if (*pstart == '=') {
            ZCERROR("config file parse error! not key at line:%d\n", lineno);
            fclose(f);
            return -1;
        }

        char *sp = strchr(pstart, '=');
        if (sp == NULL) {
            //ZCINFO("not found =");
            sp = strchr(pstart, ' ');
            if (sp == NULL) {
                sp = strchr(pstart, '\t');
            }
            if (sp == NULL) {
                if (*key == '\0') {
                    ZCERROR("config file parse error! \"=\" not found at line:%d\n", lineno);
                    fclose(f);
                    return -1;
                }
            }
            char *end = sp - 1;
            while (end > pstart && isblank(*end)) {
                end--;
            }
            *(end + 1) = 0;
            strcpy(key, pstart);
            //ZCINFO("key:||%s||, %s, %p", key, sp+1, sp+1);
            pstart = sp + 1;
            while (isblank(*pstart)) pstart++;
            //ZCINFO("value:%p %s", pstart, pstart);
            if (*pstart == '\r' || *pstart == '\n') {
                ZCERROR("config file parse error! value error at line:%d", lineno);
                continue;
            }
            clean_end(pstart);
            if (strcmp(key, "include") == 0) {
                char include_path[PATH_MAX] = {0};
                //ZCINFO("parse include: %s", pstart);
                if (pstart[0] != '/') { // get absolute path
                    char *dirend = strrchr(parser->filename, '/');
                    if (dirend == NULL) {
                        char ipath[PATH_MAX] = {0};
                        snprintf(include_path, PATH_MAX, "%s/%s", getcwd(ipath, PATH_MAX), pstart);    
                    }else{
                        int cn = dirend-parser->filename;
                        strncpy(include_path, parser->filename, cn);
                        snprintf(include_path+cn, PATH_MAX-cn, "/%s", pstart);    
                    }
                }else{
                    snprintf(include_path, PATH_MAX, "%s", pstart);
                }
                //ZCINFO("include path:%s", include_path);
                zc_confparser_parse_file(parser, include_path); 
                continue;
            }

            ZCERROR("command error! at line:%d\n", lineno);
            fclose(f);
            return -1;
        } else {
            char *end = sp - 1;
            while (end > pstart && isblank(*end)) {
                end--;
            }
            *(end + 1) = 0;
            strcpy(key, pstart);

            pstart = sp + 1;
            while (isblank(*pstart)) pstart++;
            if (*pstart == '\r' || *pstart == '\n')
                continue;
        }
        clean_end(pstart);
            
        zcConfParam *param = parser->params;
        char *tmp = NULL, *item = NULL;
        while (param) {
            if (strcmp(param->name, key) == 0) {
            //    lastname = param->name;
                if (param->arraysize && param->dsti >= param->arraysize) {
                    ZCERROR("config parser error, too many items: %s line:%d\n", param->name, lineno);
                    fclose(f);
                    return -1;
                }
                switch(param->type) {
                case ZC_CONF_INT:
                    if (param->arraysize) {
                        while ((item = find_array_item(pstart, &tmp)) != NULL) {
                            ((int*)param->dst)[param->dsti] = atoi(item);
                            //ZCERROR("%d\n", ((int*)param->dst)[param->dsti]);
                            param->dsti++;
                        }
                    }else{
                        *(int*)param->dst = atoi(pstart);
                    }
                    break;
                case ZC_CONF_FLOAT:
                    if (param->arraysize) {
                        while ((item = find_array_item(pstart, &tmp)) != NULL) {
                            ((float*)param->dst)[param->dsti] = atof(item);
                            param->dsti++;
                        }
                    }else{
                        *(float*)param->dst = atof(pstart);
                    }
                    break;
                case ZC_CONF_STRING: 
                    if (param->arraysize) {
                        while ((item = find_array_item(pstart, &tmp)) != NULL) {
                            //ZCINFO("string item:%s, %d, %p\n", item, param->dsti, param->dst);
                            //snprintf(((char**)param->dst)[param->dsti], 1024, "%s", item);
                            //param->dsti++;
                            zc_list_append((zcList*)param->dst, zc_strdup(item, 0));
                        }
                    }else{
                        snprintf((char*)param->dst, 1024, "%s", pstart);
                    }
                    break;
                case ZC_CONF_BOOL: {
                    int v = 0;    
                    if (param->arraysize) {
                        while ((item = find_array_item(pstart, &tmp)) != NULL) {
                            v = bool_check(item);
                            if (v < 0) {
                                ZCERROR("conf error, %s must yes/no true/false at line:%d\n", 
                                            param->name, lineno);
                                fclose(f);
                                return -1;
                            }

                            ((int*)param->dst)[param->dsti] = v;
                            param->dsti++;
                        }
                    }else{
                        v = bool_check(pstart);
                        if (v < 0) {
                            ZCERROR("conf error, %s must yes/no true/false at line:%d\n", 
                                        param->name, lineno);
                            fclose(f);
                            return -1;
                        }
                        *(int*)param->dst = v;
                    }
                    break;
                }
                case ZC_CONF_ENUM: { // 枚举只能是整型
                    int v = 0;
                    if (param->arraysize) {
                        while ((item = find_array_item(pstart, &tmp)) != NULL) {
                            if (enum_check(param, pstart, &v) == ZC_FALSE) {
                                ZCERROR("conf error, %s value error at line:%d\n", param->name, lineno);
                                fclose(f);
                                return -1;
                            }
                            ((int*)param->dst)[param->dsti] = v;
                            param->dsti++;
                        }
                    }else{
                        if (enum_check(param, pstart, &v) == ZC_FALSE) {
                            ZCERROR("conf error, %s value error at line:%d\n", param->name, lineno);
                            fclose(f);
                            return -1;
                        }
                        *(int*)param->dst = v;
                    }
                    break;
                }
                case ZC_CONF_USER: {
                    if (param->arraysize) {
                        while ((item = find_array_item(pstart, &tmp)) != NULL) {
                            if (param->param.userfunc(param->dst, key, item, param->dsti) == ZC_FALSE) {
                                ZCERROR("conf error, %s value error at line:%d\n", param->name, lineno);
                                fclose(f);
                                return -1;
                            }
                            //((int*)param->dst)[param->dsti] = v;
                            param->dsti++;
                        }
                    }else{
                        if (param->param.userfunc(param->dst, key, pstart, 0) == ZC_FALSE) {
                            ZCERROR("conf error, %s value error at line:%d\n", param->name, lineno);
                            fclose(f);
                            return -1;
                        }
                        //*(int*)param->dst = v;
                    }

                    break;
                }
                default:
                    ZCERROR("conf param type error at %s with type:%d\n", param->name, param->type);
                    fclose(f);
                    return -1;
                }
                break;
            }
            param = param->next;
        }
    }

    fclose(f);
    return 0;
}


int    
zc_confparser_parse(zcConfParser* parser)
{
    return zc_confparser_parse_file(parser, parser->filename);;
}



