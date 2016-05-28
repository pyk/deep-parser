/* DeepParse is distributed under BSD license reproduced below. */

/* Copyright (c) 2014 Idiap Research Institute, http://www.idiap.ch/ */
/* Written by Joël Legrand <joel.legrand@idiap.ch> */

/* Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met: */

/* 1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer. */

/* 2. Redistributions in binary form must reproduce the above
   copyright notice, this list of conditions and the following
   disclaimer in the documentation and/or other materials provided
   with the distribution. */

/* 3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived
   from this software without specific prior written permission. */

/* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
   FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
   COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
   INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
   (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
   SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
   HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
   STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
   OF THE POSSIBILITY OF SUCH DAMAGE. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef USE_ATLAS_BLAS
#define USE_BLAS
#include "cblas.h"
#endif

#ifdef USE_MKL_BLAS
#define USE_BLAS
#include "mkl_cblas.h"
#endif

#ifdef USE_APPLE_BLAS
#define USE_BLAS
#include "Accelerate/Accelerate.h"
#endif

void nn_linear(float *output, int output_size, float *weights, float *biases, float *input, int input_size, int add)
{
#ifdef USE_BLAS
  if(biases){
    if (add==0){
      cblas_scopy(output_size, biases, 1, output, 1);
    }
    else{
      cblas_saxpy(output_size, 1, biases, 1, output, 1);
    }
  }
  cblas_sgemv(CblasRowMajor, CblasNoTrans, output_size, input_size,
	      1., weights, input_size, input, 1, (biases ? 1.0 : 0.0), output, 1);
#else
  //||add==1
  int i, j;
  
  for(i = 0; i < output_size; i++)
    {
      float z = (biases ? biases[i] : 0);
      float *weights_row = weights + i*input_size;
      for(j = 0; j < input_size; j++)
	z += input[j]*weights_row[j];
      output[i] = z;
    }
#endif
}

void nn_copy(float *input, float *output, int size){
#ifdef USE_BLAS
  cblas_scopy(size, input, 1, output, 1);
#else
  memcpy(output, input, size*sizeof(float));
#endif
}

void nn_hardTanh(float *v, int length){
  int i;
  for (i=0;i<length;i++){
    if (v[i]<-1){ v[i]=-1; }
    else {if (v[i]>1){ v[i]=1; }}
  }
}

void nn_vmul(float *v, int length, float x){
  int i;
  for (i=0;i<length;i++){
    v[i] = x*v[i];
  }
}
