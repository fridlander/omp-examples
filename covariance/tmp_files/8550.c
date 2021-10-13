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
#include "covariance.h"


/* Array initialization. */
static
void init_array (int m, int n,
		 DATA_TYPE *float_n,
		 DATA_TYPE POLYBENCH_2D_CUDA(data,M,N,m,n))
{
  int i, j;

  *float_n = 1.2;

  for (i = 0; i < M; i++)
    for (j = 0; j < N; j++)
      POLYBENCH_2D_REF(data,M,N,i,j) = ((DATA_TYPE) i*j) / M;
}


/* DCE code. Must scan the entire live-out data.
   Can be used also to check the correctness of the output. */
static
void print_array(int m,
		 DATA_TYPE POLYBENCH_2D_CUDA(symmat,M,M,m,m))

{
  int i, j;

  for (i = 0; i < m; i++)
    for (j = 0; j < m; j++) {
      fprintf (stderr, DATA_PRINTF_MODIFIER, POLYBENCH_2D_REF(symmat,M,M,i,j));
      if ((i * m + j) % 20 == 0) fprintf (stderr, "\n");
    }
  fprintf (stderr, "\n");
}


/* Main computational kernel. The whole function will be timed,
   including the call and return. */
static
void kernel_covariance(int m, int n,
		       DATA_TYPE float_n,
		       DATA_TYPE POLYBENCH_2D_CUDA(data,M,N,m,n),
		       DATA_TYPE POLYBENCH_2D_CUDA(symmat,M,M,m,m),
		       DATA_TYPE POLYBENCH_1D_CUDA(mean,M,m))
{
  //int i, j, j1, j2;
  
  //#pragma scop
  /* Determine mean of column vectors of input data matrix */
  //#pragma omp parallel
  //{
    //#pragma omp for private (i)
    #pragma omp target teams distribute parallel for   schedule(dynamic) num_threads(32)     is_device_ptr(data, symmat, mean)
    for (int j = 0; j < _PB_M; j++)
      {
        POLYBENCH_1D_REF(mean,M,j) = 0.0;
	for (int i = 0; i < _PB_N; i++)
	  POLYBENCH_1D_REF(mean,M,j) += POLYBENCH_2D_REF(data,M,N,i,j);
	POLYBENCH_1D_REF(mean,M,j) /= float_n;
      }
      
    /* Center the column vectors. */
    //#pragma omp for private (j)
    #pragma omp target teams distribute parallel for   schedule(dynamic) num_threads(32)     is_device_ptr(data, symmat, mean)
    for (int i = 0; i < _PB_N; i++)
      for (int j = 0; j < _PB_M; j++)
	POLYBENCH_2D_REF(data,M,N,i,j) -= POLYBENCH_1D_REF(mean,M,j);
      
    /* Calculate the m * m covariance matrix. */
    //#pragma omp for private (j2, i)
    #pragma omp target teams distribute parallel for   schedule(dynamic) num_threads(32)     is_device_ptr(data, symmat, mean)
    for (int j1 = 0; j1 < _PB_M; j1++)
      for (int j2 = j1; j2 < _PB_M; j2++)
	{
          POLYBENCH_2D_REF(symmat,M,M,j1,j2) = 0.0;
	  for (int i = 0; i < _PB_N; i++)
	    POLYBENCH_2D_REF(symmat,M,M,j1,j2) += POLYBENCH_2D_REF(data,M,N,i,j1) * POLYBENCH_2D_REF(data,M,N,i,j2);
	  POLYBENCH_2D_REF(symmat,M,M,j2,j1) = POLYBENCH_2D_REF(symmat,M,M,j1,j2);
        }
    //}
  //#pragma endscop
}

int main(int argc, char** argv)
{
  /* Retrieve problem size. */
  int n = N;
  int m = M;

  /* Variable declaration/allocation. */
  DATA_TYPE float_n;
  POLYBENCH_2D_ARRAY_DECL_CUDA(data,DATA_TYPE,M,N,m,n);
  POLYBENCH_2D_ARRAY_DECL_CUDA(symmat,DATA_TYPE,M,M,m,m);
  POLYBENCH_1D_ARRAY_DECL_CUDA(mean,DATA_TYPE,M,m);
  
  /* Initialize array(s). */
  init_array (m, n, &float_n, POLYBENCH_ARRAY_CUDA(data));
  
  /* Start timer. */
  polybench_start_instruments;

  /* Run kernel. */
  kernel_covariance (m, n, float_n,
		     POLYBENCH_ARRAY_CUDA(data),
		     POLYBENCH_ARRAY_CUDA(symmat),
		     POLYBENCH_ARRAY_CUDA(mean));

  /* Stop and print timer. */
  polybench_stop_instruments;
  polybench_print_instruments;

  /* Prevent dead-code elimination. All live-out data must be printed
     by the function call in argument. */
  polybench_prevent_dce(print_array(m, POLYBENCH_ARRAY_CUDA(symmat)));

  /* Be clean.
  POLYBENCH_FREE_ARRAY(data);
  POLYBENCH_FREE_ARRAY(symmat);
  POLYBENCH_FREE_ARRAY(mean);
  */
  return 0;
}
