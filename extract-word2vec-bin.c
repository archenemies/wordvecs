// -*- c-basic-offset: 2; my-source-command: "make extract-word2vec-bin" -*-
// FHE 28 Apr 2025
// New version of list-word2vec-bin.c
// With help from Grok
// https://grok.com/chat/0f989c40-9a23-423a-813a-b61c872f8b2f

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define MAX_WORD_LEN 200

long int total_read = 0;
long int can_seek = 1;
long int word_num=0;
long vocab_size, vector_size;

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
  if(length==0) return 0;
  /* fprintf(stderr, "can_seek: %d\n", can_seek); */
  if(can_seek) {
    total_read += length;
    return fseek(stdin, length, SEEK_CUR);
  } else {
    /* fprintf(stderr, "simulating seek of length %d\n", length); */
    int res;
    while(length>MAX_READ) {
      res = my_fread(MAX_READ, dummy_buffer);
      /* fprintf(stderr, "result %d: %d\n", length, res);  */
      if(res==-1) return res;
      length -= MAX_READ;
    }
    res = my_fread(length, dummy_buffer);
    /* fprintf(stderr, "result %d: %d\n", length, res); */
    if(res==1) return 0;
    else return -1;
  }
}

void usage(char* name) {
  fprintf(stderr,
    "Usage:\n"
    "%s -l < BINARY_FILE\n"
    "%s OFFSET_TABLE OUTPUT_PREFIX < BINARY_FILE\n",
    name, name);
}

int list_mode = 0;
int do_list() {
  char word[MAX_WORD_LEN];
  int c;
  while(!feof(stdin)) {
    // Read word until space
    long int word_start = total_read;
    int i = 0;
    while ((c = my_getchar()) != ' ' && c != EOF) {
      if (i < MAX_WORD_LEN - 1) {
        word[i++] = c;
      }
    }
    word[i] = '\0';

    // Print the word
    printf("%ld %ld %s\n", word_num, word_start, word);

    if(my_fseek(sizeof(float)*vector_size) != 0) {
      fprintf(stderr, "Error reading vector for word \"%s\" at index %ld: %s\n", word, word_num, strerror(errno));
    }

    word_num++;
  }

  return 0;
}

int do_extract(char *offset_table, char *output_prefix) {
  // Name and open output files
  char wvs_path[MAX_WORD_LEN], tab_path[MAX_WORD_LEN], hdr_path[MAX_WORD_LEN];
  snprintf(wvs_path, MAX_WORD_LEN, "%s.wvs", output_prefix);
  snprintf(tab_path, MAX_WORD_LEN, "%s.tab", output_prefix);
  snprintf(hdr_path, MAX_WORD_LEN, "%s.hdr", output_prefix);

  FILE *wvs_file = fopen(wvs_path, "wb");
  if (!wvs_file) {
    fprintf(stderr, "Error opening %s: %s\n", wvs_path, strerror(errno));
    exit(1);
  }
  FILE *tab_file = fopen(tab_path, "w");
  if (!tab_file) {
    fprintf(stderr, "Error opening %s: %s\n", tab_path, strerror(errno));
    exit(1);
  }
  FILE *hdr_file = fopen(hdr_path, "w");
  if (!hdr_file) {
    fprintf(stderr, "Error opening %s: %s\n", hdr_path, strerror(errno));
    exit(1);
  }

  // Open offset table
  FILE *table_file = fopen(offset_table, "r");
  if (!table_file) {
    fprintf(stderr, "Error opening offset table %s: %s\n", offset_table, strerror(errno));
    exit(1);
  }

  // Allocate vector buffer
  float *vector = malloc(vector_size * sizeof(float));
  if (!vector) {
    fprintf(stderr, "Memory allocation failed\n");
    exit(1);
  }

  // Loop until EOF on the offset table
  char line[MAX_WORD_LEN];
  char table_word[MAX_WORD_LEN];
  long word_count = 0;
  while (fgets(line, MAX_WORD_LEN, table_file)) {
    // Parse the line
    long index, offset;
    if (sscanf(line, "%ld %ld %s", &index, &offset, table_word) != 3) {
      fprintf(stderr, "Invalid line in offset table: %s\n", line);
      exit(1);
//      continue;
    }

    // Seek to the offset in stdin with my_fseek
    long current_pos = total_read;
    long seek_distance = offset - current_pos;
    if (seek_distance < 0) {
      fprintf(stderr, "Invalid offset %ld for word %s: cannot seek backward\n", offset, table_word);
      exit(1);
    }
    if (my_fseek(seek_distance) != 0) {
      fprintf(stderr, "Error seeking %ld to offset %ld for word \"%s\": %s\n", seek_distance, offset, table_word, strerror(errno));
      exit(1);
    }

    // Read the word
    char file_word[MAX_WORD_LEN];
    int i = 0;
    int c;
    while ((c = my_getchar()) != ' ' && c != EOF) {
      if (i < MAX_WORD_LEN - 1) {
        file_word[i++] = c;
      }
    }
    file_word[i] = '\0';

    // Check the word
    if (strcmp(table_word, file_word) != 0) {
      fprintf(stderr, "Word mismatch at offset %ld: expected \"%s\", found \"%s\"\n", offset, table_word, file_word);
      exit(1);
    }

    // Read the vector
    if (my_fread(sizeof(float) * vector_size, vector) != 1) {
      fprintf(stderr, "Error reading vector for word %s at index %ld: %s\n", table_word, index, strerror(errno));
      exit(1);
    }

    // Output the vector and table entry
    fwrite(vector, sizeof(float), vector_size, wvs_file);
    fprintf(tab_file, "%ld %ld %s\n", word_count, index, table_word);
    word_count++;
  }

  // Output the header
  fprintf(hdr_file, "%ld %ld\n", vector_size, word_count);

  // Cleanup
  free(vector);
  fclose(table_file);
  fclose(wvs_file);
  fclose(tab_file);
  fclose(hdr_file);

  return 0;
}

int main(int argc, char **argv) {
  if(argc >= 2 && (strcmp(argv[1], "-h")==0)) {
    usage(argv[0]);
    exit(0);
  } else if(argc >= 2 && strcmp(argv[1], "-l")==0) {
    list_mode = 1;
    if(argc != 2) {
      fprintf(stderr, "List mode: expected no arguments\n");
      exit(1);
    }
  } else {
    if(argc != 3) {
      fprintf(stderr, "Extract mode: expected two arguments\n");
      exit(1);
    }
  }

  if (ftell(stdin) == -1) {
    fprintf(stderr, "Warning: STDIN may not be seekable: %s\n",
      strerror(errno));
    can_seek = 0;
  }

  int scan_count;
  // Read vocab size and vector size from first line
  if (scanf("%ld %ld\n%n", &vocab_size, &vector_size, &scan_count) != 2) {
    fprintf(stderr, "Failed to read header\n");
    return 1;
  }
  total_read += scan_count;

  fprintf(stderr, "Reading %d word vectors of size %d\n",
    vocab_size, vector_size);

  if(list_mode) {
    return do_list();
  } else {
    char *offset_table = argv[1];
    char *output_prefix = argv[2];
    return do_extract(offset_table, output_prefix);
  }
}
