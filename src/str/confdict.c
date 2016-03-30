#include <zocle/str/confdict.h>
#include <zocle/mem/alloc.h>
#include <zocle/log/logfile.h>
#include <zocle/str/cstring.h>
#include <string.h>
#include <ctype.h>

#define ZC_CONFDICT_GROUP   "__default__"

zcConfDict*	
zc_confdict_new(const char *filename)
{
    zcConfDict *cd = (zcConfDict*)zc_malloc(sizeof(zcConfDict));
    memset(cd, 0, sizeof(zcConfDict));
    strcpy(cd->filename, filename);
    cd->groups = zc_dict_new(256, 0);
    if (NULL == cd->groups) {
        ZCERROR("create groups error!\n");
        zc_free(cd);
        return NULL;
    }
    cd->groups->valdel = zc_dict_delete;
    
    cd->group_default = zc_dict_new(1024, 0);
    cd->group_default->valdel = zc_free_func;

    cd->kvsp = '=';
    cd->vvsp = ',';

    return cd;
}

void		
zc_confdict_delete(void *x)
{
    zcConfDict *cd = (zcConfDict*)x;
    zc_dict_delete(cd->groups);

    zc_free(cd); 
}

int zc_confdict_parse(zcConfDict *cd)
{
    FILE *f;

    f = fopen(cd->filename, "r");
    if (NULL == f) {
        ZCWARN("open file error: %s\n", cd->filename);
        return ZC_FAILED;
    }
    
    char buf[4096];
    char *pstart, *pend;
    int  lineno = 0;
    char key[256] = {0};
    //char value[8192] = {0};
    char group[256] = {0};
    zcDict *group_table = cd->group_default;
    zcCString *value = (zcCString*)alloca(sizeof(zcCString) + 8192);
    zc_cstr_init(value, 8192);

    while (fgets(buf, 4096, f) != NULL) {
        pstart = buf;
        lineno++;
        
        //ZCINFO("buf:%s", buf);
        // resume last line
        if (pstart[0] == ' ' || pstart[0] == '\t') {
            while (*pstart == ' ' || *pstart == '\t') pstart++;
            pend = pstart; 
            while (*pend != '\r' && *pend != '\n' && *pend != 0) pend++;
            *pend = 0;
            zc_cstr_append(value, pstart);
            continue;
        }else{
            if (key[0] != 0) {
                //ZCDEBUG("set %s:%s\n", key, value->data);
                zc_dict_add(group_table, key, 0, zc_strdup(value->data,0));
                key[0] = 0;
                zc_cstr_clear(value);
            }
        }

        //while (isblank(*pstart)) pstart++;
        if (pstart[0] == '#' || pstart[0] == '\r' || pstart[0] == '\n') {
            continue;
        }

        if (pstart[0] == '[') { // found group
            char *sp = strchr(pstart, ']');
            if (NULL == sp) {
                ZCERROR("config file error, not found ] at line:%d\n", lineno);
                fclose(f);
                return -1;
            }
            
            pstart++;
            int i = 0;
            while (*pstart != ']') {
                group[i] = *pstart;
                i++;
                pstart++;
            }
            group[i] = 0;
            //ZCINFO("found group: %s\n", group);
            group_table = zc_dict_new(1024, 0);
            group_table->valdel = zc_free_func;
            zc_dict_add(cd->groups, group, 0, group_table);

            key[0] = 0;
            continue; 
        }
        char *sp = strchr(pstart, cd->kvsp);
        if (sp == NULL) {
            ZCERROR("config file parse error! \"=\" not found at line:%d\n", lineno);
            fclose(f);
            return -1;
        } 
        // key
        char *end = sp - 1;
        while (end > pstart && isblank(*end)) {
            end--;
        }
        *(end + 1) = 0;
        strcpy(key, pstart);

        // value
        pstart = sp + 1;
        while (isblank(*pstart)) pstart++;
        if (*pstart == '\r' || *pstart == '\n' || *pstart == 0) {
            continue;
        }
        end = pstart;
        while (*end != '\r' && *end != '\n' && *end != 0) {
            end++;
        }
        *end = 0;  
        zc_cstr_append(value, pstart);
        //zc_dict_add_data(group_table, key, pstart);
        //ZCDEBUG("key:%s value:%s\n", key, value->data); 
    }
    if (key[0] != 0) {
        //ZCINFO("set %s:%s", key, value->data);
        zc_dict_add(group_table, key, 0, zc_strdup(value->data,0));
        key[0] = 0;
        zc_cstr_clear(value);
    }

    return ZC_OK;
}

void*		
zc_confdict_get(zcConfDict *cd, const char *group, const char *key)
{
    zcDict *table = NULL;
    if (group == NULL) {
        table = cd->group_default;
    }else{
        table = (zcDict*)zc_dict_get(cd->groups, (char*)group, strlen(group), NULL);
        if (NULL == table)
            return NULL;
    }
    return zc_dict_get(table, (char*)key, strlen(key), NULL);
}

char*		
zc_confdict_get_str(zcConfDict *cd, const char *group, const char *key, char *defv)
{
    char *s = (char*)zc_confdict_get(cd, group, key);
    if (NULL == s)
        return defv;
    return s;
}

