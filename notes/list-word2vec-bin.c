// -*- c-basic-offset: 2; my-source-command: "make list-word2vec-bin" -*-
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define MAX_WORD_LEN 100

long int total_read = 0;
long int can_seek = 1;
long int j=0;

int my_getchar() {
  total_read++;
  return getchar();
}
int my_fread(int length, void *dummy) {
  total_read += length;
  return fread(dummy, length, 1, stdin);
}
#define MAX_READ 1024*4
char dummy_buffer[MAX_READ];

int my_fseek(int length) {
  if(can_seek) {
    total_read += length;
    return fseek(stdin, length, SEEK_CUR);
  } else {
    /* fprintf(stderr, "simulating seek of length %d\n", length); */
    while(length>MAX_READ) {
      int res = my_fread(MAX_READ, dummy_buffer);
      if(res==-1) return res;
      length -= MAX_READ;
    }
    return my_fread(length, dummy_buffer);
  }
}

int main() {
  long vocab_size, vector_size;
  char word[MAX_WORD_LEN];
  int c;
  long int i;
  int scan_count;

  if (ftell(stdin) == -1) {
    fprintf(stderr, "Warning: STDIN may not be seekable: %s\n",
      strerror(errno));
    can_seek = 0;
  }
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
  while(!feof(stdin)) {
    // Read word until space
    long int word_start = total_read;
    i = 0;
    while ((c = my_getchar()) != ' ' && c != EOF) {
      if (i < MAX_WORD_LEN - 1) {
        word[i++] = c;
      }
    }
    word[i] = '\0';
        
    // Print the word
    printf("%ld %ld %s\n", j, word_start, word);

    my_fseek(sizeof(float)*vector_size);

    j++;
  }

  return 0;
}
