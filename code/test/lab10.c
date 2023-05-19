#include "syscall.h"

#define SIZE (5)

int A[SIZE]; /* size of physical memory; with code, we'll run out of space!*/

#define N 15

int main()
{
    int i,j=1;
    int table[N][N];
    for (i = 1; i <= N; ++i)
        for (j = 1; j <= N; ++j)
            table[i - 1][j - 1] = i * j;
  
    Halt();
}
// int main()
// {
//     int i, j, tmp;

//     /* first initialize the array, in reverse sorted order */
//     for (i = 0; i < SIZE; i++)
//     {
//         A[i] = (SIZE - 1) - i;
//     }

//     /* then sort! */
//     for (i = 0; i < SIZE; i++)
//     {
//         for (j = 0; j < (SIZE - 1); j++)
//         {
//             if (A[j] > A[j + 1])
//             { /* out of order -> need to swap ! */
//                 tmp = A[j];
//                 A[j] = A[j + 1];
//                 A[j + 1] = tmp;
//             }
//         }
//     }
//     Halt();
// }
