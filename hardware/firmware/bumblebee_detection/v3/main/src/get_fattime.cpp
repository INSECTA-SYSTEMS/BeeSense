extern "C" {
#include "ff.h"
#include <time.h>

DWORD get_fattime(void) {
    time_t now = time(NULL);
    struct tm *tm_now = localtime(&now);
    if (!tm_now) return 0;

    return  ((DWORD)(tm_now->tm_year - 80) << 25)
          | ((DWORD)(tm_now->tm_mon + 1) << 21)
          | ((DWORD)tm_now->tm_mday << 16)
          | ((DWORD)tm_now->tm_hour << 11)
          | ((DWORD)tm_now->tm_min << 5)
          | ((DWORD)tm_now->tm_sec >> 1);
}
}
