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
/* Default data type is double, default size is 256x256x256. */
#include "fdtd-apml.h"


/* Array initialization. */
static
void init_array (int cz,
		 int cxm,
		 int cym,
		 DATA_TYPE *mui,
		 DATA_TYPE *ch,
		 DATA_TYPE POLYBENCH_2D(Ax,CZ+1,CYM+1,cz+1,cym+1),
		 DATA_TYPE POLYBENCH_2D(Ry,CZ+1,CYM+1,cz+1,cym+1),
		 DATA_TYPE POLYBENCH_3D(Ex,CZ+1,CYM+1,CXM+1,cz+1,cym+1,cxm+1),
		 DATA_TYPE POLYBENCH_3D(Ey,CZ+1,CYM+1,CXM+1,cz+1,cym+1,cxm+1),
		 DATA_TYPE POLYBENCH_3D(Hz,CZ+1,CYM+1,CXM+1,cz+1,cym+1,cxm+1),
		 DATA_TYPE POLYBENCH_1D(czm,CZ+1,cz+1),
		 DATA_TYPE POLYBENCH_1D(czp,CZ+1,cz+1),
		 DATA_TYPE POLYBENCH_1D(cxmh,CXM+1,cxm+1),
		 DATA_TYPE POLYBENCH_1D(cxph,CXM+1,cxm+1),
		 DATA_TYPE POLYBENCH_1D(cymh,CYM+1,cym+1),
		 DATA_TYPE POLYBENCH_1D(cyph,CYM+1,cym+1))
{
  int i, j, k;
  *mui = 2341;
  *ch = 42;
  for (i = 0; i <= cz; i++)
    {
      POLYBENCH_1D_REF(czm,CZ+1,i) = ((DATA_TYPE) i + 1) / cxm;
      POLYBENCH_1D_REF(czp,CZ+1,i) = ((DATA_TYPE) i + 2) / cxm;
    }
  for (i = 0; i <= cxm; i++)
    {
      POLYBENCH_1D_REF(cxmh,CXM+1,i) = ((DATA_TYPE) i + 3) / cxm;
      POLYBENCH_1D_REF(cxph,CXM+1,i) = ((DATA_TYPE) i + 4) / cxm;
    }
  for (i = 0; i <= cym; i++)
    {
      POLYBENCH_1D_REF(cymh,CYM+1,i) = ((DATA_TYPE) i + 5) / cxm;
      POLYBENCH_1D_REF(cyph,CYM+1,i) = ((DATA_TYPE) i + 6) / cxm;
    }

  for (i = 0; i <= cz; i++)
    for (j = 0; j <= cym; j++)
      {
	POLYBENCH_2D_REF(Ry,CZ+1,CYM+1,i,j) = ((DATA_TYPE) i*(j+1) + 10) / cym;
	POLYBENCH_2D_REF(Ax,CZ+1,CYM+1,i,j) = ((DATA_TYPE) i*(j+2) + 11) / cym;
	for (k = 0; k <= cxm; k++)
	  { 
	    POLYBENCH_3D_REF(Ex,CZ+1,CYM+1,CXM+1,i,j,k) = ((DATA_TYPE) i*(j+3) + k + 1) / cxm;
	    POLYBENCH_3D_REF(Ey,CZ+1,CYM+1,CXM+1,i,j,k) = ((DATA_TYPE) i*(j+4) + k + 2) / cym;
	    POLYBENCH_3D_REF(Hz,CZ+1,CYM+1,CXM+1,i,j,k) = ((DATA_TYPE) i*(j+5) + k + 3) / cz;
	  }
      }
}


/* DCE code. Must scan the entire live-out data.
   Can be used also to check the correctness of the output. */
