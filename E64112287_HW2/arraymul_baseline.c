"loop_start:\n\t"

    "lh t0, 0(%[h])\n\t" // t0 = h[i]
    "lh t1, 0(%[x])\n\t" // t1 = x[i]
    "mul t0, t0, t1\n\t" // t0 = h[i] * x[i]
    "add t0, t0, %[id]\n\t" // t0 = h[i] * x[i] + id
    "sh t0, 0(%[y])\n\t" // y[i] = h[i] * x[i] + id

    "addi %[y], %[y], 2\n\t" // move to next element
    "addi %[h], %[h], 2\n\t" // move 2 byte because of halfword per element
    "addi %[x], %[x], 2\n\t"

    "addi %[arr_size], %[arr_size], -1\n\t" // use arr_size to count because it will not change after the inline asm

    "addi %[add_cnt], %[add_cnt], 5\n\t" // count instructions
    "addi %[mul_cnt], %[mul_cnt], 1\n\t"
    "addi %[lw_cnt], %[lw_cnt], 2\n\t"
    "addi %[sw_cnt], %[sw_cnt], 1\n\t"
    "addi %[others_cnt], %[others_cnt], 1\n\t"

    "bnez %[arr_size], loop_start\n\t"

"loop_end:\n\t"