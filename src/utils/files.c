#include <zocle/utils/files.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <zocle/base/defines.h>
#include <time.h>
#include <string.h>
#include <zocle/base/compact.h>


int
zc_isfile (const char *path)
{
    struct stat buf;
    if (stat(path, &buf) != 0)
        return ZC_FALSE;
    if (!S_ISREG(buf.st_mode))
        return ZC_FALSE;
    return ZC_TRUE;
}

int
zc_isdir (const char *path)
{
    struct stat buf;
    if (stat(path, &buf) != 0)
        return ZC_FALSE;
    if (!S_ISDIR(buf.st_mode))
        return ZC_FALSE;
    return ZC_TRUE;
}

int
zc_islink (const char *path)
{
    struct stat buf;
    if (stat(path, &buf) != 0)
        return ZC_FALSE;
    if (!S_ISLNK(buf.st_mode))
        return ZC_FALSE;
    return ZC_TRUE;
}

int
zc_isexists (const char *path)
{
    struct stat buf;
    if (stat(path, &buf) != 0)
        return ZC_FALSE;
    if (!S_ISDIR(buf.st_mode) && !S_ISREG(buf.st_mode) && !S_ISLNK(buf.st_mode))
        return ZC_FALSE;
    return ZC_TRUE;
}

int64_t
zc_file_size(const char *path)
{
    struct stat buf;
    if (stat(path, &buf) != 0) {
        return -1;
    }
    if (S_ISREG(buf.st_mode)) {
        return buf.st_size;
    }
    return -1;
}

int 
zc_file_path(const char *argv, char *path)
{
    if (argv[0] == '/') {
        strcpy(path, argv);
        return 0;
    }
    const char *start = argv;
    const char *sp;
    int n = 0;

    while ((sp=strchr(start, '/')) != NULL) {
        int len = sp-start;
        if (len > 2)
            break;
        if (len == 1 && *start != '.')
            break;
        if (len == 2 && (*start != '.' || *(start+1) != '.'))
            break;
        if (len == 2)
            n++;
        start = sp + 1;
    }
    char cwd[1024] = {0};
    getcwd(cwd, sizeof(cwd));
    const char *c = cwd + strlen(cwd);
    while (n>0) {
        while (c>cwd && *c != '/') c++;
        if (c == cwd)
            return ZC_ERR;
        c++; // skip /
        n--;
    }
    int len = c-cwd;
    int slen = 0;
    memcpy(path, cwd, c-cwd);
    sp = strrchr(start, '/');
    if (sp != NULL) {
        path[len] = '/';
        len++;
        sp--; // skip /
        slen = sp-start;
        if (slen > 0) memcpy(path+len+1, start, slen);
    }
    path[len+slen] = 0;

    return 0;
}