static
void print_array(int cz,
		 int cxm,
		 int cym,
		 DATA_TYPE POLYBENCH_3D(Bza,CZ+1,CYM+1,CXM+1,cz+1,cym+1,cxm+1),
		 DATA_TYPE POLYBENCH_3D(Ex,CZ+1,CYM+1,CXM+1,cz+1,cym+1,cxm+1),
		 DATA_TYPE POLYBENCH_3D(Ey,CZ+1,CYM+1,CXM+1,cz+1,cym+1,cxm+1),
		 DATA_TYPE POLYBENCH_3D(Hz,CZ+1,CYM+1,CXM+1,cz+1,cym+1,cxm+1))
{
  int i, j, k;
  
  for (i = 0; i <= cz; i++)
    for (j = 0; j <= cym; j++)
      for (k = 0; k <= cxm; k++) {
	fprintf(stderr, DATA_PRINTF_MODIFIER, POLYBENCH_3D_REF(Bza,CZ+1,CYM+1,CXM+1,i,j,k));
	fprintf(stderr, DATA_PRINTF_MODIFIER, POLYBENCH_3D_REF(Ex,CZ+1,CYM+1,CXM+1,i,j,k));
	fprintf(stderr, DATA_PRINTF_MODIFIER, POLYBENCH_3D_REF(Ey,CZ+1,CYM+1,CXM+1,i,j,k));
	fprintf(stderr, DATA_PRINTF_MODIFIER, POLYBENCH_3D_REF(Hz,CZ+1,CYM+1,CXM+1,i,j,k));
	if ((i * cxm + j) % 20 == 0) fprintf(stderr, "\n");
      }
  fprintf(stderr, "\n");
}


/* Main computational kernel. The whole function will be timed,
   including the call and return. */
