/* POLYBENCH/GPU-OPENMP
 *
 * This file is a part of the Polybench/GPU-OpenMP suite
 *
 * Contact:
 * William Killian <killian@udel.edu>
 * 
 * Copyright 2013, The University of Delaware
 */
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

/* Include polybench common header. */
#include <polybench.h>

/* Include benchmark-specific header. */
/* Default data type is double, default size is 4000. */
#include "mvt.h"


/* Array initialization. */
static
void init_array(int n,
		DATA_TYPE POLYBENCH_1D_CUDA(x1,N,n),
		DATA_TYPE POLYBENCH_1D_CUDA(x2,N,n),
		DATA_TYPE POLYBENCH_1D_CUDA(y_1,N,n),
		DATA_TYPE POLYBENCH_1D_CUDA(y_2,N,n),
		DATA_TYPE POLYBENCH_2D_CUDA(A,N,N,n,n))
{
  int i, j;

  for (i = 0; i < n; i++)
    {
      POLYBENCH_1D_REF(x1,N,i) = ((DATA_TYPE) i) / n;
      POLYBENCH_1D_REF(x2,N,i) = ((DATA_TYPE) i + 1) / n;
      POLYBENCH_1D_REF(y_1,N,i) = ((DATA_TYPE) i + 3) / n;
      POLYBENCH_1D_REF(y_2,N,i) = ((DATA_TYPE) i + 4) / n;
      for (j = 0; j < n; j++)
	POLYBENCH_2D_REF(A,N,N,i,j) = ((DATA_TYPE) i*j) / N;
    }
}


/* DCE code. Must scan the entire live-out data.
   Can be used also to check the correctness of the output. */
static
void print_array(int n,
		 DATA_TYPE POLYBENCH_1D_CUDA(x1,N,n),
		 DATA_TYPE POLYBENCH_1D_CUDA(x2,N,n))

{
  int i;

  for (i = 0; i < n; i++) {
    fprintf (stderr, DATA_PRINTF_MODIFIER, POLYBENCH_1D_REF(x1,N,i));
    fprintf (stderr, DATA_PRINTF_MODIFIER, POLYBENCH_1D_REF(x2,N,i));
    if (i % 20 == 0) fprintf (stderr, "\n");
  }
}


/* Main computational kernel. The whole function will be timed,
   including the call and return. */
static
void kernel_mvt(int n,
		DATA_TYPE POLYBENCH_1D_CUDA(x1,N,n),
		DATA_TYPE POLYBENCH_1D_CUDA(x2,N,n),
		DATA_TYPE POLYBENCH_1D_CUDA(y_1,N,n),
		DATA_TYPE POLYBENCH_1D_CUDA(y_2,N,n),
		DATA_TYPE POLYBENCH_2D_CUDA(A,N,N,n,n))
{
  //int i, j;
  //  #pragma scop
  //#pragma omp parallel private (j)

#P1
  for (int i = 0; i < _PB_N; i++)
    for (int j = 0; j < _PB_N; j++)
      POLYBENCH_1D_REF(x1,N,i) = POLYBENCH_1D_REF(x1,N,i) + POLYBENCH_2D_REF(A,N,N,i,j) * POLYBENCH_1D_REF(y_1,N,j);

#P1
  for (int i = 0; i < _PB_N; i++)
    for (int j = 0; j < _PB_N; j++)
      POLYBENCH_1D_REF(x2,N,i) = POLYBENCH_1D_REF(x2,N,i) + POLYBENCH_2D_REF(A,N,N,j,i) * POLYBENCH_1D_REF(y_2,N,j);

  // #pragma endscop
}


int main(int argc, char** argv)
{
  /* Retrieve problem size. */
  int n = N;

  /* Variable declaration/allocation. */
  POLYBENCH_2D_ARRAY_DECL_CUDA(A, DATA_TYPE, N, N, n, n);
  POLYBENCH_1D_ARRAY_DECL_CUDA(x1, DATA_TYPE, N, n);
  POLYBENCH_1D_ARRAY_DECL_CUDA(x2, DATA_TYPE, N, n);
  POLYBENCH_1D_ARRAY_DECL_CUDA(y_1, DATA_TYPE, N, n);
  POLYBENCH_1D_ARRAY_DECL_CUDA(y_2, DATA_TYPE, N, n);


  /* Initialize array(s). */
  init_array (n,
	      POLYBENCH_ARRAY_CUDA(x1),
	      POLYBENCH_ARRAY_CUDA(x2),
	      POLYBENCH_ARRAY_CUDA(y_1),
	      POLYBENCH_ARRAY_CUDA(y_2),
	      POLYBENCH_ARRAY_CUDA(A));

  /* Start timer. */
  polybench_start_instruments;

  /* Run kernel. */
  kernel_mvt (n,
	      POLYBENCH_ARRAY_CUDA(x1),
	      POLYBENCH_ARRAY_CUDA(x2),
	      POLYBENCH_ARRAY_CUDA(y_1),
	      POLYBENCH_ARRAY_CUDA(y_2),
	      POLYBENCH_ARRAY_CUDA(A));

  /* Stop and print timer. */
  polybench_stop_instruments;
  polybench_print_instruments;

  /* Prevent dead-code elimination. All live-out data must be printed
     by the function call in argument. */
  polybench_prevent_dce(print_array(n, POLYBENCH_ARRAY_CUDA(x1), POLYBENCH_ARRAY_CUDA(x2)));

  /* Be clean. 
  POLYBENCH_FREE_ARRAY(A);
  POLYBENCH_FREE_ARRAY(x1);
  POLYBENCH_FREE_ARRAY(x2);
  POLYBENCH_FREE_ARRAY(y_1);
  POLYBENCH_FREE_ARRAY(y_2);
  */
  return 0;
}
