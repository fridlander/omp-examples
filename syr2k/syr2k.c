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
#include "polybench.h"

/* Include benchmark-specific header. */
/* Default data type is double, default size is 4000. */
#include "syr2k.h"


/* Array initialization. */
static
void init_array(int ni, int nj,
		DATA_TYPE *alpha,
		DATA_TYPE *beta,
		DATA_TYPE POLYBENCH_2D_CUDA(C,NI,NI,ni,ni),
		DATA_TYPE POLYBENCH_2D_CUDA(A,NI,NJ,ni,nj),
		DATA_TYPE POLYBENCH_2D_CUDA(B,NI,NJ,ni,nj))
{
  int i, j;

  *alpha = 32412;
  *beta = 2123;
  for (i = 0; i < ni; i++)
    for (j = 0; j < nj; j++) {
      POLYBENCH_2D_REF(A,NI,NJ,i,j) = ((DATA_TYPE) i*j) / ni;
      POLYBENCH_2D_REF(B,NI,NJ,i,j) = ((DATA_TYPE) i*j) / ni;
    }
  for (i = 0; i < ni; i++)
    for (j = 0; j < ni; j++)
      POLYBENCH_2D_REF(C,NI,NI,i,j) = ((DATA_TYPE) i*j) / ni;
}


/* DCE code. Must scan the entire live-out data.
   Can be used also to check the correctness of the output. */
static
void print_array(int ni,
		 DATA_TYPE POLYBENCH_2D_CUDA(C,NI,NI,ni,ni))
{
  int i, j;

  for (i = 0; i < ni; i++)
    for (j = 0; j < ni; j++) {
      fprintf (stderr, DATA_PRINTF_MODIFIER, POLYBENCH_2D_REF(C,NI,NI,i,j));
      if ((i * ni + j) % 20 == 0) fprintf (stderr, "\n");
    }
  fprintf (stderr, "\n");
}


/* Main computational kernel. The whole function will be timed,
   including the call and return. */
static
void kernel_syr2k(int ni, int nj,
		  DATA_TYPE alpha,
		  DATA_TYPE beta,
		  DATA_TYPE POLYBENCH_2D_CUDA(C,NI,NI,ni,ni),
		  DATA_TYPE POLYBENCH_2D_CUDA(A,NI,NJ,ni,nj),
		  DATA_TYPE POLYBENCH_2D_CUDA(B,NI,NJ,ni,nj))
{
  //int i, j, k;
  //  #pragma scop
  //  #pragma omp parallel
  //{
    /*    C := alpha*A*B' + alpha*B*A' + beta*C */
#P1
    for (int i = 0; i < _PB_NI; i++)
      for (int j = 0; j < _PB_NI; j++)
        POLYBENCH_2D_REF(C,NI,NI,i,j) *= beta;
#P1
    for (int i = 0; i < _PB_NI; i++)
      for (int j = 0; j < _PB_NI; j++)
        for (int k = 0; k < _PB_NJ; k++)
	  {
	    POLYBENCH_2D_REF(C,NI,NI,i,j) += alpha * POLYBENCH_2D_REF(A,NI,NJ,i,k) * POLYBENCH_2D_REF(B,NI,NJ,j,k);
	    POLYBENCH_2D_REF(C,NI,NI,i,j) += alpha * POLYBENCH_2D_REF(B,NI,NJ,i,k) * POLYBENCH_2D_REF(A,NI,NJ,j,k);
	  }
    //}
  //  #pragma endscop
}


int main(int argc, char** argv)
{
  /* Retrieve problem size. */
  int ni = NI;
  int nj = NJ;

  /* Variable declaration/allocation. */
  DATA_TYPE alpha;
  DATA_TYPE beta;
  POLYBENCH_2D_ARRAY_DECL_CUDA(C,DATA_TYPE,NI,NI,ni,ni);
  POLYBENCH_2D_ARRAY_DECL_CUDA(A,DATA_TYPE,NI,NJ,ni,nj);
  POLYBENCH_2D_ARRAY_DECL_CUDA(B,DATA_TYPE,NI,NJ,ni,nj);

  /* Initialize array(s). */
  init_array (ni, nj, &alpha, &beta,
	      POLYBENCH_ARRAY_CUDA(C),
	      POLYBENCH_ARRAY_CUDA(A),
	      POLYBENCH_ARRAY_CUDA(B));

  /* Start timer. */
  polybench_start_instruments;

  /* Run kernel. */
  kernel_syr2k (ni, nj,
		alpha, beta,
		POLYBENCH_ARRAY_CUDA(C),
		POLYBENCH_ARRAY_CUDA(A),
		POLYBENCH_ARRAY_CUDA(B));

  /* Stop and print timer. */
  polybench_stop_instruments;
  polybench_print_instruments;

  /* Prevent dead-code elimination. All live-out data must be printed
     by the function call in argument. */
  polybench_prevent_dce(print_array(ni, POLYBENCH_ARRAY_CUDA(C)));

  /* Be clean. */
  //POLYBENCH_FREE_ARRAY(C);
  //POLYBENCH_FREE_ARRAY(A);
  //POLYBENCH_FREE_ARRAY(B);

  return 0;
}