static
void kernel_fdtd_apml(int cz,
		      int cxm,
		      int cym,
		      DATA_TYPE mui,
		      DATA_TYPE ch,
		      DATA_TYPE POLYBENCH_2D(Ax,CZ+1,CYM+1,cz+1,cym+1),
		      DATA_TYPE POLYBENCH_2D(Ry,CZ+1,CYM+1,cz+1,cym+1),
		      DATA_TYPE POLYBENCH_2D(clf,CYM+1,CXM+1,cym+1,cxm+1),
		      DATA_TYPE POLYBENCH_2D(tmp,CYM+1,CXM+1,cym+1,cxm+1),
		      DATA_TYPE POLYBENCH_3D(Bza,CZ+1,CYM+1,CXM+1,cz+1,cym+1,cxm+1),
		      DATA_TYPE POLYBENCH_3D(Ex,CZ+1,CYM+1,CXM+1,cz+1,cym+1,cxm+1),
		      DATA_TYPE POLYBENCH_3D(Ey,CZ+1,CYM+1,CXM+1,cz+1,cym+1,cxm+1),
		      DATA_TYPE POLYBENCH_3D(Hz,CZ+1,CYM+1,CXM+1,cz+1,cym+1,cxm+1),
		      DATA_TYPE POLYBENCH_1D(czm,CZ+1,cz+1),
		      DATA_TYPE POLYBENCH_1D(czp,CZ+1,cz+1),
		      DATA_TYPE POLYBENCH_1D(cxmh,CXM+1,cxm+1),
		      DATA_TYPE POLYBENCH_1D(cxph,CXM+1,cxm+1),
		      DATA_TYPE POLYBENCH_1D(cymh,CYM+1,cym+1),
		      DATA_TYPE POLYBENCH_1D(cyph,CYM+1,cym+1))
{
  //int iz, iy, ix;

  //#pragma scop
  
  //#pragma omp parallel
    {
      //#pragma omp for private (iy, ix)
      #P1
      for (int iz = 0; iz < _PB_CZ; iz++)
        {
	  for (int iy = 0; iy < _PB_CYM; iy++)
	    {
	      for (int ix = 0; ix < _PB_CXM; ix++)
		{ //clf,CYM+1,CXM+1,
		  //clf[iz][iy] = Ex[iz][iy][ix] - Ex[iz][iy+1][ix] + Ey[iz][iy][ix+1] - Ey[iz][iy][ix];
		  POLYBENCH_2D_REF(clf,CYM+1,CXM+1,iz,iy) = POLYBENCH_3D_REF(Ex,CZ+1,CYM+1,CXM+1,iz,iy,ix) - POLYBENCH_3D_REF(Ex,CZ+1,CYM+1,CXM+1,iz,iy+1,ix) + POLYBENCH_3D_REF(Ey,CZ+1,CYM+1,CXM+1,iz,iy,ix+1) - POLYBENCH_3D_REF(Ex,CZ+1,CYM+1,CXM+1,iz,iy,ix);
		  
		  //tmp[iz][iy] = (cymh[iy] / cyph[iy]) * Bza[iz][iy][ix] - (ch / cyph[iy]) * clf[iz][iy];
		  POLYBENCH_2D_REF(tmp,CYM+1,CXM+1,iz,iy) = (POLYBENCH_1D_REF(cymh,CYM+1,iy) / POLYBENCH_1D_REF(cyph,CYM+1,iy)) * POLYBENCH_3D_REF(Bza,CZ+1,CYM+1,CXM+1,iz,iy,ix) - (ch / POLYBENCH_1D_REF(cyph,CYM+1,iy)) * POLYBENCH_2D_REF(clf,CYM+1,CXM+1,iz,iy);

		  POLYBENCH_3D_REF(Hz,CZ+1,CYM+1,CXM+1,iz,iy,ix) = (POLYBENCH_1D_REF(cxmh,CXM+1,ix) /POLYBENCH_1D_REF(cxph,CXM+1,ix)) * POLYBENCH_3D_REF(Hz,CZ+1,CYM+1,CXM+1,iz,iy,ix)
		    + (mui * POLYBENCH_1D_REF(czp,CZ+1,iz) / POLYBENCH_1D_REF(cxph,CXM+1,ix)) * POLYBENCH_2D_REF(tmp,CYM+1,CXM+1,iz,iy)
		    - (mui * POLYBENCH_1D_REF(czm,CZ+1,iz) / POLYBENCH_1D_REF(cxph,CXM+1,ix)) * POLYBENCH_3D_REF(Bza,CZ+1,CYM+1,CXM+1,iz,iy,ix);
		  POLYBENCH_3D_REF(Bza,CZ+1,CYM+1,CXM+1,iz,iy,ix) = POLYBENCH_2D_REF(tmp,CYM+1,CXM+1,iz,iy);
		}
	      
	      //clf[iz][iy] = Ex[iz][iy][_PB_CXM] - Ex[iz][iy+1][_PB_CXM] + Ry[iz][iy] - Ey[iz][iy][_PB_CXM];
	      POLYBENCH_2D_REF(clf,CYM+1,CXM+1,iz,iy) = POLYBENCH_3D_REF(Ex,CZ+1,CYM+1,CXM+1,iz,iy,_PB_CXM) - POLYBENCH_3D_REF(Ex,CZ+1,CYM+1,CXM+1,iz,iy+1,_PB_CXM) + POLYBENCH_2D_REF(Ry,CZ+1,CYM+1,iz,iy) - POLYBENCH_3D_REF(Ey,CZ+1,CYM+1,CXM+1,iz,iy,_PB_CXM);

	      //tmp[iz][iy] = (cymh[iy] / cyph[iy]) * Bza[iz][iy][_PB_CXM] - (ch / cyph[iy]) * clf[iz][iy];
	      POLYBENCH_2D_REF(tmp,CYM+1,CXM+1,iz,iy) = (POLYBENCH_1D_REF(cymh,CYM+1,iy) / POLYBENCH_1D_REF(cyph,CYM+1,iy)) * POLYBENCH_3D_REF(Bza,CZ+1,CYM+1,CXM+1,iz,iy,_PB_CXM) - (ch / POLYBENCH_1D_REF(cyph,CYM+1,iy)) * POLYBENCH_2D_REF(clf,CYM+1,CXM+1,iz,iy);

	      //Hz[iz][iy][_PB_CXM]=(cxmh[_PB_CXM] / cxph[_PB_CXM]) * Hz[iz][iy][_PB_CXM]
	      POLYBENCH_3D_REF(Hz,CZ+1,CYM+1,CXM+1,iz,iy,_PB_CXM)=(POLYBENCH_1D_REF(cxmh,CXM+1,_PB_CXM) / POLYBENCH_1D_REF(cxph,CXM+1,_PB_CXM)) * POLYBENCH_3D_REF(Hz,CZ+1,CYM+1,CXM+1,iz,iy,_PB_CXM)
		+ (mui * POLYBENCH_1D_REF(czp,CZ+1,iz) / POLYBENCH_1D_REF(cxph,CXM+1,_PB_CXM)) * POLYBENCH_2D_REF(tmp,CYM+1,CXM+1,iz,iy)
		- (mui * POLYBENCH_1D_REF(czm,CZ+1,iz) / POLYBENCH_1D_REF(cxph,CXM+1,_PB_CXM)) * POLYBENCH_3D_REF(Bza,CZ+1,CYM+1,CXM+1,iz,iy,_PB_CXM);

	      POLYBENCH_3D_REF(Bza,CZ+1,CYM+1,CXM+1,iz,iy,_PB_CXM) = POLYBENCH_2D_REF(tmp,CYM+1,CXM+1,iz,iy);
	      for (int ix = 0; ix < _PB_CXM; ix++)
		{
		  //clf[iz][iy] = Ex[iz][_PB_CYM][ix] - Ax[iz][ix] + Ey[iz][_PB_CYM][ix+1] - Ey[iz][_PB_CYM][ix];
		  POLYBENCH_2D_REF(clf,CYM+1,CXM+1,iz,iy) = POLYBENCH_3D_REF(Ex,CZ+1,CYM+1,CXM+1,iz,_PB_CYM,ix) - POLYBENCH_2D_REF(Ax,CZ+1,CYM+1,iz,ix) + POLYBENCH_3D_REF(Ey,CZ+1,CYM+1,CXM+1,iz,_PB_CYM,ix+1) - POLYBENCH_3D_REF(Ey,CZ+1,CYM+1,CXM+1,iz,_PB_CYM,ix);

		  //tmp[iz][iy] = (cymh[_PB_CYM] / cyph[iy]) * Bza[iz][iy][ix] - (ch / cyph[iy]) * clf[iz][iy];
		  POLYBENCH_2D_REF(tmp,CYM+1,CXM+1,iz,iy) = (POLYBENCH_1D_REF(cymh,CYM+1,_PB_CYM) / POLYBENCH_1D_REF(cyph,CYM+1,iy)) * POLYBENCH_3D_REF(Bza,CZ+1,CYM+1,CXM+1,iz,iy,ix) - (ch / POLYBENCH_1D_REF(cyph,CYM+1,iy)) * POLYBENCH_2D_REF(clf,CYM+1,CXM+1,iz,iy);

		  //Hz[iz][_PB_CYM][ix] = (cxmh[ix] / cxph[ix]) * Hz[iz][_PB_CYM][ix]
		  POLYBENCH_3D_REF(Hz,CZ+1,CYM+1,CXM+1,iz,_PB_CYM,ix) = (POLYBENCH_1D_REF(cxmh,CXM+1,ix) / POLYBENCH_1D_REF(cxph,CXM+1,ix)) * POLYBENCH_3D_REF(Hz,CZ+1,CYM+1,CXM+1,iz,_PB_CYM,ix)
		    + (mui * POLYBENCH_1D_REF(czp,CZ+1,iz) / POLYBENCH_1D_REF(cxph,CXM+1,ix)) * POLYBENCH_2D_REF(tmp,CYM+1,CXM+1,iz,iy)
		    - (mui * POLYBENCH_1D_REF(czm,CZ+1,iz) / POLYBENCH_1D_REF(cxph,CXM+1,ix)) * POLYBENCH_3D_REF(Bza,CZ+1,CYM+1,CXM+1,iz,_PB_CYM,ix);
		  POLYBENCH_3D_REF(Bza,CZ+1,CYM+1,CXM+1,iz,_PB_CYM,ix) = POLYBENCH_2D_REF(tmp,CYM+1,CXM+1,iz,iy);
		}

	      //clf[iz][iy] = Ex[iz][_PB_CYM][_PB_CXM] - Ax[iz][_PB_CXM] + Ry[iz][_PB_CYM] - Ey[iz][_PB_CYM][_PB_CXM];
	      POLYBENCH_2D_REF(clf,CYM+1,CXM+1,iz,iy) = POLYBENCH_3D_REF(Ex,CZ+1,CYM+1,CXM+1,iz,_PB_CYM,_PB_CXM) - POLYBENCH_2D_REF(Ax,CZ+1,CYM+1,iz,_PB_CXM) + POLYBENCH_2D_REF(Ry,CZ+1,CYM+1,iz,_PB_CYM) - POLYBENCH_3D_REF(Ey,CZ+1,CYM+1,CXM+1,iz,_PB_CYM,_PB_CXM);

	      //tmp[iz][iy] = (cymh[_PB_CYM] / cyph[_PB_CYM]) * Bza[iz][_PB_CYM][_PB_CXM] - (ch / cyph[_PB_CYM]) * clf[iz][iy];
	      POLYBENCH_2D_REF(tmp,CYM+1,CXM+1,iz,iy) = (POLYBENCH_1D_REF(cymh,CYM+1,_PB_CYM) / POLYBENCH_1D_REF(cyph,CYM+1,_PB_CYM)) * POLYBENCH_3D_REF(Bza,CZ+1,CYM+1,CXM+1,iz,_PB_CYM,_PB_CXM) - (ch / POLYBENCH_1D_REF(cyph,CYM+1,_PB_CYM)) * POLYBENCH_2D_REF(clf,CYM+1,CXM+1,iz,iy);

	      // Hz[iz][_PB_CYM][_PB_CXM] = (cxmh[_PB_CXM] / cxph[_PB_CXM]) * Hz[iz][_PB_CYM][_PB_CXM]
	      POLYBENCH_3D_REF(Hz,CZ+1,CYM+1,CXM+1,iz,_PB_CYM,_PB_CXM) = (POLYBENCH_1D_REF(cxmh,CXM+1,_PB_CXM) / POLYBENCH_1D_REF(cxph,CXM+1,_PB_CXM)) * POLYBENCH_3D_REF(Hz,CZ+1,CYM+1,CXM+1,iz,_PB_CYM,_PB_CXM)
		+ (mui * POLYBENCH_1D_REF(czp,CZ+1,iz) / POLYBENCH_1D_REF(cxph,CXM+1,_PB_CXM)) * POLYBENCH_2D_REF(tmp,CYM+1,CXM+1,iz,iy)
		- (mui * POLYBENCH_1D_REF(czm,CZ+1,iz) / POLYBENCH_1D_REF(cxph,CXM+1,_PB_CXM)) * POLYBENCH_3D_REF(Bza,CZ+1,CYM+1,CXM+1,iz,_PB_CYM,_PB_CXM);
	      POLYBENCH_3D_REF(Bza,CZ+1,CYM+1,CXM+1,iz,_PB_CYM,_PB_CXM) = POLYBENCH_2D_REF(tmp,CYM+1,CXM+1,iz,iy);
	    }
	}
  }
    //#pragma endscop
}


