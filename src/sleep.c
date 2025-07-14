/*cruel*/
#include "sleep.h"
#include <stdio.h>

#ifdef WIN32
#include <windows.h>
void ttd_sleep(size_t ms) {
    Sleep((DWORD)ms);
}
#else
#include <unistd.h>
void ttd_sleep(size_t ms) {
    usleep(ms * 1000);
}
#endif
