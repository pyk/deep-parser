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
#include "tree.h"
#include "model.h"
#include "dict.h"
#include "nn.h"

float* input_compose;

void tree_new(void *mod_){
  Model *mod = (Model*) mod_;
  input_compose =  malloc(100* (mod->wfsz+mod->tfsz) *sizeof(float));
}

void tree_print(Node *node, int t){
  int i;
  for (i=0;i<t;i++){printf("\t");}
  if (node->nson!=0) {printf("%d %d\n", node->tag, node->value);}
  else {
    printf("%d %d\n", node->tag, node->value);
  }
  for (i=0;i<node->nson;i++){
    tree_print(node->sons[i], t+1);
  }
}

void tree_compose(int *words, int *tags, int size, void *model_, int output){
  Model *model = (Model*) model_;
  if (size>model->ncomp){
    fprintf(stderr, "Compression size too large : %d\n", size);
    exit(EXIT_FAILURE);
  }
  int i;
  //copying features
  for (i=0;i<size ;i++){
    nn_copy(model->words + (words[i]-1) * model->wfsz, input_compose + i*(model->wfsz + model->tfsz), model->wfsz);
    nn_copy(model->tags + (tags[i]-1) * model->tfsz, input_compose + i*(model->wfsz + model->tfsz) + model->wfsz, model->tfsz);
  }

  nn_linear(model->words + (output-1) * model->wfsz, model->wfsz, model->weights_composition[size-1],
	    NULL, input_compose, size*(model->wfsz+model->tfsz), 0);

  nn_hardTanh(model->words + (output-1) *model->wfsz, model->wfsz);
  nn_vmul(model->words + (output-1) *model->wfsz, model->wfsz, 0.75);
}

int tree_print_words(Node *node, int t, int n, char** leaves, void* dict_tags_){
  int i;
  dict_tag_t *dict_tags = (dict_tag_t*) dict_tags_;
  for (i=0;i<t;i++){printf("\t");}
  if (node->nson!=0) {
    printf("%s\n", dict_tags->tags[node->tag-1]);
  }
  else {
    printf("%s %s\n", dict_tags->tags[node->tag-1], leaves[n]);
    n++;
  }
  for (i=0;i<node->nson;i++){
    n = tree_print_words(node->sons[i], t+1, n, leaves, dict_tags);
  }
  return n++;
}

int tree_2evalb_words(Node *node, int n, char** leaves, void* dict_tags_){
  int i, j;
  dict_tag_t *dict_tags = (dict_tag_t*) dict_tags_;
  if (node->nson!=0) {
    printf("(%s ", dict_tags->tags[node->tag-1]);
  }
  else {
    printf("(%s %s)", dict_tags->tags[node->tag-1], leaves[n]);
    n++;
  }
  for (i=0;i<node->nson;i++){
    n = tree_2evalb_words(node->sons[i], n, leaves, dict_tags_);
  }
  if (node->nson!=0) {
    for (j=0;j<dict_tags->sizes[node->tag-1];j++){printf(")");}
  }
  return n;
}

void tree_free(){
  free(input_compose);
}

