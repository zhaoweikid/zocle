#include <zocle/zocle.h>
#include <zlib.h>
#include <assert.h>
   
int test1()
{
    ZCINFO("======== test1 ========");
    zcCompress *c1 = zc_compress_new(ZC_GZIP_ENC, 0);
    zcCompress *c2 = zc_compress_new(ZC_GZIP_DEC, 0);

    char *s = "我的一次测试结果而已哈哈哈哈哈哈哈哈哈哈哈哈垃圾撒旦法大家"
              "按时附件啊是发送发是否为如我iu按发送方大是大非阿飞是大法师打发士大夫"
              "杰弗里斯京东方啦简单发；拉技术的；附件哎算了； 发了数据打法是家乐福我euro"
              "来决定否人情味若无iuerpuqfadiofuaps读飞啊发芽货到付款氨分解dsl附件了飞"
              "法拉克是的金牛座v的肌肤啦地方阿里；封建时代附件啊是来决定啊按时大风扇"
              "啊是来得及发； 我eurouf阿发达发阿斯顿发水费电费撒旦法师打发撒旦法撒旦法按时"
              "的vvas的vasdvasdva大厦发撒旦法萨芬撒旦法的发生打发士大夫打发阿士大夫的沙发上";
   
    int slen = strlen(s);
    int ret;
    char enc[4096] = {0};

    ZCINFO("string len:%lu", strlen(s));
    int i;
    char *start = enc;
    char *from;
    int zlen = 0;
    bool flag = false;
    for (i=0; i<(slen/100+1); i++) {
        int blen = 100;
        int tlen = 200;
        from = s + i*100;
        if (i == 7) {
            blen = slen % 100;
            flag = true;
        }
        ZCINFO("blen:%d", blen);
        ret = zc_compress_do(c1, from, blen, start, tlen, flag);
        ZCINFO("enc ret: %d, avail_in:%d total_in:%ld, avail_out:%d total_out:%ld", 
                ret, c1->stream.avail_in, c1->stream.total_in, c1->stream.avail_out, c1->stream.total_out);
        //start = enc + c1->stream.total_out;
        while (ret == ZC_ERR_BUF) {
            start = enc + c1->stream.total_out;
            tlen = tlen*2;
            ret = zc_compress_do(c1, NULL, 0, start, tlen, flag);
        }
        if (ret < 0) {
            ZCERROR("compress error: %d", ret);
            return -1;
        }
        start += c1->lastsize;
    }
    zlen = c1->stream.total_out;
    ZCINFO("compress len:%d", zlen); 
    ZCINFO("====== compress complete ======"); 
    zcString *dec = zc_str_new(zlen);
    char dbuf[100] = {0};
    int64_t declen = 0;
    start = enc;
    flag = false;
    while (1) {
        memset(dbuf, 0, sizeof(dbuf));
        if (zlen - (start-enc) < 50) {
            flag = true;
        }
        ret = zc_compress_do(c2, start, 50, dbuf, sizeof(dbuf)-1, flag);
        ZCINFO("enc ret: %d, avail_in:%d total_in:%ld, avail_out:%d total_out:%ld", 
                ret, c2->stream.avail_in, c2->stream.total_in, c2->stream.avail_out, c2->stream.total_out);
        ZCINFO("dbbuf: %s", dbuf);
        while (ret == ZC_ERR_BUF) {
            zc_str_append_len(dec, dbuf, sizeof(dbuf)-1);
            memset(dbuf, 0, sizeof(dbuf));
            ret = zc_compress_do(c2, NULL, 0, dbuf, sizeof(dbuf)-1, flag);
            ZCINFO("dbbuf: %s", dbuf);
        }

        if (ret < 0) {
            ZCERROR("compress error: %d", ret);
            return -1;
        }
        zc_str_append_len(dec, dbuf, ret);
        start = enc + c2->stream.total_in;
        if (start - enc == zlen)
            break;
    }
    ZCINFO("====== uncompress complete ======"); 
    ZCINFO("uncompres: %s", dec->data);

    zc_compress_delete(c1);
    zc_compress_delete(c2);

    assert(strcmp(s, dec->data) == 0);

    return 0;
}

int test2()
{
    ZCINFO("======== test2 ========");
    char *s = "我的一次测试结果而已哈哈哈哈哈哈哈哈哈哈哈哈垃圾撒旦法大家"
              "按时附件啊是发送发是否为如我iu按发送方大是大非阿飞是大法师打发士大夫"
              "杰弗里斯京东方啦简单发；拉技术的；附件哎算了； 发了数据打法是家乐福我euro"
              "来决定否人情味若无iuerpuqfadiofuaps读飞啊发芽货到付款氨分解dsl附件了飞"
              "法拉克是的金牛座v的肌肤啦地方阿里；封建时代附件啊是来决定啊按时大风扇"
              "啊是来得及发； 我eurouf阿发达发阿斯顿发水费电费撒旦法师打发撒旦法撒旦法按时"
              "的vvas的vasdvasdva大厦发撒旦法萨芬撒旦法的发生打发士大夫打发阿士大夫的沙发上";
  
    ZCINFO("string: %s", s);
    zcCompress c;
    
    zc_compress_init(&c, ZC_GZIP_ENC, 0);
    
    char enc[8192] = {0};
    int ret = zc_compress_do(&c, s, strlen(s), enc, sizeof(enc)-1, true);
    if (ret < 0) {
        ZCERROR("compress error! %d", ret);
        return -1;
    }

    int enclen = c.stream.total_out;
    ZCINFO("enclen:%d", enclen);
    zc_compress_destroy(&c);
    ZCINFO("====== enc complete ======");

    
    zcString *dec = zc_str_new(enclen);

    zcCompress c2;
    zc_compress_init(&c2, ZC_GZIP_DEC, 0);
    ret = zc_compress_dec(&c2, enc, enclen, dec, true);
    if (ret < 0) {
        ZCERROR("compress decode error: %d", ret);
        return -1;
    }

    ZCINFO("decode string: %s", dec->data); 
    zc_compress_destroy(&c2);

    assert(strcmp(s, dec->data) == 0);
    
    return 0;
}

int test3()
{
    ZCINFO("======== test3 ========");
    zcString *s = zc_str_new(40960);
    const char *filename = "/Users/zhaowei/projects/c/zocle2/test/http/a.dat";
    int flen = (int)zc_file_size(filename);
    FILE *f;
    f = fopen(filename, "rb");
    fread(s->data, 1, flen, f);
    fclose(f);

    s->len = flen;
    
    zcCompress c2;
    zc_compress_init(&c2, ZC_GZIP_DEC, 0);
    ZCINFO("go to dec ...");
    zcString *dec = zc_str_new(1024);
    //zcString *dec = zc_str_new(386064);
    int ret = zc_compress_dec(&c2, s->data+10, s->len-10-8, dec, true);
    if (ret < 0) {
        ZCERROR("compress decode error: %d", ret);
        return -1;
    }
    dec->data[dec->len] = 0;
    ZCINFO("ret: %d, decode string: %d, %s", ret, dec->len, dec->data); 
    zc_compress_destroy(&c2);

    return 0;
}

int main()
{
    zc_mem_init(ZC_MEM_GLIBC|ZC_MEM_DBG_OVERFLOW);
    zc_log_new("stdout", ZC_LOG_ALL);
    zc_log_whole(_zc_log, 1);

    test1();
    test2();
    test3();

    return 0;
}
