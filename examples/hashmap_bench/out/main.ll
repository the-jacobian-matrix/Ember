; emp_codegen: emp_items=12
; emp_codegen: fn_protos=12
; ModuleID = 'main.em'
source_filename = "main.em"

%emp.string = type { ptr, i32, i32 }

@_fltused = global i32 0
@.str.0 = private unnamed_addr constant [2 x i8] c"\0A\00", align 1
@.str.1 = private unnamed_addr constant [2 x i8] c"\0A\00", align 1
@.fstr.1000000 = private unnamed_addr constant [1 x i8] zeroinitializer, align 1
@.fstr.1000001 = private unnamed_addr constant [7 x i8] c"<null>\00", align 1
@.fstr.1000002 = private unnamed_addr constant [5 x i8] c"true\00", align 1
@.fstr.1000003 = private unnamed_addr constant [6 x i8] c"false\00", align 1
@.fstr.1000004 = private unnamed_addr constant [11 x i8] c"init_used=\00", align 1
@.fstr.1000005 = private unnamed_addr constant [1 x i8] zeroinitializer, align 1
@.fstr.1000006 = private unnamed_addr constant [7 x i8] c"<null>\00", align 1
@.fstr.1000007 = private unnamed_addr constant [5 x i8] c"true\00", align 1
@.fstr.1000008 = private unnamed_addr constant [6 x i8] c"false\00", align 1
@.fstr.1000009 = private unnamed_addr constant [10 x i8] c"inserted=\00", align 1
@.fstr.1000010 = private unnamed_addr constant [10 x i8] c" time_ms=\00", align 1
@.str.2 = private unnamed_addr constant [14 x i8] c"hashmap_bench\00", align 1
@.fstr.1000011 = private unnamed_addr constant [1 x i8] zeroinitializer, align 1
@.fstr.1000012 = private unnamed_addr constant [7 x i8] c"<null>\00", align 1
@.fstr.1000013 = private unnamed_addr constant [5 x i8] c"true\00", align 1
@.fstr.1000014 = private unnamed_addr constant [6 x i8] c"false\00", align 1
@.fstr.1000015 = private unnamed_addr constant [16 x i8] c"entries_target=\00", align 1
@.fstr.1000016 = private unnamed_addr constant [14 x i8] c" entries_run=\00", align 1
@.fstr.1000017 = private unnamed_addr constant [11 x i8] c" inserted=\00", align 1
@.fstr.1000018 = private unnamed_addr constant [6 x i8] c" cap=\00", align 1
@.fstr.1000019 = private unnamed_addr constant [10 x i8] c" time_ms=\00", align 1
@.fstr.1000020 = private unnamed_addr constant [1 x i8] zeroinitializer, align 1
@.fstr.1000021 = private unnamed_addr constant [7 x i8] c"<null>\00", align 1
@.fstr.1000022 = private unnamed_addr constant [5 x i8] c"true\00", align 1
@.fstr.1000023 = private unnamed_addr constant [6 x i8] c"false\00", align 1
@.fstr.1000024 = private unnamed_addr constant [10 x i8] c"checksum=\00", align 1

declare void @ExitProcess(i32)

declare ptr @GetStdHandle(i32)

declare i32 @lstrlenA(ptr)

declare i32 @WriteFile(ptr, ptr, i32, ptr, ptr)

declare i32 @QueryPerformanceCounter(ptr)

declare i32 @QueryPerformanceFrequency(ptr)

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

define void @println__PNu8(ptr %0) {
entry:
  %s = alloca ptr, align 8
  store ptr %0, ptr %s, align 8
  %s1 = load ptr, ptr %s, align 8
  call void @print(ptr %s1)
  call void @print(ptr @.str.0)
  ret void
}

define void @println__Nstring(%emp.string %0) {
entry:
  %p = alloca ptr, align 8
  %s = alloca %emp.string, align 8
  store %emp.string %0, ptr %s, align 8
  %s1 = load %emp.string, ptr %s, align 8
  %ptr = extractvalue %emp.string %s1, 0
  store ptr %ptr, ptr %p, align 8
  %p2 = load ptr, ptr %p, align 8
  call void @print(ptr %p2)
  call void @print(ptr @.str.1)
  %str.cur = load %emp.string, ptr %s, align 8
  %str.ptr = extractvalue %emp.string %str.cur, 0
  %str.isnull = icmp eq ptr %str.ptr, null
  br i1 %str.isnull, label %str.drop.end, label %str.drop

str.drop:                                         ; preds = %entry
  %heap = call ptr @GetProcessHeap()
  %1 = call i32 @HeapFree(ptr %heap, i32 0, ptr %str.ptr)
  br label %str.drop.end

str.drop.end:                                     ; preds = %str.drop, %entry
  store %emp.string zeroinitializer, ptr %s, align 8
  ret void
}

define i64 @now_ticks() {
entry:
  %t = alloca i64, align 8
  store i64 0, ptr %t, align 4
  %t1 = load i64, ptr %t, align 4
  %calltmp = call i32 @QueryPerformanceCounter(ptr %t)
  %t2 = load i64, ptr %t, align 4
  ret i64 %t2
}

define i64 @qpc_freq() {
entry:
  %f = alloca i64, align 8
  store i64 0, ptr %f, align 4
  %f1 = load i64, ptr %f, align 4
  %calltmp = call i32 @QueryPerformanceFrequency(ptr %f)
  %f2 = load i64, ptr %f, align 4
  ret i64 %f2
}

define i64 @splitmix64(i64 %0) {
entry:
  %z24 = alloca i64, align 8
  %z1 = alloca i64, align 8
  %z = alloca i64, align 8
  %x = alloca i64, align 8
  store i64 %0, ptr %x, align 4
  %x1 = load i64, ptr %x, align 4
  %addtmp = add i64 %x1, 0
  store i64 %addtmp, ptr %z, align 4
  %z2 = load i64, ptr %z, align 4
  %z3 = load i64, ptr %z, align 4
  %shrtmp = ashr i64 %z3, 30
  %xortmp = xor i64 %z2, %shrtmp
  %multmp = mul i64 %xortmp, 0
  store i64 %multmp, ptr %z1, align 4
  %z15 = load i64, ptr %z1, align 4
  %z16 = load i64, ptr %z1, align 4
  %shrtmp7 = ashr i64 %z16, 27
  %xortmp8 = xor i64 %z15, %shrtmp7
  %multmp9 = mul i64 %xortmp8, 0
  store i64 %multmp9, ptr %z24, align 4
  %z210 = load i64, ptr %z24, align 4
  %z211 = load i64, ptr %z24, align 4
  %shrtmp12 = ashr i64 %z211, 31
  %xortmp13 = xor i64 %z210, %shrtmp12
  ret i64 %xortmp13
}

