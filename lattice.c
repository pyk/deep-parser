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
#include <math.h>
#include <stdbool.h>
#include <string.h>
#include "lattice.h"

LatticeNode **nodes;
LatticeEdge **edges;
int *path;
int size_max;
int posO;
int posS;

void lattice_new(int size, int ntags, float *scores){
  posO = ntags;
  posS = 20;
  size_max = size;
  nodes = malloc((size+1) * sizeof(LatticeNode*)); // +1 for end node
  edges = malloc((size) * sizeof(LatticeEdge*));
  path = malloc(((2*size)+1)*sizeof(int));

  int i, j;
  for (i=0;i<size;i++){
    nodes[i] = malloc(ntags * sizeof(LatticeNode));
    for (j=0;j<ntags-1;j++){
      nodes[i][j].col = i+1;
      nodes[i][j].idx = j+1;
      nodes[i][j].next = &nodes[i][j+1];
      nodes[i][j].score = scores + (i*ntags) + j;
    }
    nodes[i][ntags-1].col = i+1;
    nodes[i][ntags-1].idx = ntags;
    nodes[i][ntags-1].score = scores + (i*ntags) + ntags-1;
    nodes[i][ntags-1].next = NULL;
  }

  int nprefixpercol = (ntags-1)/4; //number of prefix (BIES) per col | -1:Other | /4:BIES
  int nedgepercol = ((2*nprefixpercol) + 1) * nprefixpercol * 2 //nb edges for all tags B and S
    + (2*nprefixpercol) + 1                                    //nb edges for the tag O
    +  (2 * nprefixpercol) * 2;                               //nb edges for all tags I and E
  int ind, ind2, k;
  for (i=1;i<size;i++){
    edges[i-1] = malloc(nedgepercol * sizeof(LatticeEdge));
    //printf("pointeur pour col %d : %p\n", i+1, edges);
    int nedge = 0;
    LatticeEdge *tempedge;

    ind = 0;
    for (j=0;j<ntags;j++){
      tempedge=NULL;
      if (j!=posO-1){ ind++; }
      //printf("j : %d -- ind %d\n", j, ind);
      switch (ind) {
      case 0 : //node O
      case 1 : //nodes B-X
      case 4 : //nodes S-X
	ind2 = 0;
	for (k=0;k<ntags;k++){
	  if (k!=posO-1){ ind2++; }
	  if ((ind2==3) | (ind2==4) | (ind2==0)){ //comming from E-X or S-X or 0
	    if (!tempedge){
	      tempedge = &edges[i-1][nedge]; nedge++;
	      nodes[i][j].edge = tempedge;
	    }
	    else{
	      tempedge->next = &edges[i-1][nedge];
	      tempedge = &edges[i-1][nedge]; nedge++;
	      
	    }
	    tempedge->src_node = &nodes[i-1][k];
	    tempedge->next=NULL;
	  }
	  ind2 = ind2 % 4;
	}
	break;
      case 2 : //node I-X
	//printf("\tcase%d\n", ind);
	tempedge = &edges[i-1][nedge]; nedge++;
	tempedge->src_node = &nodes[i-1][j];//comming from I-X
	nodes[i][j].edge = tempedge;
	tempedge->next = &edges[i-1][nedge];
	tempedge->next->next=NULL;
	tempedge = &edges[i-1][nedge]; nedge++;
	tempedge->src_node = &nodes[i-1][j-1];//comming from B-X
	break;
      case 3 : //node E-X
	//printf("\tcase%d\n", ind);
	tempedge = &edges[i-1][nedge]; nedge++;
	tempedge->src_node = &nodes[i-1][j-1];//comming from I-X
	nodes[i][j].edge = tempedge;
	tempedge->next = &edges[i-1][nedge];
	tempedge->next->next=NULL;
	tempedge = &edges[i-1][nedge]; nedge++;
	tempedge->src_node = &nodes[i-1][j-2];//comming from B-X
	break;
      }
      ind = ind % 4;
    }
  }
  //end node
  int nedge = 2;
  edges[size-1] = malloc(ntags * sizeof(LatticeEdge));
  LatticeEdge *tempedge;
  tempedge = &edges[size-1][nedge];
  nodes[size] = malloc(sizeof(LatticeNode));
  nodes[size]->col = size+1; nodes[size]->idx = 1;
  nodes[size]->score = malloc(sizeof(float));
  nodes[size]->score[0] = 0;
  nodes[size]->edge = tempedge;
  nodes[size]->next = NULL;
  tempedge->src_node = &nodes[size-1][nedge];
  nedge++;
  for (k=3;k<ntags;k++){
    if ((k%4!=0 && k%4!=1) || k==ntags-1){
      tempedge->next = &edges[size-1][nedge];
      tempedge->next->next=NULL;
      tempedge = &edges[size-1][nedge];
      tempedge->src_node = &nodes[size-1][nedge];
    }
    nedge++;
  }
}


