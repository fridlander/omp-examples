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
#include "doitgen.h"


/* Array initialization. */
static
void init_array(int nr, int nq, int np,
		DATA_TYPE POLYBENCH_3D_CUDA(A,NR,NQ,NP,nr,nq,np),
		DATA_TYPE POLYBENCH_2D_CUDA(C4,NP,NP,np,np))
{
  int i, j, k;

  for (i = 0; i < nr; i++)
    for (j = 0; j < nq; j++)
      for (k = 0; k < np; k++)
	POLYBENCH_3D_REF(A,NR,NQ,NP,i,j,k) = ((DATA_TYPE) i*j + k) / np;
  for (i = 0; i < np; i++)
    for (j = 0; j < np; j++)
      POLYBENCH_2D_REF(C4,NP,NP,i,j) = ((DATA_TYPE) i*j) / np;
}


/* DCE code. Must scan the entire live-out data.
   Can be used also to check the correctness of the output. */
static
void print_array(int nr, int nq, int np,
		 DATA_TYPE POLYBENCH_3D_CUDA(A,NR,NQ,NP,nr,nq,np))
{
  int i, j, k;

  for (i = 0; i < nr; i++)
    for (j = 0; j < nq; j++)
      for (k = 0; k < np; k++) {
	fprintf (stderr, DATA_PRINTF_MODIFIER, POLYBENCH_3D_REF(A,NR,NQ,NP,i,j,k));
	if (i % 20 == 0) fprintf (stderr, "\n");
      }
  fprintf (stderr, "\n");
}


/* Main computational kernel. The whole function will be timed,
   including the call and return. */
static
void kernel_doitgen(int nr, int nq, int np,
		    DATA_TYPE POLYBENCH_3D_CUDA(A,NR,NQ,NP,nr,nq,np),
		    DATA_TYPE POLYBENCH_2D_CUDA(C4,NP,NP,np,np),
		    DATA_TYPE POLYBENCH_3D_CUDA(sum,NR,NQ,NP,nr,nq,np))
{
  //int r, q, p, s;
  //#pragma scop
  
  
  //#pragma omp for private (q, p, s)
  #P1
    for (int r = 0; r < _PB_NR; r++){
      for (int q = 0; q < _PB_NQ; q++) 
	{
          for (int p = 0; p < _PB_NP; p++)
	    {
              POLYBENCH_3D_REF(sum,NR,NQ,NP,r,q,p) = 0;
	      for (int s = 0; s < _PB_NP; s++)
		POLYBENCH_3D_REF(sum,NR,NQ,NP,r,q,p) = POLYBENCH_3D_REF(sum,NR,NQ,NP,r,q,p) + POLYBENCH_3D_REF(A,NR,NQ,NP,r,q,s) * POLYBENCH_2D_REF(C4,NP,NP,s,p);
            }
	  for (int p = 0; p < _PB_NR; p++)
	    POLYBENCH_3D_REF(A,NR,NQ,NP,r,q,p) = POLYBENCH_3D_REF(sum,NR,NQ,NP,r,q,p);
        }
  }
 //#pragma endscop
}

int main(int argc, char** argv)
{
  /* Retrieve problem size. */
  int nr = NR;
  int nq = NQ;
  int np = NP;

  /* Variable declaration/allocation. */
  POLYBENCH_3D_ARRAY_DECL_CUDA(A,DATA_TYPE,NR,NQ,NP,nr,nq,np);
  POLYBENCH_3D_ARRAY_DECL_CUDA(sum,DATA_TYPE,NR,NQ,NP,nr,nq,np);
  POLYBENCH_2D_ARRAY_DECL_CUDA(C4,DATA_TYPE,NP,NP,np,np);

  /* Initialize array(s). */
  init_array (nr, nq, np,
	      POLYBENCH_ARRAY_CUDA(A),
	      POLYBENCH_ARRAY_CUDA(C4));

  /* Start timer. */
  polybench_start_instruments;

  /* Run kernel. */
  kernel_doitgen (nr, nq, np,
		  POLYBENCH_ARRAY_CUDA(A),
		  POLYBENCH_ARRAY_CUDA(C4),
		  POLYBENCH_ARRAY_CUDA(sum));

  /* Stop and print timer. */
  polybench_stop_instruments;
  polybench_print_instruments;

  /* Prevent dead-code elimination. All live-out data must be printed
     by the function call in argument. */
  polybench_prevent_dce(print_array(nr, nq, np,  POLYBENCH_ARRAY_CUDA(A)));

  /* Be clean. 
  POLYBENCH_FREE_ARRAY(A);
  POLYBENCH_FREE_ARRAY(sum);
  POLYBENCH_FREE_ARRAY(C4);
  */
  return 0;
}
