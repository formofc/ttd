#ifndef CLI_H
#define CLI_H 1

#include <stddef.h>
#include "ttd_bool.h"

typedef struct {
    const char* opt_output_path;
    const char* config_path;
    ttd_bool random_speak;
    size_t random_speak_count;
} config_t;

ttd_bool parse_config(config_t* cfg, int argc, char** argv);

#endif
