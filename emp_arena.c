#include "emp_arena.h"

#include <stdlib.h>
#include <string.h>

static size_t emp_align_up(size_t x, size_t align) {
    return (x + (align - 1)) & ~(align - 1);
}

typedef struct EmpArenaBlock {
    struct EmpArenaBlock *next;
    size_t cap;
    size_t len;
    // Flexible array member
    uint8_t data[];
} EmpArenaBlock;

void emp_arena_init(EmpArena *a) {
    a->head = NULL;
    a->cur = NULL;
}

void emp_arena_free(EmpArena *a) {
    EmpArenaBlock *b = (EmpArenaBlock *)a->head;
    while (b) {
        EmpArenaBlock *next = b->next;
        free(b);
        b = next;
    }
    a->head = NULL;
    a->cur = NULL;
}

void *emp_arena_alloc(EmpArena *a, size_t size, size_t align) {
    if (align == 0) align = sizeof(void *);
    if ((align & (align - 1)) != 0) align = sizeof(void *);

    const size_t default_block_cap = 4096;

    EmpArenaBlock *cur = (EmpArenaBlock *)a->cur;
    if (!cur) {
        size_t cap = default_block_cap;
        if (cap < size + align) cap = emp_align_up(size + align, 64);
        EmpArenaBlock *b = (EmpArenaBlock *)malloc(sizeof(EmpArenaBlock) + cap);
        if (!b) return NULL;
        b->next = NULL;
        b->cap = cap;
        b->len = 0;
        // zero for determinism/safety
        memset(b->data, 0, cap);
        a->head = (struct EmpArenaBlock *)b;
        a->cur = (struct EmpArenaBlock *)b;
        cur = b;
    }

    size_t at = emp_align_up(cur->len, align);
    size_t need = at + size;
    if (need > cur->cap) {
        size_t cap = default_block_cap;
        if (cap < size + align) cap = emp_align_up(size + align, 64);
        EmpArenaBlock *b = (EmpArenaBlock *)malloc(sizeof(EmpArenaBlock) + cap);
        if (!b) return NULL;
        b->next = NULL;
        b->cap = cap;
        b->len = 0;
        memset(b->data, 0, cap);

        cur->next = b;
        a->cur = (struct EmpArenaBlock *)b;
        cur = b;
        at = emp_align_up(cur->len, align);
        need = at + size;
    }

    void *ptr = cur->data + at;
    cur->len = need;
    return ptr;
}
