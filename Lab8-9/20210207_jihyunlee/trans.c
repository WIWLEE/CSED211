//20210207 ÀÌÁöÇö
//ID : io0818

/*
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */
#include <stdio.h>
#include "cachelab.h"
int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/*
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded.
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, k, l = 16;
    int b = 0;

    //32x32
    if (N == 32 && M == 32) {

        b = 8;
        int temp[8];

        for (k = 0; k < M; k += 8)
            for (i = 0; i < N; i += 8)
                for (j = 0; j < b; j++) {
                    for (l = 0; l < b; l++) {
                        temp[l] = A[k + j][i + l];
                    }

                    for (l = 0; l < b; l++) {
                        B[i + l][k + j] = temp[l];
                    }

                }

                

    }
    
   
    else if (N == 64 && M == 64) {

        b = 4;
        int temp[4];

        for (k = 0; k < M; k+=4)
            for (i = 0; i < N; i+=4)
                for (j = 0; j < b; j++) {
                    for (l = 0; l < b; l++) {
                        temp[l] = A[k + j][i + l];
                    }

                    for (l = 0; l < b; l++) {
                        B[i + l][k + j] = temp[l];
                    }

                }

    }

    else {
       

        b = 16;
        int temp;
        int x;

        for (k = 0; k < M; k += 16)
            for (i = 0; i < N; i += 16)
                for (j = i; j < i + 16 && j<N; j++) {
                    for (l = k; l < k+16 && l<M; l++) {

                        // column != row
                        if (j != l)
                            B[l][j] = A[j][l];
                        // column == row
                        else {
                            temp = A[j][l];
                            x =l;
                        }

                    }

                    if (k == i) B[x][x] = temp;

                }

    }

}

/*
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started.
 */

 /*
  * trans - A simple baseline transpose function, not optimized for the cache.
  */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }

}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc);

    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc);

}

/*
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}
