#ifndef __MYMALLOC_H
#define __MYMALLOC_H
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>

struct block {
  int size;
  int free;
  struct block *next;
};

struct block *find_free_block(struct block **last, size_t size);

struct block *request_space(struct block* last, size_t size);

void *malloc(size_t size);

void *malloc(size_t size);

void *calloc(size_t nelem, size_t elsize);

struct block *get_block_ptr(void *ptr);

void *realloc(void *ptr, size_t size);

#endif /*__MYMALLOC_H*/
