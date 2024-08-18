"slli %[arr_size], %[arr_size], 1\n\t"
"addi %[others_cnt], %[others_cnt], 1\n\t"

"improved_loop:\n\t" // VLEN = 256
    "vsetvli t0, %[arr_size], e16\n\t" // set SEW=16, vl=min(VLEN/SEW, %[arr_size]), t0=vl, I wish to load whole array a time

    "vle16.v v1, 0(%[x])\n\t" // load halfword elements into v1, v2 from &arr[0] ~ &arr[arr_size-1]
    "vle16.v v2, 0(%[h])\n\t"
    "vmul.vv v0, v1, v2\n\t" // element multiply with corresponding element
    "vadd.vx v0, v0, %[id]\n\t" // each element adds id, v0 stores result
    "vse16.v v0, 0(%[y])\n\t" // assign halfword elements to array y

    "sub %[arr_size], %[arr_size], t0\n\t" // change counter
    "slli t0, t0, 1\n\t" // 2 bytes per halfword, using t0 to move
    "add %[h], %[h], t0\n\t" // move to the beginning of next vector
    "add %[x], %[x], t0\n\t"
    "add %[y], %[y], t0\n\t"

    "addi %[add_cnt], %[add_cnt], 4\n\t" // 3 count instructions 
    "addi %[sub_cnt], %[sub_cnt], 1\n\t" // 3
    "addi %[mul_cnt], %[mul_cnt], 1\n\t" // 4
    "addi %[lw_cnt], %[lw_cnt], 2\n\t" // 20
    "addi %[sw_cnt], %[sw_cnt], 1\n\t" // 15
    "addi %[others_cnt], %[others_cnt], 3\n\t" // 3

    "bnez %[arr_size], improved_loop\n\t"