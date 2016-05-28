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
#include "parse.h"
#include "lattice.h"
#include "nn.h"

int input_size_max;
int tagO = 41;
int tagS = 20;
int padding;
int paddingsz;
int wpadding;
int tpadding;
int nbNodes;
float* input;
float* output;
float* scores;
Model** models;
int nmodels;
Node *treeNodes;

int *words_input_compose;
int *tags_input_compose;
int size_input_compose;

void parse_new(int size, Model **mods, int nmods, int wpad, int tpad){
  models = mods;
  nmodels = nmods;
  padding = (models[0]->wsz-1)/2;
  paddingsz = padding * (models[0]->wfsz + models[0]->tfsz);
  wpadding = wpad;
  tpadding = tpad;
  input_size_max = size;
  input = malloc(input_size_max * models[0]->wfsz * models[0]->tfsz * sizeof(float));
  output = malloc(input_size_max * models[0]->nhu * sizeof(float));
  scores = malloc(input_size_max * models[0]->nparsetags * sizeof(float));
  //We add the padding of the beginning of the sentence here to avoid doing it for each input
  int i;
  for (i=0;i<padding ;i++){
    nn_copy(models[0]->words + (wpadding-1) * models[0]->wfsz,
	    input + i*(models[0]->wfsz + models[0]->tfsz), models[0]->wfsz);
    nn_copy(models[0]->tags + (tpadding-1) * models[0]->tfsz, input + i*(models[0]->wfsz + models[0]->tfsz) + models[0]->wfsz, models[0]->tfsz);
  }
  
  //Set of nodes
  nbNodes = 200;
  treeNodes = malloc(nbNodes * sizeof(Node));
  for (i=0;i<nbNodes;i++)
    treeNodes[i].sons = malloc(input_size_max*sizeof(Node*));
  
  //lattice
  lattice_new(input_size_max, models[0]->nparsetags, scores);
  
  //composition
  tree_new(models[0]);
  words_input_compose = malloc(input_size_max*sizeof(int));
  tags_input_compose = malloc(input_size_max*sizeof(int));
}

void parse_score(int* words, int* tags, int size){
  int i, j;
  for (j=0;j<nmodels;j++){
    //add padding at the beginning 
    if (nmodels!=1){//if 1 model, the paddind is always the same (set in parse_new)
      for (i=0;i<padding ;i++){
	nn_copy(models[j]->words + (wpadding-1) * models[j]->wfsz,
		input + i*(models[j]->wfsz + models[j]->tfsz), models[j]->wfsz);
	nn_copy(models[j]->tags + (tpadding-1) * models[j]->tfsz, input + i*(models[j]->wfsz + models[j]->tfsz) + models[j]->wfsz, models[j]->tfsz);
      }
    }
    //Copying the input features
    for (i=0;i<size ;i++){
      nn_copy(models[j]->words + (words[i]-1) * models[j]->wfsz, input + paddingsz + i*(models[j]->wfsz + models[j]->tfsz), models[j]->wfsz);
      nn_copy(models[j]->tags + (tags[i]-1) * models[j]->tfsz, input + paddingsz + i*(models[j]->wfsz + models[j]->tfsz) + models[j]->wfsz, models[j]->tfsz);
    }
    //add padding at the end
    for (i=0;i<padding ;i++){
      nn_copy(models[j]->words + (wpadding-1) * models[j]->wfsz, input + paddingsz + (size+i)*(models[j]->wfsz + models[j]->tfsz), models[j]->wfsz);
      nn_copy(models[j]->tags + (tpadding-1) * models[j]->tfsz, input + models[j]->wfsz + paddingsz + (size+i) *(models[j]->wfsz + models[j]->tfsz), models[j]->tfsz);
    }
        
    //computing hidden layer
    for (i=0;i<size;i++){
      nn_linear(output + i*models[j]->nhu, models[j]->nhu, models[j]->weights_scorer_nhu, models[j]->bias_scorer_nhu, input + i*(models[j]->wfsz+models[j]->tfsz), models[j]->wsz*(models[j]->wfsz+models[j]->tfsz), 0);
    }
    nn_hardTanh(output, size*models[j]->nhu);

    //we copy the scores at the end of "scores" wich correspond to the end of the lattice 
    float *startp = scores + (input_size_max - size) * models[j]->nparsetags;
    for (i=0;i<size;i++){
      nn_linear(startp + i*models[j]->nparsetags, models[j]->nparsetags, models[j]->weights_scorer, models[j]->bias_scorer, output + i*models[j]->nhu, models[j]->nhu, j==0 ? 0 : 1);
    }    
  }  
}

int* parse_viterbi(int size){
  return lattice_forward(size);
}


void* parse_parse(int *words, int *tags, int size){
  int depth = 0;
  int nNode = 0;
  int nRep = 1;
  int i, j, k, pos, indice;
  Node **trees = malloc(size*sizeof(Node*));
  Node *tempNode;
  Node **newTrees;

  //initialising leaf nodes 
  for (i=0;i<size;i++){
    trees[i] = &treeNodes[nNode];
    trees[i]->value = words[i];
    trees[i]->tag = tags[i];
    trees[i]->nson = 0;
    nNode++;
  }
  
  while (size!=1){
    depth = depth + 1;
    pos = 0;
    indice = 0;
    
    parse_score(words, tags, size);
    int *path = parse_viterbi(size);

    //building new subtrees
    newTrees = malloc(path[0]*sizeof(Node*));
    for (i=(2*path[0])-1;i>0;i-=2){
      if (path[i+1]!=tagO){
	tempNode = &treeNodes[nNode];
	tempNode->tag = path[i+1];
	tempNode->value = models[0]->nwords + nRep;
	tempNode->nson = path[i];
	for (j=0;j<path[i];j++){
	  tempNode->sons[j] = trees[indice];
	  words_input_compose[j] = trees[indice]->value;
	  tags_input_compose[j] = trees[indice]->tag;
	  indice++;
	}
	newTrees[pos] = tempNode;
	words[pos] = models[0]->nwords + nRep;
	tags[pos] = tempNode->tag;
	
	//Computing Subtree Compositional Representation
	for (k=0;k<nmodels;k++){
	  if (path[0]!=1){
	    size_input_compose = path[i];
	    tree_compose(words_input_compose, tags_input_compose, size_input_compose, models[k], models[k]->nwords + nRep);
	  }
	}
	nRep++; nNode++;
	pos++;
	}
      else{
	newTrees[pos] = trees[indice];
	words[pos] = trees[indice]->value;
	tags[pos] = trees[indice]->tag;
	indice++;
	pos++;
      }
    }
    if (size==path[0] && depth>2){
      tempNode = &treeNodes[nNode];
      tempNode->tag = tagS;
      tempNode->value = -1;
      tempNode->nson = size;
      for (j=0;j<size;j++){
	tempNode->sons[j] = trees[j];
      }
      trees[0] = tempNode;
      break;
    }

    size = path[0];
    free(trees);
    trees = newTrees;

  }
  
  Node *tree = trees[0];
  free(trees);
  return tree;
}

void parse_free(){
  free(input);
  free(output);
  free(scores);
  int i;
  for (i=0;i<nbNodes;i++)
    free(treeNodes[i].sons);
  free(treeNodes);
  free(words_input_compose);
  free(tags_input_compose);
  lattice_free();
  tree_free();
}

