; emp_codegen: emp_items=14
; emp_codegen: fn_protos=11
; ModuleID = 'src\main.em'
source_filename = "src\\main.em"

@_fltused = global i32 0
@.str.0 = private unnamed_addr constant [7 x i8] c"FAILED\00", align 1
@.str.1 = private unnamed_addr constant [70 x i8] c"EMP XOR neural net demo\0A(module-based; uses a tiny MLP forward pass)\0A\00", align 1
@.str.2 = private unnamed_addr constant [12 x i8] c"case 0,0 OK\00", align 1
@.str.3 = private unnamed_addr constant [12 x i8] c"case 1,0 OK\00", align 1
@.str.4 = private unnamed_addr constant [12 x i8] c"case 0,1 OK\00", align 1
@.str.5 = private unnamed_addr constant [12 x i8] c"case 1,1 OK\00", align 1
@.str.6 = private unnamed_addr constant [2 x i8] c"\0A\00", align 1

declare void @ExitProcess(i32)

define void @check(double %0, double %1, i32 %2, ptr %3) {
entry:
  %got = alloca i32, align 4
  %x0 = alloca double, align 8
  store double %0, ptr %x0, align 8
  %x1 = alloca double, align 8
  store double %1, ptr %x1, align 8
  %expected = alloca i32, align 4
  store i32 %2, ptr %expected, align 4
  %label = alloca ptr, align 8
  store ptr %3, ptr %label, align 8
  %x01 = load double, ptr %x0, align 8
  %x12 = load double, ptr %x1, align 8
  %calltmp = call i32 @xor_predict(double %x01, double %x12)
  store i32 %calltmp, ptr %got, align 4
  %got3 = load i32, ptr %got, align 4
  %expected4 = load i32, ptr %expected, align 4
  %eqtmp = icmp eq i32 %got3, %expected4
  %boolz = zext i1 %eqtmp to i8
  %ifcnd = icmp ne i8 %boolz, 0
  br i1 %ifcnd, label %then, label %else

then:                                             ; preds = %entry
  %label5 = load ptr, ptr %label, align 8
  call void @println(ptr %label5)
  br label %ifend

else:                                             ; preds = %entry
  call void @println(ptr @.str.0)
  br label %ifend

ifend:                                            ; preds = %else, %then
  ret void
}

define i32 @main() {
entry:
  call void @println(ptr @.str.1)
  call void @check(double 0.000000e+00, double 0.000000e+00, i32 0, ptr @.str.2)
  call void @check(double 1.000000e+00, double 0.000000e+00, i32 1, ptr @.str.3)
  call void @check(double 0.000000e+00, double 1.000000e+00, i32 1, ptr @.str.4)
  call void @check(double 1.000000e+00, double 1.000000e+00, i32 0, ptr @.str.5)
  ret i32 0
}

declare ptr @GetStdHandle(i32)

declare i32 @lstrlenA(ptr)

declare i32 @WriteFile(ptr, ptr, i32, ptr, ptr)

define void @print(ptr %0) {
entry:
  %written = alloca i32, align 4
  %n = alloca i32, align 4
  %h = alloca ptr, align 8
  %s = alloca ptr, align 8
  store ptr %0, ptr %s, align 8
  %calltmp = call ptr @GetStdHandle(i32 -11)
  store ptr %calltmp, ptr %h, align 8
  %s1 = load ptr, ptr %s, align 8
  %calltmp2 = call i32 @lstrlenA(ptr %s1)
  store i32 %calltmp2, ptr %n, align 4
  store i32 0, ptr %written, align 4
  %h3 = load ptr, ptr %h, align 8
  %s4 = load ptr, ptr %s, align 8
  %n5 = load i32, ptr %n, align 4
  %written6 = load i32, ptr %written, align 4
  %calltmp7 = call i32 @WriteFile(ptr %h3, ptr %s4, i32 %n5, ptr %written, ptr null)
  ret void
}

define void @println(ptr %0) {
entry:
  %s = alloca ptr, align 8
  store ptr %0, ptr %s, align 8
  %s1 = load ptr, ptr %s, align 8
  call void @print(ptr %s1)
  call void @print(ptr @.str.6)
  ret void
}

