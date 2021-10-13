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
#include "gemm.h"


/* Array initialization. */
static
void init_array(int ni, int nj, int nk,
		DATA_TYPE *alpha,
		DATA_TYPE *beta,
		DATA_TYPE POLYBENCH_2D_CUDA(C,NI,NJ,ni,nj),
		DATA_TYPE POLYBENCH_2D_CUDA(A,NI,NK,ni,nk),
		DATA_TYPE POLYBENCH_2D_CUDA(B,NK,NJ,nk,nj))
{
  int i, j;

  *alpha = 32412;
  *beta = 2123;
  for (i = 0; i < ni; i++)
    for (j = 0; j < nj; j++)
      POLYBENCH_2D_REF(C,NI,NJ,i,j) = ((DATA_TYPE) i*j) / ni;
  for (i = 0; i < ni; i++)
    for (j = 0; j < nk; j++)
      POLYBENCH_2D_REF(A,NI,NK,i,j) = ((DATA_TYPE) i*j) / ni;
  for (i = 0; i < nk; i++)
    for (j = 0; j < nj; j++)
      POLYBENCH_2D_REF(B,NK,NJ,i,j) = ((DATA_TYPE) i*j) / ni;
}


/* DCE code. Must scan the entire live-out data.
   Can be used also to check the correctness of the output. */
static
void print_array(int ni, int nj,
		 DATA_TYPE POLYBENCH_2D_CUDA(C,NI,NJ,ni,nj))
{
  int i, j;
  FILE * fp;
  fp = fopen ("gemmP.txt", "w+");
  for (i = 0; i < ni; i++)
    for (j = 0; j < nj; j++) {
      fprintf (fp, DATA_PRINTF_MODIFIER, POLYBENCH_2D_REF(C,NI,NJ,i,j));
	if ((i * ni + j) % 20 == 0) fprintf (fp, "\n");
    }
  fprintf (fp, "\n");
  fclose(fp);
}


/* Main computational kernel. The whole function will be timed,
   including the call and return. */
static
void kernel_gemm(int ni, int nj, int nk,
		 DATA_TYPE alpha,
		 DATA_TYPE beta,
		 DATA_TYPE POLYBENCH_2D_CUDA(C,NI,NJ,ni,nj),
		 DATA_TYPE POLYBENCH_2D_CUDA(A,NI,NK,ni,nk),
		 DATA_TYPE POLYBENCH_2D_CUDA(B,NK,NJ,nk,nj))
{
  //int i, j, k;
  {
    /* C := alpha*A*B + beta*C */
    #P1
    for (int i = 0; i < _PB_NI; i++)
      for (int j = 0; j < _PB_NJ; j++)
	{
	  POLYBENCH_2D_REF(C,NI,NJ,i,j) *= beta;
	  for (int k = 0; k < _PB_NK; ++k)
	    POLYBENCH_2D_REF(C,NI,NJ,i,j) += alpha * POLYBENCH_2D_REF(A,NI,NK,i,k) * POLYBENCH_2D_REF(B,NK,NJ,k,j);
	}
  }
}


int main(int argc, char** argv)
{
  /* Retrieve problem size. */
  int ni = NI;
  int nj = NJ;
  int nk = NK;

  /* Variable declaration/allocation. */
  DATA_TYPE alpha;
  DATA_TYPE beta;
  POLYBENCH_2D_ARRAY_DECL_CUDA(C,DATA_TYPE,NI,NJ,ni,nj);
  POLYBENCH_2D_ARRAY_DECL_CUDA(A,DATA_TYPE,NI,NK,ni,nk);
  POLYBENCH_2D_ARRAY_DECL_CUDA(B,DATA_TYPE,NK,NJ,nk,nj);

  /* Initialize array(s). */
  init_array (ni, nj, nk, &alpha, &beta,
	      POLYBENCH_ARRAY_CUDA(C),
	      POLYBENCH_ARRAY_CUDA(A),
	      POLYBENCH_ARRAY_CUDA(B));

  /* Start timer. */
  polybench_start_instruments;

  /* Run kernel. */
  kernel_gemm (ni, nj, nk,
	       alpha, beta,
	       POLYBENCH_ARRAY_CUDA(C),
	       POLYBENCH_ARRAY_CUDA(A),
	       POLYBENCH_ARRAY_CUDA(B));

  /* Stop and print timer. */
  polybench_stop_instruments;
  polybench_print_instruments;

  
  print_array(ni, nj,  POLYBENCH_ARRAY_CUDA(C));
  /* Prevent dead-code elimination. All live-out data must be printed
     by the function call in argument. */
  polybench_prevent_dce(print_array(ni, nj,  POLYBENCH_ARRAY_CUDA(C)));

  /* Be clean. */
  //POLYBENCH_FREE_ARRAY(C);
  //POLYBENCH_FREE_ARRAY(A);
  //POLYBENCH_FREE_ARRAY(B);

  return 0;
}
