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

#include <stdlib.h>
#include <stdio.h>
#include "model.h"

Model* model_load(char* f_w, char* f_e, int *verbose){
  Model* model = malloc(sizeof(Model));
  FILE *file;
  file=fopen(f_w, "r");

  //Loading Tagger Network Weights
  fread(&model->wsz, sizeof(int), 1, file);
  fread(&model->nhu, sizeof(int), 1, file);
  fread(&model->wfsz, sizeof(int), 1, file);
  fread(&model->tfsz, sizeof(int), 1, file);
  if (verbose[0]==1){
    fprintf(stderr, "Loading model -- Tagger Network nhu\n");
    fprintf(stderr, "\tWindow size : %d\n", model->wsz);
    fprintf(stderr, "\tNumber of hidden units : %d\n", model->nhu);
    fprintf(stderr, "\tWord Feature Size : %d\n", model->wfsz);
    fprintf(stderr, "\tTag Feature Size : %d\n", model->tfsz);
    fprintf(stderr, "\tNumber of parameters for the tagger network : %d\n", model->wsz*(model->wfsz + model->tfsz)*model->nhu);
  }

  model->weights_scorer_nhu = malloc(model->wsz*(model->wfsz + model->tfsz)*model->nhu*sizeof(float));
  fread(model->weights_scorer_nhu, sizeof(float), model->wsz*(model->wfsz + model->tfsz)*model->nhu, file);

  if (verbose[0]==1){
    fprintf(stderr, "Loading model -- Tagger Network nhu -- bias\n");
  }
  model->bias_scorer_nhu = malloc(model->nhu*sizeof(float));
  fread(model->bias_scorer_nhu, sizeof(float), model->nhu, file);

  //Loading Tagger Network Weights
  fread(&model->nhu, sizeof(int), 1, file);
  fread(&model->nparsetags, sizeof(int), 1, file);
  if (verbose[0]==1){
    fprintf(stderr, "Loading model -- Tagger Network scorer\n");
    fprintf(stderr, "\tNumber of hidden units : %d\n", model->nhu);
    fprintf(stderr, "\tNumber of parsing tags to score : %d\n", model->nparsetags);
    fprintf(stderr, "\tNumber of parameters for the tagger network scorer: %d\n", model->nhu*model->nparsetags);
  }

  model->weights_scorer = malloc(model->nhu*model->nparsetags*sizeof(float));
  fread(model->weights_scorer, sizeof(float), model->nhu*model->nparsetags, file);

  if (verbose[0]==1){
    fprintf(stderr, "Loading model -- Tagger Network scorer -- bias\n");
  }
  model->bias_scorer = malloc(model->nparsetags*sizeof(float));
  fread(model->bias_scorer, sizeof(float), model->nparsetags, file);
  
  fread(&model->dropout, sizeof(int), 1, file);
  if (verbose[0]==1){
    fprintf(stderr, "Dropout probability : %f\n", model->dropout);
  }
  
  //Loading Composition Network
  fread(&model->ncomp, sizeof(int), 1, file);
  if (verbose[0]==1){
    fprintf(stderr, "Loading model -- Composition Networks\n");
    fprintf(stderr, "\tNumber of composition Networks : %d\n", model->ncomp);
  }
  
  model->weights_composition = malloc(model->ncomp*sizeof(float*));
  int paramsnumber = 0;
  int j;
  for (j=0;j<model->ncomp;j++){
    paramsnumber += (j+1)*(model->wfsz + model->tfsz)*model->wfsz;
    model->weights_composition[j] = malloc((j+1)*(model->wfsz + model->tfsz)*model->wfsz*sizeof(float));
    if (verbose[0]==1){
      fprintf(stderr, "\tNumber of parameters for network %d : %d\n" ,j+1 , (j+1)*(model->wfsz+model->tfsz)*model->wfsz);
    }
    fread(model->weights_composition[j], sizeof(float), (j+1)*(model->wfsz + model->tfsz)*model->wfsz, file);
  }
  if (verbose[0]==1){
    fprintf(stderr, "\tTotal number of parameters for the composition networks : %d\n", paramsnumber);
  }
  
  fclose(file);
  file=fopen(f_e, "r");

  //Loading Words Embeddings
  fread(&model->nwords, sizeof(int), 1, file);
  fread(&model->wfsz, sizeof(int), 1, file);
  
  if (verbose[0]==1){
    fprintf(stderr, "Loading Words Embeddings\n");
    fprintf(stderr, "\tNumber of words : %d\n", model->nwords);
    fprintf(stderr, "\tWord feature size : %d\n", model->wfsz);
    fprintf(stderr, "\tNumber of parameters words embeddings : %d\n", model->nwords*model->wfsz);
  }

  model-> words = malloc((model->nwords + 100)*model->wfsz*sizeof(float)); //+100 : free space to store sub-tree compositional representations
  fread(model->words, sizeof(float), model->nwords*model->wfsz, file);

  //Loading Words Embeddings
  fread(&model->ntags, sizeof(int), 1, file);
  fread(&model->tfsz, sizeof(int), 1, file);
  
  if (verbose[0]==1){
    fprintf(stderr, "Loading Tags Embeddings\n");
    fprintf(stderr, "\tNumber of tags : %d\n", model->ntags);
    fprintf(stderr, "\tTag feature size : %d\n", model->tfsz);
    fprintf(stderr, "\tNumber of parameters tag embeddings : %d\n", model->ntags*model->tfsz); }
  
  model-> tags = malloc(model->ntags*model->tfsz*sizeof(float));
  fread(model->tags, sizeof(float), model->ntags*model->tfsz, file);

  float check;
  fread(&check, sizeof(int), 1, file);
  
  if (verbose[0]==1){
    fprintf(stderr, "check %f (should be 0.5)\n", check);
  }

  fclose(file);

  return model;
}

void model_free(Model *model){
  free(model->weights_scorer_nhu);
  free(model->bias_scorer_nhu);
  free(model->weights_scorer);
  free(model->bias_scorer);
  int i;
  for (i=0;i<model->ncomp;i++){
    free(model->weights_composition[i]);
  }
  free(model->weights_composition);
  free(model->words);
  free(model->tags);
  free(model);
}
