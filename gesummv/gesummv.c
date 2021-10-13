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
#include "gesummv.h"


/* Array initialization. */
static
void init_array(int n,
		DATA_TYPE *alpha,
		DATA_TYPE *beta,
		DATA_TYPE POLYBENCH_2D_CUDA(A,N,N,n,n),
		DATA_TYPE POLYBENCH_2D_CUDA(B,N,N,n,n),
		DATA_TYPE POLYBENCH_1D_CUDA(x,N,n))
{
  int i, j;

  *alpha = 43532;
  *beta = 12313;
  for (i = 0; i < n; i++)
    {
      POLYBENCH_1D_REF(x,N,i) = ((DATA_TYPE) i) / n;
      for (j = 0; j < n; j++) {
	POLYBENCH_2D_REF(A,N,N,i,j) = ((DATA_TYPE) i*j) / n;
	POLYBENCH_2D_REF(B,N,N,i,j) = ((DATA_TYPE) i*j) / n;
      }
    }
}


/* DCE code. Must scan the entire live-out data.
   Can be used also to check the correctness of the output. */
static
void print_array(int n,
		 DATA_TYPE POLYBENCH_1D_CUDA(y,N,n))

{
  int i;

  for (i = 0; i < n; i++) {
    fprintf (stderr, DATA_PRINTF_MODIFIER, POLYBENCH_1D_REF(y,N,i));
    if (i % 20 == 0) fprintf (stderr, "\n");
  }
}


/* Main computational kernel. The whole function will be timed,
   including the call and return. */
static
void kernel_gesummv(int n,
		    DATA_TYPE alpha,
		    DATA_TYPE beta,
		    DATA_TYPE POLYBENCH_2D_CUDA(A,N,N,n,n),
		    DATA_TYPE POLYBENCH_2D_CUDA(B,N,N,n,n),
		    DATA_TYPE POLYBENCH_1D_CUDA(tmp,N,n),
		    DATA_TYPE POLYBENCH_1D_CUDA(x,N,n),
		    DATA_TYPE POLYBENCH_1D_CUDA(y,N,n))
{
  // int i, j;
  //#pragma scop
  //#pragma omp parallel
  
#P1
  for (int i = 0; i < _PB_N; i++)
    {
      POLYBENCH_1D_REF(tmp,N,i) = 0;
      POLYBENCH_1D_REF(y,N,i) = 0;
      for (int j = 0; j < _PB_N; j++)
	{
	  POLYBENCH_1D_REF(tmp,N,i) = POLYBENCH_2D_REF(A,N,N,i,j) * POLYBENCH_1D_REF(x,N,j) + POLYBENCH_1D_REF(tmp,N,i);
	  POLYBENCH_1D_REF(y,N,i) = POLYBENCH_2D_REF(B,N,N,i,j) * POLYBENCH_1D_REF(x,N,j) + POLYBENCH_1D_REF(y,N,i);
	}
      POLYBENCH_1D_REF(y,N,i) = alpha * POLYBENCH_1D_REF(tmp,N,i) + beta * POLYBENCH_1D_REF(y,N,i);
    }
  //#pragma endscop
}


int main(int argc, char** argv)
{
  /* Retrieve problem size. */
  int n = N;

  /* Variable declaration/allocation. */
  DATA_TYPE alpha;
  DATA_TYPE beta;
  POLYBENCH_2D_ARRAY_DECL_CUDA(A, DATA_TYPE, N, N, n, n);
  POLYBENCH_2D_ARRAY_DECL_CUDA(B, DATA_TYPE, N, N, n, n);
  POLYBENCH_1D_ARRAY_DECL_CUDA(tmp, DATA_TYPE, N, n);
  POLYBENCH_1D_ARRAY_DECL_CUDA(x, DATA_TYPE, N, n);
  POLYBENCH_1D_ARRAY_DECL_CUDA(y, DATA_TYPE, N, n);


  /* Initialize array(s). */
  init_array (n, &alpha, &beta,
	      POLYBENCH_ARRAY_CUDA(A),
	      POLYBENCH_ARRAY_CUDA(B),
	      POLYBENCH_ARRAY_CUDA(x));

  /* Start timer. */
  polybench_start_instruments;

  /* Run kernel. */
  kernel_gesummv (n, alpha, beta,
		  POLYBENCH_ARRAY_CUDA(A),
		  POLYBENCH_ARRAY_CUDA(B),
		  POLYBENCH_ARRAY_CUDA(tmp),
		  POLYBENCH_ARRAY_CUDA(x),
		  POLYBENCH_ARRAY_CUDA(y));

  /* Stop and print timer. */
  polybench_stop_instruments;
  polybench_print_instruments;

  /* Prevent dead-code elimination. All live-out data must be printed
     by the function call in argument. */
  polybench_prevent_dce(print_array(n, POLYBENCH_ARRAY_CUDA(y)));

  /* Be clean. 
  POLYBENCH_FREE_ARRAY(A);
  POLYBENCH_FREE_ARRAY(B);
  POLYBENCH_FREE_ARRAY(tmp);
  POLYBENCH_FREE_ARRAY(x);
  POLYBENCH_FREE_ARRAY(y);
  */
  
  return 0;
}
