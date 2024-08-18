#include<stdio.h>
int main()
{ 
    int f, i, j;
    int h[9] = {0}, x[6] = {0}, y[6] = {0}; 
    FILE *input = fopen("../input/3.txt", "r");
    for(i = 0; i < 9; i++) fscanf(input, "%d", &h[i]);
    for(i = 0; i < 6; i++) fscanf(input, "%d", &x[i]);
    for(i = 0; i < 6; i++) fscanf(input, "%d", &y[i]);
    fclose(input);
    int *p_x = &x[0] ;
    int *p_h = &h[0] ;
    int *p_y = &y[0] ;
    for (i = 0; i < 3; i++){ 
        for (j = 0; j < 2; j++){        	
            for (f = 0; f < 3; f++)
		asm volatile (
                    "slli t0, %[i], 1\n\t"// t0 = 2i
		    "add t1, t0, %[i]\n\t"// t1 = 3i
                    "slli t2, %[f], 1\n\t"// t2 = 2f

		    "add t3, t0, %[j]\n\t"// t3 = 2i + j
		    "slli t3, t3, 2\n\t"
		    "add t3, %[p_y], t3\n\t"// t3 -> &p_y[2i + j]

		    "add t4, t1, %[f]\n\t"// t4 = 3i + f
		    "slli t4, t4, 2\n\t"
		    "add t4, %[p_h], t4\n\t"// t4 -> &p_h[3i + f]

		    "add t5, t2, %[j]\n\t"// t5 = 2f + j
		    "slli t5, t5, 2\n\t"
		    "add t5, %[p_x], t5\n\t"//t5 -> &p_x[2f + j]
 
		    "lw t0, 0(t5)\n\t"// t0 = x(ij)
		    "lw t1, 0(t4)\n\t"// t1 = h(ik)
		    "lw t6, 0(t3)\n\t"// t6 = y(jk)

		    "mul t2, t0, t1\n\t"// t2 = x(ij) * h(jk)
		    "add t6, t6, t2\n\t"

		    "sw t6, 0(t3)\n\t"
                    :
                    : [i] "r" (i), [j] "r" (j), [f] "r" (f), [p_x] "r" (p_x), [p_h] "r" (p_h), [p_y] "r" (p_y)
                    : "t0", "t1", "t2", "t3", "t4", "t5"
                );
	}
    }
    p_y = &y[0];
    for(i = 0; i < 6; i++)
    printf("%d ", *p_y++);
    printf("\n");
    return 0; 
 
}
