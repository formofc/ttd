#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include "arena_alloc.h"
#include "miniaudio.h"
#include "sleep.h"
#include "dym.h"

typedef char ttd_bool;

#define TTD_MIN(x, y) (x > y ? y : x)

typedef struct {
    const char* word;
    const char* file;
    size_t word_length; /*cached*/ 
    double start_s;
    double length_s;

} sound_block_t;

typedef struct {
    sound_block_t* data;
    size_t size;
    size_t capacity;
} sound_blocks_t;

typedef struct sound_block_trie_t sound_block_trie_t;
typedef struct sound_block_trie_t {
    sound_block_trie_t* children[256]; /* ascii */
    sound_block_t sblock; /* non-null? leaf */
    ttd_bool is_leaf;
    ttd_bool is_contain;
} sound_block_trie_t;

typedef struct {
    const char* opt_output_path;
    const char* config_path;
} config_t;

ttd_bool init_sound_block_trie(sound_block_trie_t* node) {
    if (!node) return 0;
    
    memset(node->children, 0, sizeof(node->children));
    memset(&node->sblock, 0, sizeof(sound_block_t));
    node->is_leaf = 1;
    node->is_contain = 0;

    return 1;
}

int escaped(int c) {
    switch (c) {
        case 'n': return '\n';
        case 'b': return '\b';
        case 't': return '\t';
        case '\'': return '\'';
        case '\"': return '\"';
        case '0': return '\0';
        case 'r': return '\r';
        case 'v': return '\v';
        case 'f': return '\f';
        case 'a': return '\a';
        default: return c;
    }
}

ttd_bool insert_sound_block_trie(arena_allocator_t* arr, sound_block_trie_t* root, sound_block_t sblock) {
    const char* word;
    unsigned char uc;
    size_t i;
    sound_block_trie_t *current, *new_node;

    if (!arr || !root || !sblock.word) return 0;
    
    word = sblock.word;
    current = root;

    for (i = 0; i < sblock.word_length; ++i) {
        uc = (unsigned char)word[i];
        if (!current->children[uc]) {
            new_node = arena_allocate(arr, sizeof(sound_block_trie_t), 8);
            if (!new_node) return 0;
            if (!init_sound_block_trie(new_node)) return 0;
            current->children[uc] = new_node;
            current->is_leaf = 0;
        }
        current = current->children[uc];
    }
    current->sblock = sblock;
    current->is_contain = 1;

    return 1;
}

sound_block_t* get_longest_match(sound_block_trie_t* root, const char* word, ttd_bool* opt_is_leaf) {
    sound_block_trie_t* best_match = NULL;
    sound_block_trie_t* current;
    size_t len, i;
    unsigned char uc;

    if (!root || !word) return NULL;

    current = root;
    len = strlen(word);

    for (i = 0; i < len; ++i) {
        uc = (unsigned char)word[i];
        if (!current->children[uc]) break;
        current = current->children[uc];
        if (current->is_contain) best_match = current;
    }
    
    if (!best_match) return NULL;

    if (opt_is_leaf) *opt_is_leaf = (current->is_leaf) || (i < len);
    return &best_match->sblock;
}

/* Maybe, just maybe i should cache encoders cuz they are BLOATED
typedef struct {
    const char* file_name;
    ma_encoder encoder;
} encoder_cache_entry;
*/

void skip_whitespace(FILE* f) {
    int ch;
    while ((ch = fgetc(f)) != EOF && isspace(ch)) {}
    if (ch != EOF) ungetc(ch, f);
}

ttd_bool fscan_quoted_string(FILE* f, arena_allocator_t* arr, char** str) {
    int ch;
    size_t len = 0;
    size_t capacity = 16;
    char *buffer, *temp_buffer;
    
    if (!f || !arr || !str) return 0;

    skip_whitespace(f);
    
    ch = fgetc(f);
    if (ch != '\"') {
        ungetc(ch, f);
        return 0;
    }

    buffer = arena_allocate(arr, capacity, 1);
    if (!buffer) return 0;

    while ((ch = fgetc(f)) != EOF && ch != '\"') {
        if (ch == '\\') { /* escape */
            ch = fgetc(f);
            if (ch == EOF) break;
            ch = escaped(ch);
        }

        if (len + 1 >= capacity) {
            capacity *= 2;
            temp_buffer = arena_allocate(arr, capacity, 1);
            if (!temp_buffer) return 0;
            memcpy(temp_buffer, buffer, len);
            buffer = temp_buffer;
        }

        buffer[len++] = ch;
    }

    if (ch != '\"') {
        return 0; /* invalid quote */
    }

    buffer[len] = '\0';
    *str = buffer;
    return 1;
}

