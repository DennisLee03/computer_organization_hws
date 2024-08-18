#include<stdio.h>
int main()
{ 
    int i = 0;
    int h[9] = {0}, x[6] = {0}, y[6] = {0}; 
    FILE *input = fopen("../input/4.txt", "r");
    for(i = 0; i < 9; i++) fscanf(input, "%d", &h[i]);
    for(i = 0; i < 6; i++) fscanf(input, "%d", &x[i]);
    for(i = 0; i < 6; i++) fscanf(input, "%d", &y[i]);
    fclose(input);
    
    int *p_x = &x[0];
    int *p_h = &h[0];
    int *p_y = &y[0];

    asm volatile (
        "li t3, 3\n\t"          
        "li t4, 2\n\t"          
        "li t5, 3\n\t"          

        "li t2, 0\n\t"// i = 0          
        "loop_i:\n\t"
            "li t1, 0\n\t"// j = 0      
            "loop_j:\n\t"
                "li t0, 0\n\t"//f = 0
                "loop_f:\n\t"

			"slli s1, t2, 1\n\t"// s1 = 2i
		        "add s2, s1, t2\n\t"// s2 = 3i
                    	"slli s3, t0, 1\n\t"// s3 = 2f

		        "add s4, s1, t1\n\t"// s4 = 2i + j
		        "slli s4, s4, 2\n\t"
	    		"add s4, %[p_y], s4\n\t"// t3 -> &p_y[2i + j]
	    
			"add s5, s2, t0\n\t"// s5 = 3i + f
    			"slli s5, s5, 2\n\t"
			"add s5, %[p_h], s5\n\t"// s5 -> &p_h[3i + f]
			"add s6, s3, t1\n\t"// s6 = 2f + j
			"slli s6, s6, 2\n\t"
			"add s6, %[p_x], s6\n\t"//s6 -> &p_x[2f + j]
 

			"lw s1, 0(s6)\n\t"// s1 = x(ij)
			"lw s2, 0(s5)\n\t"// s2 = h(ik)
			"lw s7, 0(s4)\n\t"// s7 = y(jk)

			"mul s3, s1, s2\n\t"// t2 = x(ij) * h(jk)
		        "add s7, s7, s3\n\t"
		        "sw s7, 0(s4)\n\t"

                "addi t0, t0, 1\n\t"// f++    
                "blt t0, t5, loop_f\n\t"// check (f < 3)
            "addi t1, t1, 1\n\t"// j++        
            "blt t1, t4, loop_j\n\t"// check (j < 2)     
        "addi t2, t2, 1\n\t"// i++
        "blt t2, t3, loop_i\n\t"// check (i < 3)         
    : [p_y] "+r" (p_y)
    : [p_x] "r" (p_x), [p_h] "r" (p_h)
    : "t0", "t1", "t2", "t3", "t4", "t5", "t6", "s1", "s2", "s3", "s4", "s5", "s6", "s7"      
    );  

    p_y = &y[0];
    for(i = 0; i < 6; i++)
        printf("%d ", *p_y++);
    printf("\n");
    return 0; 
 
}