int			
zc_confdict_get_int(zcConfDict *cd, const char *group, const char *key, int defv)
{
    char *s = (char*)zc_confdict_get(cd, group, key);
    if (NULL == s)
        return defv;
    return atoi(s);
}

double		
zc_confdict_get_double(zcConfDict *cd, const char *group, const char *key, double defv)
{
    char *s = (char*)zc_confdict_get(cd, group, key);
    if (NULL == s)
        return defv;
    return atof(s);
}

int64_t		
zc_confdict_get_int64(zcConfDict *cd, const char *group, const char *key, int64_t defv)
{
    char *s = (char*)zc_confdict_get(cd, group, key);
    if (NULL == s)
        return defv;
    return strtoull(s, NULL, 10);
}

int			
zc_confdict_get_bool(zcConfDict *cd, const char *group, const char *key, int defv)
{
    char *s = (char*)zc_confdict_get(cd, group, key);
    if (NULL == s)
        return defv;
    if (strcasecmp(s, "yes") == 0 || strcasecmp(s, "true") == 0)
        return ZC_TRUE;
    return ZC_FALSE;
}

zcList*		
zc_confdict_get_list(zcConfDict *cd, const char *group, const char *key)
{
    char *s = (char*)zc_confdict_get(cd, group, key);
    char buf[1024];
    char *start = s;
    
    zcList *list = zc_list_new(); 
    list->del = zc_free_func;

    //ZCINFO("s:%s, vlen:%d", s, vlen);
    while (*start) {
        char *sp = strchr(start, cd->vvsp);
        if (sp == NULL) {
            strncpy(buf, start, sizeof(buf)-1);
            //ZCINFO("buf:%d %s", i, buf);
            zc_list_append(list, zc_strdup(buf,0));
            break;
        }
        
        strncpy(buf, start, sp-start);
        buf[sp-start] = 0;
        //ZCINFO("buf:%d %s", i, buf);
        zc_list_append(list, zc_strdup(buf,0));
        start = sp+1;
    }

    return list;
}


int
zc_confdict_get_array_int(zcConfDict *cd, const char *group, const char *key, int *value, int vlen)
{
    char *s = (char*)zc_confdict_get(cd, group, key);
    int  i  = 0;
    char buf[16] = {0};
    char *start = s;
 
    //ZCINFO("s:%s, vlen:%d", s, vlen);
    while (*start) {
        char *sp = strchr(start, cd->vvsp);
        if (sp == NULL) {
            strncpy(buf, start, sizeof(buf)-1);
            //ZCINFO("buf:%d %s", i, buf);
            value[i] = atoi(buf);
            i++;
            break;
        }
        
        strncpy(buf, start, sp-start);
        buf[sp-start] = 0;
        //ZCINFO("buf:%d %s", i, buf);
        value[i] = atoi(buf);
        i++;
        if (i >= vlen)
            return ZC_ERR;
        start = sp+1;
    }

    return i;
}

int
zc_confdict_get_array_int64(zcConfDict *cd, const char *group, const char *key, int64_t *value, int vlen)
{
    char *s = (char*)zc_confdict_get(cd, group, key);
    int  i  = 0;
    char buf[16] = {0};
    char *start = s;
 
    //ZCINFO("s:%s, vlen:%d", s, vlen);
    while (*start) {
        char *sp = strchr(start, cd->vvsp);
        if (sp == NULL) {
            strncpy(buf, start, sizeof(buf)-1);
            value[i] = (int64_t)strtoull(buf, NULL, 10);
            i++;
            break;
        }
        
        strncpy(buf, start, sp-start);
        buf[sp-start] = 0;
        //ZCINFO("buf:%d %s", i, buf);
        value[i] = (int64_t)strtoull(buf, NULL, 10);
        i++;
        if (i >= vlen)
            return ZC_ERR;
        start = sp+1;
    }

    return i;
}

int
zc_confdict_get_array_double(zcConfDict *cd, const char *group, const char *key, double *value, int vlen)
{
    char *s = (char*)zc_confdict_get(cd, group, key);
    int  i  = 0;
    char buf[16] = {0};
    char *start = s;
 
    //ZCINFO("s:%s, vlen:%d", s, vlen);
    while (*start) {
        char *sp = strchr(start, cd->vvsp);
        if (sp == NULL) {
            strncpy(buf, start, sizeof(buf)-1);
            //ZCINFO("buf:%d %s", i, buf);
            value[i] = atof(buf);
            i++;
            break;
        }
        
        strncpy(buf, start, sp-start);
        buf[sp-start] = 0;
        //ZCINFO("buf:%d %s", i, buf);
        value[i] = atof(buf);
        i++;
        if (i >= vlen)
            return ZC_ERR;
        start = sp+1;
    }

    return i;
}

static int walk(const char *key, void *value, void *userdata)
{
    ZCINFO("key:%s value:%s", key, (char*)value);
    return ZC_TRUE;
}

void
zc_confdict_print(zcConfDict *cd)
{
    void *value;
    char *key;

    if (cd->groups) {
        zc_dict_foreach_start(cd->groups, key, value)
            zc_dict_walk((zcDict*)value, walk, NULL);
        zc_dict_foreach_end
    }else{
        zc_dict_walk(cd->group_default, walk, NULL);
    }
}

