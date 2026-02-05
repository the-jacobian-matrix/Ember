#pragma once

#include "emp_ast.h"

#ifdef __cplusplus
extern "C" {
#endif

// Drop insertion pass:
// - inserts explicit `drop` statements for owned bindings
// - runs after ownership+borrow checking (but does not rely on type checking)
// - does NOT insert drops inside `@emp off` blocks
//
// Diagnostics are appended to `diags` and message strings are allocated in `arena`.
void emp_sem_insert_drops(EmpArena *arena, EmpProgram *program, EmpDiags *diags);

#ifdef __cplusplus
}
#endif
