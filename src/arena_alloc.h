#ifndef ARENA_ALLOC_H
#define ARENA_ALLOC_H 1
#include <stdlib.h>

typedef struct arena_t arena_t;
typedef struct arena_t {
    arena_t* next;
    void* data;
    size_t size;
    size_t capacity;

} arena_t;

typedef struct {
    arena_t* head;
    arena_t tail;

} arena_allocator_t;

void* arena_allocate(arena_allocator_t* arr, size_t size, size_t align); 
void* arena_reallocate(arena_allocator_t* arr, void* old, size_t old_size, size_t size); 
void arena_free(arena_allocator_t* arr);
#endif
