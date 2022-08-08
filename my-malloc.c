/*
* my-malloc.c
* Fall 2021
* Assignment 3
* Steven and Ethan Nov 25
*/

#include <assert.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdint.h>
#include "my-malloc.h"

/*Used for debugging, checks how many mallocs were made*/
int count = 0;

static void *head = NULL;

/*Use this code to print memory locations*/
char* pointer_to_hex_le(void *ptr){
    static char hex[sizeof(ptr) * 2 + 1];
    char hex_chars[] = "0123456789abcdef";
    int i;
    uint8_t nibble;

    uint64_t mask = 0xf;
    uint64_t shift = 0;

    for(i=sizeof(ptr) * 2 - 1; i>=0; i-=1) {
      nibble = ((uint64_t) ptr & mask) >> shift;
      hex[i] = hex_chars[nibble];

      mask = mask << 4;
      shift += 4;
    }
    hex[sizeof(ptr) * 2] = '\0';

    return hex;
}

/*Use this code to print integers*/
char* uint64_to_string(uint64_t n){
    static char s[32] = "0000000000000000000000000000000\0";
    int i;

    for(i=30; i>0; i--) {
      s[i] = (n % 10) + '0';
      n = n / 10;
      if(n == 0)
        break;
    }
    return &s[i];
}


/*This code loops through the linked list of mallocs and prints information
about each*/
void memory_info(char *prefix){
    write(1, "size of struct: ", 16);
    char* sizeOfStruct = uint64_to_string(sizeof(struct block));
    write(1, sizeOfStruct, sizeof(sizeOfStruct) * 2);
    write(1, "\n", 2);
    write(1, prefix, 6);
    write(1, "program break: ", 15);
    void* temp = sbrk(0);
    write(1, pointer_to_hex_le(temp), sizeof(temp) * 2);
    write(1, "\n", 2);
    struct block *ptr = head;
    int c = 0;
    while (ptr) {
      write(1, "malloc number: ", 15);
      write(1, uint64_to_string(c), sizeof(c) * 2);
      write(1, ", malloc pointer <", 19);
      write(1, pointer_to_hex_le(ptr), sizeof(ptr) * 2);
      write(1, "> malloc size: ", 15);
      char* size = uint64_to_string(ptr->size);
      write(1, size, sizeof(size) * 2);
      write(1, " malloc free: ", 14);
      char* free = uint64_to_string(ptr->free);
      write(1, free, sizeof(size) * 2);
      write(1, "\n", 2);
      ptr = ptr->next;
      c++;
	   }
}

/* Iterate through blocks until we find one that's large enough. */
struct block *find_free_block(struct block **temp, size_t size) {

    struct block *current = head;

    //Loops through until we find a free block with the following citerea
      //Must be free
      //Must be large enough
      //Must "exit"
    while (current && !(current->free && current->size >= size)) {
      *temp = current;
      current = current->next;
    }

    //Return this block if it exists
    return current;
}

/*We use this function if there is not enough space to fit a malloc */
struct block *request_space(struct block* temp, size_t size) {
    struct block *block;
    //Assigns the start of the struct to where the end of the heap use to be
    block = sbrk(0);

    //Making sure sbrk works correctly
    if(block == (void *)-1){
      //sbrk failed
      return NULL;
    }

    //We wante to make sure we are requesting a multiple of 16.
    size_t increment_value = size + sizeof(struct block);
    if(increment_value % 16 != 0){
      int missing_value = 16 - (increment_value % 16);
      increment_value = increment_value + missing_value;
    }

    //We request the amount of space necessary for a malloc
    void *request = sbrk(increment_value);

    //Error check to make sure we were able to request and obtain space
    if (request == (void*) -1) {
      // sbrk failed.
      return NULL;
    }

    //Set the correct metadata information for our new block
    if (temp) {
      temp->next = block;
    }

    block->size = size;
    block->next = NULL;
    block->free = 0;
    return block;
}

/*Making way for a new chunk allocation by splitting a free chunk

If we find a free chunk which exactly fits the required size, we don't
need to do the splitting. So this function is only required if we have what is
more than required.*/
void split(struct block* b, size_t size){
    //Error checking, must have a size greater than 0
    if(b->size < 0){
      abort();
    }

    //We need to "save" the value of the next node for when we've inserted
    //a new block.
    struct block* temp = b->next;
    //If there is no block afterwards, all we need to is adjust the size
    if(b->next == NULL){
      b->size = size;
    } else {
      //Gets the address of the metadata
      void *mem_block = (void *)((char*)b + sizeof(struct block));

      //Create a new struct block to indicate a free block
      struct block *newptr = (struct block*) ((char*)mem_block + size);

      //Update the metadata to indicate a free block is available
      newptr->size = b->size - (size + sizeof(struct block));

      //Error checking
      if(newptr->size < 0){
        abort();
      }
      //Update the fields accordingly
      newptr->free = 1;
      b->size = size;
      b->next = newptr;
      newptr->next = temp;
    }
}

