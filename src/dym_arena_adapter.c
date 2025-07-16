#include "dym_arena_adapter.h"

void* dym_arena_allocate(size_t count, arena_allocator_t* arr) {
    return arena_allocate(arr, count, 32);
}
