#ifndef MEM_BLOCKS_TYPES_H
#define MEM_BLOCKS_TYPES_H

#include <stdlib.h>

struct free_block{
    size_t size;
    struct free_block* next;
};
struct used_block{
    size_t size;
    long int magic;
};

enum type_block {FREE, USED};

struct mem_block{
    size_t size;
    long int magic;
    enum type_block type;
    struct mem_block *pred;
    struct mem_block *next;
};

typedef struct free_block free_block_t;
typedef struct used_block used_block_t;
typedef struct mem_block mem_block_t;


#endif