int* lattice_forward(int size){
  int startNode = size_max - size;
  LatticeNode *node = nodes[startNode];
  LatticeEdge *edge, *maxedge;
  float maxscore;
  int i;

  //Set previous layer to NULL and 0
  while (node){
    node->max_edge = NULL;
    node->accScore = node->score[0];
    node = node->next;
  }
  
  //let's forward
  for (i=startNode+1;i<=size_max;i++){
    node = nodes[i];
    while (node){
      //printf("forward node %d %d  || ", node->col, node->idx);
      edge = node->edge;
      maxedge = NULL;
      maxscore = -INFINITY;
      while (edge){
	if (edge->src_node->accScore > maxscore && edge->src_node->col>startNode){
	  maxscore = edge->src_node->accScore;
	  maxedge = edge;
	}
	edge = edge->next;
      }
      node->max_edge = maxedge;
      node->accScore = node->score[0] + maxscore;
      node = node->next;
    }   
  }
  
  node = nodes[size_max]->max_edge->src_node;
  int ind=1;
  int sz=0;
  int idx=0;
  int nchunk = 0;
  bool allO = true;
  while (node){
    //printf("node %d %d\n" ,node->col, node->idx);
    if (node->idx!=posO){
      switch (node->idx%4){
      case 0 ://S-X
	allO=false;
	nchunk++;
	//printf("S tag n %d (indice %d) : \n" , ((int) node->idx/4), node->idx);
	path[ind] = 1;
	path[ind+1] = ((int) node->idx/4);
	ind = ind + 2;
	break;
      case 3 : //E-X
	allO=false;
	//printf("End-node %d %d\n" ,node->col, node->idx);
	nchunk++;
	path[ind+1] = ((int) node->idx/4)+1;
	sz = 2; idx = node->idx;
	//printf("%p\n", node->max_edge);
	if (!node->max_edge){
	  node = NULL;
	  sz = 1;
	}
	else{
	  node = node->max_edge->src_node;
	  //printf("BI-node %d %d\n" ,node->col, node->idx);
	}
	while (node  && node->col!=startNode+1 && node->idx!=idx-2){
	  if (node->max_edge){node = node->max_edge->src_node;}
	  else{printf("NULL\n");node=NULL;} //beark node
	  sz++;
	  //printf("I-node %d %d\n" ,node->col, node->idx);
	}
	path[ind]=sz; ind = ind + 2;
	//if (node) printf("B tag n %d (indice %d) size %d \n" , ((int) node->idx/4) +1, node->idx, sz);
	break;
      }
    } else{
      nchunk++;
      //printf("O tag n %d (indice %d) : \n" , ((int) node->idx/4)+1, node->idx);
      path[ind] = 1;
      path[ind+1] = ((int) node->idx/4)+1;
      ind = ind + 2;
    }
    if (node && node->max_edge){node = node->max_edge->src_node;}//beark node
    else{node=NULL;}
  }
  if (!allO){
    path[0]=nchunk;
  }
  else{
    path[0]=1;
    path[1]=size;
    path[2]=posS;
  }
  //printf("score : %.13f\n", maxscore);

  return path;
}

void lattice_print(int sz){
  int startNode = size_max - sz;
  printf("nodes adresse %p\n", nodes);
  printf("nodes[0] adresse %p\n", nodes[0]);
  printf("size %d | size max %d | startNode %d\n", sz, size_max, startNode);
  int compteurEdge = 0;
  int compteurNode = 0;
  int i;
  for (i=0+startNode;i<=size_max;i++){
    printf("colonne %d\n", i+1);
    LatticeNode *node = nodes[i];
    while (node){
      compteurNode++;
      printf("node %d %d s(%f) as(%f)\n", node->col, node->idx, node->score[0], node->accScore);
      LatticeEdge *edge = node->edge;
      printf("\t\t\t\t");
      while (edge){
	compteurEdge++;
	printf("<- %d %d    ", edge->src_node->col, edge->src_node->idx);
	edge = edge->next;
      }
      printf("\n");
      node = node->next;
    }
  }
}

void lattice_path_print(int* path){
  int i;
  printf("Path size : %d\n", path[0]);
  for (i=(2*path[0])-1;i>0;i-=2){
    printf("%d-chunk %d\n", path[i], path[i+1]);
  }
}

void lattice_free(){
  int i;
  free(nodes[size_max]->score);
  for (i=0;i<size_max+1;i++){
    free(nodes[i]);
  }
  for (i=0;i<size_max;i++){
    free(edges[i]);
  }
  free(nodes);
  free(edges);
  free(path);
}