/* format: "%s" "%s" %fl %fl */
ttd_bool fscan_sound_block(FILE* f, arena_allocator_t* arr, sound_block_t* out) {
    char* str_dummy;

    if (!f || !arr || !out) return 0;

    if (!fscan_quoted_string(f, arr, &str_dummy)) {
        return 0;
    }
    out->word = str_dummy;

    if (!fscan_quoted_string(f, arr, &str_dummy)) {
        return 0;
    }
    out->file = str_dummy;

    if (fscanf(f, "%lf %lf", &out->start_s, &out->length_s) != 2) {
        return 0;
    }

    out->word_length = strlen(out->word);

    return 1;
}

void ma_data_callback_play_file(ma_device* device, void* output, const void* input, ma_uint32 frame_count) {
    ma_uint64 dummy;
    ma_decoder* decoder = (ma_decoder*)device->pUserData;
    if (!decoder) return;

    ma_decoder_read_pcm_frames(decoder, output, frame_count, &dummy);
    (void)dummy;
    (void)input;
}

ttd_bool save_range(const char* file_path, double start_s, double length_s, ma_encoder* encoder) {
    ttd_bool decoder_inited = 0, success = 0;
    ma_decoder decoder;
    ma_decoder_config decoder_config;
    ma_uint64 start_pcm_frame, frames_to_save, frames_saved;
    float buffer[1024];

    if (!file_path || (length_s < 0) || (start_s < 0) || !encoder) return 0;

    decoder_config = ma_decoder_config_init(
        ma_format_f32,
        1, /* channels */ 
        44100
    );

    if (ma_decoder_init_file(file_path, &decoder_config, &decoder) != MA_SUCCESS) {
        fprintf(stderr, "Failed to play sound from file\n");
        goto failure;
    }
    decoder_inited = 1;
    
    start_pcm_frame = (ma_uint64)(decoder.outputSampleRate * start_s);
    if (ma_decoder_seek_to_pcm_frame(&decoder, start_pcm_frame) != MA_SUCCESS) {
        fprintf(stderr, "Failed to play seek audio file\n");
        goto failure;
    }
    frames_to_save = (ma_uint64)(decoder.outputSampleRate * length_s);
    
    while (frames_to_save) { /* too easy */
        if (ma_decoder_read_pcm_frames(&decoder, (void*)buffer, TTD_MIN(frames_to_save, 1024), &frames_saved) != MA_SUCCESS) {
            fprintf(stderr, "Failed to read samples from file\n");
            goto failure;
        }
        if (ma_encoder_write_pcm_frames(encoder, (void*)buffer, frames_saved, &frames_saved) != MA_SUCCESS) {
            fprintf(stderr, "Failed to write samples to file\n");
            goto failure;
        }
        frames_to_save -= frames_saved;
    }
    
    success = 1;
failure:
    if (decoder_inited) ma_decoder_uninit(&decoder);

    return success;
}

ttd_bool play_range(const char* file_path, double start_s, double length_s) {
    ttd_bool device_inited = 0, decoder_inited = 0, success = 0;
    ma_decoder decoder;
    ma_device device;
    ma_device_config device_config;
    ma_uint64 start_pcm_frame;

    if (!file_path || (start_s < 0) || (length_s < 0)) return 0;

    if (ma_decoder_init_file(file_path, NULL, &decoder) != MA_SUCCESS) {
        fprintf(stderr, "Failed to play sound from file\n");
        goto failure;
    }
    decoder_inited = 1;
    
    start_pcm_frame = (ma_uint64)(decoder.outputSampleRate * start_s);
    if (ma_decoder_seek_to_pcm_frame(&decoder, start_pcm_frame) != MA_SUCCESS) {
        fprintf(stderr, "Failed to play seek audio file\n");
        goto failure;
    }

    device_config = ma_device_config_init(ma_device_type_playback); /* just play sound */
    device_config.playback.format = decoder.outputFormat;
    device_config.playback.channels = decoder.outputChannels;
    device_config.sampleRate = decoder.outputSampleRate;
    device_config.dataCallback = ma_data_callback_play_file;
    device_config.pUserData = &decoder;
    if (ma_device_init(NULL, &device_config, &device) != MA_SUCCESS) {
        fprintf(stderr, "Failed to initialize audio device\n");
        goto failure;
    }
    device_inited = 1;

    /* run and wait */
    ma_device_start(&device);
    ttd_sleep((size_t)(length_s * 1000));
    ma_device_stop(&device);
    
    success = 1;
failure:
    if (device_inited) ma_device_uninit(&device);
    if (decoder_inited) ma_decoder_uninit(&decoder);

    return success;
}