int main(int argc, char** argv)
{
  /* Retrieve problem size. */
  int cz = CZ;
  int cym = CYM;
  int cxm = CXM;

  /* Variable declaration/allocation. */
  DATA_TYPE mui;
  DATA_TYPE ch;
  POLYBENCH_2D_ARRAY_DECL(Ax,DATA_TYPE,CZ+1,CYM+1,cz+1,cym+1);
  POLYBENCH_2D_ARRAY_DECL(Ry,DATA_TYPE,CZ+1,CYM+1,cz+1,cym+1);
  POLYBENCH_2D_ARRAY_DECL(clf,DATA_TYPE,CYM+1,CXM+1,cym+1,cxm+1);
  POLYBENCH_2D_ARRAY_DECL(tmp,DATA_TYPE,CYM+1,CXM+1,cym+1,cxm+1);
  POLYBENCH_3D_ARRAY_DECL(Bza,DATA_TYPE,CZ+1,CYM+1,CXM+1,cz+1,cym+1,cxm+1);
  POLYBENCH_3D_ARRAY_DECL(Ex,DATA_TYPE,CZ+1,CYM+1,CXM+1,cz+1,cym+1,cxm+1);
  POLYBENCH_3D_ARRAY_DECL(Ey,DATA_TYPE,CZ+1,CYM+1,CXM+1,cz+1,cym+1,cxm+1);
  POLYBENCH_3D_ARRAY_DECL(Hz,DATA_TYPE,CZ+1,CYM+1,CXM+1,cz+1,cym+1,cxm+1);
  POLYBENCH_1D_ARRAY_DECL(czm,DATA_TYPE,CZ+1,cz+1);
  POLYBENCH_1D_ARRAY_DECL(czp,DATA_TYPE,CZ+1,cz+1);
  POLYBENCH_1D_ARRAY_DECL(cxmh,DATA_TYPE,CXM+1,cxm+1);
  POLYBENCH_1D_ARRAY_DECL(cxph,DATA_TYPE,CXM+1,cxm+1);
  POLYBENCH_1D_ARRAY_DECL(cymh,DATA_TYPE,CYM+1,cym+1);
  POLYBENCH_1D_ARRAY_DECL(cyph,DATA_TYPE,CYM+1,cym+1);

  /* Initialize array(s). */
  init_array (cz, cxm, cym, &mui, &ch,
  	      POLYBENCH_ARRAY(Ax),
  	      POLYBENCH_ARRAY(Ry),
  	      POLYBENCH_ARRAY(Ex),
  	      POLYBENCH_ARRAY(Ey),
  	      POLYBENCH_ARRAY(Hz),
  	      POLYBENCH_ARRAY(czm),
  	      POLYBENCH_ARRAY(czp),
  	      POLYBENCH_ARRAY(cxmh),
  	      POLYBENCH_ARRAY(cxph),
  	      POLYBENCH_ARRAY(cymh),
  	      POLYBENCH_ARRAY(cyph));

  /* Start timer. */
  polybench_start_instruments;

  /* Run kernel. */
  kernel_fdtd_apml (cz, cxm, cym, mui, ch,
  		    POLYBENCH_ARRAY(Ax),
  		    POLYBENCH_ARRAY(Ry),
  		    POLYBENCH_ARRAY(clf),
  		    POLYBENCH_ARRAY(tmp),
  		    POLYBENCH_ARRAY(Bza),
  		    POLYBENCH_ARRAY(Ex),
  		    POLYBENCH_ARRAY(Ey),
  		    POLYBENCH_ARRAY(Hz),
  		    POLYBENCH_ARRAY(czm),
  		    POLYBENCH_ARRAY(czp),
  		    POLYBENCH_ARRAY(cxmh),
  		    POLYBENCH_ARRAY(cxph),
  		    POLYBENCH_ARRAY(cymh),
  		    POLYBENCH_ARRAY(cyph));

  /* Stop and print timer. */
  polybench_stop_instruments;
  polybench_print_instruments;

  /* Prevent dead-code elimination. All live-out data must be printed
     by the function call in argument. */
  polybench_prevent_dce(print_array(cz, cxm, cym,
  				    POLYBENCH_ARRAY(Bza),
  				    POLYBENCH_ARRAY(Ex),
  				    POLYBENCH_ARRAY(Ey),
  				    POLYBENCH_ARRAY(Hz)));

  /* Be clean. 
  POLYBENCH_FREE_ARRAY(Ax);
  POLYBENCH_FREE_ARRAY(Ry);
  POLYBENCH_FREE_ARRAY(clf);
  POLYBENCH_FREE_ARRAY(tmp);
  POLYBENCH_FREE_ARRAY(Bza);
  POLYBENCH_FREE_ARRAY(Ex);
  POLYBENCH_FREE_ARRAY(Ey);
  POLYBENCH_FREE_ARRAY(Hz);
  POLYBENCH_FREE_ARRAY(czm);
  POLYBENCH_FREE_ARRAY(czp);
  POLYBENCH_FREE_ARRAY(cxmh);
  POLYBENCH_FREE_ARRAY(cxph);
  POLYBENCH_FREE_ARRAY(cymh);
  POLYBENCH_FREE_ARRAY(cyph);
  */
  return 0;
}