define i32 @main() {
entry:
  %fstr.cpy.i1821 = alloca i32, align 4
  %fstr.cpy.i1761 = alloca i32, align 4
  %fstr.itoa.cur1694 = alloca i64, align 8
  %fstr.itoa.idx1693 = alloca i32, align 4
  %fstr.cpy.i1675 = alloca i32, align 4
  %fstr.buf1632 = alloca %emp.string, align 8
  %fstr.itoa.buf1633 = alloca [32 x i8], align 1
  %msg1631 = alloca %emp.string, align 8
  %checksum = alloca i32, align 4
  %fstr.cpy.i1603 = alloca i32, align 4
  %fstr.cpy.i1543 = alloca i32, align 4
  %fstr.itoa.cur1476 = alloca i64, align 8
  %fstr.itoa.idx1475 = alloca i32, align 4
  %fstr.cpy.i1458 = alloca i32, align 4
  %fstr.cpy.i1407 = alloca i32, align 4
  %fstr.itoa.cur1340 = alloca i64, align 8
  %fstr.itoa.idx1339 = alloca i32, align 4
  %fstr.cpy.i1321 = alloca i32, align 4
  %fstr.cpy.i1270 = alloca i32, align 4
  %fstr.itoa.cur1203 = alloca i64, align 8
  %fstr.itoa.idx1202 = alloca i32, align 4
  %fstr.cpy.i1184 = alloca i32, align 4
  %fstr.cpy.i1133 = alloca i32, align 4
  %fstr.itoa.cur1066 = alloca i64, align 8
  %fstr.itoa.idx1065 = alloca i32, align 4
  %fstr.cpy.i1047 = alloca i32, align 4
  %fstr.cpy.i996 = alloca i32, align 4
  %fstr.itoa.cur929 = alloca i64, align 8
  %fstr.itoa.idx928 = alloca i32, align 4
  %fstr.cpy.i910 = alloca i32, align 4
  %fstr.buf867 = alloca %emp.string, align 8
  %fstr.itoa.buf868 = alloca [32 x i8], align 1
  %msg866 = alloca %emp.string, align 8
  %ms = alloca i64, align 8
  %dt = alloca i64, align 8
  %t1 = alloca i64, align 8
  %fstr.cpy.i840 = alloca i32, align 4
  %fstr.cpy.i780 = alloca i32, align 4
  %fstr.itoa.cur713 = alloca i64, align 8
  %fstr.itoa.idx712 = alloca i32, align 4
  %fstr.cpy.i695 = alloca i32, align 4
  %fstr.cpy.i644 = alloca i32, align 4
  %fstr.itoa.cur577 = alloca i64, align 8
  %fstr.itoa.idx576 = alloca i32, align 4
  %fstr.cpy.i558 = alloca i32, align 4
  %fstr.buf515 = alloca %emp.string, align 8
  %fstr.itoa.buf516 = alloca [32 x i8], align 1
  %msg514 = alloca %emp.string, align 8
  %ms_now = alloca i64, align 8
  %t = alloca i64, align 8
  %idx = alloca i32, align 4
  %probes = alloca i32, align 4
  %pos = alloca i32, align 4
  %step = alloca i32, align 4
  %h = alloca i64, align 8
  %val = alloca i32, align 4
  %key = alloca i32, align 4
  %next_report = alloca i32, align 4
  %k = alloca i32, align 4
  %len411 = alloca i32, align 4
  %t0 = alloca i64, align 8
  %freq = alloca i64, align 8
  %seed = alloca i64, align 8
  %fstr.cpy.i394 = alloca i32, align 4
  %fstr.cpy.i341 = alloca i32, align 4
  %fstr.itoa.cur = alloca i64, align 8
  %fstr.itoa.idx = alloca i32, align 4
  %fstr.cpy.i = alloca i32, align 4
  %fstr.buf = alloca %emp.string, align 8
  %fstr.itoa.buf = alloca [32 x i8], align 1
  %msg = alloca %emp.string, align 8
  %next_init_report = alloca i32, align 4
  %i = alloca i32, align 4
  %_u0 = alloca i8, align 1
  %_v0 = alloca i32, align 4
  %_k0 = alloca i32, align 4
  %listlit88 = alloca { ptr, i32, i32 }, align 8
  %used = alloca { ptr, i32, i32 }, align 8
  %listlit34 = alloca { ptr, i32, i32 }, align 8
  %vals = alloca { ptr, i32, i32 }, align 8
  %listlit = alloca { ptr, i32, i32 }, align 8
  %keys = alloca { ptr, i32, i32 }, align 8
  %random_until = alloca i32, align 4
  %mask = alloca i32, align 4
  %cap = alloca i32, align 4
  %need = alloca i32, align 4
  %progress_step = alloca i32, align 4
  %entries_run = alloca i32, align 4
  %entries_max = alloca i32, align 4
  store i32 10000000, ptr %entries_max, align 4
  store i32 1000000, ptr %entries_run, align 4
  store i32 1000, ptr %progress_step, align 4
  %entries_run1 = load i32, ptr %entries_run, align 4
  %multmp = mul i32 %entries_run1, 4
  %divtmp = sdiv i32 %multmp, 3
  %addtmp = add i32 %divtmp, 1
  store i32 %addtmp, ptr %need, align 4
  store i32 1, ptr %cap, align 4
  br label %while.cond

while.cond:                                       ; preds = %while.body, %entry
  %cap2 = load i32, ptr %cap, align 4
  %need3 = load i32, ptr %need, align 4
  %lttmp = icmp slt i32 %cap2, %need3
  %boolz = zext i1 %lttmp to i8
  %whilecnd = icmp ne i8 %boolz, 0
  br i1 %whilecnd, label %while.body, label %while.end

while.body:                                       ; preds = %while.cond
  %cap4 = load i32, ptr %cap, align 4
  %shltmp = shl i32 %cap4, 1
  store i32 %shltmp, ptr %cap, align 4
  br label %while.cond

while.end:                                        ; preds = %while.cond
  %cap5 = load i32, ptr %cap, align 4
  %subtmp = sub i32 %cap5, 1
  store i32 %subtmp, ptr %mask, align 4
  %cap6 = load i32, ptr %cap, align 4
  %multmp7 = mul i32 %cap6, 3
  %divtmp8 = sdiv i32 %multmp7, 4
  store i32 %divtmp8, ptr %random_until, align 4
  %list.ptr.addr = getelementptr inbounds nuw { ptr, i32, i32 }, ptr %listlit, i32 0, i32 0
  %list.len.addr = getelementptr inbounds nuw { ptr, i32, i32 }, ptr %listlit, i32 0, i32 1
  %list.cap.addr = getelementptr inbounds nuw { ptr, i32, i32 }, ptr %listlit, i32 0, i32 2
  store ptr null, ptr %list.ptr.addr, align 8
  store i32 0, ptr %list.len.addr, align 4
  store i32 0, ptr %list.cap.addr, align 4
  %list.ptr.addr9 = getelementptr inbounds nuw { ptr, i32, i32 }, ptr %listlit, i32 0, i32 0
  %list.cap.addr10 = getelementptr inbounds nuw { ptr, i32, i32 }, ptr %listlit, i32 0, i32 2
  %cap11 = load i32, ptr %list.cap.addr10, align 4
  %needcap = icmp ugt i32 1, %cap11
  br i1 %needcap, label %list.reserve, label %list.reserve.end

list.reserve:                                     ; preds = %while.end
  %heap = call ptr @GetProcessHeap()
  %ptr = load ptr, ptr %list.ptr.addr9, align 8
  %isnull = icmp eq ptr %ptr, null
  br i1 %isnull, label %list.alloc, label %list.realloc

list.reserve.end:                                 ; preds = %list.alloc.merge, %while.end
  %list.ptr.addr13 = getelementptr inbounds nuw { ptr, i32, i32 }, ptr %listlit, i32 0, i32 0
  %list.len.addr14 = getelementptr inbounds nuw { ptr, i32, i32 }, ptr %listlit, i32 0, i32 1
  %list.cap.addr15 = getelementptr inbounds nuw { ptr, i32, i32 }, ptr %listlit, i32 0, i32 2
  %len = load i32, ptr %list.len.addr14, align 4
  %cap16 = load i32, ptr %list.cap.addr15, align 4
  %needgrow = icmp uge i32 %len, %cap16
  br i1 %needgrow, label %list.grow, label %list.push.cont

list.alloc:                                       ; preds = %list.reserve
  %mem = call ptr @HeapAlloc(ptr %heap, i32 0, i64 ptrtoint (ptr getelementptr (i32, ptr null, i32 1) to i64))
  br label %list.alloc.merge

list.realloc:                                     ; preds = %list.reserve
  %mem12 = call ptr @HeapReAlloc(ptr %heap, i32 0, ptr %ptr, i64 ptrtoint (ptr getelementptr (i32, ptr null, i32 1) to i64))
  br label %list.alloc.merge

list.alloc.merge:                                 ; preds = %list.realloc, %list.alloc
  %memphi = phi ptr [ %mem, %list.alloc ], [ %mem12, %list.realloc ]
  store ptr %memphi, ptr %list.ptr.addr9, align 8
  store i32 1, ptr %list.cap.addr10, align 4
  br label %list.reserve.end

list.grow:                                        ; preds = %list.reserve.end
  %cap0 = icmp eq i32 %cap16, 0
  %cap217 = mul i32 %cap16, 2
  %newcap = select i1 %cap0, i32 4, i32 %cap217
  %need1 = add i32 %len, 1
  %caplt = icmp ult i32 %newcap, %need1
  %finalcap = select i1 %caplt, i32 %need1, i32 %newcap
  %list.ptr.addr18 = getelementptr inbounds nuw { ptr, i32, i32 }, ptr %listlit, i32 0, i32 0
  %list.cap.addr19 = getelementptr inbounds nuw { ptr, i32, i32 }, ptr %listlit, i32 0, i32 2
  %cap20 = load i32, ptr %list.cap.addr19, align 4
  %needcap21 = icmp ugt i32 %finalcap, %cap20
  br i1 %needcap21, label %list.reserve22, label %list.reserve.end23

list.push.cont:                                   ; preds = %list.reserve.end23, %list.reserve.end
  %ptr33 = load ptr, ptr %list.ptr.addr13, align 8
  %push.elem.ptr = getelementptr inbounds i32, ptr %ptr33, i32 %len
  store i32 0, ptr %push.elem.ptr, align 4
  %len.next = add i32 %len, 1
  store i32 %len.next, ptr %list.len.addr14, align 4
  %listlit.val = load { ptr, i32, i32 }, ptr %listlit, align 8
  store { ptr, i32, i32 } %listlit.val, ptr %keys, align 8
  %list.ptr.addr35 = getelementptr inbounds nuw { ptr, i32, i32 }, ptr %listlit34, i32 0, i32 0
  %list.len.addr36 = getelementptr inbounds nuw { ptr, i32, i32 }, ptr %listlit34, i32 0, i32 1
  %list.cap.addr37 = getelementptr inbounds nuw { ptr, i32, i32 }, ptr %listlit34, i32 0, i32 2
  store ptr null, ptr %list.ptr.addr35, align 8
  store i32 0, ptr %list.len.addr36, align 4
  store i32 0, ptr %list.cap.addr37, align 4
  %list.ptr.addr38 = getelementptr inbounds nuw { ptr, i32, i32 }, ptr %listlit34, i32 0, i32 0
  %list.cap.addr39 = getelementptr inbounds nuw { ptr, i32, i32 }, ptr %listlit34, i32 0, i32 2
  %cap40 = load i32, ptr %list.cap.addr39, align 4
  %needcap41 = icmp ugt i32 1, %cap40
  br i1 %needcap41, label %list.reserve42, label %list.reserve.end43

list.reserve22:                                   ; preds = %list.grow
  %cap64 = zext i32 %finalcap to i64
  %bytes = mul i64 %cap64, ptrtoint (ptr getelementptr (i32, ptr null, i32 1) to i64)
  %heap24 = call ptr @GetProcessHeap()
  %ptr25 = load ptr, ptr %list.ptr.addr18, align 8
  %isnull26 = icmp eq ptr %ptr25, null
  br i1 %isnull26, label %list.alloc27, label %list.realloc28

list.reserve.end23:                               ; preds = %list.alloc.merge29, %list.grow
  br label %list.push.cont

list.alloc27:                                     ; preds = %list.reserve22
  %mem30 = call ptr @HeapAlloc(ptr %heap24, i32 0, i64 %bytes)
  br label %list.alloc.merge29

list.realloc28:                                   ; preds = %list.reserve22
  %mem31 = call ptr @HeapReAlloc(ptr %heap24, i32 0, ptr %ptr25, i64 %bytes)
  br label %list.alloc.merge29

list.alloc.merge29:                               ; preds = %list.realloc28, %list.alloc27
  %memphi32 = phi ptr [ %mem30, %list.alloc27 ], [ %mem31, %list.realloc28 ]
  store ptr %memphi32, ptr %list.ptr.addr18, align 8
  store i32 %finalcap, ptr %list.cap.addr19, align 4
  br label %list.reserve.end23

list.reserve42:                                   ; preds = %list.push.cont
  %heap44 = call ptr @GetProcessHeap()
  %ptr45 = load ptr, ptr %list.ptr.addr38, align 8
  %isnull46 = icmp eq ptr %ptr45, null
  br i1 %isnull46, label %list.alloc47, label %list.realloc48

list.reserve.end43:                               ; preds = %list.alloc.merge49, %list.push.cont
  %list.ptr.addr53 = getelementptr inbounds nuw { ptr, i32, i32 }, ptr %listlit34, i32 0, i32 0
  %list.len.addr54 = getelementptr inbounds nuw { ptr, i32, i32 }, ptr %listlit34, i32 0, i32 1
  %list.cap.addr55 = getelementptr inbounds nuw { ptr, i32, i32 }, ptr %listlit34, i32 0, i32 2
  %len56 = load i32, ptr %list.len.addr54, align 4
  %cap57 = load i32, ptr %list.cap.addr55, align 4
  %needgrow58 = icmp uge i32 %len56, %cap57
  br i1 %needgrow58, label %list.grow59, label %list.push.cont60

list.alloc47:                                     ; preds = %list.reserve42
  %mem50 = call ptr @HeapAlloc(ptr %heap44, i32 0, i64 ptrtoint (ptr getelementptr (i32, ptr null, i32 1) to i64))
  br label %list.alloc.merge49

list.realloc48:                                   ; preds = %list.reserve42
  %mem51 = call ptr @HeapReAlloc(ptr %heap44, i32 0, ptr %ptr45, i64 ptrtoint (ptr getelementptr (i32, ptr null, i32 1) to i64))
  br label %list.alloc.merge49

list.alloc.merge49:                               ; preds = %list.realloc48, %list.alloc47
  %memphi52 = phi ptr [ %mem50, %list.alloc47 ], [ %mem51, %list.realloc48 ]
  store ptr %memphi52, ptr %list.ptr.addr38, align 8
  store i32 1, ptr %list.cap.addr39, align 4
  br label %list.reserve.end43

list.grow59:                                      ; preds = %list.reserve.end43
  %cap061 = icmp eq i32 %cap57, 0
  %cap262 = mul i32 %cap57, 2
  %newcap63 = select i1 %cap061, i32 4, i32 %cap262
  %need164 = add i32 %len56, 1
  %caplt65 = icmp ult i32 %newcap63, %need164
  %finalcap66 = select i1 %caplt65, i32 %need164, i32 %newcap63
  %list.ptr.addr67 = getelementptr inbounds nuw { ptr, i32, i32 }, ptr %listlit34, i32 0, i32 0
  %list.cap.addr68 = getelementptr inbounds nuw { ptr, i32, i32 }, ptr %listlit34, i32 0, i32 2
  %cap69 = load i32, ptr %list.cap.addr68, align 4
  %needcap70 = icmp ugt i32 %finalcap66, %cap69
  br i1 %needcap70, label %list.reserve71, label %list.reserve.end72

list.push.cont60:                                 ; preds = %list.reserve.end72, %list.reserve.end43
  %ptr84 = load ptr, ptr %list.ptr.addr53, align 8
  %push.elem.ptr85 = getelementptr inbounds i32, ptr %ptr84, i32 %len56
  store i32 0, ptr %push.elem.ptr85, align 4
  %len.next86 = add i32 %len56, 1
  store i32 %len.next86, ptr %list.len.addr54, align 4
  %listlit.val87 = load { ptr, i32, i32 }, ptr %listlit34, align 8
  store { ptr, i32, i32 } %listlit.val87, ptr %vals, align 8
  %list.ptr.addr89 = getelementptr inbounds nuw { ptr, i32, i32 }, ptr %listlit88, i32 0, i32 0
  %list.len.addr90 = getelementptr inbounds nuw { ptr, i32, i32 }, ptr %listlit88, i32 0, i32 1
  %list.cap.addr91 = getelementptr inbounds nuw { ptr, i32, i32 }, ptr %listlit88, i32 0, i32 2
  store ptr null, ptr %list.ptr.addr89, align 8
  store i32 0, ptr %list.len.addr90, align 4
  store i32 0, ptr %list.cap.addr91, align 4
  %list.ptr.addr92 = getelementptr inbounds nuw { ptr, i32, i32 }, ptr %listlit88, i32 0, i32 0
  %list.cap.addr93 = getelementptr inbounds nuw { ptr, i32, i32 }, ptr %listlit88, i32 0, i32 2
  %cap94 = load i32, ptr %list.cap.addr93, align 4
  %needcap95 = icmp ugt i32 1, %cap94
  br i1 %needcap95, label %list.reserve96, label %list.reserve.end97

list.reserve71:                                   ; preds = %list.grow59
  %cap6473 = zext i32 %finalcap66 to i64
  %bytes74 = mul i64 %cap6473, ptrtoint (ptr getelementptr (i32, ptr null, i32 1) to i64)
  %heap75 = call ptr @GetProcessHeap()
  %ptr76 = load ptr, ptr %list.ptr.addr67, align 8
  %isnull77 = icmp eq ptr %ptr76, null
  br i1 %isnull77, label %list.alloc78, label %list.realloc79

list.reserve.end72:                               ; preds = %list.alloc.merge80, %list.grow59
  br label %list.push.cont60

list.alloc78:                                     ; preds = %list.reserve71
  %mem81 = call ptr @HeapAlloc(ptr %heap75, i32 0, i64 %bytes74)
  br label %list.alloc.merge80

list.realloc79:                                   ; preds = %list.reserve71
  %mem82 = call ptr @HeapReAlloc(ptr %heap75, i32 0, ptr %ptr76, i64 %bytes74)
  br label %list.alloc.merge80

list.alloc.merge80:                               ; preds = %list.realloc79, %list.alloc78
  %memphi83 = phi ptr [ %mem81, %list.alloc78 ], [ %mem82, %list.realloc79 ]
  store ptr %memphi83, ptr %list.ptr.addr67, align 8
  store i32 %finalcap66, ptr %list.cap.addr68, align 4
  br label %list.reserve.end72

list.reserve96:                                   ; preds = %list.push.cont60
  %heap98 = call ptr @GetProcessHeap()
  %ptr99 = load ptr, ptr %list.ptr.addr92, align 8
  %isnull100 = icmp eq ptr %ptr99, null
  br i1 %isnull100, label %list.alloc101, label %list.realloc102

list.reserve.end97:                               ; preds = %list.alloc.merge103, %list.push.cont60
  %list.ptr.addr107 = getelementptr inbounds nuw { ptr, i32, i32 }, ptr %listlit88, i32 0, i32 0
  %list.len.addr108 = getelementptr inbounds nuw { ptr, i32, i32 }, ptr %listlit88, i32 0, i32 1
  %list.cap.addr109 = getelementptr inbounds nuw { ptr, i32, i32 }, ptr %listlit88, i32 0, i32 2
  %len110 = load i32, ptr %list.len.addr108, align 4
  %cap111 = load i32, ptr %list.cap.addr109, align 4
  %needgrow112 = icmp uge i32 %len110, %cap111
  br i1 %needgrow112, label %list.grow113, label %list.push.cont114

list.alloc101:                                    ; preds = %list.reserve96
  %mem104 = call ptr @HeapAlloc(ptr %heap98, i32 0, i64 ptrtoint (ptr getelementptr (i8, ptr null, i32 1) to i64))
  br label %list.alloc.merge103

list.realloc102:                                  ; preds = %list.reserve96
  %mem105 = call ptr @HeapReAlloc(ptr %heap98, i32 0, ptr %ptr99, i64 ptrtoint (ptr getelementptr (i8, ptr null, i32 1) to i64))
  br label %list.alloc.merge103

list.alloc.merge103:                              ; preds = %list.realloc102, %list.alloc101
  %memphi106 = phi ptr [ %mem104, %list.alloc101 ], [ %mem105, %list.realloc102 ]
  store ptr %memphi106, ptr %list.ptr.addr92, align 8
  store i32 1, ptr %list.cap.addr93, align 4
  br label %list.reserve.end97

list.grow113:                                     ; preds = %list.reserve.end97
  %cap0115 = icmp eq i32 %cap111, 0
  %cap2116 = mul i32 %cap111, 2
  %newcap117 = select i1 %cap0115, i32 4, i32 %cap2116
  %need1118 = add i32 %len110, 1
  %caplt119 = icmp ult i32 %newcap117, %need1118
  %finalcap120 = select i1 %caplt119, i32 %need1118, i32 %newcap117
  %list.ptr.addr121 = getelementptr inbounds nuw { ptr, i32, i32 }, ptr %listlit88, i32 0, i32 0
  %list.cap.addr122 = getelementptr inbounds nuw { ptr, i32, i32 }, ptr %listlit88, i32 0, i32 2
  %cap123 = load i32, ptr %list.cap.addr122, align 4
  %needcap124 = icmp ugt i32 %finalcap120, %cap123
  br i1 %needcap124, label %list.reserve125, label %list.reserve.end126

list.push.cont114:                                ; preds = %list.reserve.end126, %list.reserve.end97
  %ptr138 = load ptr, ptr %list.ptr.addr107, align 8
  %push.elem.ptr139 = getelementptr inbounds i8, ptr %ptr138, i32 %len110
  store i8 0, ptr %push.elem.ptr139, align 1
  %len.next140 = add i32 %len110, 1
  store i32 %len.next140, ptr %list.len.addr108, align 4
  %listlit.val141 = load { ptr, i32, i32 }, ptr %listlit88, align 8
  store { ptr, i32, i32 } %listlit.val141, ptr %used, align 8
  %list.ptr.addr142 = getelementptr inbounds nuw { ptr, i32, i32 }, ptr %keys, i32 0, i32 0
  %list.len.addr143 = getelementptr inbounds nuw { ptr, i32, i32 }, ptr %keys, i32 0, i32 1
  %len144 = load i32, ptr %list.len.addr143, align 4
  %isempty = icmp eq i32 %len144, 0
  br i1 %isempty, label %list.pop.empty, label %list.pop.do

list.reserve125:                                  ; preds = %list.grow113
  %cap64127 = zext i32 %finalcap120 to i64
  %bytes128 = mul i64 %cap64127, ptrtoint (ptr getelementptr (i8, ptr null, i32 1) to i64)
  %heap129 = call ptr @GetProcessHeap()
  %ptr130 = load ptr, ptr %list.ptr.addr121, align 8
  %isnull131 = icmp eq ptr %ptr130, null
  br i1 %isnull131, label %list.alloc132, label %list.realloc133

list.reserve.end126:                              ; preds = %list.alloc.merge134, %list.grow113
  br label %list.push.cont114

list.alloc132:                                    ; preds = %list.reserve125
  %mem135 = call ptr @HeapAlloc(ptr %heap129, i32 0, i64 %bytes128)
  br label %list.alloc.merge134

list.realloc133:                                  ; preds = %list.reserve125
  %mem136 = call ptr @HeapReAlloc(ptr %heap129, i32 0, ptr %ptr130, i64 %bytes128)
  br label %list.alloc.merge134

list.alloc.merge134:                              ; preds = %list.realloc133, %list.alloc132
  %memphi137 = phi ptr [ %mem135, %list.alloc132 ], [ %mem136, %list.realloc133 ]
  store ptr %memphi137, ptr %list.ptr.addr121, align 8
  store i32 %finalcap120, ptr %list.cap.addr122, align 4
  br label %list.reserve.end126

list.pop.empty:                                   ; preds = %list.push.cont114
  br label %list.pop.end

list.pop.do:                                      ; preds = %list.push.cont114
  %len1 = sub i32 %len144, 1
  %ptr145 = load ptr, ptr %list.ptr.addr142, align 8
  %pop.elem.ptr = getelementptr inbounds i32, ptr %ptr145, i32 %len1
  %popv = load i32, ptr %pop.elem.ptr, align 4
  store i32 %len1, ptr %list.len.addr143, align 4
  br label %list.pop.end

list.pop.end:                                     ; preds = %list.pop.do, %list.pop.empty
  %pop.phi = phi i32 [ 0, %list.pop.empty ], [ %popv, %list.pop.do ]
  store i32 %pop.phi, ptr %_k0, align 4
  %list.ptr.addr146 = getelementptr inbounds nuw { ptr, i32, i32 }, ptr %vals, i32 0, i32 0
  %list.len.addr147 = getelementptr inbounds nuw { ptr, i32, i32 }, ptr %vals, i32 0, i32 1
  %len148 = load i32, ptr %list.len.addr147, align 4
  %isempty149 = icmp eq i32 %len148, 0
  br i1 %isempty149, label %list.pop.empty150, label %list.pop.do151

list.pop.empty150:                                ; preds = %list.pop.end
  br label %list.pop.end152

list.pop.do151:                                   ; preds = %list.pop.end
  %len1153 = sub i32 %len148, 1
  %ptr154 = load ptr, ptr %list.ptr.addr146, align 8
  %pop.elem.ptr155 = getelementptr inbounds i32, ptr %ptr154, i32 %len1153
  %popv156 = load i32, ptr %pop.elem.ptr155, align 4
  store i32 %len1153, ptr %list.len.addr147, align 4
  br label %list.pop.end152

list.pop.end152:                                  ; preds = %list.pop.do151, %list.pop.empty150
  %pop.phi157 = phi i32 [ 0, %list.pop.empty150 ], [ %popv156, %list.pop.do151 ]
  store i32 %pop.phi157, ptr %_v0, align 4
  %list.ptr.addr158 = getelementptr inbounds nuw { ptr, i32, i32 }, ptr %used, i32 0, i32 0
  %list.len.addr159 = getelementptr inbounds nuw { ptr, i32, i32 }, ptr %used, i32 0, i32 1
  %len160 = load i32, ptr %list.len.addr159, align 4
  %isempty161 = icmp eq i32 %len160, 0
  br i1 %isempty161, label %list.pop.empty162, label %list.pop.do163

list.pop.empty162:                                ; preds = %list.pop.end152
  br label %list.pop.end164

list.pop.do163:                                   ; preds = %list.pop.end152
  %len1165 = sub i32 %len160, 1
  %ptr166 = load ptr, ptr %list.ptr.addr158, align 8
  %pop.elem.ptr167 = getelementptr inbounds i8, ptr %ptr166, i32 %len1165
  %popv168 = load i8, ptr %pop.elem.ptr167, align 1
  store i32 %len1165, ptr %list.len.addr159, align 4
  br label %list.pop.end164

list.pop.end164:                                  ; preds = %list.pop.do163, %list.pop.empty162
  %pop.phi169 = phi i8 [ 0, %list.pop.empty162 ], [ %popv168, %list.pop.do163 ]
  store i8 %pop.phi169, ptr %_u0, align 1
  %cap170 = load i32, ptr %cap, align 4
  %list.ptr.addr171 = getelementptr inbounds nuw { ptr, i32, i32 }, ptr %keys, i32 0, i32 0
  %list.cap.addr172 = getelementptr inbounds nuw { ptr, i32, i32 }, ptr %keys, i32 0, i32 2
  %cap173 = load i32, ptr %list.cap.addr172, align 4
  %needcap174 = icmp ugt i32 %cap170, %cap173
  br i1 %needcap174, label %list.reserve175, label %list.reserve.end176

list.reserve175:                                  ; preds = %list.pop.end164
  %cap64177 = zext i32 %cap170 to i64
  %bytes178 = mul i64 %cap64177, ptrtoint (ptr getelementptr (i32, ptr null, i32 1) to i64)
  %heap179 = call ptr @GetProcessHeap()
  %ptr180 = load ptr, ptr %list.ptr.addr171, align 8
  %isnull181 = icmp eq ptr %ptr180, null
  br i1 %isnull181, label %list.alloc182, label %list.realloc183

list.reserve.end176:                              ; preds = %list.alloc.merge184, %list.pop.end164
  %cap188 = load i32, ptr %cap, align 4
  %list.ptr.addr189 = getelementptr inbounds nuw { ptr, i32, i32 }, ptr %vals, i32 0, i32 0
  %list.cap.addr190 = getelementptr inbounds nuw { ptr, i32, i32 }, ptr %vals, i32 0, i32 2
  %cap191 = load i32, ptr %list.cap.addr190, align 4
  %needcap192 = icmp ugt i32 %cap188, %cap191
  br i1 %needcap192, label %list.reserve193, label %list.reserve.end194

list.alloc182:                                    ; preds = %list.reserve175
  %mem185 = call ptr @HeapAlloc(ptr %heap179, i32 0, i64 %bytes178)
  br label %list.alloc.merge184

list.realloc183:                                  ; preds = %list.reserve175
  %mem186 = call ptr @HeapReAlloc(ptr %heap179, i32 0, ptr %ptr180, i64 %bytes178)
  br label %list.alloc.merge184

list.alloc.merge184:                              ; preds = %list.realloc183, %list.alloc182
  %memphi187 = phi ptr [ %mem185, %list.alloc182 ], [ %mem186, %list.realloc183 ]
  store ptr %memphi187, ptr %list.ptr.addr171, align 8
  store i32 %cap170, ptr %list.cap.addr172, align 4
  br label %list.reserve.end176

list.reserve193:                                  ; preds = %list.reserve.end176
  %cap64195 = zext i32 %cap188 to i64
  %bytes196 = mul i64 %cap64195, ptrtoint (ptr getelementptr (i32, ptr null, i32 1) to i64)
  %heap197 = call ptr @GetProcessHeap()
  %ptr198 = load ptr, ptr %list.ptr.addr189, align 8
  %isnull199 = icmp eq ptr %ptr198, null
  br i1 %isnull199, label %list.alloc200, label %list.realloc201

list.reserve.end194:                              ; preds = %list.alloc.merge202, %list.reserve.end176
  %cap206 = load i32, ptr %cap, align 4
  %list.ptr.addr207 = getelementptr inbounds nuw { ptr, i32, i32 }, ptr %used, i32 0, i32 0
  %list.cap.addr208 = getelementptr inbounds nuw { ptr, i32, i32 }, ptr %used, i32 0, i32 2
  %cap209 = load i32, ptr %list.cap.addr208, align 4
  %needcap210 = icmp ugt i32 %cap206, %cap209
  br i1 %needcap210, label %list.reserve211, label %list.reserve.end212

list.alloc200:                                    ; preds = %list.reserve193
  %mem203 = call ptr @HeapAlloc(ptr %heap197, i32 0, i64 %bytes196)
  br label %list.alloc.merge202

list.realloc201:                                  ; preds = %list.reserve193
  %mem204 = call ptr @HeapReAlloc(ptr %heap197, i32 0, ptr %ptr198, i64 %bytes196)
  br label %list.alloc.merge202

list.alloc.merge202:                              ; preds = %list.realloc201, %list.alloc200
  %memphi205 = phi ptr [ %mem203, %list.alloc200 ], [ %mem204, %list.realloc201 ]
  store ptr %memphi205, ptr %list.ptr.addr189, align 8
  store i32 %cap188, ptr %list.cap.addr190, align 4
  br label %list.reserve.end194

list.reserve211:                                  ; preds = %list.reserve.end194
  %cap64213 = zext i32 %cap206 to i64
  %bytes214 = mul i64 %cap64213, ptrtoint (ptr getelementptr (i8, ptr null, i32 1) to i64)
  %heap215 = call ptr @GetProcessHeap()
  %ptr216 = load ptr, ptr %list.ptr.addr207, align 8
  %isnull217 = icmp eq ptr %ptr216, null
  br i1 %isnull217, label %list.alloc218, label %list.realloc219

list.reserve.end212:                              ; preds = %list.alloc.merge220, %list.reserve.end194
  store i32 0, ptr %i, align 4
  store i32 1000000, ptr %next_init_report, align 4
  br label %while.cond224

list.alloc218:                                    ; preds = %list.reserve211
  %mem221 = call ptr @HeapAlloc(ptr %heap215, i32 0, i64 %bytes214)
  br label %list.alloc.merge220

list.realloc219:                                  ; preds = %list.reserve211
  %mem222 = call ptr @HeapReAlloc(ptr %heap215, i32 0, ptr %ptr216, i64 %bytes214)
  br label %list.alloc.merge220

list.alloc.merge220:                              ; preds = %list.realloc219, %list.alloc218
  %memphi223 = phi ptr [ %mem221, %list.alloc218 ], [ %mem222, %list.realloc219 ]
  store ptr %memphi223, ptr %list.ptr.addr207, align 8
  store i32 %cap206, ptr %list.cap.addr208, align 4
  br label %list.reserve.end212

while.cond224:                                    ; preds = %ifend, %list.reserve.end212
  %i227 = load i32, ptr %i, align 4
  %cap228 = load i32, ptr %cap, align 4
  %lttmp229 = icmp slt i32 %i227, %cap228
  %boolz230 = zext i1 %lttmp229 to i8
  %whilecnd231 = icmp ne i8 %boolz230, 0
  br i1 %whilecnd231, label %while.body225, label %while.end226

while.body225:                                    ; preds = %while.cond224
  %list.ptr.addr232 = getelementptr inbounds nuw { ptr, i32, i32 }, ptr %used, i32 0, i32 0
  %list.len.addr233 = getelementptr inbounds nuw { ptr, i32, i32 }, ptr %used, i32 0, i32 1
  %list.cap.addr234 = getelementptr inbounds nuw { ptr, i32, i32 }, ptr %used, i32 0, i32 2
  %len235 = load i32, ptr %list.len.addr233, align 4
  %cap236 = load i32, ptr %list.cap.addr234, align 4
  %needgrow237 = icmp uge i32 %len235, %cap236
  br i1 %needgrow237, label %list.grow238, label %list.push.cont239

while.end226:                                     ; preds = %while.cond224
  store i64 0, ptr %seed, align 4
  %calltmp = call i64 @qpc_freq()
  store i64 %calltmp, ptr %freq, align 4
  %calltmp410 = call i64 @now_ticks()
  store i64 %calltmp410, ptr %t0, align 4
  store i32 0, ptr %len411, align 4
  store i32 0, ptr %k, align 4
  %progress_step412 = load i32, ptr %progress_step, align 4
  store i32 %progress_step412, ptr %next_report, align 4
  br label %while.cond413

list.grow238:                                     ; preds = %while.body225
  %cap0240 = icmp eq i32 %cap236, 0
  %cap2241 = mul i32 %cap236, 2
  %newcap242 = select i1 %cap0240, i32 4, i32 %cap2241
  %need1243 = add i32 %len235, 1
  %caplt244 = icmp ult i32 %newcap242, %need1243
  %finalcap245 = select i1 %caplt244, i32 %need1243, i32 %newcap242
  %list.ptr.addr246 = getelementptr inbounds nuw { ptr, i32, i32 }, ptr %used, i32 0, i32 0
  %list.cap.addr247 = getelementptr inbounds nuw { ptr, i32, i32 }, ptr %used, i32 0, i32 2
  %cap248 = load i32, ptr %list.cap.addr247, align 4
  %needcap249 = icmp ugt i32 %finalcap245, %cap248
  br i1 %needcap249, label %list.reserve250, label %list.reserve.end251

list.push.cont239:                                ; preds = %list.reserve.end251, %while.body225
  %ptr263 = load ptr, ptr %list.ptr.addr232, align 8
  %push.elem.ptr264 = getelementptr inbounds i8, ptr %ptr263, i32 %len235
  store i8 0, ptr %push.elem.ptr264, align 1
  %len.next265 = add i32 %len235, 1
  store i32 %len.next265, ptr %list.len.addr233, align 4
  %cur = load i32, ptr %i, align 4
  %addas = add i32 %cur, 1
  store i32 %addas, ptr %i, align 4
  %i266 = load i32, ptr %i, align 4
  %next_init_report267 = load i32, ptr %next_init_report, align 4
  %eqtmp = icmp eq i32 %i266, %next_init_report267
  %boolz268 = zext i1 %eqtmp to i8
  %ifcnd = icmp ne i8 %boolz268, 0
  br i1 %ifcnd, label %then, label %else

list.reserve250:                                  ; preds = %list.grow238
  %cap64252 = zext i32 %finalcap245 to i64
  %bytes253 = mul i64 %cap64252, ptrtoint (ptr getelementptr (i8, ptr null, i32 1) to i64)
  %heap254 = call ptr @GetProcessHeap()
  %ptr255 = load ptr, ptr %list.ptr.addr246, align 8
  %isnull256 = icmp eq ptr %ptr255, null
  br i1 %isnull256, label %list.alloc257, label %list.realloc258

list.reserve.end251:                              ; preds = %list.alloc.merge259, %list.grow238
  br label %list.push.cont239

list.alloc257:                                    ; preds = %list.reserve250
  %mem260 = call ptr @HeapAlloc(ptr %heap254, i32 0, i64 %bytes253)
  br label %list.alloc.merge259

list.realloc258:                                  ; preds = %list.reserve250
  %mem261 = call ptr @HeapReAlloc(ptr %heap254, i32 0, ptr %ptr255, i64 %bytes253)
  br label %list.alloc.merge259

list.alloc.merge259:                              ; preds = %list.realloc258, %list.alloc257
  %memphi262 = phi ptr [ %mem260, %list.alloc257 ], [ %mem261, %list.realloc258 ]
  store ptr %memphi262, ptr %list.ptr.addr246, align 8
  store i32 %finalcap245, ptr %list.cap.addr247, align 4
  br label %list.reserve.end251

then:                                             ; preds = %list.push.cont239
  %list.ptr.addr269 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf, i32 0, i32 0
  %list.len.addr270 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf, i32 0, i32 1
  %list.cap.addr271 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf, i32 0, i32 2
  store ptr null, ptr %list.ptr.addr269, align 8
  store i32 0, ptr %list.len.addr270, align 4
  store i32 0, ptr %list.cap.addr271, align 4
  %buf.ptr.addr = getelementptr inbounds nuw %emp.string, ptr %fstr.buf, i32 0, i32 0
  %buf.len.addr = getelementptr inbounds nuw %emp.string, ptr %fstr.buf, i32 0, i32 1
  %buf.cap.addr = getelementptr inbounds nuw %emp.string, ptr %fstr.buf, i32 0, i32 2
  %len272 = load i32, ptr %buf.len.addr, align 4
  %cap273 = load i32, ptr %buf.cap.addr, align 4
  %want.tmp = add i32 %len272, 10
  %want = add i32 %want.tmp, 1
  %needgrow274 = icmp ugt i32 %want, %cap273
  br i1 %needgrow274, label %fstr.grow, label %fstr.grow.cont

else:                                             ; preds = %list.push.cont239
  br label %ifend

ifend:                                            ; preds = %else, %fstr.empty.cont
  br label %while.cond224

fstr.grow:                                        ; preds = %then
  %cap0275 = icmp eq i32 %cap273, 0
  %cap2276 = mul i32 %cap273, 2
  %basecap = select i1 %cap0275, i32 64, i32 %cap2276
  %caplt277 = icmp ult i32 %basecap, %want
  %finalcap278 = select i1 %caplt277, i32 %want, i32 %basecap
  %list.ptr.addr279 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf, i32 0, i32 0
  %list.cap.addr280 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf, i32 0, i32 2
  %cap281 = load i32, ptr %list.cap.addr280, align 4
  %needcap282 = icmp ugt i32 %finalcap278, %cap281
  br i1 %needcap282, label %list.reserve283, label %list.reserve.end284

fstr.grow.cont:                                   ; preds = %list.reserve.end284, %then
  %ptr296 = load ptr, ptr %buf.ptr.addr, align 8
  %len2 = load i32, ptr %buf.len.addr, align 4
  br i1 false, label %fstr.zero, label %fstr.cpy

list.reserve283:                                  ; preds = %fstr.grow
  %cap64285 = zext i32 %finalcap278 to i64
  %bytes286 = mul i64 %cap64285, ptrtoint (ptr getelementptr (i8, ptr null, i32 1) to i64)
  %heap287 = call ptr @GetProcessHeap()
  %ptr288 = load ptr, ptr %list.ptr.addr279, align 8
  %isnull289 = icmp eq ptr %ptr288, null
  br i1 %isnull289, label %list.alloc290, label %list.realloc291

list.reserve.end284:                              ; preds = %list.alloc.merge292, %fstr.grow
  br label %fstr.grow.cont

list.alloc290:                                    ; preds = %list.reserve283
  %mem293 = call ptr @HeapAlloc(ptr %heap287, i32 0, i64 %bytes286)
  br label %list.alloc.merge292

list.realloc291:                                  ; preds = %list.reserve283
  %mem294 = call ptr @HeapReAlloc(ptr %heap287, i32 0, ptr %ptr288, i64 %bytes286)
  br label %list.alloc.merge292

list.alloc.merge292:                              ; preds = %list.realloc291, %list.alloc290
  %memphi295 = phi ptr [ %mem293, %list.alloc290 ], [ %mem294, %list.realloc291 ]
  store ptr %memphi295, ptr %list.ptr.addr279, align 8
  store i32 %finalcap278, ptr %list.cap.addr280, align 4
  br label %list.reserve.end284

fstr.zero:                                        ; preds = %fstr.grow.cont
  %nul.ptr = getelementptr inbounds i8, ptr %ptr296, i32 %len2
  store i8 0, ptr %nul.ptr, align 1
  br label %fstr.cpy.end

fstr.cpy:                                         ; preds = %fstr.grow.cont
  store i32 0, ptr %fstr.cpy.i, align 4
  br label %fstr.cpy.loop

fstr.cpy.end:                                     ; preds = %fstr.cpy.done, %fstr.zero
  %i299 = load i32, ptr %i, align 4
  %sext = sext i32 %i299 to i64
  %neg = icmp slt i64 %sext, 0
  %negv = sub i64 0, %sext
  %abs = select i1 %neg, i64 %negv, i64 %sext
  store i32 31, ptr %fstr.itoa.idx, align 4
  store i64 %abs, ptr %fstr.itoa.cur, align 4
  %end.ptr = getelementptr inbounds [32 x i8], ptr %fstr.itoa.buf, i32 0, i32 31
  store i8 0, ptr %end.ptr, align 1
  %iszero = icmp eq i64 %abs, 0
  br i1 %iszero, label %itoa.zero, label %itoa.loop

fstr.cpy.loop:                                    ; preds = %fstr.cpy.body, %fstr.cpy
  %i297 = load i32, ptr %fstr.cpy.i, align 4
  %cond = icmp ult i32 %i297, 10
  br i1 %cond, label %fstr.cpy.body, label %fstr.cpy.done

fstr.cpy.body:                                    ; preds = %fstr.cpy.loop
  %s.ptr = getelementptr inbounds i8, ptr @.fstr.1000004, i32 %i297
  %ch = load i8, ptr %s.ptr, align 1
  %dst.off = add i32 %len2, %i297
  %d.ptr = getelementptr inbounds i8, ptr %ptr296, i32 %dst.off
  store i8 %ch, ptr %d.ptr, align 1
  %i2 = add i32 %i297, 1
  store i32 %i2, ptr %fstr.cpy.i, align 4
  br label %fstr.cpy.loop

fstr.cpy.done:                                    ; preds = %fstr.cpy.loop
  %newlen = add i32 %len2, 10
  store i32 %newlen, ptr %buf.len.addr, align 4
  %nul.ptr298 = getelementptr inbounds i8, ptr %ptr296, i32 %newlen
  store i8 0, ptr %nul.ptr298, align 1
  br label %fstr.cpy.end

itoa.zero:                                        ; preds = %fstr.cpy.end
  store i32 30, ptr %fstr.itoa.idx, align 4
  %z.ptr = getelementptr inbounds [32 x i8], ptr %fstr.itoa.buf, i32 0, i32 30
  store i8 48, ptr %z.ptr, align 1
  br label %itoa.after

itoa.loop:                                        ; preds = %fstr.cpy.end
  br label %itoa.cond

itoa.after:                                       ; preds = %itoa.done, %itoa.zero
  br i1 %neg, label %itoa.neg, label %itoa.merge

itoa.cond:                                        ; preds = %itoa.body, %itoa.loop
  %curv = load i64, ptr %fstr.itoa.cur, align 4
  %more = icmp ugt i64 %curv, 0
  br i1 %more, label %itoa.body, label %itoa.done

itoa.body:                                        ; preds = %itoa.cond
  %rem = urem i64 %curv, 10
  %div = udiv i64 %curv, 10
  store i64 %div, ptr %fstr.itoa.cur, align 4
  %idxv = load i32, ptr %fstr.itoa.idx, align 4
  %idx1 = sub i32 %idxv, 1
  store i32 %idx1, ptr %fstr.itoa.idx, align 4
  %rem32 = trunc i64 %rem to i32
  %ch32 = add i32 %rem32, 48
  %ch300 = trunc i32 %ch32 to i8
  %d.ptr301 = getelementptr inbounds [32 x i8], ptr %fstr.itoa.buf, i32 0, i32 %idx1
  store i8 %ch300, ptr %d.ptr301, align 1
  br label %itoa.cond

itoa.done:                                        ; preds = %itoa.cond
  br label %itoa.after

itoa.neg:                                         ; preds = %itoa.after
  %idxv302 = load i32, ptr %fstr.itoa.idx, align 4
  %idxm1 = sub i32 %idxv302, 1
  store i32 %idxm1, ptr %fstr.itoa.idx, align 4
  %m.ptr = getelementptr inbounds [32 x i8], ptr %fstr.itoa.buf, i32 0, i32 %idxm1
  store i8 45, ptr %m.ptr, align 1
  br label %itoa.merge

itoa.merge:                                       ; preds = %itoa.neg, %itoa.after
  %start = load i32, ptr %fstr.itoa.idx, align 4
  %slen = sub i32 31, %start
  %sptr = getelementptr inbounds [32 x i8], ptr %fstr.itoa.buf, i32 0, i32 %start
  %buf.ptr.addr303 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf, i32 0, i32 0
  %buf.len.addr304 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf, i32 0, i32 1
  %buf.cap.addr305 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf, i32 0, i32 2
  %len306 = load i32, ptr %buf.len.addr304, align 4
  %cap307 = load i32, ptr %buf.cap.addr305, align 4
  %want.tmp308 = add i32 %len306, %slen
  %want309 = add i32 %want.tmp308, 1
  %needgrow310 = icmp ugt i32 %want309, %cap307
  br i1 %needgrow310, label %fstr.grow311, label %fstr.grow.cont312

fstr.grow311:                                     ; preds = %itoa.merge
  %cap0313 = icmp eq i32 %cap307, 0
  %cap2314 = mul i32 %cap307, 2
  %basecap315 = select i1 %cap0313, i32 64, i32 %cap2314
  %caplt316 = icmp ult i32 %basecap315, %want309
  %finalcap317 = select i1 %caplt316, i32 %want309, i32 %basecap315
  %list.ptr.addr318 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf, i32 0, i32 0
  %list.cap.addr319 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf, i32 0, i32 2
  %cap320 = load i32, ptr %list.cap.addr319, align 4
  %needcap321 = icmp ugt i32 %finalcap317, %cap320
  br i1 %needcap321, label %list.reserve322, label %list.reserve.end323

fstr.grow.cont312:                                ; preds = %list.reserve.end323, %itoa.merge
  %ptr335 = load ptr, ptr %buf.ptr.addr303, align 8
  %len2336 = load i32, ptr %buf.len.addr304, align 4
  %srclen0 = icmp eq i32 %slen, 0
  br i1 %srclen0, label %fstr.zero337, label %fstr.cpy338

list.reserve322:                                  ; preds = %fstr.grow311
  %cap64324 = zext i32 %finalcap317 to i64
  %bytes325 = mul i64 %cap64324, ptrtoint (ptr getelementptr (i8, ptr null, i32 1) to i64)
  %heap326 = call ptr @GetProcessHeap()
  %ptr327 = load ptr, ptr %list.ptr.addr318, align 8
  %isnull328 = icmp eq ptr %ptr327, null
  br i1 %isnull328, label %list.alloc329, label %list.realloc330

list.reserve.end323:                              ; preds = %list.alloc.merge331, %fstr.grow311
  br label %fstr.grow.cont312

list.alloc329:                                    ; preds = %list.reserve322
  %mem332 = call ptr @HeapAlloc(ptr %heap326, i32 0, i64 %bytes325)
  br label %list.alloc.merge331

list.realloc330:                                  ; preds = %list.reserve322
  %mem333 = call ptr @HeapReAlloc(ptr %heap326, i32 0, ptr %ptr327, i64 %bytes325)
  br label %list.alloc.merge331

list.alloc.merge331:                              ; preds = %list.realloc330, %list.alloc329
  %memphi334 = phi ptr [ %mem332, %list.alloc329 ], [ %mem333, %list.realloc330 ]
  store ptr %memphi334, ptr %list.ptr.addr318, align 8
  store i32 %finalcap317, ptr %list.cap.addr319, align 4
  br label %list.reserve.end323

fstr.zero337:                                     ; preds = %fstr.grow.cont312
  %nul.ptr340 = getelementptr inbounds i8, ptr %ptr335, i32 %len2336
  store i8 0, ptr %nul.ptr340, align 1
  br label %fstr.cpy.end339

fstr.cpy338:                                      ; preds = %fstr.grow.cont312
  store i32 0, ptr %fstr.cpy.i341, align 4
  br label %fstr.cpy.loop342

fstr.cpy.end339:                                  ; preds = %fstr.cpy.done344, %fstr.zero337
  %buf.len.addr354 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf, i32 0, i32 1
  %buf.len = load i32, ptr %buf.len.addr354, align 4
  %fstr.empty = icmp eq i32 %buf.len, 0
  %buf.ptr.addr355 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf, i32 0, i32 0
  %outp0 = load ptr, ptr %buf.ptr.addr355, align 8
  %out.null0 = icmp eq ptr %outp0, null
  %needalloc = and i1 %fstr.empty, %out.null0
  br i1 %needalloc, label %fstr.empty.alloc, label %fstr.empty.cont

fstr.cpy.loop342:                                 ; preds = %fstr.cpy.body343, %fstr.cpy338
  %i345 = load i32, ptr %fstr.cpy.i341, align 4
  %cond346 = icmp ult i32 %i345, %slen
  br i1 %cond346, label %fstr.cpy.body343, label %fstr.cpy.done344

fstr.cpy.body343:                                 ; preds = %fstr.cpy.loop342
  %s.ptr347 = getelementptr inbounds i8, ptr %sptr, i32 %i345
  %ch348 = load i8, ptr %s.ptr347, align 1
  %dst.off349 = add i32 %len2336, %i345
  %d.ptr350 = getelementptr inbounds i8, ptr %ptr335, i32 %dst.off349
  store i8 %ch348, ptr %d.ptr350, align 1
  %i2351 = add i32 %i345, 1
  store i32 %i2351, ptr %fstr.cpy.i341, align 4
  br label %fstr.cpy.loop342

fstr.cpy.done344:                                 ; preds = %fstr.cpy.loop342
  %newlen352 = add i32 %len2336, %slen
  store i32 %newlen352, ptr %buf.len.addr304, align 4
  %nul.ptr353 = getelementptr inbounds i8, ptr %ptr335, i32 %newlen352
  store i8 0, ptr %nul.ptr353, align 1
  br label %fstr.cpy.end339

fstr.empty.alloc:                                 ; preds = %fstr.cpy.end339
  %buf.ptr.addr356 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf, i32 0, i32 0
  %buf.len.addr357 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf, i32 0, i32 1
  %buf.cap.addr358 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf, i32 0, i32 2
  %len359 = load i32, ptr %buf.len.addr357, align 4
  %cap360 = load i32, ptr %buf.cap.addr358, align 4
  %want.tmp361 = add i32 %len359, 0
  %want362 = add i32 %want.tmp361, 1
  %needgrow363 = icmp ugt i32 %want362, %cap360
  br i1 %needgrow363, label %fstr.grow364, label %fstr.grow.cont365

fstr.empty.cont:                                  ; preds = %fstr.cpy.end392, %fstr.cpy.end339
  %fstr.val = load %emp.string, ptr %fstr.buf, align 8
  store %emp.string %fstr.val, ptr %msg, align 8
  %msg407 = load %emp.string, ptr %msg, align 8
  call void @println__Nstring(%emp.string %msg407)
  %cur408 = load i32, ptr %next_init_report, align 4
  %addas409 = add i32 %cur408, 1000000
  store i32 %addas409, ptr %next_init_report, align 4
  br label %ifend

fstr.grow364:                                     ; preds = %fstr.empty.alloc
  %cap0366 = icmp eq i32 %cap360, 0
  %cap2367 = mul i32 %cap360, 2
  %basecap368 = select i1 %cap0366, i32 64, i32 %cap2367
  %caplt369 = icmp ult i32 %basecap368, %want362
  %finalcap370 = select i1 %caplt369, i32 %want362, i32 %basecap368
  %list.ptr.addr371 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf, i32 0, i32 0
  %list.cap.addr372 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf, i32 0, i32 2
  %cap373 = load i32, ptr %list.cap.addr372, align 4
  %needcap374 = icmp ugt i32 %finalcap370, %cap373
  br i1 %needcap374, label %list.reserve375, label %list.reserve.end376

fstr.grow.cont365:                                ; preds = %list.reserve.end376, %fstr.empty.alloc
  %ptr388 = load ptr, ptr %buf.ptr.addr356, align 8
  %len2389 = load i32, ptr %buf.len.addr357, align 4
  br i1 true, label %fstr.zero390, label %fstr.cpy391

list.reserve375:                                  ; preds = %fstr.grow364
  %cap64377 = zext i32 %finalcap370 to i64
  %bytes378 = mul i64 %cap64377, ptrtoint (ptr getelementptr (i8, ptr null, i32 1) to i64)
  %heap379 = call ptr @GetProcessHeap()
  %ptr380 = load ptr, ptr %list.ptr.addr371, align 8
  %isnull381 = icmp eq ptr %ptr380, null
  br i1 %isnull381, label %list.alloc382, label %list.realloc383

list.reserve.end376:                              ; preds = %list.alloc.merge384, %fstr.grow364
  br label %fstr.grow.cont365

list.alloc382:                                    ; preds = %list.reserve375
  %mem385 = call ptr @HeapAlloc(ptr %heap379, i32 0, i64 %bytes378)
  br label %list.alloc.merge384

list.realloc383:                                  ; preds = %list.reserve375
  %mem386 = call ptr @HeapReAlloc(ptr %heap379, i32 0, ptr %ptr380, i64 %bytes378)
  br label %list.alloc.merge384

list.alloc.merge384:                              ; preds = %list.realloc383, %list.alloc382
  %memphi387 = phi ptr [ %mem385, %list.alloc382 ], [ %mem386, %list.realloc383 ]
  store ptr %memphi387, ptr %list.ptr.addr371, align 8
  store i32 %finalcap370, ptr %list.cap.addr372, align 4
  br label %list.reserve.end376

fstr.zero390:                                     ; preds = %fstr.grow.cont365
  %nul.ptr393 = getelementptr inbounds i8, ptr %ptr388, i32 %len2389
  store i8 0, ptr %nul.ptr393, align 1
  br label %fstr.cpy.end392

fstr.cpy391:                                      ; preds = %fstr.grow.cont365
  store i32 0, ptr %fstr.cpy.i394, align 4
  br label %fstr.cpy.loop395

fstr.cpy.end392:                                  ; preds = %fstr.cpy.done397, %fstr.zero390
  br label %fstr.empty.cont

fstr.cpy.loop395:                                 ; preds = %fstr.cpy.body396, %fstr.cpy391
  %i398 = load i32, ptr %fstr.cpy.i394, align 4
  %cond399 = icmp ult i32 %i398, 0
  br i1 %cond399, label %fstr.cpy.body396, label %fstr.cpy.done397

fstr.cpy.body396:                                 ; preds = %fstr.cpy.loop395
  %s.ptr400 = getelementptr inbounds i8, ptr @.fstr.1000000, i32 %i398
  %ch401 = load i8, ptr %s.ptr400, align 1
  %dst.off402 = add i32 %len2389, %i398
  %d.ptr403 = getelementptr inbounds i8, ptr %ptr388, i32 %dst.off402
  store i8 %ch401, ptr %d.ptr403, align 1
  %i2404 = add i32 %i398, 1
  store i32 %i2404, ptr %fstr.cpy.i394, align 4
  br label %fstr.cpy.loop395

fstr.cpy.done397:                                 ; preds = %fstr.cpy.loop395
  %newlen405 = add i32 %len2389, 0
  store i32 %newlen405, ptr %buf.len.addr357, align 4
  %nul.ptr406 = getelementptr inbounds i8, ptr %ptr388, i32 %newlen405
  store i8 0, ptr %nul.ptr406, align 1
  br label %fstr.cpy.end392

while.cond413:                                    ; preds = %ifend506, %while.end226
  %k416 = load i32, ptr %k, align 4
  %entries_run417 = load i32, ptr %entries_run, align 4
  %lttmp418 = icmp slt i32 %k416, %entries_run417
  %boolz419 = zext i1 %lttmp418 to i8
  %whilecnd420 = icmp ne i8 %boolz419, 0
  br i1 %whilecnd420, label %while.body414, label %while.end415

while.body414:                                    ; preds = %while.cond413
  %len421 = load i32, ptr %len411, align 4
  %cap422 = load i32, ptr %cap, align 4
  %getmp = icmp sge i32 %len421, %cap422
  %boolz423 = zext i1 %getmp to i8
  %ifcnd424 = icmp ne i8 %boolz423, 0
  br i1 %ifcnd424, label %then425, label %else426

while.end415:                                     ; preds = %then425, %while.cond413
  %calltmp858 = call i64 @now_ticks()
  store i64 %calltmp858, ptr %t1, align 4
  %t1859 = load i64, ptr %t1, align 4
  %t0860 = load i64, ptr %t0, align 4
  %subtmp861 = sub i64 %t1859, %t0860
  store i64 %subtmp861, ptr %dt, align 4
  %dt862 = load i64, ptr %dt, align 4
  %multmp863 = mul i64 %dt862, 1000
  %freq864 = load i64, ptr %freq, align 4
  %divtmp865 = sdiv i64 %multmp863, %freq864
  store i64 %divtmp865, ptr %ms, align 4
  call void @println__PNu8(ptr @.str.2)
  %list.ptr.addr869 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf867, i32 0, i32 0
  %list.len.addr870 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf867, i32 0, i32 1
  %list.cap.addr871 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf867, i32 0, i32 2
  store ptr null, ptr %list.ptr.addr869, align 8
  store i32 0, ptr %list.len.addr870, align 4
  store i32 0, ptr %list.cap.addr871, align 4
  %buf.ptr.addr872 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf867, i32 0, i32 0
  %buf.len.addr873 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf867, i32 0, i32 1
  %buf.cap.addr874 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf867, i32 0, i32 2
  %len875 = load i32, ptr %buf.len.addr873, align 4
  %cap876 = load i32, ptr %buf.cap.addr874, align 4
  %want.tmp877 = add i32 %len875, 15
  %want878 = add i32 %want.tmp877, 1
  %needgrow879 = icmp ugt i32 %want878, %cap876
  br i1 %needgrow879, label %fstr.grow880, label %fstr.grow.cont881

then425:                                          ; preds = %while.body414
  br label %while.end415

else426:                                          ; preds = %while.body414
  br label %ifend427

ifend427:                                         ; preds = %else426
  %k428 = load i32, ptr %k, align 4
  store i32 %k428, ptr %key, align 4
  %key429 = load i32, ptr %key, align 4
  %xortmp = xor i32 %key429, 0
  store i32 %xortmp, ptr %val, align 4
  %key430 = load i32, ptr %key, align 4
  %sext431 = sext i32 %key430 to i64
  %seed432 = load i64, ptr %seed, align 4
  %xortmp433 = xor i64 %sext431, %seed432
  %calltmp434 = call i64 @splitmix64(i64 %xortmp433)
  store i64 %calltmp434, ptr %h, align 4
  %h435 = load i64, ptr %h, align 4
  %calltmp436 = call i64 @splitmix64(i64 %h435)
  %shrtmp = ashr i64 %calltmp436, 32
  %trunc = trunc i64 %shrtmp to i32
  %ortmp = or i32 %trunc, 1
  store i32 %ortmp, ptr %step, align 4
  %h437 = load i64, ptr %h, align 4
  %trunc438 = trunc i64 %h437 to i32
  %mask439 = load i32, ptr %mask, align 4
  %andtmp = and i32 %trunc438, %mask439
  store i32 %andtmp, ptr %pos, align 4
  store i32 0, ptr %probes, align 4
  br label %while.cond440

while.cond440:                                    ; preds = %ifend485, %ifend427
  br i1 true, label %while.body441, label %while.end442

while.body441:                                    ; preds = %while.cond440
  %probes443 = load i32, ptr %probes, align 4
  %cap444 = load i32, ptr %cap, align 4
  %getmp445 = icmp sge i32 %probes443, %cap444
  %boolz446 = zext i1 %getmp445 to i8
  %ifcnd447 = icmp ne i8 %boolz446, 0
  br i1 %ifcnd447, label %then448, label %else449

while.end442:                                     ; preds = %then459, %then448, %while.cond440
  %cur497 = load i32, ptr %k, align 4
  %addas498 = add i32 %cur497, 1
  store i32 %addas498, ptr %k, align 4
  %k499 = load i32, ptr %k, align 4
  %next_report500 = load i32, ptr %next_report, align 4
  %eqtmp501 = icmp eq i32 %k499, %next_report500
  %boolz502 = zext i1 %eqtmp501 to i8
  %ifcnd503 = icmp ne i8 %boolz502, 0
  br i1 %ifcnd503, label %then504, label %else505

then448:                                          ; preds = %while.body441
  br label %while.end442

else449:                                          ; preds = %while.body441
  br label %ifend450

ifend450:                                         ; preds = %else449
  %pos451 = load i32, ptr %pos, align 4
  store i32 %pos451, ptr %idx, align 4
  %used452 = load { ptr, i32, i32 }, ptr %used, align 8
  %idx453 = load i32, ptr %idx, align 4
  %list.ptr.addr454 = getelementptr inbounds nuw { ptr, i32, i32 }, ptr %used, i32 0, i32 0
  %list.ptr = load ptr, ptr %list.ptr.addr454, align 8
  %list.elem.ptr = getelementptr inbounds i8, ptr %list.ptr, i32 %idx453
  %idx455 = load i8, ptr %list.elem.ptr, align 1
  %eqtmp456 = icmp eq i8 %idx455, 0
  %boolz457 = zext i1 %eqtmp456 to i8
  %ifcnd458 = icmp ne i8 %boolz457, 0
  br i1 %ifcnd458, label %then459, label %else460

then459:                                          ; preds = %ifend450
  %idx462 = load i32, ptr %idx, align 4
  %list.ptr.addr463 = getelementptr inbounds nuw { ptr, i32, i32 }, ptr %used, i32 0, i32 0
  %list.ptr464 = load ptr, ptr %list.ptr.addr463, align 8
  %list.elem.ptr465 = getelementptr inbounds i8, ptr %list.ptr464, i32 %idx462
  store i8 1, ptr %list.elem.ptr465, align 1
  %idx466 = load i32, ptr %idx, align 4
  %list.ptr.addr467 = getelementptr inbounds nuw { ptr, i32, i32 }, ptr %keys, i32 0, i32 0
  %list.ptr468 = load ptr, ptr %list.ptr.addr467, align 8
  %list.elem.ptr469 = getelementptr inbounds i32, ptr %list.ptr468, i32 %idx466
  %key470 = load i32, ptr %key, align 4
  store i32 %key470, ptr %list.elem.ptr469, align 4
  %idx471 = load i32, ptr %idx, align 4
  %list.ptr.addr472 = getelementptr inbounds nuw { ptr, i32, i32 }, ptr %vals, i32 0, i32 0
  %list.ptr473 = load ptr, ptr %list.ptr.addr472, align 8
  %list.elem.ptr474 = getelementptr inbounds i32, ptr %list.ptr473, i32 %idx471
  %val475 = load i32, ptr %val, align 4
  store i32 %val475, ptr %list.elem.ptr474, align 4
  %cur476 = load i32, ptr %len411, align 4
  %addas477 = add i32 %cur476, 1
  store i32 %addas477, ptr %len411, align 4
  br label %while.end442

else460:                                          ; preds = %ifend450
  br label %ifend461

ifend461:                                         ; preds = %else460
  %len478 = load i32, ptr %len411, align 4
  %random_until479 = load i32, ptr %random_until, align 4
  %lttmp480 = icmp slt i32 %len478, %random_until479
  %boolz481 = zext i1 %lttmp480 to i8
  %ifcnd482 = icmp ne i8 %boolz481, 0
  br i1 %ifcnd482, label %then483, label %else484

then483:                                          ; preds = %ifend461
  %pos486 = load i32, ptr %pos, align 4
  %step487 = load i32, ptr %step, align 4
  %addtmp488 = add i32 %pos486, %step487
  %mask489 = load i32, ptr %mask, align 4
  %andtmp490 = and i32 %addtmp488, %mask489
  store i32 %andtmp490, ptr %pos, align 4
  br label %ifend485

else484:                                          ; preds = %ifend461
  %pos491 = load i32, ptr %pos, align 4
  %addtmp492 = add i32 %pos491, 1
  %mask493 = load i32, ptr %mask, align 4
  %andtmp494 = and i32 %addtmp492, %mask493
  store i32 %andtmp494, ptr %pos, align 4
  br label %ifend485

ifend485:                                         ; preds = %else484, %then483
  %cur495 = load i32, ptr %probes, align 4
  %addas496 = add i32 %cur495, 1
  store i32 %addas496, ptr %probes, align 4
  br label %while.cond440

then504:                                          ; preds = %while.end442
  %calltmp507 = call i64 @now_ticks()
  store i64 %calltmp507, ptr %t, align 4
  %t508 = load i64, ptr %t, align 4
  %t0509 = load i64, ptr %t0, align 4
  %subtmp510 = sub i64 %t508, %t0509
  %multmp511 = mul i64 %subtmp510, 1000
  %freq512 = load i64, ptr %freq, align 4
  %divtmp513 = sdiv i64 %multmp511, %freq512
  store i64 %divtmp513, ptr %ms_now, align 4
  %list.ptr.addr517 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf515, i32 0, i32 0
  %list.len.addr518 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf515, i32 0, i32 1
  %list.cap.addr519 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf515, i32 0, i32 2
  store ptr null, ptr %list.ptr.addr517, align 8
  store i32 0, ptr %list.len.addr518, align 4
  store i32 0, ptr %list.cap.addr519, align 4
  %buf.ptr.addr520 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf515, i32 0, i32 0
  %buf.len.addr521 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf515, i32 0, i32 1
  %buf.cap.addr522 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf515, i32 0, i32 2
  %len523 = load i32, ptr %buf.len.addr521, align 4
  %cap524 = load i32, ptr %buf.cap.addr522, align 4
  %want.tmp525 = add i32 %len523, 9
  %want526 = add i32 %want.tmp525, 1
  %needgrow527 = icmp ugt i32 %want526, %cap524
  br i1 %needgrow527, label %fstr.grow528, label %fstr.grow.cont529

else505:                                          ; preds = %while.end442
  br label %ifend506

ifend506:                                         ; preds = %else505, %fstr.empty.cont801
  br label %while.cond413

fstr.grow528:                                     ; preds = %then504
  %cap0530 = icmp eq i32 %cap524, 0
  %cap2531 = mul i32 %cap524, 2
  %basecap532 = select i1 %cap0530, i32 64, i32 %cap2531
  %caplt533 = icmp ult i32 %basecap532, %want526
  %finalcap534 = select i1 %caplt533, i32 %want526, i32 %basecap532
  %list.ptr.addr535 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf515, i32 0, i32 0
  %list.cap.addr536 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf515, i32 0, i32 2
  %cap537 = load i32, ptr %list.cap.addr536, align 4
  %needcap538 = icmp ugt i32 %finalcap534, %cap537
  br i1 %needcap538, label %list.reserve539, label %list.reserve.end540

fstr.grow.cont529:                                ; preds = %list.reserve.end540, %then504
  %ptr552 = load ptr, ptr %buf.ptr.addr520, align 8
  %len2553 = load i32, ptr %buf.len.addr521, align 4
  br i1 false, label %fstr.zero554, label %fstr.cpy555

list.reserve539:                                  ; preds = %fstr.grow528
  %cap64541 = zext i32 %finalcap534 to i64
  %bytes542 = mul i64 %cap64541, ptrtoint (ptr getelementptr (i8, ptr null, i32 1) to i64)
  %heap543 = call ptr @GetProcessHeap()
  %ptr544 = load ptr, ptr %list.ptr.addr535, align 8
  %isnull545 = icmp eq ptr %ptr544, null
  br i1 %isnull545, label %list.alloc546, label %list.realloc547

list.reserve.end540:                              ; preds = %list.alloc.merge548, %fstr.grow528
  br label %fstr.grow.cont529

list.alloc546:                                    ; preds = %list.reserve539
  %mem549 = call ptr @HeapAlloc(ptr %heap543, i32 0, i64 %bytes542)
  br label %list.alloc.merge548

list.realloc547:                                  ; preds = %list.reserve539
  %mem550 = call ptr @HeapReAlloc(ptr %heap543, i32 0, ptr %ptr544, i64 %bytes542)
  br label %list.alloc.merge548

list.alloc.merge548:                              ; preds = %list.realloc547, %list.alloc546
  %memphi551 = phi ptr [ %mem549, %list.alloc546 ], [ %mem550, %list.realloc547 ]
  store ptr %memphi551, ptr %list.ptr.addr535, align 8
  store i32 %finalcap534, ptr %list.cap.addr536, align 4
  br label %list.reserve.end540

fstr.zero554:                                     ; preds = %fstr.grow.cont529
  %nul.ptr557 = getelementptr inbounds i8, ptr %ptr552, i32 %len2553
  store i8 0, ptr %nul.ptr557, align 1
  br label %fstr.cpy.end556

fstr.cpy555:                                      ; preds = %fstr.grow.cont529
  store i32 0, ptr %fstr.cpy.i558, align 4
  br label %fstr.cpy.loop559

fstr.cpy.end556:                                  ; preds = %fstr.cpy.done561, %fstr.zero554
  %k571 = load i32, ptr %k, align 4
  %sext572 = sext i32 %k571 to i64
  %neg573 = icmp slt i64 %sext572, 0
  %negv574 = sub i64 0, %sext572
  %abs575 = select i1 %neg573, i64 %negv574, i64 %sext572
  store i32 31, ptr %fstr.itoa.idx576, align 4
  store i64 %abs575, ptr %fstr.itoa.cur577, align 4
  %end.ptr578 = getelementptr inbounds [32 x i8], ptr %fstr.itoa.buf516, i32 0, i32 31
  store i8 0, ptr %end.ptr578, align 1
  %iszero579 = icmp eq i64 %abs575, 0
  br i1 %iszero579, label %itoa.zero580, label %itoa.loop581

fstr.cpy.loop559:                                 ; preds = %fstr.cpy.body560, %fstr.cpy555
  %i562 = load i32, ptr %fstr.cpy.i558, align 4
  %cond563 = icmp ult i32 %i562, 9
  br i1 %cond563, label %fstr.cpy.body560, label %fstr.cpy.done561

fstr.cpy.body560:                                 ; preds = %fstr.cpy.loop559
  %s.ptr564 = getelementptr inbounds i8, ptr @.fstr.1000009, i32 %i562
  %ch565 = load i8, ptr %s.ptr564, align 1
  %dst.off566 = add i32 %len2553, %i562
  %d.ptr567 = getelementptr inbounds i8, ptr %ptr552, i32 %dst.off566
  store i8 %ch565, ptr %d.ptr567, align 1
  %i2568 = add i32 %i562, 1
  store i32 %i2568, ptr %fstr.cpy.i558, align 4
  br label %fstr.cpy.loop559

fstr.cpy.done561:                                 ; preds = %fstr.cpy.loop559
  %newlen569 = add i32 %len2553, 9
  store i32 %newlen569, ptr %buf.len.addr521, align 4
  %nul.ptr570 = getelementptr inbounds i8, ptr %ptr552, i32 %newlen569
  store i8 0, ptr %nul.ptr570, align 1
  br label %fstr.cpy.end556

itoa.zero580:                                     ; preds = %fstr.cpy.end556
  store i32 30, ptr %fstr.itoa.idx576, align 4
  %z.ptr583 = getelementptr inbounds [32 x i8], ptr %fstr.itoa.buf516, i32 0, i32 30
  store i8 48, ptr %z.ptr583, align 1
  br label %itoa.after582

itoa.loop581:                                     ; preds = %fstr.cpy.end556
  br label %itoa.cond584

itoa.after582:                                    ; preds = %itoa.done586, %itoa.zero580
  br i1 %neg573, label %itoa.neg597, label %itoa.merge598

itoa.cond584:                                     ; preds = %itoa.body585, %itoa.loop581
  %curv587 = load i64, ptr %fstr.itoa.cur577, align 4
  %more588 = icmp ugt i64 %curv587, 0
  br i1 %more588, label %itoa.body585, label %itoa.done586

itoa.body585:                                     ; preds = %itoa.cond584
  %rem589 = urem i64 %curv587, 10
  %div590 = udiv i64 %curv587, 10
  store i64 %div590, ptr %fstr.itoa.cur577, align 4
  %idxv591 = load i32, ptr %fstr.itoa.idx576, align 4
  %idx1592 = sub i32 %idxv591, 1
  store i32 %idx1592, ptr %fstr.itoa.idx576, align 4
  %rem32593 = trunc i64 %rem589 to i32
  %ch32594 = add i32 %rem32593, 48
  %ch595 = trunc i32 %ch32594 to i8
  %d.ptr596 = getelementptr inbounds [32 x i8], ptr %fstr.itoa.buf516, i32 0, i32 %idx1592
  store i8 %ch595, ptr %d.ptr596, align 1
  br label %itoa.cond584

itoa.done586:                                     ; preds = %itoa.cond584
  br label %itoa.after582

itoa.neg597:                                      ; preds = %itoa.after582
  %idxv599 = load i32, ptr %fstr.itoa.idx576, align 4
  %idxm1600 = sub i32 %idxv599, 1
  store i32 %idxm1600, ptr %fstr.itoa.idx576, align 4
  %m.ptr601 = getelementptr inbounds [32 x i8], ptr %fstr.itoa.buf516, i32 0, i32 %idxm1600
  store i8 45, ptr %m.ptr601, align 1
  br label %itoa.merge598

itoa.merge598:                                    ; preds = %itoa.neg597, %itoa.after582
  %start602 = load i32, ptr %fstr.itoa.idx576, align 4
  %slen603 = sub i32 31, %start602
  %sptr604 = getelementptr inbounds [32 x i8], ptr %fstr.itoa.buf516, i32 0, i32 %start602
  %buf.ptr.addr605 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf515, i32 0, i32 0
  %buf.len.addr606 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf515, i32 0, i32 1
  %buf.cap.addr607 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf515, i32 0, i32 2
  %len608 = load i32, ptr %buf.len.addr606, align 4
  %cap609 = load i32, ptr %buf.cap.addr607, align 4
  %want.tmp610 = add i32 %len608, %slen603
  %want611 = add i32 %want.tmp610, 1
  %needgrow612 = icmp ugt i32 %want611, %cap609
  br i1 %needgrow612, label %fstr.grow613, label %fstr.grow.cont614

fstr.grow613:                                     ; preds = %itoa.merge598
  %cap0615 = icmp eq i32 %cap609, 0
  %cap2616 = mul i32 %cap609, 2
  %basecap617 = select i1 %cap0615, i32 64, i32 %cap2616
  %caplt618 = icmp ult i32 %basecap617, %want611
  %finalcap619 = select i1 %caplt618, i32 %want611, i32 %basecap617
  %list.ptr.addr620 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf515, i32 0, i32 0
  %list.cap.addr621 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf515, i32 0, i32 2
  %cap622 = load i32, ptr %list.cap.addr621, align 4
  %needcap623 = icmp ugt i32 %finalcap619, %cap622
  br i1 %needcap623, label %list.reserve624, label %list.reserve.end625

fstr.grow.cont614:                                ; preds = %list.reserve.end625, %itoa.merge598
  %ptr637 = load ptr, ptr %buf.ptr.addr605, align 8
  %len2638 = load i32, ptr %buf.len.addr606, align 4
  %srclen0639 = icmp eq i32 %slen603, 0
  br i1 %srclen0639, label %fstr.zero640, label %fstr.cpy641

list.reserve624:                                  ; preds = %fstr.grow613
  %cap64626 = zext i32 %finalcap619 to i64
  %bytes627 = mul i64 %cap64626, ptrtoint (ptr getelementptr (i8, ptr null, i32 1) to i64)
  %heap628 = call ptr @GetProcessHeap()
  %ptr629 = load ptr, ptr %list.ptr.addr620, align 8
  %isnull630 = icmp eq ptr %ptr629, null
  br i1 %isnull630, label %list.alloc631, label %list.realloc632

list.reserve.end625:                              ; preds = %list.alloc.merge633, %fstr.grow613
  br label %fstr.grow.cont614

list.alloc631:                                    ; preds = %list.reserve624
  %mem634 = call ptr @HeapAlloc(ptr %heap628, i32 0, i64 %bytes627)
  br label %list.alloc.merge633

list.realloc632:                                  ; preds = %list.reserve624
  %mem635 = call ptr @HeapReAlloc(ptr %heap628, i32 0, ptr %ptr629, i64 %bytes627)
  br label %list.alloc.merge633

list.alloc.merge633:                              ; preds = %list.realloc632, %list.alloc631
  %memphi636 = phi ptr [ %mem634, %list.alloc631 ], [ %mem635, %list.realloc632 ]
  store ptr %memphi636, ptr %list.ptr.addr620, align 8
  store i32 %finalcap619, ptr %list.cap.addr621, align 4
  br label %list.reserve.end625

fstr.zero640:                                     ; preds = %fstr.grow.cont614
  %nul.ptr643 = getelementptr inbounds i8, ptr %ptr637, i32 %len2638
  store i8 0, ptr %nul.ptr643, align 1
  br label %fstr.cpy.end642

fstr.cpy641:                                      ; preds = %fstr.grow.cont614
  store i32 0, ptr %fstr.cpy.i644, align 4
  br label %fstr.cpy.loop645

fstr.cpy.end642:                                  ; preds = %fstr.cpy.done647, %fstr.zero640
  %buf.ptr.addr657 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf515, i32 0, i32 0
  %buf.len.addr658 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf515, i32 0, i32 1
  %buf.cap.addr659 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf515, i32 0, i32 2
  %len660 = load i32, ptr %buf.len.addr658, align 4
  %cap661 = load i32, ptr %buf.cap.addr659, align 4
  %want.tmp662 = add i32 %len660, 9
  %want663 = add i32 %want.tmp662, 1
  %needgrow664 = icmp ugt i32 %want663, %cap661
  br i1 %needgrow664, label %fstr.grow665, label %fstr.grow.cont666

fstr.cpy.loop645:                                 ; preds = %fstr.cpy.body646, %fstr.cpy641
  %i648 = load i32, ptr %fstr.cpy.i644, align 4
  %cond649 = icmp ult i32 %i648, %slen603
  br i1 %cond649, label %fstr.cpy.body646, label %fstr.cpy.done647

fstr.cpy.body646:                                 ; preds = %fstr.cpy.loop645
  %s.ptr650 = getelementptr inbounds i8, ptr %sptr604, i32 %i648
  %ch651 = load i8, ptr %s.ptr650, align 1
  %dst.off652 = add i32 %len2638, %i648
  %d.ptr653 = getelementptr inbounds i8, ptr %ptr637, i32 %dst.off652
  store i8 %ch651, ptr %d.ptr653, align 1
  %i2654 = add i32 %i648, 1
  store i32 %i2654, ptr %fstr.cpy.i644, align 4
  br label %fstr.cpy.loop645

fstr.cpy.done647:                                 ; preds = %fstr.cpy.loop645
  %newlen655 = add i32 %len2638, %slen603
  store i32 %newlen655, ptr %buf.len.addr606, align 4
  %nul.ptr656 = getelementptr inbounds i8, ptr %ptr637, i32 %newlen655
  store i8 0, ptr %nul.ptr656, align 1
  br label %fstr.cpy.end642

fstr.grow665:                                     ; preds = %fstr.cpy.end642
  %cap0667 = icmp eq i32 %cap661, 0
  %cap2668 = mul i32 %cap661, 2
  %basecap669 = select i1 %cap0667, i32 64, i32 %cap2668
  %caplt670 = icmp ult i32 %basecap669, %want663
  %finalcap671 = select i1 %caplt670, i32 %want663, i32 %basecap669
  %list.ptr.addr672 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf515, i32 0, i32 0
  %list.cap.addr673 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf515, i32 0, i32 2
  %cap674 = load i32, ptr %list.cap.addr673, align 4
  %needcap675 = icmp ugt i32 %finalcap671, %cap674
  br i1 %needcap675, label %list.reserve676, label %list.reserve.end677

fstr.grow.cont666:                                ; preds = %list.reserve.end677, %fstr.cpy.end642
  %ptr689 = load ptr, ptr %buf.ptr.addr657, align 8
  %len2690 = load i32, ptr %buf.len.addr658, align 4
  br i1 false, label %fstr.zero691, label %fstr.cpy692

list.reserve676:                                  ; preds = %fstr.grow665
  %cap64678 = zext i32 %finalcap671 to i64
  %bytes679 = mul i64 %cap64678, ptrtoint (ptr getelementptr (i8, ptr null, i32 1) to i64)
  %heap680 = call ptr @GetProcessHeap()
  %ptr681 = load ptr, ptr %list.ptr.addr672, align 8
  %isnull682 = icmp eq ptr %ptr681, null
  br i1 %isnull682, label %list.alloc683, label %list.realloc684

list.reserve.end677:                              ; preds = %list.alloc.merge685, %fstr.grow665
  br label %fstr.grow.cont666

list.alloc683:                                    ; preds = %list.reserve676
  %mem686 = call ptr @HeapAlloc(ptr %heap680, i32 0, i64 %bytes679)
  br label %list.alloc.merge685

list.realloc684:                                  ; preds = %list.reserve676
  %mem687 = call ptr @HeapReAlloc(ptr %heap680, i32 0, ptr %ptr681, i64 %bytes679)
  br label %list.alloc.merge685

list.alloc.merge685:                              ; preds = %list.realloc684, %list.alloc683
  %memphi688 = phi ptr [ %mem686, %list.alloc683 ], [ %mem687, %list.realloc684 ]
  store ptr %memphi688, ptr %list.ptr.addr672, align 8
  store i32 %finalcap671, ptr %list.cap.addr673, align 4
  br label %list.reserve.end677

fstr.zero691:                                     ; preds = %fstr.grow.cont666
  %nul.ptr694 = getelementptr inbounds i8, ptr %ptr689, i32 %len2690
  store i8 0, ptr %nul.ptr694, align 1
  br label %fstr.cpy.end693

fstr.cpy692:                                      ; preds = %fstr.grow.cont666
  store i32 0, ptr %fstr.cpy.i695, align 4
  br label %fstr.cpy.loop696

fstr.cpy.end693:                                  ; preds = %fstr.cpy.done698, %fstr.zero691
  %ms_now708 = load i64, ptr %ms_now, align 4
  %neg709 = icmp slt i64 %ms_now708, 0
  %negv710 = sub i64 0, %ms_now708
  %abs711 = select i1 %neg709, i64 %negv710, i64 %ms_now708
  store i32 31, ptr %fstr.itoa.idx712, align 4
  store i64 %abs711, ptr %fstr.itoa.cur713, align 4
  %end.ptr714 = getelementptr inbounds [32 x i8], ptr %fstr.itoa.buf516, i32 0, i32 31
  store i8 0, ptr %end.ptr714, align 1
  %iszero715 = icmp eq i64 %abs711, 0
  br i1 %iszero715, label %itoa.zero716, label %itoa.loop717

fstr.cpy.loop696:                                 ; preds = %fstr.cpy.body697, %fstr.cpy692
  %i699 = load i32, ptr %fstr.cpy.i695, align 4
  %cond700 = icmp ult i32 %i699, 9
  br i1 %cond700, label %fstr.cpy.body697, label %fstr.cpy.done698

fstr.cpy.body697:                                 ; preds = %fstr.cpy.loop696
  %s.ptr701 = getelementptr inbounds i8, ptr @.fstr.1000010, i32 %i699
  %ch702 = load i8, ptr %s.ptr701, align 1
  %dst.off703 = add i32 %len2690, %i699
  %d.ptr704 = getelementptr inbounds i8, ptr %ptr689, i32 %dst.off703
  store i8 %ch702, ptr %d.ptr704, align 1
  %i2705 = add i32 %i699, 1
  store i32 %i2705, ptr %fstr.cpy.i695, align 4
  br label %fstr.cpy.loop696

fstr.cpy.done698:                                 ; preds = %fstr.cpy.loop696
  %newlen706 = add i32 %len2690, 9
  store i32 %newlen706, ptr %buf.len.addr658, align 4
  %nul.ptr707 = getelementptr inbounds i8, ptr %ptr689, i32 %newlen706
  store i8 0, ptr %nul.ptr707, align 1
  br label %fstr.cpy.end693

itoa.zero716:                                     ; preds = %fstr.cpy.end693
  store i32 30, ptr %fstr.itoa.idx712, align 4
  %z.ptr719 = getelementptr inbounds [32 x i8], ptr %fstr.itoa.buf516, i32 0, i32 30
  store i8 48, ptr %z.ptr719, align 1
  br label %itoa.after718

itoa.loop717:                                     ; preds = %fstr.cpy.end693
  br label %itoa.cond720

itoa.after718:                                    ; preds = %itoa.done722, %itoa.zero716
  br i1 %neg709, label %itoa.neg733, label %itoa.merge734

itoa.cond720:                                     ; preds = %itoa.body721, %itoa.loop717
  %curv723 = load i64, ptr %fstr.itoa.cur713, align 4
  %more724 = icmp ugt i64 %curv723, 0
  br i1 %more724, label %itoa.body721, label %itoa.done722

itoa.body721:                                     ; preds = %itoa.cond720
  %rem725 = urem i64 %curv723, 10
  %div726 = udiv i64 %curv723, 10
  store i64 %div726, ptr %fstr.itoa.cur713, align 4
  %idxv727 = load i32, ptr %fstr.itoa.idx712, align 4
  %idx1728 = sub i32 %idxv727, 1
  store i32 %idx1728, ptr %fstr.itoa.idx712, align 4
  %rem32729 = trunc i64 %rem725 to i32
  %ch32730 = add i32 %rem32729, 48
  %ch731 = trunc i32 %ch32730 to i8
  %d.ptr732 = getelementptr inbounds [32 x i8], ptr %fstr.itoa.buf516, i32 0, i32 %idx1728
  store i8 %ch731, ptr %d.ptr732, align 1
  br label %itoa.cond720

itoa.done722:                                     ; preds = %itoa.cond720
  br label %itoa.after718

itoa.neg733:                                      ; preds = %itoa.after718
  %idxv735 = load i32, ptr %fstr.itoa.idx712, align 4
  %idxm1736 = sub i32 %idxv735, 1
  store i32 %idxm1736, ptr %fstr.itoa.idx712, align 4
  %m.ptr737 = getelementptr inbounds [32 x i8], ptr %fstr.itoa.buf516, i32 0, i32 %idxm1736
  store i8 45, ptr %m.ptr737, align 1
  br label %itoa.merge734

itoa.merge734:                                    ; preds = %itoa.neg733, %itoa.after718
  %start738 = load i32, ptr %fstr.itoa.idx712, align 4
  %slen739 = sub i32 31, %start738
  %sptr740 = getelementptr inbounds [32 x i8], ptr %fstr.itoa.buf516, i32 0, i32 %start738
  %buf.ptr.addr741 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf515, i32 0, i32 0
  %buf.len.addr742 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf515, i32 0, i32 1
  %buf.cap.addr743 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf515, i32 0, i32 2
  %len744 = load i32, ptr %buf.len.addr742, align 4
  %cap745 = load i32, ptr %buf.cap.addr743, align 4
  %want.tmp746 = add i32 %len744, %slen739
  %want747 = add i32 %want.tmp746, 1
  %needgrow748 = icmp ugt i32 %want747, %cap745
  br i1 %needgrow748, label %fstr.grow749, label %fstr.grow.cont750

fstr.grow749:                                     ; preds = %itoa.merge734
  %cap0751 = icmp eq i32 %cap745, 0
  %cap2752 = mul i32 %cap745, 2
  %basecap753 = select i1 %cap0751, i32 64, i32 %cap2752
  %caplt754 = icmp ult i32 %basecap753, %want747
  %finalcap755 = select i1 %caplt754, i32 %want747, i32 %basecap753
  %list.ptr.addr756 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf515, i32 0, i32 0
  %list.cap.addr757 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf515, i32 0, i32 2
  %cap758 = load i32, ptr %list.cap.addr757, align 4
  %needcap759 = icmp ugt i32 %finalcap755, %cap758
  br i1 %needcap759, label %list.reserve760, label %list.reserve.end761

fstr.grow.cont750:                                ; preds = %list.reserve.end761, %itoa.merge734
  %ptr773 = load ptr, ptr %buf.ptr.addr741, align 8
  %len2774 = load i32, ptr %buf.len.addr742, align 4
  %srclen0775 = icmp eq i32 %slen739, 0
  br i1 %srclen0775, label %fstr.zero776, label %fstr.cpy777

list.reserve760:                                  ; preds = %fstr.grow749
  %cap64762 = zext i32 %finalcap755 to i64
  %bytes763 = mul i64 %cap64762, ptrtoint (ptr getelementptr (i8, ptr null, i32 1) to i64)
  %heap764 = call ptr @GetProcessHeap()
  %ptr765 = load ptr, ptr %list.ptr.addr756, align 8
  %isnull766 = icmp eq ptr %ptr765, null
  br i1 %isnull766, label %list.alloc767, label %list.realloc768

list.reserve.end761:                              ; preds = %list.alloc.merge769, %fstr.grow749
  br label %fstr.grow.cont750

list.alloc767:                                    ; preds = %list.reserve760
  %mem770 = call ptr @HeapAlloc(ptr %heap764, i32 0, i64 %bytes763)
  br label %list.alloc.merge769

list.realloc768:                                  ; preds = %list.reserve760
  %mem771 = call ptr @HeapReAlloc(ptr %heap764, i32 0, ptr %ptr765, i64 %bytes763)
  br label %list.alloc.merge769

list.alloc.merge769:                              ; preds = %list.realloc768, %list.alloc767
  %memphi772 = phi ptr [ %mem770, %list.alloc767 ], [ %mem771, %list.realloc768 ]
  store ptr %memphi772, ptr %list.ptr.addr756, align 8
  store i32 %finalcap755, ptr %list.cap.addr757, align 4
  br label %list.reserve.end761

fstr.zero776:                                     ; preds = %fstr.grow.cont750
  %nul.ptr779 = getelementptr inbounds i8, ptr %ptr773, i32 %len2774
  store i8 0, ptr %nul.ptr779, align 1
  br label %fstr.cpy.end778

fstr.cpy777:                                      ; preds = %fstr.grow.cont750
  store i32 0, ptr %fstr.cpy.i780, align 4
  br label %fstr.cpy.loop781

fstr.cpy.end778:                                  ; preds = %fstr.cpy.done783, %fstr.zero776
  %buf.len.addr793 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf515, i32 0, i32 1
  %buf.len794 = load i32, ptr %buf.len.addr793, align 4
  %fstr.empty795 = icmp eq i32 %buf.len794, 0
  %buf.ptr.addr796 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf515, i32 0, i32 0
  %outp0797 = load ptr, ptr %buf.ptr.addr796, align 8
  %out.null0798 = icmp eq ptr %outp0797, null
  %needalloc799 = and i1 %fstr.empty795, %out.null0798
  br i1 %needalloc799, label %fstr.empty.alloc800, label %fstr.empty.cont801

fstr.cpy.loop781:                                 ; preds = %fstr.cpy.body782, %fstr.cpy777
  %i784 = load i32, ptr %fstr.cpy.i780, align 4
  %cond785 = icmp ult i32 %i784, %slen739
  br i1 %cond785, label %fstr.cpy.body782, label %fstr.cpy.done783

fstr.cpy.body782:                                 ; preds = %fstr.cpy.loop781
  %s.ptr786 = getelementptr inbounds i8, ptr %sptr740, i32 %i784
  %ch787 = load i8, ptr %s.ptr786, align 1
  %dst.off788 = add i32 %len2774, %i784
  %d.ptr789 = getelementptr inbounds i8, ptr %ptr773, i32 %dst.off788
  store i8 %ch787, ptr %d.ptr789, align 1
  %i2790 = add i32 %i784, 1
  store i32 %i2790, ptr %fstr.cpy.i780, align 4
  br label %fstr.cpy.loop781

fstr.cpy.done783:                                 ; preds = %fstr.cpy.loop781
  %newlen791 = add i32 %len2774, %slen739
  store i32 %newlen791, ptr %buf.len.addr742, align 4
  %nul.ptr792 = getelementptr inbounds i8, ptr %ptr773, i32 %newlen791
  store i8 0, ptr %nul.ptr792, align 1
  br label %fstr.cpy.end778

fstr.empty.alloc800:                              ; preds = %fstr.cpy.end778
  %buf.ptr.addr802 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf515, i32 0, i32 0
  %buf.len.addr803 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf515, i32 0, i32 1
  %buf.cap.addr804 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf515, i32 0, i32 2
  %len805 = load i32, ptr %buf.len.addr803, align 4
  %cap806 = load i32, ptr %buf.cap.addr804, align 4
  %want.tmp807 = add i32 %len805, 0
  %want808 = add i32 %want.tmp807, 1
  %needgrow809 = icmp ugt i32 %want808, %cap806
  br i1 %needgrow809, label %fstr.grow810, label %fstr.grow.cont811

fstr.empty.cont801:                               ; preds = %fstr.cpy.end838, %fstr.cpy.end778
  %fstr.val853 = load %emp.string, ptr %fstr.buf515, align 8
  store %emp.string %fstr.val853, ptr %msg514, align 8
  %msg854 = load %emp.string, ptr %msg514, align 8
  call void @println__Nstring(%emp.string %msg854)
  %progress_step855 = load i32, ptr %progress_step, align 4
  %cur856 = load i32, ptr %next_report, align 4
  %addas857 = add i32 %cur856, %progress_step855
  store i32 %addas857, ptr %next_report, align 4
  br label %ifend506

fstr.grow810:                                     ; preds = %fstr.empty.alloc800
  %cap0812 = icmp eq i32 %cap806, 0
  %cap2813 = mul i32 %cap806, 2
  %basecap814 = select i1 %cap0812, i32 64, i32 %cap2813
  %caplt815 = icmp ult i32 %basecap814, %want808
  %finalcap816 = select i1 %caplt815, i32 %want808, i32 %basecap814
  %list.ptr.addr817 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf515, i32 0, i32 0
  %list.cap.addr818 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf515, i32 0, i32 2
  %cap819 = load i32, ptr %list.cap.addr818, align 4
  %needcap820 = icmp ugt i32 %finalcap816, %cap819
  br i1 %needcap820, label %list.reserve821, label %list.reserve.end822

fstr.grow.cont811:                                ; preds = %list.reserve.end822, %fstr.empty.alloc800
  %ptr834 = load ptr, ptr %buf.ptr.addr802, align 8
  %len2835 = load i32, ptr %buf.len.addr803, align 4
  br i1 true, label %fstr.zero836, label %fstr.cpy837

list.reserve821:                                  ; preds = %fstr.grow810
  %cap64823 = zext i32 %finalcap816 to i64
  %bytes824 = mul i64 %cap64823, ptrtoint (ptr getelementptr (i8, ptr null, i32 1) to i64)
  %heap825 = call ptr @GetProcessHeap()
  %ptr826 = load ptr, ptr %list.ptr.addr817, align 8
  %isnull827 = icmp eq ptr %ptr826, null
  br i1 %isnull827, label %list.alloc828, label %list.realloc829

list.reserve.end822:                              ; preds = %list.alloc.merge830, %fstr.grow810
  br label %fstr.grow.cont811

list.alloc828:                                    ; preds = %list.reserve821
  %mem831 = call ptr @HeapAlloc(ptr %heap825, i32 0, i64 %bytes824)
  br label %list.alloc.merge830

list.realloc829:                                  ; preds = %list.reserve821
  %mem832 = call ptr @HeapReAlloc(ptr %heap825, i32 0, ptr %ptr826, i64 %bytes824)
  br label %list.alloc.merge830

list.alloc.merge830:                              ; preds = %list.realloc829, %list.alloc828
  %memphi833 = phi ptr [ %mem831, %list.alloc828 ], [ %mem832, %list.realloc829 ]
  store ptr %memphi833, ptr %list.ptr.addr817, align 8
  store i32 %finalcap816, ptr %list.cap.addr818, align 4
  br label %list.reserve.end822

fstr.zero836:                                     ; preds = %fstr.grow.cont811
  %nul.ptr839 = getelementptr inbounds i8, ptr %ptr834, i32 %len2835
  store i8 0, ptr %nul.ptr839, align 1
  br label %fstr.cpy.end838

fstr.cpy837:                                      ; preds = %fstr.grow.cont811
  store i32 0, ptr %fstr.cpy.i840, align 4
  br label %fstr.cpy.loop841

fstr.cpy.end838:                                  ; preds = %fstr.cpy.done843, %fstr.zero836
  br label %fstr.empty.cont801

fstr.cpy.loop841:                                 ; preds = %fstr.cpy.body842, %fstr.cpy837
  %i844 = load i32, ptr %fstr.cpy.i840, align 4
  %cond845 = icmp ult i32 %i844, 0
  br i1 %cond845, label %fstr.cpy.body842, label %fstr.cpy.done843

fstr.cpy.body842:                                 ; preds = %fstr.cpy.loop841
  %s.ptr846 = getelementptr inbounds i8, ptr @.fstr.1000005, i32 %i844
  %ch847 = load i8, ptr %s.ptr846, align 1
  %dst.off848 = add i32 %len2835, %i844
  %d.ptr849 = getelementptr inbounds i8, ptr %ptr834, i32 %dst.off848
  store i8 %ch847, ptr %d.ptr849, align 1
  %i2850 = add i32 %i844, 1
  store i32 %i2850, ptr %fstr.cpy.i840, align 4
  br label %fstr.cpy.loop841

fstr.cpy.done843:                                 ; preds = %fstr.cpy.loop841
  %newlen851 = add i32 %len2835, 0
  store i32 %newlen851, ptr %buf.len.addr803, align 4
  %nul.ptr852 = getelementptr inbounds i8, ptr %ptr834, i32 %newlen851
  store i8 0, ptr %nul.ptr852, align 1
  br label %fstr.cpy.end838

fstr.grow880:                                     ; preds = %while.end415
  %cap0882 = icmp eq i32 %cap876, 0
  %cap2883 = mul i32 %cap876, 2
  %basecap884 = select i1 %cap0882, i32 64, i32 %cap2883
  %caplt885 = icmp ult i32 %basecap884, %want878
  %finalcap886 = select i1 %caplt885, i32 %want878, i32 %basecap884
  %list.ptr.addr887 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf867, i32 0, i32 0
  %list.cap.addr888 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf867, i32 0, i32 2
  %cap889 = load i32, ptr %list.cap.addr888, align 4
  %needcap890 = icmp ugt i32 %finalcap886, %cap889
  br i1 %needcap890, label %list.reserve891, label %list.reserve.end892

fstr.grow.cont881:                                ; preds = %list.reserve.end892, %while.end415
  %ptr904 = load ptr, ptr %buf.ptr.addr872, align 8
  %len2905 = load i32, ptr %buf.len.addr873, align 4
  br i1 false, label %fstr.zero906, label %fstr.cpy907

list.reserve891:                                  ; preds = %fstr.grow880
  %cap64893 = zext i32 %finalcap886 to i64
  %bytes894 = mul i64 %cap64893, ptrtoint (ptr getelementptr (i8, ptr null, i32 1) to i64)
  %heap895 = call ptr @GetProcessHeap()
  %ptr896 = load ptr, ptr %list.ptr.addr887, align 8
  %isnull897 = icmp eq ptr %ptr896, null
  br i1 %isnull897, label %list.alloc898, label %list.realloc899

list.reserve.end892:                              ; preds = %list.alloc.merge900, %fstr.grow880
  br label %fstr.grow.cont881

list.alloc898:                                    ; preds = %list.reserve891
  %mem901 = call ptr @HeapAlloc(ptr %heap895, i32 0, i64 %bytes894)
  br label %list.alloc.merge900

list.realloc899:                                  ; preds = %list.reserve891
  %mem902 = call ptr @HeapReAlloc(ptr %heap895, i32 0, ptr %ptr896, i64 %bytes894)
  br label %list.alloc.merge900

list.alloc.merge900:                              ; preds = %list.realloc899, %list.alloc898
  %memphi903 = phi ptr [ %mem901, %list.alloc898 ], [ %mem902, %list.realloc899 ]
  store ptr %memphi903, ptr %list.ptr.addr887, align 8
  store i32 %finalcap886, ptr %list.cap.addr888, align 4
  br label %list.reserve.end892

fstr.zero906:                                     ; preds = %fstr.grow.cont881
  %nul.ptr909 = getelementptr inbounds i8, ptr %ptr904, i32 %len2905
  store i8 0, ptr %nul.ptr909, align 1
  br label %fstr.cpy.end908

fstr.cpy907:                                      ; preds = %fstr.grow.cont881
  store i32 0, ptr %fstr.cpy.i910, align 4
  br label %fstr.cpy.loop911

fstr.cpy.end908:                                  ; preds = %fstr.cpy.done913, %fstr.zero906
  %entries_max923 = load i32, ptr %entries_max, align 4
  %sext924 = sext i32 %entries_max923 to i64
  %neg925 = icmp slt i64 %sext924, 0
  %negv926 = sub i64 0, %sext924
  %abs927 = select i1 %neg925, i64 %negv926, i64 %sext924
  store i32 31, ptr %fstr.itoa.idx928, align 4
  store i64 %abs927, ptr %fstr.itoa.cur929, align 4
  %end.ptr930 = getelementptr inbounds [32 x i8], ptr %fstr.itoa.buf868, i32 0, i32 31
  store i8 0, ptr %end.ptr930, align 1
  %iszero931 = icmp eq i64 %abs927, 0
  br i1 %iszero931, label %itoa.zero932, label %itoa.loop933

fstr.cpy.loop911:                                 ; preds = %fstr.cpy.body912, %fstr.cpy907
  %i914 = load i32, ptr %fstr.cpy.i910, align 4
  %cond915 = icmp ult i32 %i914, 15
  br i1 %cond915, label %fstr.cpy.body912, label %fstr.cpy.done913

fstr.cpy.body912:                                 ; preds = %fstr.cpy.loop911
  %s.ptr916 = getelementptr inbounds i8, ptr @.fstr.1000015, i32 %i914
  %ch917 = load i8, ptr %s.ptr916, align 1
  %dst.off918 = add i32 %len2905, %i914
  %d.ptr919 = getelementptr inbounds i8, ptr %ptr904, i32 %dst.off918
  store i8 %ch917, ptr %d.ptr919, align 1
  %i2920 = add i32 %i914, 1
  store i32 %i2920, ptr %fstr.cpy.i910, align 4
  br label %fstr.cpy.loop911

fstr.cpy.done913:                                 ; preds = %fstr.cpy.loop911
  %newlen921 = add i32 %len2905, 15
  store i32 %newlen921, ptr %buf.len.addr873, align 4
  %nul.ptr922 = getelementptr inbounds i8, ptr %ptr904, i32 %newlen921
  store i8 0, ptr %nul.ptr922, align 1
  br label %fstr.cpy.end908

itoa.zero932:                                     ; preds = %fstr.cpy.end908
  store i32 30, ptr %fstr.itoa.idx928, align 4
  %z.ptr935 = getelementptr inbounds [32 x i8], ptr %fstr.itoa.buf868, i32 0, i32 30
  store i8 48, ptr %z.ptr935, align 1
  br label %itoa.after934

itoa.loop933:                                     ; preds = %fstr.cpy.end908
  br label %itoa.cond936

itoa.after934:                                    ; preds = %itoa.done938, %itoa.zero932
  br i1 %neg925, label %itoa.neg949, label %itoa.merge950

itoa.cond936:                                     ; preds = %itoa.body937, %itoa.loop933
  %curv939 = load i64, ptr %fstr.itoa.cur929, align 4
  %more940 = icmp ugt i64 %curv939, 0
  br i1 %more940, label %itoa.body937, label %itoa.done938

itoa.body937:                                     ; preds = %itoa.cond936
  %rem941 = urem i64 %curv939, 10
  %div942 = udiv i64 %curv939, 10
  store i64 %div942, ptr %fstr.itoa.cur929, align 4
  %idxv943 = load i32, ptr %fstr.itoa.idx928, align 4
  %idx1944 = sub i32 %idxv943, 1
  store i32 %idx1944, ptr %fstr.itoa.idx928, align 4
  %rem32945 = trunc i64 %rem941 to i32
  %ch32946 = add i32 %rem32945, 48
  %ch947 = trunc i32 %ch32946 to i8
  %d.ptr948 = getelementptr inbounds [32 x i8], ptr %fstr.itoa.buf868, i32 0, i32 %idx1944
  store i8 %ch947, ptr %d.ptr948, align 1
  br label %itoa.cond936

itoa.done938:                                     ; preds = %itoa.cond936
  br label %itoa.after934

itoa.neg949:                                      ; preds = %itoa.after934
  %idxv951 = load i32, ptr %fstr.itoa.idx928, align 4
  %idxm1952 = sub i32 %idxv951, 1
  store i32 %idxm1952, ptr %fstr.itoa.idx928, align 4
  %m.ptr953 = getelementptr inbounds [32 x i8], ptr %fstr.itoa.buf868, i32 0, i32 %idxm1952
  store i8 45, ptr %m.ptr953, align 1
  br label %itoa.merge950

itoa.merge950:                                    ; preds = %itoa.neg949, %itoa.after934
  %start954 = load i32, ptr %fstr.itoa.idx928, align 4
  %slen955 = sub i32 31, %start954
  %sptr956 = getelementptr inbounds [32 x i8], ptr %fstr.itoa.buf868, i32 0, i32 %start954
  %buf.ptr.addr957 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf867, i32 0, i32 0
  %buf.len.addr958 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf867, i32 0, i32 1
  %buf.cap.addr959 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf867, i32 0, i32 2
  %len960 = load i32, ptr %buf.len.addr958, align 4
  %cap961 = load i32, ptr %buf.cap.addr959, align 4
  %want.tmp962 = add i32 %len960, %slen955
  %want963 = add i32 %want.tmp962, 1
  %needgrow964 = icmp ugt i32 %want963, %cap961
  br i1 %needgrow964, label %fstr.grow965, label %fstr.grow.cont966

fstr.grow965:                                     ; preds = %itoa.merge950
  %cap0967 = icmp eq i32 %cap961, 0
  %cap2968 = mul i32 %cap961, 2
  %basecap969 = select i1 %cap0967, i32 64, i32 %cap2968
  %caplt970 = icmp ult i32 %basecap969, %want963
  %finalcap971 = select i1 %caplt970, i32 %want963, i32 %basecap969
  %list.ptr.addr972 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf867, i32 0, i32 0
  %list.cap.addr973 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf867, i32 0, i32 2
  %cap974 = load i32, ptr %list.cap.addr973, align 4
  %needcap975 = icmp ugt i32 %finalcap971, %cap974
  br i1 %needcap975, label %list.reserve976, label %list.reserve.end977

fstr.grow.cont966:                                ; preds = %list.reserve.end977, %itoa.merge950
  %ptr989 = load ptr, ptr %buf.ptr.addr957, align 8
  %len2990 = load i32, ptr %buf.len.addr958, align 4
  %srclen0991 = icmp eq i32 %slen955, 0
  br i1 %srclen0991, label %fstr.zero992, label %fstr.cpy993

list.reserve976:                                  ; preds = %fstr.grow965
  %cap64978 = zext i32 %finalcap971 to i64
  %bytes979 = mul i64 %cap64978, ptrtoint (ptr getelementptr (i8, ptr null, i32 1) to i64)
  %heap980 = call ptr @GetProcessHeap()
  %ptr981 = load ptr, ptr %list.ptr.addr972, align 8
  %isnull982 = icmp eq ptr %ptr981, null
  br i1 %isnull982, label %list.alloc983, label %list.realloc984

list.reserve.end977:                              ; preds = %list.alloc.merge985, %fstr.grow965
  br label %fstr.grow.cont966

list.alloc983:                                    ; preds = %list.reserve976
  %mem986 = call ptr @HeapAlloc(ptr %heap980, i32 0, i64 %bytes979)
  br label %list.alloc.merge985

list.realloc984:                                  ; preds = %list.reserve976
  %mem987 = call ptr @HeapReAlloc(ptr %heap980, i32 0, ptr %ptr981, i64 %bytes979)
  br label %list.alloc.merge985

list.alloc.merge985:                              ; preds = %list.realloc984, %list.alloc983
  %memphi988 = phi ptr [ %mem986, %list.alloc983 ], [ %mem987, %list.realloc984 ]
  store ptr %memphi988, ptr %list.ptr.addr972, align 8
  store i32 %finalcap971, ptr %list.cap.addr973, align 4
  br label %list.reserve.end977

fstr.zero992:                                     ; preds = %fstr.grow.cont966
  %nul.ptr995 = getelementptr inbounds i8, ptr %ptr989, i32 %len2990
  store i8 0, ptr %nul.ptr995, align 1
  br label %fstr.cpy.end994

fstr.cpy993:                                      ; preds = %fstr.grow.cont966
  store i32 0, ptr %fstr.cpy.i996, align 4
  br label %fstr.cpy.loop997

fstr.cpy.end994:                                  ; preds = %fstr.cpy.done999, %fstr.zero992
  %buf.ptr.addr1009 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf867, i32 0, i32 0
  %buf.len.addr1010 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf867, i32 0, i32 1
  %buf.cap.addr1011 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf867, i32 0, i32 2
  %len1012 = load i32, ptr %buf.len.addr1010, align 4
  %cap1013 = load i32, ptr %buf.cap.addr1011, align 4
  %want.tmp1014 = add i32 %len1012, 13
  %want1015 = add i32 %want.tmp1014, 1
  %needgrow1016 = icmp ugt i32 %want1015, %cap1013
  br i1 %needgrow1016, label %fstr.grow1017, label %fstr.grow.cont1018

fstr.cpy.loop997:                                 ; preds = %fstr.cpy.body998, %fstr.cpy993
  %i1000 = load i32, ptr %fstr.cpy.i996, align 4
  %cond1001 = icmp ult i32 %i1000, %slen955
  br i1 %cond1001, label %fstr.cpy.body998, label %fstr.cpy.done999

fstr.cpy.body998:                                 ; preds = %fstr.cpy.loop997
  %s.ptr1002 = getelementptr inbounds i8, ptr %sptr956, i32 %i1000
  %ch1003 = load i8, ptr %s.ptr1002, align 1
  %dst.off1004 = add i32 %len2990, %i1000
  %d.ptr1005 = getelementptr inbounds i8, ptr %ptr989, i32 %dst.off1004
  store i8 %ch1003, ptr %d.ptr1005, align 1
  %i21006 = add i32 %i1000, 1
  store i32 %i21006, ptr %fstr.cpy.i996, align 4
  br label %fstr.cpy.loop997

fstr.cpy.done999:                                 ; preds = %fstr.cpy.loop997
  %newlen1007 = add i32 %len2990, %slen955
  store i32 %newlen1007, ptr %buf.len.addr958, align 4
  %nul.ptr1008 = getelementptr inbounds i8, ptr %ptr989, i32 %newlen1007
  store i8 0, ptr %nul.ptr1008, align 1
  br label %fstr.cpy.end994

fstr.grow1017:                                    ; preds = %fstr.cpy.end994
  %cap01019 = icmp eq i32 %cap1013, 0
  %cap21020 = mul i32 %cap1013, 2
  %basecap1021 = select i1 %cap01019, i32 64, i32 %cap21020
  %caplt1022 = icmp ult i32 %basecap1021, %want1015
  %finalcap1023 = select i1 %caplt1022, i32 %want1015, i32 %basecap1021
  %list.ptr.addr1024 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf867, i32 0, i32 0
  %list.cap.addr1025 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf867, i32 0, i32 2
  %cap1026 = load i32, ptr %list.cap.addr1025, align 4
  %needcap1027 = icmp ugt i32 %finalcap1023, %cap1026
  br i1 %needcap1027, label %list.reserve1028, label %list.reserve.end1029

fstr.grow.cont1018:                               ; preds = %list.reserve.end1029, %fstr.cpy.end994
  %ptr1041 = load ptr, ptr %buf.ptr.addr1009, align 8
  %len21042 = load i32, ptr %buf.len.addr1010, align 4
  br i1 false, label %fstr.zero1043, label %fstr.cpy1044

list.reserve1028:                                 ; preds = %fstr.grow1017
  %cap641030 = zext i32 %finalcap1023 to i64
  %bytes1031 = mul i64 %cap641030, ptrtoint (ptr getelementptr (i8, ptr null, i32 1) to i64)
  %heap1032 = call ptr @GetProcessHeap()
  %ptr1033 = load ptr, ptr %list.ptr.addr1024, align 8
  %isnull1034 = icmp eq ptr %ptr1033, null
  br i1 %isnull1034, label %list.alloc1035, label %list.realloc1036

list.reserve.end1029:                             ; preds = %list.alloc.merge1037, %fstr.grow1017
  br label %fstr.grow.cont1018

list.alloc1035:                                   ; preds = %list.reserve1028
  %mem1038 = call ptr @HeapAlloc(ptr %heap1032, i32 0, i64 %bytes1031)
  br label %list.alloc.merge1037

list.realloc1036:                                 ; preds = %list.reserve1028
  %mem1039 = call ptr @HeapReAlloc(ptr %heap1032, i32 0, ptr %ptr1033, i64 %bytes1031)
  br label %list.alloc.merge1037

list.alloc.merge1037:                             ; preds = %list.realloc1036, %list.alloc1035
  %memphi1040 = phi ptr [ %mem1038, %list.alloc1035 ], [ %mem1039, %list.realloc1036 ]
  store ptr %memphi1040, ptr %list.ptr.addr1024, align 8
  store i32 %finalcap1023, ptr %list.cap.addr1025, align 4
  br label %list.reserve.end1029

fstr.zero1043:                                    ; preds = %fstr.grow.cont1018
  %nul.ptr1046 = getelementptr inbounds i8, ptr %ptr1041, i32 %len21042
  store i8 0, ptr %nul.ptr1046, align 1
  br label %fstr.cpy.end1045

fstr.cpy1044:                                     ; preds = %fstr.grow.cont1018
  store i32 0, ptr %fstr.cpy.i1047, align 4
  br label %fstr.cpy.loop1048

fstr.cpy.end1045:                                 ; preds = %fstr.cpy.done1050, %fstr.zero1043
  %entries_run1060 = load i32, ptr %entries_run, align 4
  %sext1061 = sext i32 %entries_run1060 to i64
  %neg1062 = icmp slt i64 %sext1061, 0
  %negv1063 = sub i64 0, %sext1061
  %abs1064 = select i1 %neg1062, i64 %negv1063, i64 %sext1061
  store i32 31, ptr %fstr.itoa.idx1065, align 4
  store i64 %abs1064, ptr %fstr.itoa.cur1066, align 4
  %end.ptr1067 = getelementptr inbounds [32 x i8], ptr %fstr.itoa.buf868, i32 0, i32 31
  store i8 0, ptr %end.ptr1067, align 1
  %iszero1068 = icmp eq i64 %abs1064, 0
  br i1 %iszero1068, label %itoa.zero1069, label %itoa.loop1070

fstr.cpy.loop1048:                                ; preds = %fstr.cpy.body1049, %fstr.cpy1044
  %i1051 = load i32, ptr %fstr.cpy.i1047, align 4
  %cond1052 = icmp ult i32 %i1051, 13
  br i1 %cond1052, label %fstr.cpy.body1049, label %fstr.cpy.done1050

fstr.cpy.body1049:                                ; preds = %fstr.cpy.loop1048
  %s.ptr1053 = getelementptr inbounds i8, ptr @.fstr.1000016, i32 %i1051
  %ch1054 = load i8, ptr %s.ptr1053, align 1
  %dst.off1055 = add i32 %len21042, %i1051
  %d.ptr1056 = getelementptr inbounds i8, ptr %ptr1041, i32 %dst.off1055
  store i8 %ch1054, ptr %d.ptr1056, align 1
  %i21057 = add i32 %i1051, 1
  store i32 %i21057, ptr %fstr.cpy.i1047, align 4
  br label %fstr.cpy.loop1048

fstr.cpy.done1050:                                ; preds = %fstr.cpy.loop1048
  %newlen1058 = add i32 %len21042, 13
  store i32 %newlen1058, ptr %buf.len.addr1010, align 4
  %nul.ptr1059 = getelementptr inbounds i8, ptr %ptr1041, i32 %newlen1058
  store i8 0, ptr %nul.ptr1059, align 1
  br label %fstr.cpy.end1045

itoa.zero1069:                                    ; preds = %fstr.cpy.end1045
  store i32 30, ptr %fstr.itoa.idx1065, align 4
  %z.ptr1072 = getelementptr inbounds [32 x i8], ptr %fstr.itoa.buf868, i32 0, i32 30
  store i8 48, ptr %z.ptr1072, align 1
  br label %itoa.after1071

itoa.loop1070:                                    ; preds = %fstr.cpy.end1045
  br label %itoa.cond1073

itoa.after1071:                                   ; preds = %itoa.done1075, %itoa.zero1069
  br i1 %neg1062, label %itoa.neg1086, label %itoa.merge1087

itoa.cond1073:                                    ; preds = %itoa.body1074, %itoa.loop1070
  %curv1076 = load i64, ptr %fstr.itoa.cur1066, align 4
  %more1077 = icmp ugt i64 %curv1076, 0
  br i1 %more1077, label %itoa.body1074, label %itoa.done1075

itoa.body1074:                                    ; preds = %itoa.cond1073
  %rem1078 = urem i64 %curv1076, 10
  %div1079 = udiv i64 %curv1076, 10
  store i64 %div1079, ptr %fstr.itoa.cur1066, align 4
  %idxv1080 = load i32, ptr %fstr.itoa.idx1065, align 4
  %idx11081 = sub i32 %idxv1080, 1
  store i32 %idx11081, ptr %fstr.itoa.idx1065, align 4
  %rem321082 = trunc i64 %rem1078 to i32
  %ch321083 = add i32 %rem321082, 48
  %ch1084 = trunc i32 %ch321083 to i8
  %d.ptr1085 = getelementptr inbounds [32 x i8], ptr %fstr.itoa.buf868, i32 0, i32 %idx11081
  store i8 %ch1084, ptr %d.ptr1085, align 1
  br label %itoa.cond1073

itoa.done1075:                                    ; preds = %itoa.cond1073
  br label %itoa.after1071

itoa.neg1086:                                     ; preds = %itoa.after1071
  %idxv1088 = load i32, ptr %fstr.itoa.idx1065, align 4
  %idxm11089 = sub i32 %idxv1088, 1
  store i32 %idxm11089, ptr %fstr.itoa.idx1065, align 4
  %m.ptr1090 = getelementptr inbounds [32 x i8], ptr %fstr.itoa.buf868, i32 0, i32 %idxm11089
  store i8 45, ptr %m.ptr1090, align 1
  br label %itoa.merge1087

itoa.merge1087:                                   ; preds = %itoa.neg1086, %itoa.after1071
  %start1091 = load i32, ptr %fstr.itoa.idx1065, align 4
  %slen1092 = sub i32 31, %start1091
  %sptr1093 = getelementptr inbounds [32 x i8], ptr %fstr.itoa.buf868, i32 0, i32 %start1091
  %buf.ptr.addr1094 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf867, i32 0, i32 0
  %buf.len.addr1095 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf867, i32 0, i32 1
  %buf.cap.addr1096 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf867, i32 0, i32 2
  %len1097 = load i32, ptr %buf.len.addr1095, align 4
  %cap1098 = load i32, ptr %buf.cap.addr1096, align 4
  %want.tmp1099 = add i32 %len1097, %slen1092
  %want1100 = add i32 %want.tmp1099, 1
  %needgrow1101 = icmp ugt i32 %want1100, %cap1098
  br i1 %needgrow1101, label %fstr.grow1102, label %fstr.grow.cont1103

fstr.grow1102:                                    ; preds = %itoa.merge1087
  %cap01104 = icmp eq i32 %cap1098, 0
  %cap21105 = mul i32 %cap1098, 2
  %basecap1106 = select i1 %cap01104, i32 64, i32 %cap21105
  %caplt1107 = icmp ult i32 %basecap1106, %want1100
  %finalcap1108 = select i1 %caplt1107, i32 %want1100, i32 %basecap1106
  %list.ptr.addr1109 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf867, i32 0, i32 0
  %list.cap.addr1110 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf867, i32 0, i32 2
  %cap1111 = load i32, ptr %list.cap.addr1110, align 4
  %needcap1112 = icmp ugt i32 %finalcap1108, %cap1111
  br i1 %needcap1112, label %list.reserve1113, label %list.reserve.end1114

fstr.grow.cont1103:                               ; preds = %list.reserve.end1114, %itoa.merge1087
  %ptr1126 = load ptr, ptr %buf.ptr.addr1094, align 8
  %len21127 = load i32, ptr %buf.len.addr1095, align 4
  %srclen01128 = icmp eq i32 %slen1092, 0
  br i1 %srclen01128, label %fstr.zero1129, label %fstr.cpy1130

list.reserve1113:                                 ; preds = %fstr.grow1102
  %cap641115 = zext i32 %finalcap1108 to i64
  %bytes1116 = mul i64 %cap641115, ptrtoint (ptr getelementptr (i8, ptr null, i32 1) to i64)
  %heap1117 = call ptr @GetProcessHeap()
  %ptr1118 = load ptr, ptr %list.ptr.addr1109, align 8
  %isnull1119 = icmp eq ptr %ptr1118, null
  br i1 %isnull1119, label %list.alloc1120, label %list.realloc1121

list.reserve.end1114:                             ; preds = %list.alloc.merge1122, %fstr.grow1102
  br label %fstr.grow.cont1103

list.alloc1120:                                   ; preds = %list.reserve1113
  %mem1123 = call ptr @HeapAlloc(ptr %heap1117, i32 0, i64 %bytes1116)
  br label %list.alloc.merge1122

list.realloc1121:                                 ; preds = %list.reserve1113
  %mem1124 = call ptr @HeapReAlloc(ptr %heap1117, i32 0, ptr %ptr1118, i64 %bytes1116)
  br label %list.alloc.merge1122

list.alloc.merge1122:                             ; preds = %list.realloc1121, %list.alloc1120
  %memphi1125 = phi ptr [ %mem1123, %list.alloc1120 ], [ %mem1124, %list.realloc1121 ]
  store ptr %memphi1125, ptr %list.ptr.addr1109, align 8
  store i32 %finalcap1108, ptr %list.cap.addr1110, align 4
  br label %list.reserve.end1114

fstr.zero1129:                                    ; preds = %fstr.grow.cont1103
  %nul.ptr1132 = getelementptr inbounds i8, ptr %ptr1126, i32 %len21127
  store i8 0, ptr %nul.ptr1132, align 1
  br label %fstr.cpy.end1131

fstr.cpy1130:                                     ; preds = %fstr.grow.cont1103
  store i32 0, ptr %fstr.cpy.i1133, align 4
  br label %fstr.cpy.loop1134

fstr.cpy.end1131:                                 ; preds = %fstr.cpy.done1136, %fstr.zero1129
  %buf.ptr.addr1146 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf867, i32 0, i32 0
  %buf.len.addr1147 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf867, i32 0, i32 1
  %buf.cap.addr1148 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf867, i32 0, i32 2
  %len1149 = load i32, ptr %buf.len.addr1147, align 4
  %cap1150 = load i32, ptr %buf.cap.addr1148, align 4
  %want.tmp1151 = add i32 %len1149, 10
  %want1152 = add i32 %want.tmp1151, 1
  %needgrow1153 = icmp ugt i32 %want1152, %cap1150
  br i1 %needgrow1153, label %fstr.grow1154, label %fstr.grow.cont1155

fstr.cpy.loop1134:                                ; preds = %fstr.cpy.body1135, %fstr.cpy1130
  %i1137 = load i32, ptr %fstr.cpy.i1133, align 4
  %cond1138 = icmp ult i32 %i1137, %slen1092
  br i1 %cond1138, label %fstr.cpy.body1135, label %fstr.cpy.done1136

fstr.cpy.body1135:                                ; preds = %fstr.cpy.loop1134
  %s.ptr1139 = getelementptr inbounds i8, ptr %sptr1093, i32 %i1137
  %ch1140 = load i8, ptr %s.ptr1139, align 1
  %dst.off1141 = add i32 %len21127, %i1137
  %d.ptr1142 = getelementptr inbounds i8, ptr %ptr1126, i32 %dst.off1141
  store i8 %ch1140, ptr %d.ptr1142, align 1
  %i21143 = add i32 %i1137, 1
  store i32 %i21143, ptr %fstr.cpy.i1133, align 4
  br label %fstr.cpy.loop1134

fstr.cpy.done1136:                                ; preds = %fstr.cpy.loop1134
  %newlen1144 = add i32 %len21127, %slen1092
  store i32 %newlen1144, ptr %buf.len.addr1095, align 4
  %nul.ptr1145 = getelementptr inbounds i8, ptr %ptr1126, i32 %newlen1144
  store i8 0, ptr %nul.ptr1145, align 1
  br label %fstr.cpy.end1131

fstr.grow1154:                                    ; preds = %fstr.cpy.end1131
  %cap01156 = icmp eq i32 %cap1150, 0
  %cap21157 = mul i32 %cap1150, 2
  %basecap1158 = select i1 %cap01156, i32 64, i32 %cap21157
  %caplt1159 = icmp ult i32 %basecap1158, %want1152
  %finalcap1160 = select i1 %caplt1159, i32 %want1152, i32 %basecap1158
  %list.ptr.addr1161 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf867, i32 0, i32 0
  %list.cap.addr1162 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf867, i32 0, i32 2
  %cap1163 = load i32, ptr %list.cap.addr1162, align 4
  %needcap1164 = icmp ugt i32 %finalcap1160, %cap1163
  br i1 %needcap1164, label %list.reserve1165, label %list.reserve.end1166

fstr.grow.cont1155:                               ; preds = %list.reserve.end1166, %fstr.cpy.end1131
  %ptr1178 = load ptr, ptr %buf.ptr.addr1146, align 8
  %len21179 = load i32, ptr %buf.len.addr1147, align 4
  br i1 false, label %fstr.zero1180, label %fstr.cpy1181

list.reserve1165:                                 ; preds = %fstr.grow1154
  %cap641167 = zext i32 %finalcap1160 to i64
  %bytes1168 = mul i64 %cap641167, ptrtoint (ptr getelementptr (i8, ptr null, i32 1) to i64)
  %heap1169 = call ptr @GetProcessHeap()
  %ptr1170 = load ptr, ptr %list.ptr.addr1161, align 8
  %isnull1171 = icmp eq ptr %ptr1170, null
  br i1 %isnull1171, label %list.alloc1172, label %list.realloc1173

list.reserve.end1166:                             ; preds = %list.alloc.merge1174, %fstr.grow1154
  br label %fstr.grow.cont1155

list.alloc1172:                                   ; preds = %list.reserve1165
  %mem1175 = call ptr @HeapAlloc(ptr %heap1169, i32 0, i64 %bytes1168)
  br label %list.alloc.merge1174

list.realloc1173:                                 ; preds = %list.reserve1165
  %mem1176 = call ptr @HeapReAlloc(ptr %heap1169, i32 0, ptr %ptr1170, i64 %bytes1168)
  br label %list.alloc.merge1174

list.alloc.merge1174:                             ; preds = %list.realloc1173, %list.alloc1172
  %memphi1177 = phi ptr [ %mem1175, %list.alloc1172 ], [ %mem1176, %list.realloc1173 ]
  store ptr %memphi1177, ptr %list.ptr.addr1161, align 8
  store i32 %finalcap1160, ptr %list.cap.addr1162, align 4
  br label %list.reserve.end1166

fstr.zero1180:                                    ; preds = %fstr.grow.cont1155
  %nul.ptr1183 = getelementptr inbounds i8, ptr %ptr1178, i32 %len21179
  store i8 0, ptr %nul.ptr1183, align 1
  br label %fstr.cpy.end1182

fstr.cpy1181:                                     ; preds = %fstr.grow.cont1155
  store i32 0, ptr %fstr.cpy.i1184, align 4
  br label %fstr.cpy.loop1185

fstr.cpy.end1182:                                 ; preds = %fstr.cpy.done1187, %fstr.zero1180
  %k1197 = load i32, ptr %k, align 4
  %sext1198 = sext i32 %k1197 to i64
  %neg1199 = icmp slt i64 %sext1198, 0
  %negv1200 = sub i64 0, %sext1198
  %abs1201 = select i1 %neg1199, i64 %negv1200, i64 %sext1198
  store i32 31, ptr %fstr.itoa.idx1202, align 4
  store i64 %abs1201, ptr %fstr.itoa.cur1203, align 4
  %end.ptr1204 = getelementptr inbounds [32 x i8], ptr %fstr.itoa.buf868, i32 0, i32 31
  store i8 0, ptr %end.ptr1204, align 1
  %iszero1205 = icmp eq i64 %abs1201, 0
  br i1 %iszero1205, label %itoa.zero1206, label %itoa.loop1207

fstr.cpy.loop1185:                                ; preds = %fstr.cpy.body1186, %fstr.cpy1181
  %i1188 = load i32, ptr %fstr.cpy.i1184, align 4
  %cond1189 = icmp ult i32 %i1188, 10
  br i1 %cond1189, label %fstr.cpy.body1186, label %fstr.cpy.done1187

fstr.cpy.body1186:                                ; preds = %fstr.cpy.loop1185
  %s.ptr1190 = getelementptr inbounds i8, ptr @.fstr.1000017, i32 %i1188
  %ch1191 = load i8, ptr %s.ptr1190, align 1
  %dst.off1192 = add i32 %len21179, %i1188
  %d.ptr1193 = getelementptr inbounds i8, ptr %ptr1178, i32 %dst.off1192
  store i8 %ch1191, ptr %d.ptr1193, align 1
  %i21194 = add i32 %i1188, 1
  store i32 %i21194, ptr %fstr.cpy.i1184, align 4
  br label %fstr.cpy.loop1185

fstr.cpy.done1187:                                ; preds = %fstr.cpy.loop1185
  %newlen1195 = add i32 %len21179, 10
  store i32 %newlen1195, ptr %buf.len.addr1147, align 4
  %nul.ptr1196 = getelementptr inbounds i8, ptr %ptr1178, i32 %newlen1195
  store i8 0, ptr %nul.ptr1196, align 1
  br label %fstr.cpy.end1182

itoa.zero1206:                                    ; preds = %fstr.cpy.end1182
  store i32 30, ptr %fstr.itoa.idx1202, align 4
  %z.ptr1209 = getelementptr inbounds [32 x i8], ptr %fstr.itoa.buf868, i32 0, i32 30
  store i8 48, ptr %z.ptr1209, align 1
  br label %itoa.after1208

itoa.loop1207:                                    ; preds = %fstr.cpy.end1182
  br label %itoa.cond1210

itoa.after1208:                                   ; preds = %itoa.done1212, %itoa.zero1206
  br i1 %neg1199, label %itoa.neg1223, label %itoa.merge1224

itoa.cond1210:                                    ; preds = %itoa.body1211, %itoa.loop1207
  %curv1213 = load i64, ptr %fstr.itoa.cur1203, align 4
  %more1214 = icmp ugt i64 %curv1213, 0
  br i1 %more1214, label %itoa.body1211, label %itoa.done1212

itoa.body1211:                                    ; preds = %itoa.cond1210
  %rem1215 = urem i64 %curv1213, 10
  %div1216 = udiv i64 %curv1213, 10
  store i64 %div1216, ptr %fstr.itoa.cur1203, align 4
  %idxv1217 = load i32, ptr %fstr.itoa.idx1202, align 4
  %idx11218 = sub i32 %idxv1217, 1
  store i32 %idx11218, ptr %fstr.itoa.idx1202, align 4
  %rem321219 = trunc i64 %rem1215 to i32
  %ch321220 = add i32 %rem321219, 48
  %ch1221 = trunc i32 %ch321220 to i8
  %d.ptr1222 = getelementptr inbounds [32 x i8], ptr %fstr.itoa.buf868, i32 0, i32 %idx11218
  store i8 %ch1221, ptr %d.ptr1222, align 1
  br label %itoa.cond1210

itoa.done1212:                                    ; preds = %itoa.cond1210
  br label %itoa.after1208

itoa.neg1223:                                     ; preds = %itoa.after1208
  %idxv1225 = load i32, ptr %fstr.itoa.idx1202, align 4
  %idxm11226 = sub i32 %idxv1225, 1
  store i32 %idxm11226, ptr %fstr.itoa.idx1202, align 4
  %m.ptr1227 = getelementptr inbounds [32 x i8], ptr %fstr.itoa.buf868, i32 0, i32 %idxm11226
  store i8 45, ptr %m.ptr1227, align 1
  br label %itoa.merge1224

itoa.merge1224:                                   ; preds = %itoa.neg1223, %itoa.after1208
  %start1228 = load i32, ptr %fstr.itoa.idx1202, align 4
  %slen1229 = sub i32 31, %start1228
  %sptr1230 = getelementptr inbounds [32 x i8], ptr %fstr.itoa.buf868, i32 0, i32 %start1228
  %buf.ptr.addr1231 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf867, i32 0, i32 0
  %buf.len.addr1232 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf867, i32 0, i32 1
  %buf.cap.addr1233 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf867, i32 0, i32 2
  %len1234 = load i32, ptr %buf.len.addr1232, align 4
  %cap1235 = load i32, ptr %buf.cap.addr1233, align 4
  %want.tmp1236 = add i32 %len1234, %slen1229
  %want1237 = add i32 %want.tmp1236, 1
  %needgrow1238 = icmp ugt i32 %want1237, %cap1235
  br i1 %needgrow1238, label %fstr.grow1239, label %fstr.grow.cont1240

fstr.grow1239:                                    ; preds = %itoa.merge1224
  %cap01241 = icmp eq i32 %cap1235, 0
  %cap21242 = mul i32 %cap1235, 2
  %basecap1243 = select i1 %cap01241, i32 64, i32 %cap21242
  %caplt1244 = icmp ult i32 %basecap1243, %want1237
  %finalcap1245 = select i1 %caplt1244, i32 %want1237, i32 %basecap1243
  %list.ptr.addr1246 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf867, i32 0, i32 0
  %list.cap.addr1247 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf867, i32 0, i32 2
  %cap1248 = load i32, ptr %list.cap.addr1247, align 4
  %needcap1249 = icmp ugt i32 %finalcap1245, %cap1248
  br i1 %needcap1249, label %list.reserve1250, label %list.reserve.end1251

fstr.grow.cont1240:                               ; preds = %list.reserve.end1251, %itoa.merge1224
  %ptr1263 = load ptr, ptr %buf.ptr.addr1231, align 8
  %len21264 = load i32, ptr %buf.len.addr1232, align 4
  %srclen01265 = icmp eq i32 %slen1229, 0
  br i1 %srclen01265, label %fstr.zero1266, label %fstr.cpy1267

list.reserve1250:                                 ; preds = %fstr.grow1239
  %cap641252 = zext i32 %finalcap1245 to i64
  %bytes1253 = mul i64 %cap641252, ptrtoint (ptr getelementptr (i8, ptr null, i32 1) to i64)
  %heap1254 = call ptr @GetProcessHeap()
  %ptr1255 = load ptr, ptr %list.ptr.addr1246, align 8
  %isnull1256 = icmp eq ptr %ptr1255, null
  br i1 %isnull1256, label %list.alloc1257, label %list.realloc1258

list.reserve.end1251:                             ; preds = %list.alloc.merge1259, %fstr.grow1239
  br label %fstr.grow.cont1240

list.alloc1257:                                   ; preds = %list.reserve1250
  %mem1260 = call ptr @HeapAlloc(ptr %heap1254, i32 0, i64 %bytes1253)
  br label %list.alloc.merge1259

list.realloc1258:                                 ; preds = %list.reserve1250
  %mem1261 = call ptr @HeapReAlloc(ptr %heap1254, i32 0, ptr %ptr1255, i64 %bytes1253)
  br label %list.alloc.merge1259

list.alloc.merge1259:                             ; preds = %list.realloc1258, %list.alloc1257
  %memphi1262 = phi ptr [ %mem1260, %list.alloc1257 ], [ %mem1261, %list.realloc1258 ]
  store ptr %memphi1262, ptr %list.ptr.addr1246, align 8
  store i32 %finalcap1245, ptr %list.cap.addr1247, align 4
  br label %list.reserve.end1251

fstr.zero1266:                                    ; preds = %fstr.grow.cont1240
  %nul.ptr1269 = getelementptr inbounds i8, ptr %ptr1263, i32 %len21264
  store i8 0, ptr %nul.ptr1269, align 1
  br label %fstr.cpy.end1268

fstr.cpy1267:                                     ; preds = %fstr.grow.cont1240
  store i32 0, ptr %fstr.cpy.i1270, align 4
  br label %fstr.cpy.loop1271

fstr.cpy.end1268:                                 ; preds = %fstr.cpy.done1273, %fstr.zero1266
  %buf.ptr.addr1283 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf867, i32 0, i32 0
  %buf.len.addr1284 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf867, i32 0, i32 1
  %buf.cap.addr1285 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf867, i32 0, i32 2
  %len1286 = load i32, ptr %buf.len.addr1284, align 4
  %cap1287 = load i32, ptr %buf.cap.addr1285, align 4
  %want.tmp1288 = add i32 %len1286, 5
  %want1289 = add i32 %want.tmp1288, 1
  %needgrow1290 = icmp ugt i32 %want1289, %cap1287
  br i1 %needgrow1290, label %fstr.grow1291, label %fstr.grow.cont1292

fstr.cpy.loop1271:                                ; preds = %fstr.cpy.body1272, %fstr.cpy1267
  %i1274 = load i32, ptr %fstr.cpy.i1270, align 4
  %cond1275 = icmp ult i32 %i1274, %slen1229
  br i1 %cond1275, label %fstr.cpy.body1272, label %fstr.cpy.done1273

fstr.cpy.body1272:                                ; preds = %fstr.cpy.loop1271
  %s.ptr1276 = getelementptr inbounds i8, ptr %sptr1230, i32 %i1274
  %ch1277 = load i8, ptr %s.ptr1276, align 1
  %dst.off1278 = add i32 %len21264, %i1274
  %d.ptr1279 = getelementptr inbounds i8, ptr %ptr1263, i32 %dst.off1278
  store i8 %ch1277, ptr %d.ptr1279, align 1
  %i21280 = add i32 %i1274, 1
  store i32 %i21280, ptr %fstr.cpy.i1270, align 4
  br label %fstr.cpy.loop1271

fstr.cpy.done1273:                                ; preds = %fstr.cpy.loop1271
  %newlen1281 = add i32 %len21264, %slen1229
  store i32 %newlen1281, ptr %buf.len.addr1232, align 4
  %nul.ptr1282 = getelementptr inbounds i8, ptr %ptr1263, i32 %newlen1281
  store i8 0, ptr %nul.ptr1282, align 1
  br label %fstr.cpy.end1268

fstr.grow1291:                                    ; preds = %fstr.cpy.end1268
  %cap01293 = icmp eq i32 %cap1287, 0
  %cap21294 = mul i32 %cap1287, 2
  %basecap1295 = select i1 %cap01293, i32 64, i32 %cap21294
  %caplt1296 = icmp ult i32 %basecap1295, %want1289
  %finalcap1297 = select i1 %caplt1296, i32 %want1289, i32 %basecap1295
  %list.ptr.addr1298 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf867, i32 0, i32 0
  %list.cap.addr1299 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf867, i32 0, i32 2
  %cap1300 = load i32, ptr %list.cap.addr1299, align 4
  %needcap1301 = icmp ugt i32 %finalcap1297, %cap1300
  br i1 %needcap1301, label %list.reserve1302, label %list.reserve.end1303

fstr.grow.cont1292:                               ; preds = %list.reserve.end1303, %fstr.cpy.end1268
  %ptr1315 = load ptr, ptr %buf.ptr.addr1283, align 8
  %len21316 = load i32, ptr %buf.len.addr1284, align 4
  br i1 false, label %fstr.zero1317, label %fstr.cpy1318

list.reserve1302:                                 ; preds = %fstr.grow1291
  %cap641304 = zext i32 %finalcap1297 to i64
  %bytes1305 = mul i64 %cap641304, ptrtoint (ptr getelementptr (i8, ptr null, i32 1) to i64)
  %heap1306 = call ptr @GetProcessHeap()
  %ptr1307 = load ptr, ptr %list.ptr.addr1298, align 8
  %isnull1308 = icmp eq ptr %ptr1307, null
  br i1 %isnull1308, label %list.alloc1309, label %list.realloc1310

list.reserve.end1303:                             ; preds = %list.alloc.merge1311, %fstr.grow1291
  br label %fstr.grow.cont1292

list.alloc1309:                                   ; preds = %list.reserve1302
  %mem1312 = call ptr @HeapAlloc(ptr %heap1306, i32 0, i64 %bytes1305)
  br label %list.alloc.merge1311

list.realloc1310:                                 ; preds = %list.reserve1302
  %mem1313 = call ptr @HeapReAlloc(ptr %heap1306, i32 0, ptr %ptr1307, i64 %bytes1305)
  br label %list.alloc.merge1311

list.alloc.merge1311:                             ; preds = %list.realloc1310, %list.alloc1309
  %memphi1314 = phi ptr [ %mem1312, %list.alloc1309 ], [ %mem1313, %list.realloc1310 ]
  store ptr %memphi1314, ptr %list.ptr.addr1298, align 8
  store i32 %finalcap1297, ptr %list.cap.addr1299, align 4
  br label %list.reserve.end1303

fstr.zero1317:                                    ; preds = %fstr.grow.cont1292
  %nul.ptr1320 = getelementptr inbounds i8, ptr %ptr1315, i32 %len21316
  store i8 0, ptr %nul.ptr1320, align 1
  br label %fstr.cpy.end1319

fstr.cpy1318:                                     ; preds = %fstr.grow.cont1292
  store i32 0, ptr %fstr.cpy.i1321, align 4
  br label %fstr.cpy.loop1322

fstr.cpy.end1319:                                 ; preds = %fstr.cpy.done1324, %fstr.zero1317
  %cap1334 = load i32, ptr %cap, align 4
  %sext1335 = sext i32 %cap1334 to i64
  %neg1336 = icmp slt i64 %sext1335, 0
  %negv1337 = sub i64 0, %sext1335
  %abs1338 = select i1 %neg1336, i64 %negv1337, i64 %sext1335
  store i32 31, ptr %fstr.itoa.idx1339, align 4
  store i64 %abs1338, ptr %fstr.itoa.cur1340, align 4
  %end.ptr1341 = getelementptr inbounds [32 x i8], ptr %fstr.itoa.buf868, i32 0, i32 31
  store i8 0, ptr %end.ptr1341, align 1
  %iszero1342 = icmp eq i64 %abs1338, 0
  br i1 %iszero1342, label %itoa.zero1343, label %itoa.loop1344

fstr.cpy.loop1322:                                ; preds = %fstr.cpy.body1323, %fstr.cpy1318
  %i1325 = load i32, ptr %fstr.cpy.i1321, align 4
  %cond1326 = icmp ult i32 %i1325, 5
  br i1 %cond1326, label %fstr.cpy.body1323, label %fstr.cpy.done1324

fstr.cpy.body1323:                                ; preds = %fstr.cpy.loop1322
  %s.ptr1327 = getelementptr inbounds i8, ptr @.fstr.1000018, i32 %i1325
  %ch1328 = load i8, ptr %s.ptr1327, align 1
  %dst.off1329 = add i32 %len21316, %i1325
  %d.ptr1330 = getelementptr inbounds i8, ptr %ptr1315, i32 %dst.off1329
  store i8 %ch1328, ptr %d.ptr1330, align 1
  %i21331 = add i32 %i1325, 1
  store i32 %i21331, ptr %fstr.cpy.i1321, align 4
  br label %fstr.cpy.loop1322

fstr.cpy.done1324:                                ; preds = %fstr.cpy.loop1322
  %newlen1332 = add i32 %len21316, 5
  store i32 %newlen1332, ptr %buf.len.addr1284, align 4
  %nul.ptr1333 = getelementptr inbounds i8, ptr %ptr1315, i32 %newlen1332
  store i8 0, ptr %nul.ptr1333, align 1
  br label %fstr.cpy.end1319

itoa.zero1343:                                    ; preds = %fstr.cpy.end1319
  store i32 30, ptr %fstr.itoa.idx1339, align 4
  %z.ptr1346 = getelementptr inbounds [32 x i8], ptr %fstr.itoa.buf868, i32 0, i32 30
  store i8 48, ptr %z.ptr1346, align 1
  br label %itoa.after1345

itoa.loop1344:                                    ; preds = %fstr.cpy.end1319
  br label %itoa.cond1347

itoa.after1345:                                   ; preds = %itoa.done1349, %itoa.zero1343
  br i1 %neg1336, label %itoa.neg1360, label %itoa.merge1361

itoa.cond1347:                                    ; preds = %itoa.body1348, %itoa.loop1344
  %curv1350 = load i64, ptr %fstr.itoa.cur1340, align 4
  %more1351 = icmp ugt i64 %curv1350, 0
  br i1 %more1351, label %itoa.body1348, label %itoa.done1349

itoa.body1348:                                    ; preds = %itoa.cond1347
  %rem1352 = urem i64 %curv1350, 10
  %div1353 = udiv i64 %curv1350, 10
  store i64 %div1353, ptr %fstr.itoa.cur1340, align 4
  %idxv1354 = load i32, ptr %fstr.itoa.idx1339, align 4
  %idx11355 = sub i32 %idxv1354, 1
  store i32 %idx11355, ptr %fstr.itoa.idx1339, align 4
  %rem321356 = trunc i64 %rem1352 to i32
  %ch321357 = add i32 %rem321356, 48
  %ch1358 = trunc i32 %ch321357 to i8
  %d.ptr1359 = getelementptr inbounds [32 x i8], ptr %fstr.itoa.buf868, i32 0, i32 %idx11355
  store i8 %ch1358, ptr %d.ptr1359, align 1
  br label %itoa.cond1347

itoa.done1349:                                    ; preds = %itoa.cond1347
  br label %itoa.after1345

itoa.neg1360:                                     ; preds = %itoa.after1345
  %idxv1362 = load i32, ptr %fstr.itoa.idx1339, align 4
  %idxm11363 = sub i32 %idxv1362, 1
  store i32 %idxm11363, ptr %fstr.itoa.idx1339, align 4
  %m.ptr1364 = getelementptr inbounds [32 x i8], ptr %fstr.itoa.buf868, i32 0, i32 %idxm11363
  store i8 45, ptr %m.ptr1364, align 1
  br label %itoa.merge1361

itoa.merge1361:                                   ; preds = %itoa.neg1360, %itoa.after1345
  %start1365 = load i32, ptr %fstr.itoa.idx1339, align 4
  %slen1366 = sub i32 31, %start1365
  %sptr1367 = getelementptr inbounds [32 x i8], ptr %fstr.itoa.buf868, i32 0, i32 %start1365
  %buf.ptr.addr1368 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf867, i32 0, i32 0
  %buf.len.addr1369 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf867, i32 0, i32 1
  %buf.cap.addr1370 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf867, i32 0, i32 2
  %len1371 = load i32, ptr %buf.len.addr1369, align 4
  %cap1372 = load i32, ptr %buf.cap.addr1370, align 4
  %want.tmp1373 = add i32 %len1371, %slen1366
  %want1374 = add i32 %want.tmp1373, 1
  %needgrow1375 = icmp ugt i32 %want1374, %cap1372
  br i1 %needgrow1375, label %fstr.grow1376, label %fstr.grow.cont1377

fstr.grow1376:                                    ; preds = %itoa.merge1361
  %cap01378 = icmp eq i32 %cap1372, 0
  %cap21379 = mul i32 %cap1372, 2
  %basecap1380 = select i1 %cap01378, i32 64, i32 %cap21379
  %caplt1381 = icmp ult i32 %basecap1380, %want1374
  %finalcap1382 = select i1 %caplt1381, i32 %want1374, i32 %basecap1380
  %list.ptr.addr1383 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf867, i32 0, i32 0
  %list.cap.addr1384 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf867, i32 0, i32 2
  %cap1385 = load i32, ptr %list.cap.addr1384, align 4
  %needcap1386 = icmp ugt i32 %finalcap1382, %cap1385
  br i1 %needcap1386, label %list.reserve1387, label %list.reserve.end1388

fstr.grow.cont1377:                               ; preds = %list.reserve.end1388, %itoa.merge1361
  %ptr1400 = load ptr, ptr %buf.ptr.addr1368, align 8
  %len21401 = load i32, ptr %buf.len.addr1369, align 4
  %srclen01402 = icmp eq i32 %slen1366, 0
  br i1 %srclen01402, label %fstr.zero1403, label %fstr.cpy1404

list.reserve1387:                                 ; preds = %fstr.grow1376
  %cap641389 = zext i32 %finalcap1382 to i64
  %bytes1390 = mul i64 %cap641389, ptrtoint (ptr getelementptr (i8, ptr null, i32 1) to i64)
  %heap1391 = call ptr @GetProcessHeap()
  %ptr1392 = load ptr, ptr %list.ptr.addr1383, align 8
  %isnull1393 = icmp eq ptr %ptr1392, null
  br i1 %isnull1393, label %list.alloc1394, label %list.realloc1395

list.reserve.end1388:                             ; preds = %list.alloc.merge1396, %fstr.grow1376
  br label %fstr.grow.cont1377

list.alloc1394:                                   ; preds = %list.reserve1387
  %mem1397 = call ptr @HeapAlloc(ptr %heap1391, i32 0, i64 %bytes1390)
  br label %list.alloc.merge1396

list.realloc1395:                                 ; preds = %list.reserve1387
  %mem1398 = call ptr @HeapReAlloc(ptr %heap1391, i32 0, ptr %ptr1392, i64 %bytes1390)
  br label %list.alloc.merge1396

list.alloc.merge1396:                             ; preds = %list.realloc1395, %list.alloc1394
  %memphi1399 = phi ptr [ %mem1397, %list.alloc1394 ], [ %mem1398, %list.realloc1395 ]
  store ptr %memphi1399, ptr %list.ptr.addr1383, align 8
  store i32 %finalcap1382, ptr %list.cap.addr1384, align 4
  br label %list.reserve.end1388

fstr.zero1403:                                    ; preds = %fstr.grow.cont1377
  %nul.ptr1406 = getelementptr inbounds i8, ptr %ptr1400, i32 %len21401
  store i8 0, ptr %nul.ptr1406, align 1
  br label %fstr.cpy.end1405

fstr.cpy1404:                                     ; preds = %fstr.grow.cont1377
  store i32 0, ptr %fstr.cpy.i1407, align 4
  br label %fstr.cpy.loop1408

fstr.cpy.end1405:                                 ; preds = %fstr.cpy.done1410, %fstr.zero1403
  %buf.ptr.addr1420 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf867, i32 0, i32 0
  %buf.len.addr1421 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf867, i32 0, i32 1
  %buf.cap.addr1422 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf867, i32 0, i32 2
  %len1423 = load i32, ptr %buf.len.addr1421, align 4
  %cap1424 = load i32, ptr %buf.cap.addr1422, align 4
  %want.tmp1425 = add i32 %len1423, 9
  %want1426 = add i32 %want.tmp1425, 1
  %needgrow1427 = icmp ugt i32 %want1426, %cap1424
  br i1 %needgrow1427, label %fstr.grow1428, label %fstr.grow.cont1429

fstr.cpy.loop1408:                                ; preds = %fstr.cpy.body1409, %fstr.cpy1404
  %i1411 = load i32, ptr %fstr.cpy.i1407, align 4
  %cond1412 = icmp ult i32 %i1411, %slen1366
  br i1 %cond1412, label %fstr.cpy.body1409, label %fstr.cpy.done1410

fstr.cpy.body1409:                                ; preds = %fstr.cpy.loop1408
  %s.ptr1413 = getelementptr inbounds i8, ptr %sptr1367, i32 %i1411
  %ch1414 = load i8, ptr %s.ptr1413, align 1
  %dst.off1415 = add i32 %len21401, %i1411
  %d.ptr1416 = getelementptr inbounds i8, ptr %ptr1400, i32 %dst.off1415
  store i8 %ch1414, ptr %d.ptr1416, align 1
  %i21417 = add i32 %i1411, 1
  store i32 %i21417, ptr %fstr.cpy.i1407, align 4
  br label %fstr.cpy.loop1408

fstr.cpy.done1410:                                ; preds = %fstr.cpy.loop1408
  %newlen1418 = add i32 %len21401, %slen1366
  store i32 %newlen1418, ptr %buf.len.addr1369, align 4
  %nul.ptr1419 = getelementptr inbounds i8, ptr %ptr1400, i32 %newlen1418
  store i8 0, ptr %nul.ptr1419, align 1
  br label %fstr.cpy.end1405

fstr.grow1428:                                    ; preds = %fstr.cpy.end1405
  %cap01430 = icmp eq i32 %cap1424, 0
  %cap21431 = mul i32 %cap1424, 2
  %basecap1432 = select i1 %cap01430, i32 64, i32 %cap21431
  %caplt1433 = icmp ult i32 %basecap1432, %want1426
  %finalcap1434 = select i1 %caplt1433, i32 %want1426, i32 %basecap1432
  %list.ptr.addr1435 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf867, i32 0, i32 0
  %list.cap.addr1436 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf867, i32 0, i32 2
  %cap1437 = load i32, ptr %list.cap.addr1436, align 4
  %needcap1438 = icmp ugt i32 %finalcap1434, %cap1437
  br i1 %needcap1438, label %list.reserve1439, label %list.reserve.end1440

fstr.grow.cont1429:                               ; preds = %list.reserve.end1440, %fstr.cpy.end1405
  %ptr1452 = load ptr, ptr %buf.ptr.addr1420, align 8
  %len21453 = load i32, ptr %buf.len.addr1421, align 4
  br i1 false, label %fstr.zero1454, label %fstr.cpy1455

list.reserve1439:                                 ; preds = %fstr.grow1428
  %cap641441 = zext i32 %finalcap1434 to i64
  %bytes1442 = mul i64 %cap641441, ptrtoint (ptr getelementptr (i8, ptr null, i32 1) to i64)
  %heap1443 = call ptr @GetProcessHeap()
  %ptr1444 = load ptr, ptr %list.ptr.addr1435, align 8
  %isnull1445 = icmp eq ptr %ptr1444, null
  br i1 %isnull1445, label %list.alloc1446, label %list.realloc1447

list.reserve.end1440:                             ; preds = %list.alloc.merge1448, %fstr.grow1428
  br label %fstr.grow.cont1429

list.alloc1446:                                   ; preds = %list.reserve1439
  %mem1449 = call ptr @HeapAlloc(ptr %heap1443, i32 0, i64 %bytes1442)
  br label %list.alloc.merge1448

list.realloc1447:                                 ; preds = %list.reserve1439
  %mem1450 = call ptr @HeapReAlloc(ptr %heap1443, i32 0, ptr %ptr1444, i64 %bytes1442)
  br label %list.alloc.merge1448

list.alloc.merge1448:                             ; preds = %list.realloc1447, %list.alloc1446
  %memphi1451 = phi ptr [ %mem1449, %list.alloc1446 ], [ %mem1450, %list.realloc1447 ]
  store ptr %memphi1451, ptr %list.ptr.addr1435, align 8
  store i32 %finalcap1434, ptr %list.cap.addr1436, align 4
  br label %list.reserve.end1440

fstr.zero1454:                                    ; preds = %fstr.grow.cont1429
  %nul.ptr1457 = getelementptr inbounds i8, ptr %ptr1452, i32 %len21453
  store i8 0, ptr %nul.ptr1457, align 1
  br label %fstr.cpy.end1456

fstr.cpy1455:                                     ; preds = %fstr.grow.cont1429
  store i32 0, ptr %fstr.cpy.i1458, align 4
  br label %fstr.cpy.loop1459

fstr.cpy.end1456:                                 ; preds = %fstr.cpy.done1461, %fstr.zero1454
  %ms1471 = load i64, ptr %ms, align 4
  %neg1472 = icmp slt i64 %ms1471, 0
  %negv1473 = sub i64 0, %ms1471
  %abs1474 = select i1 %neg1472, i64 %negv1473, i64 %ms1471
  store i32 31, ptr %fstr.itoa.idx1475, align 4
  store i64 %abs1474, ptr %fstr.itoa.cur1476, align 4
  %end.ptr1477 = getelementptr inbounds [32 x i8], ptr %fstr.itoa.buf868, i32 0, i32 31
  store i8 0, ptr %end.ptr1477, align 1
  %iszero1478 = icmp eq i64 %abs1474, 0
  br i1 %iszero1478, label %itoa.zero1479, label %itoa.loop1480

fstr.cpy.loop1459:                                ; preds = %fstr.cpy.body1460, %fstr.cpy1455
  %i1462 = load i32, ptr %fstr.cpy.i1458, align 4
  %cond1463 = icmp ult i32 %i1462, 9
  br i1 %cond1463, label %fstr.cpy.body1460, label %fstr.cpy.done1461

fstr.cpy.body1460:                                ; preds = %fstr.cpy.loop1459
  %s.ptr1464 = getelementptr inbounds i8, ptr @.fstr.1000019, i32 %i1462
  %ch1465 = load i8, ptr %s.ptr1464, align 1
  %dst.off1466 = add i32 %len21453, %i1462
  %d.ptr1467 = getelementptr inbounds i8, ptr %ptr1452, i32 %dst.off1466
  store i8 %ch1465, ptr %d.ptr1467, align 1
  %i21468 = add i32 %i1462, 1
  store i32 %i21468, ptr %fstr.cpy.i1458, align 4
  br label %fstr.cpy.loop1459

fstr.cpy.done1461:                                ; preds = %fstr.cpy.loop1459
  %newlen1469 = add i32 %len21453, 9
  store i32 %newlen1469, ptr %buf.len.addr1421, align 4
  %nul.ptr1470 = getelementptr inbounds i8, ptr %ptr1452, i32 %newlen1469
  store i8 0, ptr %nul.ptr1470, align 1
  br label %fstr.cpy.end1456

itoa.zero1479:                                    ; preds = %fstr.cpy.end1456
  store i32 30, ptr %fstr.itoa.idx1475, align 4
  %z.ptr1482 = getelementptr inbounds [32 x i8], ptr %fstr.itoa.buf868, i32 0, i32 30
  store i8 48, ptr %z.ptr1482, align 1
  br label %itoa.after1481

itoa.loop1480:                                    ; preds = %fstr.cpy.end1456
  br label %itoa.cond1483

itoa.after1481:                                   ; preds = %itoa.done1485, %itoa.zero1479
  br i1 %neg1472, label %itoa.neg1496, label %itoa.merge1497

itoa.cond1483:                                    ; preds = %itoa.body1484, %itoa.loop1480
  %curv1486 = load i64, ptr %fstr.itoa.cur1476, align 4
  %more1487 = icmp ugt i64 %curv1486, 0
  br i1 %more1487, label %itoa.body1484, label %itoa.done1485

itoa.body1484:                                    ; preds = %itoa.cond1483
  %rem1488 = urem i64 %curv1486, 10
  %div1489 = udiv i64 %curv1486, 10
  store i64 %div1489, ptr %fstr.itoa.cur1476, align 4
  %idxv1490 = load i32, ptr %fstr.itoa.idx1475, align 4
  %idx11491 = sub i32 %idxv1490, 1
  store i32 %idx11491, ptr %fstr.itoa.idx1475, align 4
  %rem321492 = trunc i64 %rem1488 to i32
  %ch321493 = add i32 %rem321492, 48
  %ch1494 = trunc i32 %ch321493 to i8
  %d.ptr1495 = getelementptr inbounds [32 x i8], ptr %fstr.itoa.buf868, i32 0, i32 %idx11491
  store i8 %ch1494, ptr %d.ptr1495, align 1
  br label %itoa.cond1483

itoa.done1485:                                    ; preds = %itoa.cond1483
  br label %itoa.after1481

itoa.neg1496:                                     ; preds = %itoa.after1481
  %idxv1498 = load i32, ptr %fstr.itoa.idx1475, align 4
  %idxm11499 = sub i32 %idxv1498, 1
  store i32 %idxm11499, ptr %fstr.itoa.idx1475, align 4
  %m.ptr1500 = getelementptr inbounds [32 x i8], ptr %fstr.itoa.buf868, i32 0, i32 %idxm11499
  store i8 45, ptr %m.ptr1500, align 1
  br label %itoa.merge1497

itoa.merge1497:                                   ; preds = %itoa.neg1496, %itoa.after1481
  %start1501 = load i32, ptr %fstr.itoa.idx1475, align 4
  %slen1502 = sub i32 31, %start1501
  %sptr1503 = getelementptr inbounds [32 x i8], ptr %fstr.itoa.buf868, i32 0, i32 %start1501
  %buf.ptr.addr1504 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf867, i32 0, i32 0
  %buf.len.addr1505 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf867, i32 0, i32 1
  %buf.cap.addr1506 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf867, i32 0, i32 2
  %len1507 = load i32, ptr %buf.len.addr1505, align 4
  %cap1508 = load i32, ptr %buf.cap.addr1506, align 4
  %want.tmp1509 = add i32 %len1507, %slen1502
  %want1510 = add i32 %want.tmp1509, 1
  %needgrow1511 = icmp ugt i32 %want1510, %cap1508
  br i1 %needgrow1511, label %fstr.grow1512, label %fstr.grow.cont1513

fstr.grow1512:                                    ; preds = %itoa.merge1497
  %cap01514 = icmp eq i32 %cap1508, 0
  %cap21515 = mul i32 %cap1508, 2
  %basecap1516 = select i1 %cap01514, i32 64, i32 %cap21515
  %caplt1517 = icmp ult i32 %basecap1516, %want1510
  %finalcap1518 = select i1 %caplt1517, i32 %want1510, i32 %basecap1516
  %list.ptr.addr1519 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf867, i32 0, i32 0
  %list.cap.addr1520 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf867, i32 0, i32 2
  %cap1521 = load i32, ptr %list.cap.addr1520, align 4
  %needcap1522 = icmp ugt i32 %finalcap1518, %cap1521
  br i1 %needcap1522, label %list.reserve1523, label %list.reserve.end1524

fstr.grow.cont1513:                               ; preds = %list.reserve.end1524, %itoa.merge1497
  %ptr1536 = load ptr, ptr %buf.ptr.addr1504, align 8
  %len21537 = load i32, ptr %buf.len.addr1505, align 4
  %srclen01538 = icmp eq i32 %slen1502, 0
  br i1 %srclen01538, label %fstr.zero1539, label %fstr.cpy1540

list.reserve1523:                                 ; preds = %fstr.grow1512
  %cap641525 = zext i32 %finalcap1518 to i64
  %bytes1526 = mul i64 %cap641525, ptrtoint (ptr getelementptr (i8, ptr null, i32 1) to i64)
  %heap1527 = call ptr @GetProcessHeap()
  %ptr1528 = load ptr, ptr %list.ptr.addr1519, align 8
  %isnull1529 = icmp eq ptr %ptr1528, null
  br i1 %isnull1529, label %list.alloc1530, label %list.realloc1531

list.reserve.end1524:                             ; preds = %list.alloc.merge1532, %fstr.grow1512
  br label %fstr.grow.cont1513

list.alloc1530:                                   ; preds = %list.reserve1523
  %mem1533 = call ptr @HeapAlloc(ptr %heap1527, i32 0, i64 %bytes1526)
  br label %list.alloc.merge1532

list.realloc1531:                                 ; preds = %list.reserve1523
  %mem1534 = call ptr @HeapReAlloc(ptr %heap1527, i32 0, ptr %ptr1528, i64 %bytes1526)
  br label %list.alloc.merge1532

list.alloc.merge1532:                             ; preds = %list.realloc1531, %list.alloc1530
  %memphi1535 = phi ptr [ %mem1533, %list.alloc1530 ], [ %mem1534, %list.realloc1531 ]
  store ptr %memphi1535, ptr %list.ptr.addr1519, align 8
  store i32 %finalcap1518, ptr %list.cap.addr1520, align 4
  br label %list.reserve.end1524

fstr.zero1539:                                    ; preds = %fstr.grow.cont1513
  %nul.ptr1542 = getelementptr inbounds i8, ptr %ptr1536, i32 %len21537
  store i8 0, ptr %nul.ptr1542, align 1
  br label %fstr.cpy.end1541

fstr.cpy1540:                                     ; preds = %fstr.grow.cont1513
  store i32 0, ptr %fstr.cpy.i1543, align 4
  br label %fstr.cpy.loop1544

fstr.cpy.end1541:                                 ; preds = %fstr.cpy.done1546, %fstr.zero1539
  %buf.len.addr1556 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf867, i32 0, i32 1
  %buf.len1557 = load i32, ptr %buf.len.addr1556, align 4
  %fstr.empty1558 = icmp eq i32 %buf.len1557, 0
  %buf.ptr.addr1559 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf867, i32 0, i32 0
  %outp01560 = load ptr, ptr %buf.ptr.addr1559, align 8
  %out.null01561 = icmp eq ptr %outp01560, null
  %needalloc1562 = and i1 %fstr.empty1558, %out.null01561
  br i1 %needalloc1562, label %fstr.empty.alloc1563, label %fstr.empty.cont1564

fstr.cpy.loop1544:                                ; preds = %fstr.cpy.body1545, %fstr.cpy1540
  %i1547 = load i32, ptr %fstr.cpy.i1543, align 4
  %cond1548 = icmp ult i32 %i1547, %slen1502
  br i1 %cond1548, label %fstr.cpy.body1545, label %fstr.cpy.done1546

fstr.cpy.body1545:                                ; preds = %fstr.cpy.loop1544
  %s.ptr1549 = getelementptr inbounds i8, ptr %sptr1503, i32 %i1547
  %ch1550 = load i8, ptr %s.ptr1549, align 1
  %dst.off1551 = add i32 %len21537, %i1547
  %d.ptr1552 = getelementptr inbounds i8, ptr %ptr1536, i32 %dst.off1551
  store i8 %ch1550, ptr %d.ptr1552, align 1
  %i21553 = add i32 %i1547, 1
  store i32 %i21553, ptr %fstr.cpy.i1543, align 4
  br label %fstr.cpy.loop1544

fstr.cpy.done1546:                                ; preds = %fstr.cpy.loop1544
  %newlen1554 = add i32 %len21537, %slen1502
  store i32 %newlen1554, ptr %buf.len.addr1505, align 4
  %nul.ptr1555 = getelementptr inbounds i8, ptr %ptr1536, i32 %newlen1554
  store i8 0, ptr %nul.ptr1555, align 1
  br label %fstr.cpy.end1541

fstr.empty.alloc1563:                             ; preds = %fstr.cpy.end1541
  %buf.ptr.addr1565 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf867, i32 0, i32 0
  %buf.len.addr1566 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf867, i32 0, i32 1
  %buf.cap.addr1567 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf867, i32 0, i32 2
  %len1568 = load i32, ptr %buf.len.addr1566, align 4
  %cap1569 = load i32, ptr %buf.cap.addr1567, align 4
  %want.tmp1570 = add i32 %len1568, 0
  %want1571 = add i32 %want.tmp1570, 1
  %needgrow1572 = icmp ugt i32 %want1571, %cap1569
  br i1 %needgrow1572, label %fstr.grow1573, label %fstr.grow.cont1574

fstr.empty.cont1564:                              ; preds = %fstr.cpy.end1601, %fstr.cpy.end1541
  %fstr.val1616 = load %emp.string, ptr %fstr.buf867, align 8
  store %emp.string %fstr.val1616, ptr %msg866, align 8
  %msg1617 = load %emp.string, ptr %msg866, align 8
  call void @println__Nstring(%emp.string %msg1617)
  %keys1618 = load { ptr, i32, i32 }, ptr %keys, align 8
  %list.ptr.addr1619 = getelementptr inbounds nuw { ptr, i32, i32 }, ptr %keys, i32 0, i32 0
  %list.ptr1620 = load ptr, ptr %list.ptr.addr1619, align 8
  %list.elem.ptr1621 = getelementptr inbounds i32, ptr %list.ptr1620, i32 1
  %idx1622 = load i32, ptr %list.elem.ptr1621, align 4
  %vals1623 = load { ptr, i32, i32 }, ptr %vals, align 8
  %list.ptr.addr1624 = getelementptr inbounds nuw { ptr, i32, i32 }, ptr %vals, i32 0, i32 0
  %list.ptr1625 = load ptr, ptr %list.ptr.addr1624, align 8
  %list.elem.ptr1626 = getelementptr inbounds i32, ptr %list.ptr1625, i32 2
  %idx1627 = load i32, ptr %list.elem.ptr1626, align 4
  %xortmp1628 = xor i32 %idx1622, %idx1627
  %len1629 = load i32, ptr %len411, align 4
  %xortmp1630 = xor i32 %xortmp1628, %len1629
  store i32 %xortmp1630, ptr %checksum, align 4
  %list.ptr.addr1634 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf1632, i32 0, i32 0
  %list.len.addr1635 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf1632, i32 0, i32 1
  %list.cap.addr1636 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf1632, i32 0, i32 2
  store ptr null, ptr %list.ptr.addr1634, align 8
  store i32 0, ptr %list.len.addr1635, align 4
  store i32 0, ptr %list.cap.addr1636, align 4
  %buf.ptr.addr1637 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf1632, i32 0, i32 0
  %buf.len.addr1638 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf1632, i32 0, i32 1
  %buf.cap.addr1639 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf1632, i32 0, i32 2
  %len1640 = load i32, ptr %buf.len.addr1638, align 4
  %cap1641 = load i32, ptr %buf.cap.addr1639, align 4
  %want.tmp1642 = add i32 %len1640, 9
  %want1643 = add i32 %want.tmp1642, 1
  %needgrow1644 = icmp ugt i32 %want1643, %cap1641
  br i1 %needgrow1644, label %fstr.grow1645, label %fstr.grow.cont1646

fstr.grow1573:                                    ; preds = %fstr.empty.alloc1563
  %cap01575 = icmp eq i32 %cap1569, 0
  %cap21576 = mul i32 %cap1569, 2
  %basecap1577 = select i1 %cap01575, i32 64, i32 %cap21576
  %caplt1578 = icmp ult i32 %basecap1577, %want1571
  %finalcap1579 = select i1 %caplt1578, i32 %want1571, i32 %basecap1577
  %list.ptr.addr1580 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf867, i32 0, i32 0
  %list.cap.addr1581 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf867, i32 0, i32 2
  %cap1582 = load i32, ptr %list.cap.addr1581, align 4
  %needcap1583 = icmp ugt i32 %finalcap1579, %cap1582
  br i1 %needcap1583, label %list.reserve1584, label %list.reserve.end1585

fstr.grow.cont1574:                               ; preds = %list.reserve.end1585, %fstr.empty.alloc1563
  %ptr1597 = load ptr, ptr %buf.ptr.addr1565, align 8
  %len21598 = load i32, ptr %buf.len.addr1566, align 4
  br i1 true, label %fstr.zero1599, label %fstr.cpy1600

list.reserve1584:                                 ; preds = %fstr.grow1573
  %cap641586 = zext i32 %finalcap1579 to i64
  %bytes1587 = mul i64 %cap641586, ptrtoint (ptr getelementptr (i8, ptr null, i32 1) to i64)
  %heap1588 = call ptr @GetProcessHeap()
  %ptr1589 = load ptr, ptr %list.ptr.addr1580, align 8
  %isnull1590 = icmp eq ptr %ptr1589, null
  br i1 %isnull1590, label %list.alloc1591, label %list.realloc1592

list.reserve.end1585:                             ; preds = %list.alloc.merge1593, %fstr.grow1573
  br label %fstr.grow.cont1574

list.alloc1591:                                   ; preds = %list.reserve1584
  %mem1594 = call ptr @HeapAlloc(ptr %heap1588, i32 0, i64 %bytes1587)
  br label %list.alloc.merge1593

list.realloc1592:                                 ; preds = %list.reserve1584
  %mem1595 = call ptr @HeapReAlloc(ptr %heap1588, i32 0, ptr %ptr1589, i64 %bytes1587)
  br label %list.alloc.merge1593

list.alloc.merge1593:                             ; preds = %list.realloc1592, %list.alloc1591
  %memphi1596 = phi ptr [ %mem1594, %list.alloc1591 ], [ %mem1595, %list.realloc1592 ]
  store ptr %memphi1596, ptr %list.ptr.addr1580, align 8
  store i32 %finalcap1579, ptr %list.cap.addr1581, align 4
  br label %list.reserve.end1585

fstr.zero1599:                                    ; preds = %fstr.grow.cont1574
  %nul.ptr1602 = getelementptr inbounds i8, ptr %ptr1597, i32 %len21598
  store i8 0, ptr %nul.ptr1602, align 1
  br label %fstr.cpy.end1601

fstr.cpy1600:                                     ; preds = %fstr.grow.cont1574
  store i32 0, ptr %fstr.cpy.i1603, align 4
  br label %fstr.cpy.loop1604

fstr.cpy.end1601:                                 ; preds = %fstr.cpy.done1606, %fstr.zero1599
  br label %fstr.empty.cont1564

fstr.cpy.loop1604:                                ; preds = %fstr.cpy.body1605, %fstr.cpy1600
  %i1607 = load i32, ptr %fstr.cpy.i1603, align 4
  %cond1608 = icmp ult i32 %i1607, 0
  br i1 %cond1608, label %fstr.cpy.body1605, label %fstr.cpy.done1606

fstr.cpy.body1605:                                ; preds = %fstr.cpy.loop1604
  %s.ptr1609 = getelementptr inbounds i8, ptr @.fstr.1000011, i32 %i1607
  %ch1610 = load i8, ptr %s.ptr1609, align 1
  %dst.off1611 = add i32 %len21598, %i1607
  %d.ptr1612 = getelementptr inbounds i8, ptr %ptr1597, i32 %dst.off1611
  store i8 %ch1610, ptr %d.ptr1612, align 1
  %i21613 = add i32 %i1607, 1
  store i32 %i21613, ptr %fstr.cpy.i1603, align 4
  br label %fstr.cpy.loop1604

fstr.cpy.done1606:                                ; preds = %fstr.cpy.loop1604
  %newlen1614 = add i32 %len21598, 0
  store i32 %newlen1614, ptr %buf.len.addr1566, align 4
  %nul.ptr1615 = getelementptr inbounds i8, ptr %ptr1597, i32 %newlen1614
  store i8 0, ptr %nul.ptr1615, align 1
  br label %fstr.cpy.end1601

fstr.grow1645:                                    ; preds = %fstr.empty.cont1564
  %cap01647 = icmp eq i32 %cap1641, 0
  %cap21648 = mul i32 %cap1641, 2
  %basecap1649 = select i1 %cap01647, i32 64, i32 %cap21648
  %caplt1650 = icmp ult i32 %basecap1649, %want1643
  %finalcap1651 = select i1 %caplt1650, i32 %want1643, i32 %basecap1649
  %list.ptr.addr1652 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf1632, i32 0, i32 0
  %list.cap.addr1653 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf1632, i32 0, i32 2
  %cap1654 = load i32, ptr %list.cap.addr1653, align 4
  %needcap1655 = icmp ugt i32 %finalcap1651, %cap1654
  br i1 %needcap1655, label %list.reserve1656, label %list.reserve.end1657

fstr.grow.cont1646:                               ; preds = %list.reserve.end1657, %fstr.empty.cont1564
  %ptr1669 = load ptr, ptr %buf.ptr.addr1637, align 8
  %len21670 = load i32, ptr %buf.len.addr1638, align 4
  br i1 false, label %fstr.zero1671, label %fstr.cpy1672

list.reserve1656:                                 ; preds = %fstr.grow1645
  %cap641658 = zext i32 %finalcap1651 to i64
  %bytes1659 = mul i64 %cap641658, ptrtoint (ptr getelementptr (i8, ptr null, i32 1) to i64)
  %heap1660 = call ptr @GetProcessHeap()
  %ptr1661 = load ptr, ptr %list.ptr.addr1652, align 8
  %isnull1662 = icmp eq ptr %ptr1661, null
  br i1 %isnull1662, label %list.alloc1663, label %list.realloc1664

list.reserve.end1657:                             ; preds = %list.alloc.merge1665, %fstr.grow1645
  br label %fstr.grow.cont1646

list.alloc1663:                                   ; preds = %list.reserve1656
  %mem1666 = call ptr @HeapAlloc(ptr %heap1660, i32 0, i64 %bytes1659)
  br label %list.alloc.merge1665

list.realloc1664:                                 ; preds = %list.reserve1656
  %mem1667 = call ptr @HeapReAlloc(ptr %heap1660, i32 0, ptr %ptr1661, i64 %bytes1659)
  br label %list.alloc.merge1665

list.alloc.merge1665:                             ; preds = %list.realloc1664, %list.alloc1663
  %memphi1668 = phi ptr [ %mem1666, %list.alloc1663 ], [ %mem1667, %list.realloc1664 ]
  store ptr %memphi1668, ptr %list.ptr.addr1652, align 8
  store i32 %finalcap1651, ptr %list.cap.addr1653, align 4
  br label %list.reserve.end1657

fstr.zero1671:                                    ; preds = %fstr.grow.cont1646
  %nul.ptr1674 = getelementptr inbounds i8, ptr %ptr1669, i32 %len21670
  store i8 0, ptr %nul.ptr1674, align 1
  br label %fstr.cpy.end1673

fstr.cpy1672:                                     ; preds = %fstr.grow.cont1646
  store i32 0, ptr %fstr.cpy.i1675, align 4
  br label %fstr.cpy.loop1676

fstr.cpy.end1673:                                 ; preds = %fstr.cpy.done1678, %fstr.zero1671
  %checksum1688 = load i32, ptr %checksum, align 4
  %sext1689 = sext i32 %checksum1688 to i64
  %neg1690 = icmp slt i64 %sext1689, 0
  %negv1691 = sub i64 0, %sext1689
  %abs1692 = select i1 %neg1690, i64 %negv1691, i64 %sext1689
  store i32 31, ptr %fstr.itoa.idx1693, align 4
  store i64 %abs1692, ptr %fstr.itoa.cur1694, align 4
  %end.ptr1695 = getelementptr inbounds [32 x i8], ptr %fstr.itoa.buf1633, i32 0, i32 31
  store i8 0, ptr %end.ptr1695, align 1
  %iszero1696 = icmp eq i64 %abs1692, 0
  br i1 %iszero1696, label %itoa.zero1697, label %itoa.loop1698

fstr.cpy.loop1676:                                ; preds = %fstr.cpy.body1677, %fstr.cpy1672
  %i1679 = load i32, ptr %fstr.cpy.i1675, align 4
  %cond1680 = icmp ult i32 %i1679, 9
  br i1 %cond1680, label %fstr.cpy.body1677, label %fstr.cpy.done1678

fstr.cpy.body1677:                                ; preds = %fstr.cpy.loop1676
  %s.ptr1681 = getelementptr inbounds i8, ptr @.fstr.1000024, i32 %i1679
  %ch1682 = load i8, ptr %s.ptr1681, align 1
  %dst.off1683 = add i32 %len21670, %i1679
  %d.ptr1684 = getelementptr inbounds i8, ptr %ptr1669, i32 %dst.off1683
  store i8 %ch1682, ptr %d.ptr1684, align 1
  %i21685 = add i32 %i1679, 1
  store i32 %i21685, ptr %fstr.cpy.i1675, align 4
  br label %fstr.cpy.loop1676

fstr.cpy.done1678:                                ; preds = %fstr.cpy.loop1676
  %newlen1686 = add i32 %len21670, 9
  store i32 %newlen1686, ptr %buf.len.addr1638, align 4
  %nul.ptr1687 = getelementptr inbounds i8, ptr %ptr1669, i32 %newlen1686
  store i8 0, ptr %nul.ptr1687, align 1
  br label %fstr.cpy.end1673

itoa.zero1697:                                    ; preds = %fstr.cpy.end1673
  store i32 30, ptr %fstr.itoa.idx1693, align 4
  %z.ptr1700 = getelementptr inbounds [32 x i8], ptr %fstr.itoa.buf1633, i32 0, i32 30
  store i8 48, ptr %z.ptr1700, align 1
  br label %itoa.after1699

itoa.loop1698:                                    ; preds = %fstr.cpy.end1673
  br label %itoa.cond1701

itoa.after1699:                                   ; preds = %itoa.done1703, %itoa.zero1697
  br i1 %neg1690, label %itoa.neg1714, label %itoa.merge1715

itoa.cond1701:                                    ; preds = %itoa.body1702, %itoa.loop1698
  %curv1704 = load i64, ptr %fstr.itoa.cur1694, align 4
  %more1705 = icmp ugt i64 %curv1704, 0
  br i1 %more1705, label %itoa.body1702, label %itoa.done1703

itoa.body1702:                                    ; preds = %itoa.cond1701
  %rem1706 = urem i64 %curv1704, 10
  %div1707 = udiv i64 %curv1704, 10
  store i64 %div1707, ptr %fstr.itoa.cur1694, align 4
  %idxv1708 = load i32, ptr %fstr.itoa.idx1693, align 4
  %idx11709 = sub i32 %idxv1708, 1
  store i32 %idx11709, ptr %fstr.itoa.idx1693, align 4
  %rem321710 = trunc i64 %rem1706 to i32
  %ch321711 = add i32 %rem321710, 48
  %ch1712 = trunc i32 %ch321711 to i8
  %d.ptr1713 = getelementptr inbounds [32 x i8], ptr %fstr.itoa.buf1633, i32 0, i32 %idx11709
  store i8 %ch1712, ptr %d.ptr1713, align 1
  br label %itoa.cond1701

itoa.done1703:                                    ; preds = %itoa.cond1701
  br label %itoa.after1699

itoa.neg1714:                                     ; preds = %itoa.after1699
  %idxv1716 = load i32, ptr %fstr.itoa.idx1693, align 4
  %idxm11717 = sub i32 %idxv1716, 1
  store i32 %idxm11717, ptr %fstr.itoa.idx1693, align 4
  %m.ptr1718 = getelementptr inbounds [32 x i8], ptr %fstr.itoa.buf1633, i32 0, i32 %idxm11717
  store i8 45, ptr %m.ptr1718, align 1
  br label %itoa.merge1715

itoa.merge1715:                                   ; preds = %itoa.neg1714, %itoa.after1699
  %start1719 = load i32, ptr %fstr.itoa.idx1693, align 4
  %slen1720 = sub i32 31, %start1719
  %sptr1721 = getelementptr inbounds [32 x i8], ptr %fstr.itoa.buf1633, i32 0, i32 %start1719
  %buf.ptr.addr1722 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf1632, i32 0, i32 0
  %buf.len.addr1723 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf1632, i32 0, i32 1
  %buf.cap.addr1724 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf1632, i32 0, i32 2
  %len1725 = load i32, ptr %buf.len.addr1723, align 4
  %cap1726 = load i32, ptr %buf.cap.addr1724, align 4
  %want.tmp1727 = add i32 %len1725, %slen1720
  %want1728 = add i32 %want.tmp1727, 1
  %needgrow1729 = icmp ugt i32 %want1728, %cap1726
  br i1 %needgrow1729, label %fstr.grow1730, label %fstr.grow.cont1731

fstr.grow1730:                                    ; preds = %itoa.merge1715
  %cap01732 = icmp eq i32 %cap1726, 0
  %cap21733 = mul i32 %cap1726, 2
  %basecap1734 = select i1 %cap01732, i32 64, i32 %cap21733
  %caplt1735 = icmp ult i32 %basecap1734, %want1728
  %finalcap1736 = select i1 %caplt1735, i32 %want1728, i32 %basecap1734
  %list.ptr.addr1737 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf1632, i32 0, i32 0
  %list.cap.addr1738 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf1632, i32 0, i32 2
  %cap1739 = load i32, ptr %list.cap.addr1738, align 4
  %needcap1740 = icmp ugt i32 %finalcap1736, %cap1739
  br i1 %needcap1740, label %list.reserve1741, label %list.reserve.end1742

fstr.grow.cont1731:                               ; preds = %list.reserve.end1742, %itoa.merge1715
  %ptr1754 = load ptr, ptr %buf.ptr.addr1722, align 8
  %len21755 = load i32, ptr %buf.len.addr1723, align 4
  %srclen01756 = icmp eq i32 %slen1720, 0
  br i1 %srclen01756, label %fstr.zero1757, label %fstr.cpy1758

list.reserve1741:                                 ; preds = %fstr.grow1730
  %cap641743 = zext i32 %finalcap1736 to i64
  %bytes1744 = mul i64 %cap641743, ptrtoint (ptr getelementptr (i8, ptr null, i32 1) to i64)
  %heap1745 = call ptr @GetProcessHeap()
  %ptr1746 = load ptr, ptr %list.ptr.addr1737, align 8
  %isnull1747 = icmp eq ptr %ptr1746, null
  br i1 %isnull1747, label %list.alloc1748, label %list.realloc1749

list.reserve.end1742:                             ; preds = %list.alloc.merge1750, %fstr.grow1730
  br label %fstr.grow.cont1731

list.alloc1748:                                   ; preds = %list.reserve1741
  %mem1751 = call ptr @HeapAlloc(ptr %heap1745, i32 0, i64 %bytes1744)
  br label %list.alloc.merge1750

list.realloc1749:                                 ; preds = %list.reserve1741
  %mem1752 = call ptr @HeapReAlloc(ptr %heap1745, i32 0, ptr %ptr1746, i64 %bytes1744)
  br label %list.alloc.merge1750

list.alloc.merge1750:                             ; preds = %list.realloc1749, %list.alloc1748
  %memphi1753 = phi ptr [ %mem1751, %list.alloc1748 ], [ %mem1752, %list.realloc1749 ]
  store ptr %memphi1753, ptr %list.ptr.addr1737, align 8
  store i32 %finalcap1736, ptr %list.cap.addr1738, align 4
  br label %list.reserve.end1742

fstr.zero1757:                                    ; preds = %fstr.grow.cont1731
  %nul.ptr1760 = getelementptr inbounds i8, ptr %ptr1754, i32 %len21755
  store i8 0, ptr %nul.ptr1760, align 1
  br label %fstr.cpy.end1759

fstr.cpy1758:                                     ; preds = %fstr.grow.cont1731
  store i32 0, ptr %fstr.cpy.i1761, align 4
  br label %fstr.cpy.loop1762

fstr.cpy.end1759:                                 ; preds = %fstr.cpy.done1764, %fstr.zero1757
  %buf.len.addr1774 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf1632, i32 0, i32 1
  %buf.len1775 = load i32, ptr %buf.len.addr1774, align 4
  %fstr.empty1776 = icmp eq i32 %buf.len1775, 0
  %buf.ptr.addr1777 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf1632, i32 0, i32 0
  %outp01778 = load ptr, ptr %buf.ptr.addr1777, align 8
  %out.null01779 = icmp eq ptr %outp01778, null
  %needalloc1780 = and i1 %fstr.empty1776, %out.null01779
  br i1 %needalloc1780, label %fstr.empty.alloc1781, label %fstr.empty.cont1782

fstr.cpy.loop1762:                                ; preds = %fstr.cpy.body1763, %fstr.cpy1758
  %i1765 = load i32, ptr %fstr.cpy.i1761, align 4
  %cond1766 = icmp ult i32 %i1765, %slen1720
  br i1 %cond1766, label %fstr.cpy.body1763, label %fstr.cpy.done1764

fstr.cpy.body1763:                                ; preds = %fstr.cpy.loop1762
  %s.ptr1767 = getelementptr inbounds i8, ptr %sptr1721, i32 %i1765
  %ch1768 = load i8, ptr %s.ptr1767, align 1
  %dst.off1769 = add i32 %len21755, %i1765
  %d.ptr1770 = getelementptr inbounds i8, ptr %ptr1754, i32 %dst.off1769
  store i8 %ch1768, ptr %d.ptr1770, align 1
  %i21771 = add i32 %i1765, 1
  store i32 %i21771, ptr %fstr.cpy.i1761, align 4
  br label %fstr.cpy.loop1762

fstr.cpy.done1764:                                ; preds = %fstr.cpy.loop1762
  %newlen1772 = add i32 %len21755, %slen1720
  store i32 %newlen1772, ptr %buf.len.addr1723, align 4
  %nul.ptr1773 = getelementptr inbounds i8, ptr %ptr1754, i32 %newlen1772
  store i8 0, ptr %nul.ptr1773, align 1
  br label %fstr.cpy.end1759

fstr.empty.alloc1781:                             ; preds = %fstr.cpy.end1759
  %buf.ptr.addr1783 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf1632, i32 0, i32 0
  %buf.len.addr1784 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf1632, i32 0, i32 1
  %buf.cap.addr1785 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf1632, i32 0, i32 2
  %len1786 = load i32, ptr %buf.len.addr1784, align 4
  %cap1787 = load i32, ptr %buf.cap.addr1785, align 4
  %want.tmp1788 = add i32 %len1786, 0
  %want1789 = add i32 %want.tmp1788, 1
  %needgrow1790 = icmp ugt i32 %want1789, %cap1787
  br i1 %needgrow1790, label %fstr.grow1791, label %fstr.grow.cont1792

fstr.empty.cont1782:                              ; preds = %fstr.cpy.end1819, %fstr.cpy.end1759
  %fstr.val1834 = load %emp.string, ptr %fstr.buf1632, align 8
  store %emp.string %fstr.val1834, ptr %msg1631, align 8
  %msg1835 = load %emp.string, ptr %msg1631, align 8
  call void @println__Nstring(%emp.string %msg1835)
  %list.ptr.addr1836 = getelementptr inbounds nuw { ptr, i32, i32 }, ptr %keys, i32 0, i32 0
  %list.len.addr1837 = getelementptr inbounds nuw { ptr, i32, i32 }, ptr %keys, i32 0, i32 1
  %list.cap.addr1838 = getelementptr inbounds nuw { ptr, i32, i32 }, ptr %keys, i32 0, i32 2
  %ptr1839 = load ptr, ptr %list.ptr.addr1836, align 8
  %isnull1840 = icmp eq ptr %ptr1839, null
  br i1 %isnull1840, label %list.free.end, label %list.free

fstr.grow1791:                                    ; preds = %fstr.empty.alloc1781
  %cap01793 = icmp eq i32 %cap1787, 0
  %cap21794 = mul i32 %cap1787, 2
  %basecap1795 = select i1 %cap01793, i32 64, i32 %cap21794
  %caplt1796 = icmp ult i32 %basecap1795, %want1789
  %finalcap1797 = select i1 %caplt1796, i32 %want1789, i32 %basecap1795
  %list.ptr.addr1798 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf1632, i32 0, i32 0
  %list.cap.addr1799 = getelementptr inbounds nuw %emp.string, ptr %fstr.buf1632, i32 0, i32 2
  %cap1800 = load i32, ptr %list.cap.addr1799, align 4
  %needcap1801 = icmp ugt i32 %finalcap1797, %cap1800
  br i1 %needcap1801, label %list.reserve1802, label %list.reserve.end1803

fstr.grow.cont1792:                               ; preds = %list.reserve.end1803, %fstr.empty.alloc1781
  %ptr1815 = load ptr, ptr %buf.ptr.addr1783, align 8
  %len21816 = load i32, ptr %buf.len.addr1784, align 4
  br i1 true, label %fstr.zero1817, label %fstr.cpy1818

list.reserve1802:                                 ; preds = %fstr.grow1791
  %cap641804 = zext i32 %finalcap1797 to i64
  %bytes1805 = mul i64 %cap641804, ptrtoint (ptr getelementptr (i8, ptr null, i32 1) to i64)
  %heap1806 = call ptr @GetProcessHeap()
  %ptr1807 = load ptr, ptr %list.ptr.addr1798, align 8
  %isnull1808 = icmp eq ptr %ptr1807, null
  br i1 %isnull1808, label %list.alloc1809, label %list.realloc1810

list.reserve.end1803:                             ; preds = %list.alloc.merge1811, %fstr.grow1791
  br label %fstr.grow.cont1792

list.alloc1809:                                   ; preds = %list.reserve1802
  %mem1812 = call ptr @HeapAlloc(ptr %heap1806, i32 0, i64 %bytes1805)
  br label %list.alloc.merge1811

list.realloc1810:                                 ; preds = %list.reserve1802
  %mem1813 = call ptr @HeapReAlloc(ptr %heap1806, i32 0, ptr %ptr1807, i64 %bytes1805)
  br label %list.alloc.merge1811

list.alloc.merge1811:                             ; preds = %list.realloc1810, %list.alloc1809
  %memphi1814 = phi ptr [ %mem1812, %list.alloc1809 ], [ %mem1813, %list.realloc1810 ]
  store ptr %memphi1814, ptr %list.ptr.addr1798, align 8
  store i32 %finalcap1797, ptr %list.cap.addr1799, align 4
  br label %list.reserve.end1803

fstr.zero1817:                                    ; preds = %fstr.grow.cont1792
  %nul.ptr1820 = getelementptr inbounds i8, ptr %ptr1815, i32 %len21816
  store i8 0, ptr %nul.ptr1820, align 1
  br label %fstr.cpy.end1819

fstr.cpy1818:                                     ; preds = %fstr.grow.cont1792
  store i32 0, ptr %fstr.cpy.i1821, align 4
  br label %fstr.cpy.loop1822

fstr.cpy.end1819:                                 ; preds = %fstr.cpy.done1824, %fstr.zero1817
  br label %fstr.empty.cont1782

fstr.cpy.loop1822:                                ; preds = %fstr.cpy.body1823, %fstr.cpy1818
  %i1825 = load i32, ptr %fstr.cpy.i1821, align 4
  %cond1826 = icmp ult i32 %i1825, 0
  br i1 %cond1826, label %fstr.cpy.body1823, label %fstr.cpy.done1824

fstr.cpy.body1823:                                ; preds = %fstr.cpy.loop1822
  %s.ptr1827 = getelementptr inbounds i8, ptr @.fstr.1000020, i32 %i1825
  %ch1828 = load i8, ptr %s.ptr1827, align 1
  %dst.off1829 = add i32 %len21816, %i1825
  %d.ptr1830 = getelementptr inbounds i8, ptr %ptr1815, i32 %dst.off1829
  store i8 %ch1828, ptr %d.ptr1830, align 1
  %i21831 = add i32 %i1825, 1
  store i32 %i21831, ptr %fstr.cpy.i1821, align 4
  br label %fstr.cpy.loop1822

fstr.cpy.done1824:                                ; preds = %fstr.cpy.loop1822
  %newlen1832 = add i32 %len21816, 0
  store i32 %newlen1832, ptr %buf.len.addr1784, align 4
  %nul.ptr1833 = getelementptr inbounds i8, ptr %ptr1815, i32 %newlen1832
  store i8 0, ptr %nul.ptr1833, align 1
  br label %fstr.cpy.end1819

list.free:                                        ; preds = %fstr.empty.cont1782
  %heap1841 = call ptr @GetProcessHeap()
  %0 = call i32 @HeapFree(ptr %heap1841, i32 0, ptr %ptr1839)
  br label %list.free.end

list.free.end:                                    ; preds = %list.free, %fstr.empty.cont1782
  store ptr null, ptr %list.ptr.addr1836, align 8
  store i32 0, ptr %list.len.addr1837, align 4
  store i32 0, ptr %list.cap.addr1838, align 4
  %list.ptr.addr1842 = getelementptr inbounds nuw { ptr, i32, i32 }, ptr %vals, i32 0, i32 0
  %list.len.addr1843 = getelementptr inbounds nuw { ptr, i32, i32 }, ptr %vals, i32 0, i32 1
  %list.cap.addr1844 = getelementptr inbounds nuw { ptr, i32, i32 }, ptr %vals, i32 0, i32 2
  %ptr1845 = load ptr, ptr %list.ptr.addr1842, align 8
  %isnull1846 = icmp eq ptr %ptr1845, null
  br i1 %isnull1846, label %list.free.end1848, label %list.free1847

list.free1847:                                    ; preds = %list.free.end
  %heap1849 = call ptr @GetProcessHeap()
  %1 = call i32 @HeapFree(ptr %heap1849, i32 0, ptr %ptr1845)
  br label %list.free.end1848

list.free.end1848:                                ; preds = %list.free1847, %list.free.end
  store ptr null, ptr %list.ptr.addr1842, align 8
  store i32 0, ptr %list.len.addr1843, align 4
  store i32 0, ptr %list.cap.addr1844, align 4
  %list.ptr.addr1850 = getelementptr inbounds nuw { ptr, i32, i32 }, ptr %used, i32 0, i32 0
  %list.len.addr1851 = getelementptr inbounds nuw { ptr, i32, i32 }, ptr %used, i32 0, i32 1
  %list.cap.addr1852 = getelementptr inbounds nuw { ptr, i32, i32 }, ptr %used, i32 0, i32 2
  %ptr1853 = load ptr, ptr %list.ptr.addr1850, align 8
  %isnull1854 = icmp eq ptr %ptr1853, null
  br i1 %isnull1854, label %list.free.end1856, label %list.free1855

list.free1855:                                    ; preds = %list.free.end1848
  %heap1857 = call ptr @GetProcessHeap()
  %2 = call i32 @HeapFree(ptr %heap1857, i32 0, ptr %ptr1853)
  br label %list.free.end1856

list.free.end1856:                                ; preds = %list.free1855, %list.free.end1848
  store ptr null, ptr %list.ptr.addr1850, align 8
  store i32 0, ptr %list.len.addr1851, align 4
  store i32 0, ptr %list.cap.addr1852, align 4
  ret i32 0
}

declare ptr @GetProcessHeap()

declare i32 @HeapFree(ptr, i32, ptr)

declare ptr @HeapAlloc(ptr, i32, i64)

declare ptr @HeapReAlloc(ptr, i32, ptr, i64)

define void @mainCRTStartup() {
entry:
  %rc = call i32 @main()
  call void @ExitProcess(i32 %rc)
  unreachable
}
