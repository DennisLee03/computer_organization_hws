#include <stdlib.h>

void matrix_multiplication(int *a, int *b, int *output, int i, int k, int j) {
    // 一個新的矩陣來存儲轉置後的 b
    int *bt = (int *)malloc(k * j * sizeof(int));

    // 轉置矩陣 b 並存儲到 bt
    for (int x = 0; x < k; x++) {
        for (int y = 0; y < j; y++) {
            bt[y * k + x] = b[x * j + y];
        }
    }

    // 進行矩陣乘法
    for (int x = 0; x < i; x++) {
        for (int y = 0; y < j; y++) {
            int sum = 0;
            for (int z = 0; z < k; z++) {
                sum += a[x * k + z] * bt[y * k + z];
            }
            output[x * j + y] = sum;
        }
    }

    free(bt);
}
