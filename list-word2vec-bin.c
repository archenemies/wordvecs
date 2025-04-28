// -*- c-basic-offset: 2; my-source-command: "make list-word2vec-bin" -*-
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define MAX_WORD_LEN 100

long int total_read = 0;

int my_getchar() {
  total_read++;
  return getchar();
}
int my_fread(int length, void *dummy) {
  total_read += length;
  return fread(dummy, length, 1, stdin);
}


int main() {
  long vocab_size, vector_size;
  char word[MAX_WORD_LEN];
  int c;
  long int i;
  int scan_count;

  // Read vocab size and vector size from first line
  if (scanf("%ld %ld\n%n", &vocab_size, &vector_size, &scan_count) != 2) {
    fprintf(stderr, "Failed to read header\n");
    return 1;
  }
  total_read += scan_count;

  fprintf(stderr, "Reading %d word vectors of size %d\n",
    vocab_size, vector_size);

  // Process each word entry
  /* for (long j = 0; j < vocab_size; j++) { */
  long int j=0;
  while(!feof(stdin)) {
    // Read word until space
    i = 0;
    while ((c = my_getchar()) != ' ' && c != EOF) {
      if (i < MAX_WORD_LEN - 1) {
        word[i++] = c;
      }
    }
    word[i] = '\0';
        
    // Print the word
    printf("%ld %ld %s\n", j, total_read, word);

    if(1) {
      // This works
      float dummy[vector_size];
      if (my_fread(sizeof(float)*vector_size, dummy) != 1) {
        fprintf(stderr, "Error reading vector data for word %d\n", j);
        return 1;
      }
    } else {
      // I wanted to do this, but apparently STDIN is not seekable
      if (fseek(stdin, sizeof(float)*vector_size, SEEK_CUR) != 0) {
        fprintf(stderr, "Error seeking after word %d: %s\n", j, strerror(errno));
        return 1;
      }
    }
    j++;
  }

  return 0;
}
