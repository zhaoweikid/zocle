#include <zocle/zocle.h>

int main()
{
    char *a = "fjldjfauerowuorajsfkash我饿m发放j阿里绥靖地方就";
    char buffer[2000];

    zc_mem_init(ZC_MEM_GLIBC|ZC_MEM_DBG_OVERFLOW);
    zc_log_new("stdout", ZC_LOG_ALL);
    zc_log_whole(_zc_log, 1);

#ifdef ZOCLE_WITH_SSL
    memset(buffer, 0, sizeof(buffer));
    zc_openssl_sha1_bcd(buffer, a, strlen(a));
    ZCINFO("openssl sha1:\t%s", buffer);
#endif

    memset(buffer, 0, sizeof(buffer));
    zc_sha1_bcd(buffer, a, strlen(a));
    ZCINFO("sha1:\t%s", buffer);

    memset(buffer, 0, sizeof(buffer));
    zc_sha224_bcd(buffer, a, strlen(a));
    ZCINFO("sha224:\t%s", buffer);

    memset(buffer, 0, sizeof(buffer));
    zc_sha256_bcd(buffer, a, strlen(a));
    ZCINFO("sha256:\t%s", buffer);

    memset(buffer, 0, sizeof(buffer));
    zc_sha384_bcd(buffer, a, strlen(a));
    ZCINFO("sha384:\t%s", buffer);

    memset(buffer, 0, sizeof(buffer));
    zc_sha512_bcd(buffer, a, strlen(a));
    ZCINFO("sha512:\t%s", buffer);

    return 0;
}
