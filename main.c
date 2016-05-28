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
#include <time.h>
#include <ctype.h>
#include "model.h"
#include "dict.h"
#include "parse.h"
#include "tree.h"
#include "senna/SENNA_Hash.h"
#include "senna/SENNA_Tokenizer.h"
#include "senna/SENNA_POS.h"

#define SENTENCE_WORD_SIZE_MAX 200 //sentence size max (nb word)
#define WORD_CHAR_SIZE_MAX 100 //max size (char) for input words
#define SENTENCE_CHAR_SIZE_MAX 20000 //max sentence size (char)

void lower_number(char *word){
  int i;
  int pos = 0;
  
  for(i = 0; word[i]; i++){
    if (isdigit(word[i])){
      word[pos] = '0';
      pos++;
      while (isdigit(word[i])){i++;} i--;
    }
    else{
      word[pos] = tolower(word[i]);
      pos++;
    }
  }
  word[pos] = 0;
}

int is_empty(char *sentence){
  int i=0;
  int res = 1;
  while (sentence[i]!='\0'){
    if (sentence[i]!=' ' && sentence[i]!='\n'){
      res = 0;
      break;
    }
    else 
      i++;
  }
  return res;
}

void copy_word(char *dest, char* src){
  if (strcmp(src, "(")==0){strcpy(dest, "-LRB-");}
  else {
    if (strcmp(src, ")")==0){strcpy(dest, "-RRB-");}
    else {
      if (strcmp(src, "{")==0){strcpy(dest, "-LCB-");}
      else {
	if (strcmp(src, "}")==0){strcpy(dest, "-RCB-");}
	else {
	  strcpy(dest, src);
	}}}}
}

static void help()
{
  printf("\n");
  printf("Syntactic Parser\n");
  printf("Legrand Joël (joel.legrand@idiap.ch) 2014\n");
  printf("\n");
  printf("Usage: ./parse [options]\n");
  printf("   Takes sentence (one line per sentence) on stdin\n");
  printf("   Outputs parse tree in Evalb format on stdout\n");
  printf("\n");
  printf("Typical usage: ./parse [options] < inputfile.txt > outputfile.txt\n");
  printf("\n");
  printf("Options:\n");
  printf("   -full          Full version (speed loss but better accuracy)\n");
  printf("\n");
  printf("Display options:\n");
  printf("   -h             Display this help\n");
  printf("   -verbose       Display model informations on stderr\n");
  printf("\n");
  printf("Input options:\n");
  printf("   -usrpos <file>     Use user's POS (space separated) instead of SENNA POS\n");
  printf("\n");

  exit(-1);
}

static void info()
{
  fprintf(stderr, "\n");
  fprintf(stderr, "Parser Copyright (C) 2014 Idiap Research Institute\n");
  fprintf(stderr, "Written by Joël Legrand <joel.legrand@idiap.ch>\n");
  fprintf(stderr, "This program comes with ABSOLUTELY NO WARRANTY\n");
  fprintf(stderr, "This is free software, and you are welcome to redistribute it\n");
  fprintf(stderr, "under certain conditions.\n\n");
}

