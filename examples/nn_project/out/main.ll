; emp_codegen: emp_items=10
; emp_codegen: fn_protos=9
; ModuleID = 'src\main.em'
source_filename = "src\\main.em"

@.str.0 = private unnamed_addr constant [7 x i8] c"FAILED\00", align 1
@.str.1 = private unnamed_addr constant [69 x i8] c"EMP XOR neural net demo\0D\0A(no floats yet; using integer thresholds)\0D\0A\00", align 1
@.str.2 = private unnamed_addr constant [12 x i8] c"case 0,0 OK\00", align 1
@.str.3 = private unnamed_addr constant [12 x i8] c"case 1,0 OK\00", align 1
@.str.4 = private unnamed_addr constant [12 x i8] c"case 0,1 OK\00", align 1
@.str.5 = private unnamed_addr constant [12 x i8] c"case 1,1 OK\00", align 1
@.str.6 = private unnamed_addr constant [2 x i8] c"\0A\00", align 1

declare void @ExitProcess(i32)

define i32 @step_gt(i32 %0, i32 %1) {
entry:
  %v = alloca i32, align 4
  store i32 %0, ptr %v, align 4
  %thr = alloca i32, align 4
  store i32 %1, ptr %thr, align 4
  %v1 = load i32, ptr %v, align 4
  %thr2 = load i32, ptr %thr, align 4
  %gttmp = icmp sgt i32 %v1, %thr2
  %boolz = zext i1 %gttmp to i8
  %ifcnd = icmp ne i8 %boolz, 0
  br i1 %ifcnd, label %then, label %else

then:                                             ; preds = %entry
  ret i32 1

else:                                             ; preds = %entry
  ret i32 0

ifend:                                            ; No predecessors!
  ret i32 0
}

define i32 @xor_nn(i32 %0, i32 %1) {
entry:
  %v = alloca i32, align 4
  %h2 = alloca i32, align 4
  %h1 = alloca i32, align 4
  %sum2 = alloca i32, align 4
  %x1 = alloca i32, align 4
  store i32 %0, ptr %x1, align 4
  %x2 = alloca i32, align 4
  store i32 %1, ptr %x2, align 4
  %x11 = load i32, ptr %x1, align 4
  %x22 = load i32, ptr %x2, align 4
  %addtmp = add i32 %x11, %x22
  %multmp = mul i32 2, %addtmp
  store i32 %multmp, ptr %sum2, align 4
  %sum23 = load i32, ptr %sum2, align 4
  %calltmp = call i32 @step_gt(i32 %sum23, i32 1)
  store i32 %calltmp, ptr %h1, align 4
  %sum24 = load i32, ptr %sum2, align 4
  %calltmp5 = call i32 @step_gt(i32 %sum24, i32 3)
  store i32 %calltmp5, ptr %h2, align 4
  %h16 = load i32, ptr %h1, align 4
  %multmp7 = mul i32 2, %h16
  %h28 = load i32, ptr %h2, align 4
  %multmp9 = mul i32 4, %h28
  %subtmp = sub i32 %multmp7, %multmp9
  store i32 %subtmp, ptr %v, align 4
  %v10 = load i32, ptr %v, align 4
  %calltmp11 = call i32 @step_gt(i32 %v10, i32 1)
  ret i32 %calltmp11
}

define void @check(i32 %0, i32 %1, i32 %2, ptr %3) {
entry:
  %got = alloca i32, align 4
  %x1 = alloca i32, align 4
  store i32 %0, ptr %x1, align 4
  %x2 = alloca i32, align 4
  store i32 %1, ptr %x2, align 4
  %expected = alloca i32, align 4
  store i32 %2, ptr %expected, align 4
  %label = alloca ptr, align 8
  store ptr %3, ptr %label, align 8
  %x11 = load i32, ptr %x1, align 4
  %x22 = load i32, ptr %x2, align 4
  %calltmp = call i32 @xor_nn(i32 %x11, i32 %x22)
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
  call void @check(i32 0, i32 0, i32 0, ptr @.str.2)
  call void @check(i32 1, i32 0, i32 1, ptr @.str.3)
  call void @check(i32 0, i32 1, i32 1, ptr @.str.4)
  call void @check(i32 1, i32 1, i32 0, ptr @.str.5)
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

define void @mainCRTStartup() {
entry:
  %rc = call i32 @main()
  call void @ExitProcess(i32 %rc)
  unreachable
}
