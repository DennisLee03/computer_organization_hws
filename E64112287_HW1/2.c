#include <stdio.h>
int main ()
{
    int a[10] = {0}, b[10] = {0}, c[10] = {0}; 
    int i, arr_size = 10;
    FILE *input = fopen("../input/2.txt", "r");
    for(i = 0; i < arr_size; i++) fscanf(input, "%d", &a[i]);
    for(i = 0; i < arr_size; i++) fscanf(input, "%d", &b[i]);
    for(i = 0; i < arr_size; i++) fscanf(input, "%d", &c[i]);
    fclose(input);
    int *p_a = &a[0];
    int *p_b = &b[0];
    int *p_c = &c[0];
    /* Original C code segment
    for (int i = 0; i < arr_size; i++){
    *p_c++ = *p_a++ - *p_b++;
    }
    */
    for (int i = 0; i < arr_size; i++)
    asm volatile(
            "lw t0, 0(%0)\n\t"   // Load p_a[i] into t0
            "addi %0, %0, 4\n\t" // Increment p_a pointer
            "lw t1, 0(%1)\n\t"   // Load p_b[i] into t1
            "addi %1, %1, 4\n\t" // Increment p_b pointer
            "sub t2, t0, t1\n\t" // Subtract p_b[i] from p_a[i] and store result in t2
            "sw t2, 0(%2)\n\t"   // Store result in p_c[i]
            "addi %2, %2, 4\n\t" // Increment p_c pointer
            : "+r" (p_a), "+r" (p_b), "+r" (p_c)
            :
            : "t0", "t1", "t2"
    );
    p_c = &c[0];
    for (int i = 0; i < arr_size; i++)
    printf("%d ", *p_c++);
    printf("\n");
    return 0;
}
