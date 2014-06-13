#include <zocle/zocle.h>

int main()
{
    zc_log_new("stdout", ZC_LOG_ALL);

    zcString *s = zc_str_new(128); 

    zc_str_append(s, "我们的测试一个");

    zcString *gbs = zc_str_convert(s, "utf-8", "gbk");
    if (NULL == gbs) {
        ZCERROR("convert error!");
        return -1;
    }
    ZCINFO("utf8:%d %s, gbk:%d", s->len, s->data, gbs->len);

    return 0;
}
