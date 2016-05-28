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
#include <assert.h>
#include <string.h>
#include "hashmap.h"
#include "dict.h"

map_t dict_load_word2ind(char* f_w){

  map_t mymap = hashmap_new();
  int i;
  data_struct_t* value;
  int error;

  int size_dict;
  
  FILE *file;
  file=fopen(f_w, "r");
  fscanf(file, "%d", &size_dict);
  
  //printf("dict size : %d\n", size_dict);

  for (i=0;i<size_dict;i++){
    value = malloc(sizeof(data_struct_t));
    fscanf(file, "%s", value->key_string);
    value->number = i+1;
    error = hashmap_put(mymap, value->key_string, value);
    assert(error==MAP_OK);
  }
  fclose(file);

  return mymap;
}

void dict_free_word2ind(map_t *dict, char* f_w){
  int size_dict = 0;
  int i, error;
  char* key = malloc(100*sizeof(char));
  data_struct_t* value;
  FILE *file;
  file=fopen(f_w, "r");
  fscanf(file, "%d", &size_dict);
  
  for (i=0;i<size_dict;i++){
    fscanf(file, "%s", key);
    hashmap_get(dict, key, (void**)(&value));
    error = hashmap_remove(dict, key);
    assert(error==MAP_OK);
    free(value);
  }
 
  fclose(file);
  free(key);
  hashmap_free(dict);
}

int dict_get(map_t dict, char *key){
  int error;
  data_struct_t *value;
  error = hashmap_get(dict, key, (void**)(&value));
  if (error!=0){
    hashmap_get(dict, "UNKNOWN", (void**)(&value));
  }
  return value->number;
}

char** dict_load_ind2word(char* f_w){
  
  FILE *file;
  int size_dict;
  file=fopen(f_w, "r");
  fscanf(file, "%d", &size_dict);

  char **dict = malloc((size_dict+1)*sizeof(char*));

  int i;
  for (i=0;i<size_dict;i++){
    char *v = malloc(100*sizeof(char));
    fscanf(file, "%s", v);
    if (strcmp(v, "(")==0){v[0]='-'; v[1]='L'; v[2]='R'; v[3]='B'; v[4]='-'; v[5]=0;}
    if (strcmp(v, ")")==0){v[0]='-'; v[1]='R'; v[2]='R'; v[3]='B'; v[4]='-'; v[5]=0;}
    if (strcmp(v, "{")==0){v[0]='-'; v[1]='L'; v[2]='C'; v[3]='B'; v[4]='-'; v[5]=0;}
    if (strcmp(v, "}")==0){v[0]='-'; v[1]='R'; v[2]='C'; v[3]='B'; v[4]='-'; v[5]=0;}
    dict[i]=v;
  }
  dict[size_dict]=NULL;
  fclose(file);

  return dict;
}

void dict_free_ind2word(char **dict){
  int i=0;
  char *w = NULL;
  w = dict[i];
  while(w){
    free(dict[i]);
    i++;
    w = dict[i];
  }
  free(dict);
}

dict_tag_t* dict_load_ind2tag(char* f_w){
  dict_tag_t *dict = malloc(sizeof(dict_tag_t));
  FILE *file;
  int size_dict;
  file=fopen(f_w, "r");
  fscanf(file, "%d", &size_dict);

  dict->tags = malloc((size_dict+1)*sizeof(char*));//+1 to know the size for deallocation (last one set to NULL)
  dict->sizes = malloc(size_dict*sizeof(int)); // size of tag (ex : S#NP -> 2 | S -> 1)

  int i, j, pos, size;
  for (i=0;i<size_dict;i++){
    char *value = malloc(100*sizeof(char));
    char *temp = malloc(100*sizeof(char));
    fscanf(file, "%s", temp);
    j=0; pos=0; size=1;
    while (temp[j]!=0){
      if (temp[j]=='#' && j!=0){
		value[pos] = ' ';
		pos++;
		value[pos]='(';
		pos++;j++;size++;
      }
      else{
	value[pos] = temp[j];
	pos++; j++;
      }
    }
    dict->tags[i]=value;
    dict->sizes[i]=size;
    free(temp);  
    value[pos]='\0';
  }
  dict->tags[size_dict]=NULL;
  fclose(file);
  return dict;
}

void dict_free_ind2tag(dict_tag_t *dict){
  int i=0;
  while(dict->tags[i]){
    free(dict->tags[i]);
    i++;
  }
  free(dict->tags);
  free(dict->sizes);
  free(dict);
}




