#ifndef __DYM_H__
#define __DYM_H__ 1

#define dym_push(dym, val) \
    do { \
        if ((dym)->capacity == 0) { \
            (dym)->data = malloc(1 * sizeof(val)); \
            (dym)->capacity = 1; \
        } \
        if ((dym)->size == (dym)->capacity) { \
            (dym)->capacity *= 2; \
            void* new_data = malloc((dym)->capacity * sizeof(val)); \
            memcpy(new_data, (dym)->data, (dym)->size * sizeof(val)); \
            free((dym)->data); \
            (dym)->data = new_data; \
        } \
        (dym)->data[(dym)->size++] = val; \
    } while(0);

#define dym_clone(dym, out) \
    do { \
        (out)->size = (dym)->size; \
        (out)->capacity = (dym)->capacity; \
        (out)->data = malloc((out)->capacity * sizeof((dym)->data[0])); \
        memcpy((out)->data, (dym)->data, (dym)->size * sizeof((dym)->data[0])); \
    } while(0);

#define dym_free(dym) \
    do { \
        if ((dym)->data) { \
            free((dym)->data); \
            (dym)->data = NULL; \
            (dym)->capacity = 0; \
            (dym)->size = 0; \
        } \
    } while(0);


#endif /* __DYM_H__ */
