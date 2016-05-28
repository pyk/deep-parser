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

#include "hashmap.h"

#ifndef H_DICT
#define H_DICT

#define KEY_MAX_LENGTH (256)

typedef struct data_struct_s
{
  char key_string[KEY_MAX_LENGTH];
  int number;
} data_struct_t;

typedef struct dict_tag_s
{
  char** tags;
  int *sizes;
} dict_tag_t;

map_t dict_load_word2ind(char* f_w);
int dict_get(map_t dict, char *key);
char** dict_load_ind2word(char* fw);
dict_tag_t* dict_load_ind2tag(char* f_w);
void dict_free_ind2tag(dict_tag_t *dict);
void dict_free_ind2word(char **dict);
void dict_free_word2ind(map_t *dict, char *f_w);

#endif
