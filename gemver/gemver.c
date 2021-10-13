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
#include "gemver.h"


/* Array initialization. */
static
void init_array (int n,
		 DATA_TYPE *alpha,
		 DATA_TYPE *beta,
		 DATA_TYPE POLYBENCH_2D_CUDA(A,N,N,n,n),
		 DATA_TYPE POLYBENCH_1D_CUDA(u1,N,n),
		 DATA_TYPE POLYBENCH_1D_CUDA(v1,N,n),
		 DATA_TYPE POLYBENCH_1D_CUDA(u2,N,n),
		 DATA_TYPE POLYBENCH_1D_CUDA(v2,N,n),
		 DATA_TYPE POLYBENCH_1D_CUDA(w,N,n),
		 DATA_TYPE POLYBENCH_1D_CUDA(x,N,n),
		 DATA_TYPE POLYBENCH_1D_CUDA(y,N,n),
		 DATA_TYPE POLYBENCH_1D_CUDA(z,N,n))
{
  int i, j;

  *alpha = 43532;
  *beta = 12313;

  for (i = 0; i < n; i++)
    {
      POLYBENCH_1D_REF(u1,N,i) = i;
      POLYBENCH_1D_REF(u2,N,i) = (i+1)/n/2.0;
      POLYBENCH_1D_REF(v1,N,i) = (i+1)/n/4.0;
      POLYBENCH_1D_REF(v2,N,i) = (i+1)/n/6.0;
      POLYBENCH_1D_REF(y,N,i) = (i+1)/n/8.0;
      POLYBENCH_1D_REF(z,N,i) = (i+1)/n/9.0;
      POLYBENCH_1D_REF(x,N,i) = 0.0;
      POLYBENCH_1D_REF(w,N,i) = 0.0;
      for (j = 0; j < n; j++)
	POLYBENCH_2D_REF(A,N,N,i,j) = ((DATA_TYPE) i*j) / n;
    }
}


/* DCE code. Must scan the entire live-out data.
   Can be used also to check the correctness of the output. */
static
void print_array(int n,
		 DATA_TYPE POLYBENCH_1D_CUDA(w,N,n))
{
  int i;

  for (i = 0; i < n; i++) {
    fprintf (stderr, DATA_PRINTF_MODIFIER, POLYBENCH_1D_REF(w,N,i));
    if (i % 20 == 0) fprintf (stderr, "\n");
  }
}


/* Main computational kernel. The whole function will be timed,
   including the call and return. */
