#include "dir.h"

#ifdef _WIN32
    #include <direct.h>
    #define chdir _chdir
    #define getcwd _getcwd
#else
    #include <unistd.h>
#endif

int ttd_goto_dir(const char* dir) {
    return chdir(dir);
}
char* ttd_current_dir(char* buffer, size_t buffer_size) {
    return getcwd(buffer, buffer_size);
}
