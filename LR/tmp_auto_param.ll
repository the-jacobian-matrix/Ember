; emp_codegen: emp_items=2
; emp_codegen: fn_protos=2
; ModuleID = '.\tmp_auto_param.em'
source_filename = ".\\tmp_auto_param.em"

@_fltused = global i32 0
@.str.0 = private unnamed_addr constant [3 x i8] c"hi\00", align 1

declare void @ExitProcess(i32)

define ptr @id(ptr %0) {
entry:
  %p = alloca ptr, align 8
  store ptr %0, ptr %p, align 8
  %p1 = load ptr, ptr %p, align 8
  ret ptr %p1
}

define i32 @main() {
entry:
  %t = alloca ptr, align 8
  %s = alloca ptr, align 8
  store ptr @.str.0, ptr %s, align 8
  %s1 = load ptr, ptr %s, align 8
  %calltmp = call ptr @id(ptr %s1)
  store ptr %calltmp, ptr %t, align 8
  %t2 = load ptr, ptr %t, align 8
  ret i32 0
}

define void @mainCRTStartup() {
entry:
  %rc = call i32 @main()
  call void @ExitProcess(i32 %rc)
  unreachable
}