int main (int argc, char *argv[]) {

info();

  int i;
  char *fpos;
  int usrpos=0;
  int verbose=0;
  int nmodel=1;

  for (i = 1; i < argc; i++) {
    if (argv[i][0] == '-') {
      if (strcmp(argv[i],"-usrpos")==0) {
	i++;
	usrpos = 1;
	fpos = argv[i];
      }
      if (strcmp(argv[i],"-verbose")==0) {
	verbose=1;
	fprintf(stderr, "Verbose mode\n");
      }
      if (strcmp(argv[i],"-h")==0) {
	help();
	exit(1);
      }
      if (strcmp(argv[i],"-full")==0){
	nmodel=4;
      }
    }
  }

  SENNA_Tokenizer *tokenizer = NULL;
  SENNA_POS *pos = NULL;
  SENNA_Hash *pos_hash = NULL;
  SENNA_Hash *word_hash = NULL;
  SENNA_Hash *caps_hash = NULL;
  SENNA_Hash *suff_hash = NULL;
  if (usrpos==0){
    word_hash = SENNA_Hash_new("", "senna/hash/words.lst");
    caps_hash = SENNA_Hash_new("", "senna/hash/caps.lst");
    suff_hash = SENNA_Hash_new("", "senna/hash/suffix.lst");
    pos_hash = SENNA_Hash_new("", "senna/hash/pos.lst");
    
    tokenizer = SENNA_Tokenizer_new(word_hash, caps_hash, suff_hash, NULL, NULL, NULL, NULL, NULL, 1);
    pos = SENNA_POS_new("", "senna/data/pos.dat");
  }

  //loading dictionary
  map_t words_dict = dict_load_word2ind("dict/words.txt");
  map_t tags_dict = dict_load_word2ind("dict/tags.txt");
  char **words_dict_ind2word = dict_load_ind2word("dict/words.txt");
  dict_tag_t *tags_dict_ind2tag = dict_load_ind2tag("dict/tags.txt");

  //loading model
  Model **models = malloc(nmodel * sizeof(Model));

  char bufWeights[100];
  char bufEmbeddings[100];
  for (i=0;i<nmodel;i++){
    sprintf(bufWeights, "model/weights-%d.bin", i+1);
    sprintf(bufEmbeddings, "model/embeddings-%d.bin", i+1);
    models[i] = model_load(bufWeights, bufEmbeddings, &verbose);
  } 
    
  //Model *model = models[0];
  //init parser
  parse_new(SENTENCE_WORD_SIZE_MAX, models, nmodel, dict_get(words_dict, "PADDING"), dict_get(tags_dict, "PADDING"));


  FILE *in_fp = NULL, *in_fp_t = NULL;
  in_fp=stdin;
  if (usrpos==1){
    in_fp_t =fopen(fpos, "r");
  }
  char *word = malloc(WORD_CHAR_SIZE_MAX*sizeof(char));
  char *tag = malloc(SENTENCE_WORD_SIZE_MAX*sizeof(char));
  char *ch_p = malloc(sizeof(char));
  char * pch;

  char **words_char = malloc(SENTENCE_WORD_SIZE_MAX*sizeof(char*));
  for (i=0;i<SENTENCE_WORD_SIZE_MAX;i++){
    words_char[i] = malloc(WORD_CHAR_SIZE_MAX*sizeof(char));
  }
  int* words = malloc(SENTENCE_WORD_SIZE_MAX*sizeof(int));
  int* tags = malloc(SENTENCE_WORD_SIZE_MAX*sizeof(int));
  int count;

  char sentence[SENTENCE_CHAR_SIZE_MAX];
  sentence[0]=0;
  SENNA_Tokens* tokens;	 

  clock_t start = clock(), diff;
  if (verbose==1){
    fprintf(stderr, "let's parse\n");
  }

  
  //output file
  FILE *f = stdout;

  Node *tree;
  if (in_fp != NULL){
    count = 0;

    while(fgets(sentence, SENTENCE_CHAR_SIZE_MAX, stdin)){
      if (is_empty(sentence))
      	continue;
	
      //comput POS tags using SENNA if needed
      if (usrpos==0){
	tokens = SENNA_Tokenizer_tokenize(tokenizer, sentence);
	SENNA_POS_forward(pos, tokens->word_idx, tokens->caps_idx, tokens->suff_idx, tokens->n);
	for (i=0;i<tokens->n;i++){
	  tags[i] = dict_get(tags_dict, (char*)SENNA_Hash_key(pos_hash, pos->labels[i]));
	}
      }

      pch = strtok (sentence," \n");
      while (pch != NULL)
	{
	  copy_word(words_char[count], pch);
	  strcpy(word, pch);
	  lower_number(word);
	  words[count] = dict_get(words_dict, word);

	  if (usrpos==1){
	    fscanf(in_fp_t, "%s", tag);
	    tags[count] = dict_get(tags_dict, tag);
	  }

	  count++;
	  pch = strtok (NULL, " \n");
	}

      tree = (Node*) parse_parse(words, tags, count);
      //tree_print_words(tree, 0, 0, words_char, tags_dict_ind2tag);
      //tree_print(tree, 0);
      tree_2evalb_words(tree, 0, words_char, tags_dict_ind2tag);
      fprintf(f, "\n");
      count = 0;
    }
    
    fclose (in_fp);
  }
        
  diff = clock() - start;
  
  int msec = diff * 1000 / CLOCKS_PER_SEC;
  if (verbose==1){
    fprintf(stderr, "Parsed in %d seconds %d milliseconds\n", msec/1000, msec%1000);
  }

  fclose(f);
  if (usrpos==1){
    fclose(in_fp_t);
  }

  parse_free();
  for (i=0;i<nmodel;i++){
    model_free(models[i]);
  }
  free(models);

  dict_free_word2ind(words_dict, "dict/words.txt");
  dict_free_word2ind(tags_dict, "dict/tags.txt");
  dict_free_ind2word(words_dict_ind2word);
  dict_free_ind2tag(tags_dict_ind2tag);

  for (i=0;i<SENTENCE_WORD_SIZE_MAX;i++){
    free(words_char[i]);
  }
  free(words_char);
  free(word);
  free(tag);
  free(ch_p);
  free(words);
  free(tags);

  if (usrpos==0){
    SENNA_POS_free(pos);
    SENNA_Tokenizer_free(tokenizer);

    SENNA_Hash_free(word_hash);
    SENNA_Hash_free(caps_hash);
    SENNA_Hash_free(suff_hash);
    SENNA_Hash_free(pos_hash);
  }

  return 0;
}
