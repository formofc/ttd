#ifndef DIR_H
#define DIR_H 1
#include <stdlib.h>

int ttd_goto_dir(const char* dir);
char* ttd_current_dir(char* buffer, size_t buffer_size);

#endif
