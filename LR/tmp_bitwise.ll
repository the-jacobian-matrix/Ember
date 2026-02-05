; emp_codegen: emp_items=1
; emp_codegen: fn_protos=1
; ModuleID = '.\tmp_bitwise.em'
source_filename = ".\\tmp_bitwise.em"

@_fltused = global i32 0

declare void @ExitProcess(i32)

define i32 @main() {
entry:
  %z = alloca i32, align 4
  %r = alloca i32, align 4
  %y = alloca i32, align 4
  %x = alloca i32, align 4
  store i32 1, ptr %x, align 4
  store i32 0, ptr %y, align 4
  %y1 = load i32, ptr %y, align 4
  %x2 = load i32, ptr %x, align 4
  %shltmp = shl i32 %x2, 3
  %ortmp = or i32 %y1, %shltmp
  store i32 %ortmp, ptr %y, align 4
  %y3 = load i32, ptr %y, align 4
  %xortmp = xor i32 %y3, 2
  store i32 %xortmp, ptr %y, align 4
  store i32 1, ptr %r, align 4
  %r4 = load i32, ptr %r, align 4
  %nottmp = xor i32 %r4, -1
  store i32 %nottmp, ptr %z, align 4
  %y5 = load i32, ptr %y, align 4
  %netmp = icmp ne i32 %y5, 0
  %boolz = zext i1 %netmp to i8
  %sc.lhs = icmp ne i8 %boolz, 0
  br i1 %sc.lhs, label %and.rhs, label %and.end

and.rhs:                                          ; preds = %entry
  %z6 = load i32, ptr %z, align 4
  %netmp7 = icmp ne i32 %z6, 0
  %boolz8 = zext i1 %netmp7 to i8
  %sc.rhs = icmp ne i8 %boolz8, 0
  %sc.rhs8 = zext i1 %sc.rhs to i8
  br label %and.end

and.end:                                          ; preds = %and.rhs, %entry
  %andv = phi i8 [ 0, %entry ], [ %sc.rhs8, %and.rhs ]
  %ifcnd = icmp ne i8 %andv, 0
  br i1 %ifcnd, label %then, label %else

then:                                             ; preds = %and.end
  %y9 = load i32, ptr %y, align 4
  %r10 = load i32, ptr %r, align 4
  %addtmp = add i32 %y9, %r10
  ret i32 %addtmp

else:                                             ; preds = %and.end
  ret i32 0

ifend:                                            ; No predecessors!
  ret i32 0
}

define void @mainCRTStartup() {
entry:
  %rc = call i32 @main()
  call void @ExitProcess(i32 %rc)
  unreachable
}
