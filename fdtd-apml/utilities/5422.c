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
#include "3mm.h"


/* Array initialization. */
static
void init_array(int ni, int nj, int nk, int nl, int nm,
		DATA_TYPE POLYBENCH_2D_CUDA(A,NI,NK,ni,nk),
		DATA_TYPE POLYBENCH_2D_CUDA(B,NK,NJ,nk,nj),
		DATA_TYPE POLYBENCH_2D_CUDA(C,NJ,NM,nj,nm),
		DATA_TYPE POLYBENCH_2D_CUDA(D,NM,NL,nm,nl))
{
  int i, j;

  for (i = 0; i < ni; i++)
    for (j = 0; j < nk; j++)
      POLYBENCH_2D_REF(A,NI,NK,i,j) = ((DATA_TYPE) i*j) / ni;
  for (i = 0; i < nk; i++)
    for (j = 0; j < nj; j++)
      POLYBENCH_2D_REF(B,NK,NJ,i,j) = ((DATA_TYPE) i*(j+1)) / nj;
  for (i = 0; i < nj; i++)
    for (j = 0; j < nm; j++)
      POLYBENCH_2D_REF(C,NJ,NM,i,j) = ((DATA_TYPE) i*(j+3)) / nl;
  for (i = 0; i < nm; i++)
    for (j = 0; j < nl; j++)
      POLYBENCH_2D_REF(D,NM,NL,i,j) = ((DATA_TYPE) i*(j+2)) / nk;
}


/* DCE code. Must scan the entire live-out data.
   Can be used also to check the correctness of the output. */
static
void print_array(int ni, int nl,
		 DATA_TYPE POLYBENCH_2D_CUDA(G,NI,NL,ni,nl))
{
  int i, j;

  for (i = 0; i < ni; i++)
    for (j = 0; j < nl; j++) {
	fprintf (stderr, DATA_PRINTF_MODIFIER, POLYBENCH_2D_REF(G,NI,NL,i,j));
	if ((i * ni + j) % 20 == 0) fprintf (stderr, "\n");
    }
  fprintf (stderr, "\n");
}


/* Main computational kernel. The whole function will be timed,
   including the call and return. */
static
void kernel_3mm(int ni, int nj, int nk, int nl, int nm,
		DATA_TYPE POLYBENCH_2D_CUDA(E,NI,NJ,ni,nj),
		DATA_TYPE POLYBENCH_2D_CUDA(A,NI,NK,ni,nk),
		DATA_TYPE POLYBENCH_2D_CUDA(B,NK,NJ,nk,nj),
		DATA_TYPE POLYBENCH_2D_CUDA(F,NJ,NL,nj,nl),
		DATA_TYPE POLYBENCH_2D_CUDA(C,NJ,NM,nj,nm),
		DATA_TYPE POLYBENCH_2D_CUDA(D,NM,NL,nm,nl),
		DATA_TYPE POLYBENCH_2D_CUDA(G,NI,NL,ni,nl))
{
  //int i, j, k;
  //#pragma scop
  //#pragma omp parallel private (j, k)
  {
    /* E := A*B */
    //#pragma omp for
    #pragma omp target teams distribute parallel for   collapse(2)   num_threads(72)   thread_limit(256) is_device_ptr(E, A, B, F, C, D, G)
    for (int i = 0; i < _PB_NI; i++)
      for (int j = 0; j < _PB_NJ; j++)
	{
          POLYBENCH_2D_REF(E,NI,NJ,i,j) = 0;
	  for (int k = 0; k < _PB_NK; ++k)
	    POLYBENCH_2D_REF(E,NI,NJ,i,j) += POLYBENCH_2D_REF(A,NI,NK,i,k) * POLYBENCH_2D_REF(B,NK,NJ,k,j);
        }
    /* F := C*D */
    //#pragma omp for
    #pragma omp target teams distribute parallel for   collapse(2)   num_threads(72)   thread_limit(256) is_device_ptr(E, A, B, F, C, D, G)
    for (int i = 0; i < _PB_NJ; i++)
      for (int j = 0; j < _PB_NL; j++)
	{
	  POLYBENCH_2D_REF(F,NJ,NL,i,j) = 0;
	  for (int k = 0; k < _PB_NM; ++k)
	    POLYBENCH_2D_REF(F,NJ,NL,i,j) += POLYBENCH_2D_REF(C,NJ,NM,i,k) * POLYBENCH_2D_REF(D,NM,NL,k,j);
        }
    /* G := E*F */
    //#pragma omp for
    #pragma omp target teams distribute parallel for   collapse(2)   num_threads(72)   thread_limit(256) is_device_ptr(E, A, B, F, C, D, G)
    for (int i = 0; i < _PB_NI; i++)
      for (int j = 0; j < _PB_NL; j++)
	{
	  POLYBENCH_2D_REF(G,NI,NL,i,j) = 0;
	  for (int k = 0; k < _PB_NJ; ++k)
	    POLYBENCH_2D_REF(G,NI,NL,i,j) += POLYBENCH_2D_REF(E,NI,NJ,i,k) * POLYBENCH_2D_REF(F,NJ,NL,k,j);
	}
  }
}

