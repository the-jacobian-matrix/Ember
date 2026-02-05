; emp_codegen: emp_items=1
; emp_codegen: fn_protos=1
; ModuleID = '.\tmp_char_match.em'
source_filename = ".\\tmp_char_match.em"

@_fltused = global i32 0

declare void @ExitProcess(i32)

define i32 @main() {
entry:
  %c = alloca i8, align 1
  store i8 97, ptr %c, align 1
  %c1 = load i8, ptr %c, align 1
  %mieq = icmp eq i8 %c1, 97
  br i1 %mieq, label %match.arm, label %match.next

match.end:                                        ; No predecessors!
  ret i32 0

match.arm:                                        ; preds = %entry
  ret i32 1

match.next:                                       ; preds = %entry
  br label %match.else

match.else:                                       ; preds = %match.next
  ret i32 0
}

define void @mainCRTStartup() {
entry:
  %rc = call i32 @main()
  call void @ExitProcess(i32 %rc)
  unreachable
}
