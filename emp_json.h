#pragma once

#include "emp_parser.h"

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

void emp_program_to_json(FILE *out, const EmpProgram *p, const EmpDiags *diags);

#ifdef __cplusplus
}
#endif
