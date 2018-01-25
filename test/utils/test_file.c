#include <zocle/zocle.h>

int myprint(char *path, char *file, int mode)
{
    ZCINFO("path:%s file:%s, mode:%x", path, file, mode);
    return 0;
}


int main(int argc, char *argv[])
{
    zc_mem_init(ZC_MEM_GLIBC|ZC_MEM_DBG_OVERFLOW);
    zc_log_new("stdout", ZC_LOG_INFO);

    ZCINFO("test file ...");
    //zcTimer *tm = zc_timer_new();

    zc_dir_walk(argv[1], 1, myprint);
    

    return 0;
}
