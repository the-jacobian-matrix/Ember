#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct EmpArena {
    struct EmpArenaBlock *head;
    struct EmpArenaBlock *cur;
} EmpArena;

void emp_arena_init(EmpArena *a);
void emp_arena_free(EmpArena *a);

// Allocates `size` bytes aligned to `align` (power of two).
void *emp_arena_alloc(EmpArena *a, size_t size, size_t align);

#ifdef __cplusplus
}
#endif
