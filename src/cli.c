#include <string.h>
#include <stdio.h>

#include "cli.h"

static void print_help(const char* selfpath) {
    printf(
        "Usage:\n"
        "%s config_path [options]\n"
        "\"-o\"/\"--output\" FILE - Set output .wav file. No playback\n"
        "\"-h\"/\"--help\" - Show this help message\n"
        "\"-r\"/\"--random\" COUNT - Takes the COUNT of random sounds and plays/saves them\n"
        "\n"
        "Config file format:\n"
        "   Each line represents a sound block in the following format:\n"
        "   \"word\" \"sound_file\" start_time length\n"
        "   Where:\n"
        "       word - the text to match\n"
        "       sound_file - path to audio file\n"
        "       start_time - start time in seconds\n"
        "       length - duration in seconds\n"
        "\n"
        "Example config line:\n"
        "   \"hello\" \"sounds/greeting.wav\" 0.5 1.2\n"
        "\n"
        "The program reads from stdin and plays matching sound blocks when\n"
        "text patterns are found.\n",
        selfpath
    );
}

ttd_bool parse_config(config_t* config, int argc, char** argv) {
    const char* selfpath;

    if (!config || !argc || !argv) return 0;

    selfpath = *argv++;
    --argc;

    config->config_path = NULL;
    config->opt_output_path = NULL;
    config->random_speak = 0;

    while (*argv) {
        if ((strcmp(*argv, "-r") == 0) || (strcmp(*argv, "--random") == 0)) {
            ++argv;
            --argc;
            if (!*argv) {
                fprintf(stderr, "Provide random words count\n");
                return 0;
            }
            config->random_speak = 1;
            if (sscanf(*argv, "%zu", &config->random_speak_count) != 1) {
                fprintf(stderr, "\"%s\" is not a valid unsigned integer\n", *argv);
                return 0;
            }
        } else if ((strcmp(*argv, "-o") == 0) || (strcmp(*argv, "--output") == 0)) {
            ++argv;
            --argc;
            if (!*argv) {
                fprintf(stderr, "Provide real file path for output\n");
                return 0;
            }
            config->opt_output_path = *argv;
        } else if ((strcmp(*argv, "-h") == 0) || (strcmp(*argv, "--help") == 0)) {
            print_help(selfpath);
            return 0;
        } else {
            config->config_path = *argv;
        }
        ++argv;
        --argc;
    }

    if (!config->config_path) return 0;

    return 1;
}
