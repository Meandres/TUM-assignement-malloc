#ifndef MEM_ALLOC_H
#define MEM_ALLOC_H

#include "mem_blocks_types.h"


int init();
void* m_alloc(size_t size);
int m_free(void* p);


#endif
