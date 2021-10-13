/* POLYBENCH/GPU-OPENMP
 *
 * This file is a part of the Polybench/GPU-OpenMP suite
 *
 * Contact:
 * William Killian <killian@udel.edu>
 * 
 * Copyright 2013, The University of Delaware
 */
//#define EXTRALARGE_DATASET

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

/* Include polybench common header. */
#include "polybench.h"

/* Include benchmark-specific header. */
/* Default data type is double, default size is 4000. */
#include "correlation.h"


/* Array initialization. */
static
void init_array (int m,
		 int n,
		 DATA_TYPE *float_n,
		 DATA_TYPE POLYBENCH_2D_CUDA(data,M,N,m,n))
{
  int i, j;

  *float_n = 1.2;

  for (i = 0; i < m; i++)
    for (j = 0; j < n; j++)
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
void kernel_correlation(int m, int n,
			DATA_TYPE float_n,
			DATA_TYPE POLYBENCH_2D_CUDA(data,M,N,m,n),
			DATA_TYPE POLYBENCH_2D_CUDA(symmat,M,M,m,m),
			DATA_TYPE POLYBENCH_1D_CUDA(mean,M,m),
			DATA_TYPE POLYBENCH_1D_CUDA(stddev,M,m))
{
  //int i, j, j1, j2;

  DATA_TYPE eps = 0.1f;

#define sqrt_of_array_cell(x,j) sqrt(x[j])

  //#pragma scop
  /* Determine mean of column vectors of input data matrix */
  //#pragma omp parallel
  //{
  //#pragma omp for
  #P1
  for (int j = 0; j < _PB_M; j++)
      {
        POLYBENCH_1D_REF(mean,M,j) = 0.0;
	for (int i = 0; i < _PB_N; i++)
	  POLYBENCH_1D_REF(mean,M,j) += POLYBENCH_2D_REF(data,M,N,i,j);
	POLYBENCH_1D_REF(mean,M,j) /= float_n;
      }
    /* Determine standard deviations of column vectors of data matrix. */
    //#pragma omp for 
  #P1
  for (int j = 0; j < _PB_M; j++)
      {
        POLYBENCH_1D_REF(stddev,M,j) = 0.0;
	for (int i = 0; i < _PB_N; i++)
	  POLYBENCH_1D_REF(stddev,M,j) += (POLYBENCH_2D_REF(data,M,N,i,j) - POLYBENCH_1D_REF(mean,M,j)) * (POLYBENCH_2D_REF(data,M,N,i,j) - POLYBENCH_1D_REF(mean,M,j));
	POLYBENCH_1D_REF(stddev,M,j) /= float_n;
	POLYBENCH_1D_REF(stddev,M,j) = sqrt_of_array_cell(stddev, j);
	/* The following in an inelegant but usual way to handle
	   near-zero std. dev. values, which below would cause a zero-
	   divide. */
	POLYBENCH_1D_REF(stddev,M,j) = POLYBENCH_1D_REF(stddev,M,j) <= eps ? 1.0 : POLYBENCH_1D_REF(stddev,M,j);
      }
    
    /* Center and reduce the column vectors. */
    //#pragma omp for
  #P1
  for (int i = 0; i < _PB_N; i++)
      for (int j = 0; j < _PB_M; j++)
	{
          POLYBENCH_2D_REF(data,M,N,i,j) -= POLYBENCH_1D_REF(mean,M,j);
          POLYBENCH_2D_REF(data,M,N,i,j) /= sqrt(float_n) * POLYBENCH_1D_REF(stddev,M,j);
	}
    
    /* Calculate the m * m correlation matrix. */
  //  #pragma omp for
  #P1 
  for (int j1 = 0; j1 < _PB_M-1; j1++)
      {
        POLYBENCH_2D_REF(symmat,M,M,j1,j1) = 1.0;
	for (int j2 = j1+1; j2 < _PB_M; j2++)
	  {
            POLYBENCH_2D_REF(symmat,M,M,j1,j2) = 0.0;
	    for (int i = 0; i < _PB_N; i++)
	      POLYBENCH_2D_REF(symmat,M,M,j1,j2) += (POLYBENCH_2D_REF(data,M,N,i,j1) * POLYBENCH_2D_REF(data,M,N,i,j2));
	    POLYBENCH_2D_REF(symmat,M,M,j2,j1) = POLYBENCH_2D_REF(symmat,M,M,j1,j2);
          }
      }
  //#pragma endscop
  POLYBENCH_2D_REF(symmat,M,M,_PB_M-1,_PB_M-1)= 1.0;
    //symmat[_PB_M-1][_PB_M-1] = 1.0;
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
  POLYBENCH_1D_ARRAY_DECL_CUDA(stddev,DATA_TYPE,M,m);

  /* Initialize array(s). */
  init_array (m, n, &float_n, POLYBENCH_ARRAY_CUDA(data));
  
  /* Start timer. */
  polybench_start_instruments;
  /* Run kernel. */
  kernel_correlation (m, n, float_n,
		      POLYBENCH_ARRAY_CUDA(data),
		      POLYBENCH_ARRAY_CUDA(symmat),
		      POLYBENCH_ARRAY_CUDA(mean),
		      POLYBENCH_ARRAY_CUDA(stddev));
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
  POLYBENCH_FREE_ARRAY(stddev);
  */
  return 0;
}
