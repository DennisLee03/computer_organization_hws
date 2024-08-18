"li t1, 0\n\t" // t1 = 0 as a counter i
"li t2, 2\n\t" // t2 = 2 as a constant

"addi %[lw_cnt], %[lw_cnt], 2\n\t"

"loop_start:\n\t"
    
    "slli t3, t1, 1\n\t" // t3 = 2i
    "addi t3, t3, 1\n\t" // t3 = 2i + 1
    "fcvt.d.w f1, t3\n\t" // convert int to double, f1 = 2i + 1

    "rem t4, t1, t2\n\t" // t4 = i % 2, t4 = 0(even) or t4 = 1(odd), 2t + 1 will be -1(odd) or 1(even)    
    "li t3, -2\n\t" // t3 = -2
    "mul t4, t4, t3\n\t" // t4 = -2(t4)
    "addi t4, t4, 1\n\t" // t4 = -1(odd) or t4 = 1(even)
    "fcvt.d.w f2, t4\n\t" // convert int to double, f2 = +-1

    "fdiv.d f2, f2, f1\n\t" // f2 = (+-1) / (2i + 1)
    "fadd.d %[pi], %[pi], f2\n\t" // pi += term

    "addi t1, t1, 1\n\t" // i++

    "addi %[add_cnt], %[add_cnt], 4\n\t" // count instructions
    "addi %[mul_cnt], %[mul_cnt], 1\n\t"
    "addi %[div_cnt], %[div_cnt], 1\n\t"
    "addi %[others_cnt], %[others_cnt], 5\n\t"
    "addi %[lw_cnt], %[lw_cnt], 1\n\t"

    "blt t1, %[N], loop_start\n\t" // check i < N

"loop_end:\n\t"