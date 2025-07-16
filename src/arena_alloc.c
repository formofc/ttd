#include "arena_alloc.h"

static size_t align_up(size_t size, size_t alignment) {
    size_t mask = alignment - 1;
    return (size + mask) & ~mask;
}

void* arena_allocate(arena_allocator_t* arr, size_t size, size_t align) {
    size_t aligned_size, new_capacity, offset;
    void *allocated, *aligned;

    if (!arr || !size) return 0;

    aligned_size = align_up(size, align);

    if (!arr->head) { /* empty arena */
        if (aligned_size > 4096) {
            new_capacity = aligned_size * 2;
        } else {
            new_capacity = 4096;
        }
    
        allocated = malloc(new_capacity + align);
        if (!allocated) return 0;

        aligned = (void*)align_up((size_t)allocated, align);

        arr->tail.capacity = new_capacity;
        arr->tail.size = size + ((size_t)aligned - (size_t)allocated);
        arr->tail.data = allocated;
        arr->head = &arr->tail;

    } else {
        offset = align_up(arr->head->size, align);

        if (offset + aligned_size > arr->head->capacity) {
            /* 
             * not optimal way but still 
             * just allocate new one
             * maybe replace with recursive call?
             * */
            if (aligned_size > 4096) {
                new_capacity = aligned_size * 2;
            } else {
                new_capacity = 4096;
            }

            arr->head->next = malloc(sizeof(arena_t));
            if (!arr->head->next) {
                return 0;
            }

            allocated = malloc(new_capacity + align);
            if (!allocated) {
                free(arr->head->next);
                arr->head->next = 0;
                return NULL;
            }

            aligned = (void*)align_up((size_t)allocated, align);

            arr->head = arr->head->next;

            arr->head->data = allocated;
            arr->head->size = aligned_size;
            arr->head->capacity = new_capacity;
            arr->head->next = NULL;

        } else {
            allocated = (void*)(arr->head->data + arr->head->size);
            aligned = (void*)align_up((size_t)allocated, align);
            arr->head->size = aligned_size + offset;

        }
    }

    return aligned;
}

void* arena_reallocate(arena_allocator_t* arr, void* old, size_t old_size, size_t size) {
    arena_t* current;

    if (!arr || !size) return NULL;

    if (!old || !old_size) return arena_allocate(arr, size, 16);

    if (old_size >= size) return old;

    current = &arr->tail;

    while (current && current->size) {
        /* in */
        if (
            ((size_t)old >= (size_t)current->data) &&
            ((size_t)old < ((size_t)current->data + current->size))
            ) { 
            /* be sure its last allocated block
             * and
             * be sure its enought space
             * */
            if (
                    ((size_t)old + old_size) == ((size_t)current->data + current->size) &&
                    ((size_t)old + size) <= ((size_t)current->data + current->capacity)
                ) {
                current->size += (size - old_size);
                return old;
            }

            break;
        }
        current = current->next;
    }

    return arena_allocate(arr, size, 16);
}

void arena_free(arena_allocator_t* arr) {
    arena_t *current, *allocated_arena = NULL;

    if (!arr) return;
    
    current = &arr->tail;

    while (current && current->capacity) {
        if (allocated_arena != &arr->tail) free(allocated_arena);

        if (current->data) free(current->data);
        current->data = NULL;
        current->size = 0;
        current->capacity = 0;
        allocated_arena = current;
        current = current->next;
    }

    arr->head = NULL;
}
