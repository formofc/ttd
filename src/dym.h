#ifndef __DYM_H__
#define __DYM_H__ 1

#define dym_push_e(dym, val, alloc_f, alloc_ud, free_f, free_ud) \
    do { \
        if ((dym)->capacity == 0) { \
            (dym)->data = malloc(1 * sizeof(val)); \
            (dym)->capacity = 1; \
        } \
        if ((dym)->size == (dym)->capacity) { \
            (dym)->capacity *= 2; \
            void* new_data = alloc_f((dym)->capacity * sizeof(val), alloc_ud); \
            memcpy(new_data, (dym)->data, (dym)->size * sizeof(val)); \
            free_f((dym)->data, free_ud); \
            (dym)->data = new_data; \
        } \
        (dym)->data[(dym)->size++] = val; \
    } while(0);

#define dym_clone_e(dym, out, alloc_f, alloc_ud) \
    do { \
        (out)->size = (dym)->size; \
        (out)->capacity = (dym)->capacity; \
        (out)->data = alloc_f((out)->capacity * sizeof((dym)->data[0]), alloc_ud); \
        memcpy((out)->data, (dym)->data, (dym)->size * sizeof((dym)->data[0])); \
    } while(0);

#define dym_free_e(dym, free_f, free_ud) \
    do { \
        if ((dym)->data) { \
            free_f((dym)->data, free_ud); \
            (dym)->data = NULL; \
            (dym)->capacity = 0; \
            (dym)->size = 0; \
        } \
    } while(0);
#ifdef DYM_STD
#include <stdlib.h>

#define dym_push(dym, val) dym_push_e(dym, val, dym_default_allocate, 0, dym_default_free, 0)
#define dym_clone(dym, out) dym_clone(dym, out, dym_default_allocate, 0)
#define dym_free(dym) dym_free_e(dym, dym_default_free, 0)
static void* dym_default_allocate(size_t count, int ud) {
    (void)ud;
    return malloc(count);
}

static void dym_default_free(void* ptr, int ud) {
    (void)ud;
    free(ptr);
}

#endif /* DYM_STD */

#endif /* __DYM_H__ */