int main(int argc, char** argv)
{
  /* Retrieve problem size. */
  int ni = NI;
  int nj = NJ;
  int nk = NK;
  int nl = NL;
  int nm = NM;

  /* Variable declaration/allocation. */
  POLYBENCH_2D_ARRAY_DECL_CUDA(E, DATA_TYPE, NI, NJ, ni, nj);
  POLYBENCH_2D_ARRAY_DECL_CUDA(A, DATA_TYPE, NI, NK, ni, nk);
  POLYBENCH_2D_ARRAY_DECL_CUDA(B, DATA_TYPE, NK, NJ, nk, nj);
  POLYBENCH_2D_ARRAY_DECL_CUDA(F, DATA_TYPE, NJ, NL, nj, nl);
  POLYBENCH_2D_ARRAY_DECL_CUDA(C, DATA_TYPE, NJ, NM, nj, nm);
  POLYBENCH_2D_ARRAY_DECL_CUDA(D, DATA_TYPE, NM, NL, nm, nl);
  POLYBENCH_2D_ARRAY_DECL_CUDA(G, DATA_TYPE, NI, NL, ni, nl);

  /* Initialize array(s). */
  init_array (ni, nj, nk, nl, nm,
	      POLYBENCH_ARRAY_CUDA(A),
	      POLYBENCH_ARRAY_CUDA(B),
	      POLYBENCH_ARRAY_CUDA(C),
	      POLYBENCH_ARRAY_CUDA(D));


  /* Start timer. */
  polybench_start_instruments;

  /* Run kernel. */
  kernel_3mm (ni, nj, nk, nl, nm,
	      POLYBENCH_ARRAY_CUDA(E),
	      POLYBENCH_ARRAY_CUDA(A),
	      POLYBENCH_ARRAY_CUDA(B),
	      POLYBENCH_ARRAY_CUDA(F),
	      POLYBENCH_ARRAY_CUDA(C),
	      POLYBENCH_ARRAY_CUDA(D),
	      POLYBENCH_ARRAY_CUDA(G));

  /* Stop and print timer. */
  polybench_stop_instruments;
  polybench_print_instruments;
  
  /* Prevent dead-code elimination. All live-out data must be printed
     by the function call in argument. */
  polybench_prevent_dce(print_array(ni, nl,  POLYBENCH_ARRAY_CUDA(G)));

  /* Be clean. 
  POLYBENCH_FREE_ARRAY(E);
  POLYBENCH_FREE_ARRAY(A);
  POLYBENCH_FREE_ARRAY(B);
  POLYBENCH_FREE_ARRAY(F);
  POLYBENCH_FREE_ARRAY(C);
  POLYBENCH_FREE_ARRAY(D);
  POLYBENCH_FREE_ARRAY(G);
  */
  return 0;
}
