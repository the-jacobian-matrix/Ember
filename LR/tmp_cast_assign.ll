; emp_codegen: emp_items=1
; emp_codegen: fn_protos=1
; ModuleID = 'tmp_cast_assign.em'
source_filename = "tmp_cast_assign.em"

@_fltused = global i32 0

declare void @ExitProcess(i32)

define i32 @main() {
entry:
  %y = alloca i32, align 4
  %d = alloca double, align 8
  %f = alloca float, align 4
  %x = alloca i32, align 4
  store i32 1, ptr %x, align 4
  %cur = load i32, ptr %x, align 4
  %addas = add i32 %cur, 2
  store i32 %addas, ptr %x, align 4
  %x1 = load i32, ptr %x, align 4
  %sitofp = sitofp i32 %x1 to float
  store float %sitofp, ptr %f, align 4
  %f2 = load float, ptr %f, align 4
  %fpext = fpext float %f2 to double
  store double %fpext, ptr %d, align 8
  %d3 = load double, ptr %d, align 8
  %fptosi = fptosi double %d3 to i32
  store i32 %fptosi, ptr %y, align 4
  %y4 = load i32, ptr %y, align 4
  ret i32 %y4
}

define void @mainCRTStartup() {
entry:
  %rc = call i32 @main()
  call void @ExitProcess(i32 %rc)
  unreachable
}
