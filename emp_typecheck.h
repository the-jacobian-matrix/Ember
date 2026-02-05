#pragma once

#include "emp_ast.h"

#ifdef __cplusplus
extern "C" {
#endif

// Type checking + minimal inference.
//
// Notes:
// - Emits diagnostics into `diags` (strings allocated in `arena`).
// - May rewrite some `auto` types into concrete types (e.g., local vars with initializers).
void emp_sem_typecheck(EmpArena *arena, EmpProgram *program, EmpDiags *diags);

#ifdef __cplusplus
}
#endif