//Currently returns the usable size of a malloc.
size_t malloc_usable_size(void *ptr) {
    if (!ptr) {
      return -1;
    }

    struct block* block_ptr = get_block_ptr(ptr);

    if(!block_ptr->free){
      return 0;
    } else {
      return block_ptr->size;
    }
}

/* If it's the first ever call, request_space and set the head.
 Otherwise, if we can find a free block, use it.
 If not, request_space. */
void* malloc(size_t size_given) {
    //Keeps track of how many mallocs have been created
    count++;

    //We need to be sure the size of the malloc is 16, simply arithmetic
    int size;
    if(size_given % 16 != 0){
      int missing_value = 16 - (size_given % 16);
      size = size_given + missing_value;
    } else {
      size = size_given;
    }

    struct block *block;

    //Error handling, we don't want to creat a malloc if it has no size
    if (size <= 0) {
      return NULL;
    }

    //First call, we request space and initialize head
    if (!head) {
      block = request_space(NULL, size);
      if (!block) {
        return NULL;
      }
      head = block;

    } else {
      //We're going to iterate through and find a space to put the new malloc
      struct block *last = head;

      //Use helper function to find place to put malloc
      block = find_free_block(&last, size);

      // Failed to find free block.
      if (!block) {

        //Use helper to request space
        block = request_space(last, size);

        if (!block) {
  				return NULL;
        }

      } else {
        // Found free block
        if(block->size - size == 0){

          //If we have a perfect fit, all we need to do is mark it as not free
          block->free = 0;

        } else {

          block->free = 0;
          //If there is a block afterwards, we need to split
          if(block->next != NULL){

            //if the block size is less than 0, we want to terminate
            if(block->size < 0){
              abort();
            }

            //We will split the block after
            split(block, size);
          } else {

            //Otherwise, there is no block afterwards so we do not need to split
            if(size < 0){
              abort();
            }
            //Simply resize the block
            block->size = size;
          }
        }
      }
    }
    //Writes the information about each malloc so we can debug
    //memory_info("Test: ");

    //Return a pointer to where the user will put information
    return (block+1);
}

//Sets the information within a malloc to 0
void *calloc(size_t number_elements, size_t element_size) {

    size_t size = number_elements * element_size;
    //Initialize the malloc with the size we want
    void *ptr = malloc(size);
    //Set the information within the malloc to 0
    memset(ptr, 0, size);
    return ptr;
}

/*Simply a helper function to return the pointer to the malloc */
struct block *get_block_ptr(void *ptr) {
    return (struct block*)ptr - 1;
}

/*Allows us to re-use space once we are done with it */
void free(void *ptr) {

    //Error handling
    if (!ptr) {
      return;
    }

    //Get the pointer to the malloc and set it's free value to 1
    struct block* block_ptr = get_block_ptr(ptr);
    assert(block_ptr->free == 0);
    block_ptr->free = 1;
}


/*Re-size a malloc */
void *realloc(void *ptr, size_t size_given) {

    //We need to make sure the value we are re-sizing to is a multiple of 16
    int size;
    if(size_given % 16 != 0){
      int missing_value = 16 - (size_given % 16);
      size = size_given + missing_value;
    } else {
      size = size_given;
    }

    if (!ptr) {
      // If the ptr is null, we just want to make a malloc.
      return malloc(size);
    }

    //Otherwise we are going to have to look at the malloc more closely
    struct block* block_ptr = get_block_ptr(ptr);

    if(size == 0){
      block_ptr->free = 1;
      return block_ptr;
    }

    //If the new size is smaller than the old, we simply shrink the malloc
    if (block_ptr->size > size) {

      // if the size realloc asks for is less than 0, we want to terminate
      if(size < 0){
        abort();
      }

      //If we do not need to make any changes
      if(block_ptr->size == size){
        return block_ptr;
      }

      //We are going to need to split the block and indicate there is free space
      split(block_ptr, size);
      return ptr;
    }

    // Need to really realloc. Malloc new space and free old space.
    // Then copy old data to new space.
    void *new_ptr;
    new_ptr = malloc(size);
    if (!new_ptr) {
      return NULL;
    }

    //Copy over the information from the old malloc to the new
    memcpy(new_ptr, ptr, block_ptr->size);

    //Free the old space
    free(ptr);

    //return the new pointer
    return new_ptr;
}
