#pragma once

#include "emp_ast.h"

#ifdef __cplusplus
extern "C" {
#endif

// Borrow checking (phase 2): lexical-scope-based borrows only.
//
// For each binding, tracks:
// - shared_count: number of active `&` borrows
// - mut_active: whether an `&mut` borrow is active
//
// Borrow lifetime ends at end of the lexical scope where the borrow expression appears.
// (Later passes can refine this to last-use / data-flow lifetimes.)
//
// Diagnostics are appended to `diags` and message strings are allocated in `arena`.
void emp_sem_check_borrows_lexical(EmpArena *arena, const EmpProgram *program, EmpDiags *diags);

#ifdef __cplusplus
}
#endif