define double @xor_forward(double %0, double %1) {
entry:
  %out = alloca double, align 8
  %s1 = alloca double, align 8
  %s0 = alloca double, align 8
  %h = alloca [2 x double], align 8
  %x = alloca [2 x double], align 8
  %x0 = alloca double, align 8
  store double %0, ptr %x0, align 8
  %x1 = alloca double, align 8
  store double %1, ptr %x1, align 8
  %elem.ptr = getelementptr inbounds [2 x double], ptr %x, i32 0, i32 0
  %x01 = load double, ptr %x0, align 8
  store double %x01, ptr %elem.ptr, align 8
  %elem.ptr2 = getelementptr inbounds [2 x double], ptr %x, i32 0, i32 1
  %x13 = load double, ptr %x1, align 8
  store double %x13, ptr %elem.ptr2, align 8
  %elem.ptr4 = getelementptr inbounds [2 x double], ptr %x, i32 0, i32 0
  %idx = load double, ptr %elem.ptr4, align 8
  %fmultmp = fmul double %idx, 1.000000e+00
  %elem.ptr5 = getelementptr inbounds [2 x double], ptr %x, i32 0, i32 1
  %idx6 = load double, ptr %elem.ptr5, align 8
  %fmultmp7 = fmul double %idx6, 1.000000e+00
  %faddtmp = fadd double %fmultmp, %fmultmp7
  %faddtmp8 = fadd double %faddtmp, -5.000000e-01
  store double %faddtmp8, ptr %s0, align 8
  %elem.ptr9 = getelementptr inbounds [2 x double], ptr %x, i32 0, i32 0
  %idx10 = load double, ptr %elem.ptr9, align 8
  %fmultmp11 = fmul double %idx10, 1.000000e+00
  %elem.ptr12 = getelementptr inbounds [2 x double], ptr %x, i32 0, i32 1
  %idx13 = load double, ptr %elem.ptr12, align 8
  %fmultmp14 = fmul double %idx13, 1.000000e+00
  %faddtmp15 = fadd double %fmultmp11, %fmultmp14
  %faddtmp16 = fadd double %faddtmp15, -1.500000e+00
  store double %faddtmp16, ptr %s1, align 8
  %elem.ptr17 = getelementptr inbounds [2 x double], ptr %h, i32 0, i32 0
  %s018 = load double, ptr %s0, align 8
  %calltmp = call double @step01(double %s018)
  store double %calltmp, ptr %elem.ptr17, align 8
  %elem.ptr19 = getelementptr inbounds [2 x double], ptr %h, i32 0, i32 1
  %s120 = load double, ptr %s1, align 8
  %calltmp21 = call double @step01(double %s120)
  store double %calltmp21, ptr %elem.ptr19, align 8
  %elem.ptr22 = getelementptr inbounds [2 x double], ptr %h, i32 0, i32 0
  %idx23 = load double, ptr %elem.ptr22, align 8
  %fmultmp24 = fmul double %idx23, 1.000000e+00
  %elem.ptr25 = getelementptr inbounds [2 x double], ptr %h, i32 0, i32 1
  %idx26 = load double, ptr %elem.ptr25, align 8
  %fmultmp27 = fmul double %idx26, -2.000000e+00
  %faddtmp28 = fadd double %fmultmp24, %fmultmp27
  %faddtmp29 = fadd double %faddtmp28, -5.000000e-01
  store double %faddtmp29, ptr %out, align 8
  %out30 = load double, ptr %out, align 8
  %calltmp31 = call double @step01(double %out30)
  ret double %calltmp31
}

define i32 @xor_predict(double %0, double %1) {
entry:
  %y = alloca double, align 8
  %x0 = alloca double, align 8
  store double %0, ptr %x0, align 8
  %x1 = alloca double, align 8
  store double %1, ptr %x1, align 8
  %x01 = load double, ptr %x0, align 8
  %x12 = load double, ptr %x1, align 8
  %calltmp = call double @xor_forward(double %x01, double %x12)
  store double %calltmp, ptr %y, align 8
  %y3 = load double, ptr %y, align 8
  %fgttmp = fcmp ogt double %y3, 5.000000e-01
  %boolz = zext i1 %fgttmp to i8
  %ifcnd = icmp ne i8 %boolz, 0
  br i1 %ifcnd, label %then, label %else

then:                                             ; preds = %entry
  ret i32 1

else:                                             ; preds = %entry
  ret i32 0

ifend:                                            ; No predecessors!
  ret i32 0
}

define double @relu(double %0) {
entry:
  %x = alloca double, align 8
  store double %0, ptr %x, align 8
  %x1 = load double, ptr %x, align 8
  %flttmp = fcmp olt double %x1, 0.000000e+00
  %boolz = zext i1 %flttmp to i8
  %ifcnd = icmp ne i8 %boolz, 0
  br i1 %ifcnd, label %then, label %else

then:                                             ; preds = %entry
  ret double 0.000000e+00

else:                                             ; preds = %entry
  %x2 = load double, ptr %x, align 8
  ret double %x2

ifend:                                            ; No predecessors!
  ret double 0.000000e+00
}

define double @step01(double %0) {
entry:
  %x = alloca double, align 8
  store double %0, ptr %x, align 8
  %x1 = load double, ptr %x, align 8
  %fgttmp = fcmp ogt double %x1, 0.000000e+00
  %boolz = zext i1 %fgttmp to i8
  %ifcnd = icmp ne i8 %boolz, 0
  br i1 %ifcnd, label %then, label %else

then:                                             ; preds = %entry
  ret double 1.000000e+00

else:                                             ; preds = %entry
  ret double 0.000000e+00

ifend:                                            ; No predecessors!
  ret double 0.000000e+00
}

define void @mainCRTStartup() {
entry:
  %rc = call i32 @main()
  call void @ExitProcess(i32 %rc)
  unreachable
}