ttd_bool process_stream(FILE* f, sound_block_trie_t* trie, const char* opt_output_path) {
    int ch;
    ma_encoder encoder;
    ma_encoder_config encoder_config;
    ttd_bool is_leaf, encoder_inited = 0, success = 0;
    size_t buffer_size = 64;
    char* buffer;
    size_t buffer_pos = 0, best_length = 0;
    const sound_block_t* best_match = NULL;
    arena_allocator_t arr = {0};

    if (!f || !trie) goto failed;

    buffer = arena_allocate(&arr, buffer_size, 1);
    if (!buffer) {
        fprintf(stderr, "Failed to allocate buffer. Buy more RAM dude\n");
        goto failed;
    }

    if (opt_output_path) { /* init encoder */
        encoder_config = ma_encoder_config_init(
                ma_encoding_format_wav,
                ma_format_f32,
                1, /* channels */
                44100
        );
        if (ma_encoder_init_file(opt_output_path, &encoder_config, &encoder) != MA_SUCCESS) {
            fprintf(stderr, "Failed to inialize encoder for \"%s\"\n", opt_output_path);
            goto failed;
        }
        encoder_inited = 1;
    }

    while ((ch = fgetc(f)) != EOF) {
        if (buffer_pos >= buffer_size - 1) {
            buffer = arena_reallocate(&arr, buffer, buffer_size, buffer_size * 2);
            if (!buffer) {
                fprintf(stderr, "Failed to reallocate buffer(from %zu to %zu). Buy more RAM dude\n", buffer_size, buffer_size * 2);
                goto failed;
            }
            buffer_size *= 2;
        }
        
        buffer[buffer_pos++] = (char)ch;
        buffer[buffer_pos] = '\0';
        
        while (1) {
            is_leaf = 0;
            best_match = get_longest_match(trie, buffer, &is_leaf);

            if (!best_match || !is_leaf) break;
            best_length = best_match->word_length; 

            memmove(buffer, &buffer[best_length], buffer_pos - best_length);
            buffer_pos -= best_length;
            buffer[buffer_pos] = '\0';

            if (encoder_inited) { /* no playback */
                save_range(best_match->file, best_match->start_s, best_match->length_s, &encoder);
            } else {
                play_range(best_match->file, best_match->start_s, best_match->length_s);
            }
            best_match = NULL;
        }
    }

    if (best_match) {
        play_range(best_match->file, best_match->start_s, best_match->length_s);
    }
    
    success = 1;
failed:
    if (encoder_inited) ma_encoder_uninit(&encoder);
    arena_free(&arr);
    return success;
}

void print_help(const char* selfpath) {
    printf(
        "Usage:\n"
        "%s config_path [options]\n"
        "\"-o\"/\"--output\" FILE - Set output .wav file. No playback\n"
        "\"-h\"/\"--help\" - Show this help message\n"
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
        "text patterns are found.\n"
        "Warning: audio file paths are relative to current working directory, not config file location\n",
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

    while (*argv) {
        if ((strcmp(*argv, "-o") == 0) || (strcmp(*argv, "--output") == 0)) {
            ++argv;
            --argc;
            if (!*argv) {
                fprintf(stderr, "provide real file path for output\n");
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

int main(int argc, char** argv) {
    config_t config;
    arena_allocator_t arr = {0};
    FILE* f;
    sound_block_t current_sblock;
    sound_block_trie_t trie;

    if (!parse_config(&config, argc, argv)) return EXIT_FAILURE;
    
    f = fopen(config.config_path, "r");
    if (!f) {
        fprintf(stderr, "Failed to open file\n");
        return EXIT_FAILURE;
    }
    
    init_sound_block_trie(&trie);
    while (1) {
        skip_whitespace(f);
        if (feof(f)) break; 
        
        int c = fgetc(f);
        ungetc(c, f);
        if (!fscan_sound_block(f, &arr, &current_sblock)) {
            fprintf(stderr, "Failed to scan file\n");
            return EXIT_FAILURE;
        }
        /* printf("Insert `%s`\n", current_sblock.word); */
        if (!insert_sound_block_trie(&arr, &trie, current_sblock)) {
            fprintf(stderr, "Failed to insert \"%s\"\n", current_sblock.word);
        }
        /* printf("Block scaned\n"); */
    }
    fclose(f);
    
    if (!process_stream(stdin, &trie, config.opt_output_path)) {
        fprintf(stderr, "Failed to parse stdin\n");
        return 1;
    }

    return 0;
}