static
void kernel_gemver(int n,
		   DATA_TYPE alpha,
		   DATA_TYPE beta,
		   DATA_TYPE POLYBENCH_2D_CUDA(A,N,N,n,n),
		   DATA_TYPE POLYBENCH_1D_CUDA(u1,N,n),
		   DATA_TYPE POLYBENCH_1D_CUDA(v1,N,n),
		   DATA_TYPE POLYBENCH_1D_CUDA(u2,N,n),
		   DATA_TYPE POLYBENCH_1D_CUDA(v2,N,n),
		   DATA_TYPE POLYBENCH_1D_CUDA(w,N,n),
		   DATA_TYPE POLYBENCH_1D_CUDA(x,N,n),
		   DATA_TYPE POLYBENCH_1D_CUDA(y,N,n),
		   DATA_TYPE POLYBENCH_1D_CUDA(z,N,n))
{
  //int i, j;
  //#pragma scop
  // #pragma omp parallel
  //{
  //#pragma omp for private (j)
  #P1
  for (int i = 0; i < _PB_N; i++){
      for (int j = 0; j < _PB_N; j++)
	POLYBENCH_2D_REF(A,N,N,i,j) = POLYBENCH_2D_REF(A,N,N,i,j) + POLYBENCH_1D_REF(u1,N,i) * POLYBENCH_1D_REF(v1,N,j) + POLYBENCH_1D_REF(u2,N,i) * POLYBENCH_1D_REF(v2,N,j);
  }

  #P1
  for (int i = 0; i < _PB_N; i++){
      for (int j = 0; j < _PB_N; j++)
	POLYBENCH_1D_REF(x,N,i) = POLYBENCH_1D_REF(x,N,i) + beta * POLYBENCH_2D_REF(A,N,N,j,i) * POLYBENCH_1D_REF(y,N,j);
  }

  #P1
  for (int i = 0; i < _PB_N; i++)
      POLYBENCH_1D_REF(x,N,i) = POLYBENCH_1D_REF(x,N,i) + POLYBENCH_1D_REF(z,N,i);

  #P1
  for (int i = 0; i < _PB_N; i++){
       for (int j = 0; j < _PB_N; j++)
         POLYBENCH_1D_REF(w,N,i) = POLYBENCH_1D_REF(w,N,i) +  alpha * POLYBENCH_2D_REF(A,N,N,i,j) * POLYBENCH_1D_REF(x,N,j);
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
  POLYBENCH_1D_ARRAY_DECL_CUDA(u1, DATA_TYPE, N, n);
  POLYBENCH_1D_ARRAY_DECL_CUDA(v1, DATA_TYPE, N, n);
  POLYBENCH_1D_ARRAY_DECL_CUDA(u2, DATA_TYPE, N, n);
  POLYBENCH_1D_ARRAY_DECL_CUDA(v2, DATA_TYPE, N, n);
  POLYBENCH_1D_ARRAY_DECL_CUDA(w, DATA_TYPE, N, n);
  POLYBENCH_1D_ARRAY_DECL_CUDA(x, DATA_TYPE, N, n);
  POLYBENCH_1D_ARRAY_DECL_CUDA(y, DATA_TYPE, N, n);
  POLYBENCH_1D_ARRAY_DECL_CUDA(z, DATA_TYPE, N, n);


  /* Initialize array(s). */
  init_array (n, &alpha, &beta,
	      POLYBENCH_ARRAY_CUDA(A),
	      POLYBENCH_ARRAY_CUDA(u1),
	      POLYBENCH_ARRAY_CUDA(v1),
	      POLYBENCH_ARRAY_CUDA(u2),
	      POLYBENCH_ARRAY_CUDA(v2),
	      POLYBENCH_ARRAY_CUDA(w),
	      POLYBENCH_ARRAY_CUDA(x),
	      POLYBENCH_ARRAY_CUDA(y),
	      POLYBENCH_ARRAY_CUDA(z));

  /* Start timer. */
  polybench_start_instruments;

  /* Run kernel. */
  kernel_gemver (n, alpha, beta,
		 POLYBENCH_ARRAY_CUDA(A),
		 POLYBENCH_ARRAY_CUDA(u1),
		 POLYBENCH_ARRAY_CUDA(v1),
		 POLYBENCH_ARRAY_CUDA(u2),
		 POLYBENCH_ARRAY_CUDA(v2),
		 POLYBENCH_ARRAY_CUDA(w),
		 POLYBENCH_ARRAY_CUDA(x),
		 POLYBENCH_ARRAY_CUDA(y),
		 POLYBENCH_ARRAY_CUDA(z));

  /* Stop and print timer. */
  polybench_stop_instruments;
  polybench_print_instruments;

  /* Prevent dead-code elimination. All live-out data must be printed
     by the function call in argument. */
  polybench_prevent_dce(print_array(n, POLYBENCH_ARRAY_CUDA(w)));

  /* Be clean. 
  POLYBENCH_FREE_ARRAY(A);
  POLYBENCH_FREE_ARRAY(u1);
  POLYBENCH_FREE_ARRAY(v1);
  POLYBENCH_FREE_ARRAY(u2);
  POLYBENCH_FREE_ARRAY(v2);
  POLYBENCH_FREE_ARRAY(w);
  POLYBENCH_FREE_ARRAY(x);
  POLYBENCH_FREE_ARRAY(y);
  POLYBENCH_FREE_ARRAY(z);
  */
  return 0;
}
