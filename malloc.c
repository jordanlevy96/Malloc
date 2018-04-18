#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stddef.h>
#include <math.h>
#include <assert.h>
#include <string.h>
#include "header.h"


#define HEADER_SIZE sizeof(struct header)

struct header *G_BASE = NULL;

struct header *find_free_block(size_t size) {
    struct header *current;

    if (!G_BASE) {
        return NULL; //there are no blocks yet
    }

    current = G_BASE;

    while (current) { //break if null pointer reached
        // printf("current: %p, debug: %d\n", current, current->debug);
        if (current->free && current->size >= size) {
            // printf("block is big enough and free!\n");
            return current + 1;
        }
        //check next block
        current = current->next;
    }

    // printf("no free block available\n");
    return NULL; //a free block was not found
}

struct header *request_space(size_t size, struct header **last) {
    struct header *start = sbrk(0);

    //make block divisible by 16
    size_t tempSize = size + HEADER_SIZE;
    if (tempSize % 16) {
        tempSize += 16 - (tempSize % 16);
    }

    // printf("allocating %d bytes of memory\n", tempSize);

    void *request = sbrk(tempSize);
    assert(start == request);

    if (request == (void*) -1) {
        return NULL; // sbrk failed.
    }

    struct header *meta = (struct header *) request;

    if (last) {
        // printf("set up linked list\n");
        (*last)->next = meta; //set up linked list
    }

    meta->size = size;
    meta->next = NULL;
    meta->free = 0;
    // meta->debug = 589;

    return meta + 1;
}

struct header *find_last() {
    struct header *last;

    if (!G_BASE) {
        return NULL;
    }

    last = G_BASE;

    while (last && last->next) {
        last = last->next;
    }

    return last;
}

void* malloc(size_t size) {
    void *memory;
    struct header *last;

    if (!G_BASE) {
        //set heap start
        G_BASE = sbrk(0);
        return request_space(size, NULL);
    }

    if ((memory = find_free_block(size))) {
        return memory;
    }

    last = find_last();

    //don't pass in a null pointer!
    if (!last) {
        if ((memory = request_space(size, NULL))) {
            // printf("this block should never get hit tbh\n");
            return memory;
        }
    }

    if ((memory = request_space(size, &last))) {
        return memory;
    }

    perror("MALLOC FAILED");

    return NULL;
}

void *calloc(size_t count, size_t s) {
  size_t size;
  void *p;

  size = count * s;
  p = malloc(size);

  memset(p, 0, size); //clear allocated memory
  return p;
}

//loop through headers and join adjacent free blocks
void join_free_blocks() {
    struct header *current = G_BASE;

    while (current->next) {
        if (current->free && current->next->free) {
            // printf("joining free blocks %p and %p!\n",
            //     current, current->next);
            current->size = current->size + current->next->size + HEADER_SIZE;
            current->next = current->next->next;
        }
        else {
            current = current->next;
        }
    }
}

void free(void *ptr) {
    if (!ptr) { //ptr is null
        return;
    }
    // printf("freeing...\n");
    //move pointer to header start, rather than memory start
    struct header *h = ((struct header*) ptr) - 1;
    // if (h->debug == 589) {
    //     printf("header found\n");
    // }
    h->free = 1;
    // h->debug = 7433;
    // printf("%p->debug set to %d\n", h, h->debug);

    join_free_blocks();
}

void *realloc(void *ptr, size_t size) {
    if (!ptr) {
        return malloc(size);
    }
    else if (!size) {
        free(ptr);
        return ptr;
    }

    //move pointer to header start, rather than memory start
    struct header *head = ((struct header*) ptr) - 1;
    // head->debug = 160;
    // printf("in realloc--head: %p\n", head);

    if (size == head->size) {
        return ptr;
    }

    //check if next block is free
    if (head->next && head->next->free) {
        if (head->next->size + HEADER_SIZE + head->size >= size) {
            // printf("adjusting old pointer!\n");
            //combining this block with the next block, which is free
            //will create a big enough block
            head->size = head->size + head->next->size + HEADER_SIZE;
            head->next = head->next->next;
            // head->debug = 43;
            return ptr;
        }
    }
    else if (!head->next) { //last pointer
        sbrk(size - head->size); //extend heap for new memory
        head->size = size;
        return ptr;
    }

    void *newptr;

    // printf("hi!\n");

    if (size > head->size) {
        // printf("size is bigger\n");
        //need a new memory block
        newptr = malloc(size);
        memcpy(newptr, ptr, head->size);
        free(ptr);
    }
    else { // size < head->size
        // printf("size is smaller\n");
        // printf("head->size: %d\n", head->size);
        size_t extra = head->size - size;
        // printf("size of extra memory: %lu\n", extra);
        // printf("size of header: %lu\n", sizeof(struct header));
        // printf("extra memory - header: %d\n", extra - sizeof(struct header));
        if (extra > sizeof(struct header)) {
            // printf("can fit new header\n");
            //can fit a new header between current and next

            //find the end of the realloced space
            char *placeholder = ptr + size;
            struct header *newhead = (struct header *) placeholder;
            newhead->size = extra - HEADER_SIZE;
            newhead->free = 1;
            newhead->next = head->next;
            head->next = newhead;
            head->size = size;

            // printf("ptr: %p\n", ptr);
            // printf("newhead: %p\n", newhead);

            return ptr;
        }
        else {
            // printf("making new pointer\n");
            newptr = malloc(size);
            memcpy(newptr, ptr, size);
            free(ptr);
        }
    }

    return newptr;
}

int main() {
    char *c, *d;

    for (int i = 0; i < 4; i++) {
        c = malloc(1);
        *c = 'a';
        printf("%c\n", *c);
        d = realloc(c, 1);
        printf("%c\n", *d);

    } //test malloc and realloc a few times

    return 0;

}
